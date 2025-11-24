#include "CurlHttpClient.hpp"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

// Callback for curl to write response data
static size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  if (userp) {
    std::string *str = static_cast<std::string *>(userp);
    str->append(contents, realsize);
  }
  return realsize;
}

// Callback for curl to write response headers
static size_t HeaderCallback(char *buffer, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  std::string header(buffer, realsize);

  // Parse header line (format: "Header-Name: value\r\n")
  size_t colon_pos = header.find(':');
  if (colon_pos != std::string::npos && userp) {
    std::string key = header.substr(0, colon_pos);
    // allow for ": " after key but be defensive
    size_t value_start = colon_pos + 1;
    if (value_start < header.size() && header[value_start] == ' ') {
      ++value_start;
    }
    std::string value = header.substr(value_start);

    // Trim trailing whitespace/CRLF
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' ')) {
      value.pop_back();
    }

    auto *mapPtr = static_cast<std::map<std::string, std::string> *>(userp);
    mapPtr->emplace(key, value);
  }

  return realsize;
}

CurlHttpClient::CurlHttpClient(size_t num_threads)
    : mShutdownRequested(false), mInitialized(false) {

  // Initialize curl globally
  curl_global_init(CURL_GLOBAL_DEFAULT);

  // Start worker threads
  for (size_t i = 0; i < num_threads; ++i) {
    mWorkerThreads.emplace_back(&CurlHttpClient::workerThread, this);
  }

  mInitialized = true;
}

CurlHttpClient::~CurlHttpClient() {
  shutdown();
}

std::shared_ptr<HttpFuture> CurlHttpClient::executeAsync(const HttpRequest &request) {
  auto promise = std::make_shared<std::promise<HttpResponse>>();
  auto future = promise->get_future();
  {
    std::lock_guard<std::mutex> lock(mQueueMutex);
    mRequestQueue.emplace(request, promise);
  }
  mQueueCv.notify_one();
  return std::make_shared<HttpFuture>(std::move(future));
}

std::shared_ptr<HttpFuture> CurlHttpClient::getAsync(const std::string &url, int timeout_ms) {
  HttpRequest request(HttpRequest::Method::GET, url);
  request.timeout_ms = timeout_ms;
  return executeAsync(request);
}

std::shared_ptr<HttpFuture> CurlHttpClient::postAsync(
    const std::string &url,
    const std::string &body,
    int timeout_ms) {
  HttpRequest request(HttpRequest::Method::POST, url, body);
  request.timeout_ms = timeout_ms;
  return executeAsync(request);
}

std::shared_ptr<HttpFuture> CurlHttpClient::putAsync(
    const std::string &url,
    const std::string &body,
    int timeout_ms) {
  HttpRequest request(HttpRequest::Method::PUT, url, body);
  request.timeout_ms = timeout_ms;
  return executeAsync(request);
}

std::shared_ptr<HttpFuture> CurlHttpClient::deleteAsync(
    const std::string &url,
    int timeout_ms) {
  HttpRequest request(HttpRequest::Method::DELETE, url);
  request.timeout_ms = timeout_ms;
  return executeAsync(request);
}

void CurlHttpClient::setDefaultHeader(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> lock(mHeadersMutex);
  default_headers_[key] = value;
}

void CurlHttpClient::clearDefaultHeaders() {
  std::lock_guard<std::mutex> lock(mHeadersMutex);
  default_headers_.clear();
}

bool CurlHttpClient::isReady() const {
  return mInitialized && !mShutdownRequested;
}

bool CurlHttpClient::initialize() {
  // Already initialized in constructor
  return mInitialized;
}

void CurlHttpClient::shutdown() {
  {
    std::lock_guard<std::mutex> lock(mQueueMutex);
    mShutdownRequested = true;
  }
  mQueueCv.notify_all();

  // Wait for all worker threads to finish
  for (auto &thread : mWorkerThreads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  curl_global_cleanup();
}

void CurlHttpClient::workerThread() {
  while (true) {
    std::unique_lock<std::mutex> lock(mQueueMutex);
    // Wait for a request or shutdown signal
    mQueueCv.wait(lock, [this]() {
      return !mRequestQueue.empty() || mShutdownRequested;
    });
    if (mShutdownRequested && mRequestQueue.empty()) {
      break;
    }
    // Get the request to process
    auto [request, promise] = mRequestQueue.front();
    mRequestQueue.pop();
    lock.unlock();

    HttpResponse response = executeSyncRequest(request);
    if (auto sPromise = promise.lock(); sPromise) {
      sPromise->set_value(response);
    }
  }
}

HttpResponse CurlHttpClient::executeSyncRequest(const HttpRequest &request) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    HttpResponse error_response(-1, "", "Failed to initialize curl");
    error_response.success = false;
    return error_response;
  }

  std::string response_body;
  std::map<std::string, std::string> response_headers;
  long response_code = 0;

  try {
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());

    // Set timeouts (connection and total transfer)
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, (long)request.timeout_ms);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)request.timeout_ms);
    // In multithreaded programs, libcurl should not install or use signals.
    // Prevent use of signals (like SIGALRM) which can interfere with threads.
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    // Set HTTP method
    switch (request.method) {
    case HttpRequest::Method::GET:
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
      break;
    case HttpRequest::Method::POST:
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      if (!request.body.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.c_str());
      }
      break;
    case HttpRequest::Method::PUT:
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
      if (!request.body.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.c_str());
      }
      break;
    case HttpRequest::Method::DELETE:
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
      break;
    case HttpRequest::Method::PATCH:
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
      if (!request.body.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.c_str());
      }
      break;
    case HttpRequest::Method::HEAD:
      curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
      break;
    case HttpRequest::Method::OPTIONS:
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
      break;
    }

    // Set headers
    struct curl_slist *headers = nullptr;
    if (!request.headers.empty()) {
      for (const auto &[key, value] : request.headers) {
        std::string header_line = key + ": " + value;
        headers = curl_slist_append(headers, header_line.c_str());
      }
    }
    if (headers) {
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Set write callbacks for response body and headers
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&response_headers);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    // Get response code
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    // Cleanup headers list
    if (headers) {
      curl_slist_free_all(headers);
    }

    // Check for curl errors
    if (res != CURLE_OK) {
      HttpResponse error_response(-1, "", std::string("Curl error: ") + curl_easy_strerror(res));
      error_response.success = false;
      curl_easy_cleanup(curl);
      return error_response;
    }

    // Create response
    HttpResponse response((int)response_code, response_body, (response_code >= 200 && response_code < 300));
    response.headers = response_headers;

    curl_easy_cleanup(curl);
    return response;

  } catch (const std::exception &e) {
    curl_easy_cleanup(curl);
    HttpResponse error_response(-1, "", std::string("Exception: ") + e.what());
    error_response.success = false;
    return error_response;
  }
}

std::string CurlHttpClient::methodToString(HttpRequest::Method method) {
  switch (method) {
  case HttpRequest::Method::GET:
    return "GET";
  case HttpRequest::Method::POST:
    return "POST";
  case HttpRequest::Method::PUT:
    return "PUT";
  case HttpRequest::Method::DELETE:
    return "DELETE";
  case HttpRequest::Method::PATCH:
    return "PATCH";
  case HttpRequest::Method::HEAD:
    return "HEAD";
  case HttpRequest::Method::OPTIONS:
    return "OPTIONS";
  default:
    return "UNKNOWN";
  }
}
