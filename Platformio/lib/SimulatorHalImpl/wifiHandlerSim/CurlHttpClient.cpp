#include "CurlHttpClient.hpp"
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

auto CurlHttpClient::GetHeaderList(const HttpRequest &aRequest) const {
  auto listDeleter = [&](curl_slist *ptr) { curl_slist_free_all(ptr); };
  std::unique_ptr<curl_slist, decltype(listDeleter)> memManagedHeaders(nullptr,
                                                                       listDeleter);
  struct curl_slist *headers = nullptr;
  for (const auto &[key, value] : aRequest.headers) {
    std::string header_line = key + ":";
    if (!value.empty()) {
      header_line += " " + value;
    }
    headers = curl_slist_append(headers, header_line.c_str());
  }
  memManagedHeaders.reset(headers);
  return memManagedHeaders;
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
  auto curl = SetupEasyCurl(request.url);
  if (!curl) {
    HttpResponse error_response(-1, "", "Failed to initialize curl");
    error_response.success = false;
    return error_response;
  }

  std::string response_body;
  std::map<std::string, std::string> response_headers;
  long response_code = 0;

  // Set timeouts (connection and total transfer)
  // These must be set BEFORE perform() to prevent indefinite hangs
  curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS, (long)request.timeout_ms);
  curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, (long)request.timeout_ms);

  // DNS timeout (resolves "DNS hangs" on unreachable hosts)
  // Set to same as connection timeout to fail fast
  curl_easy_setopt(curl.get(), CURLOPT_DNS_CACHE_TIMEOUT, (long)(request.timeout_ms / 1000));

  // In multithreaded programs, libcurl should not install or use signals.
  // Prevent use of signals (like SIGALRM) which can interfere with threads.
  curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);

  // Force IPv4 to avoid potential IPv6 DNS issues
  curl_easy_setopt(curl.get(), CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

  // Enable verbose logging for debugging
  curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1L);

  SetupMethodOptions(curl, request);
  auto headers = GetHeaderList(request);
  if (headers) {
    // Debug: print headers being sent
    struct curl_slist *h = headers.get();
    while (h) {
      std::cout << "[CurlHttpClient] Sending header: " << h->data << std::endl;
      h = h->next;
    }
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
  }

  // Set write callbacks for response body and headers
  curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, (void *)&response_body);
  curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, HeaderCallback);
  curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA, (void *)&response_headers);

  // Allow empty response bodies (common for fire-and-forget APIs like Roku)
  // curl_easy_setopt(curl.get(), CURLOPT_FAILONERROR, 0L);

  // Perform the request
  std::cout << "[CurlHttpClient] Starting request to: " << request.url << std::endl;
  CURLcode res = curl_easy_perform(curl.get());
  std::cout << "[CurlHttpClient] Request completed with code: " << res << " (" << curl_easy_strerror(res) << ")" << std::endl;

  // Get response code
  curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);

  // Create response
  HttpResponse response((int)response_code, response_body, (response_code >= 200 && response_code < 300));
  response.headers = response_headers;
  return response;
}

void CurlHttpClient::SetupMethodOptions(AutoCleanupCurl &aCurl, const HttpRequest &aRequest) {
  switch (aRequest.method) {
  case HttpRequest::Method::GET:
    curl_easy_setopt(aCurl.get(), CURLOPT_HTTPGET, 1L);
    break;
  case HttpRequest::Method::POST:
    curl_easy_setopt(aCurl.get(), CURLOPT_POST, 1L);
    if (!aRequest.body.empty()) {
      curl_easy_setopt(aCurl.get(), CURLOPT_POSTFIELDS, aRequest.body.c_str());
      curl_easy_setopt(aCurl.get(), CURLOPT_POSTFIELDSIZE, (long)aRequest.body.size());
    } else {
      // Explicitly tell libcurl that the POST has an empty body. If we don't
      // set POSTFIELDS/POSTFIELDSIZE, libcurl may use chunked encoding and
      // send an "Expect: 100-continue" handshake (or wait). Explicitly
      // setting an empty body forces Content-Length: 0 and avoids hangs
      // with servers that don't complete the 100-continue flow.
      curl_easy_setopt(aCurl.get(), CURLOPT_POSTFIELDS, "");
      curl_easy_setopt(aCurl.get(), CURLOPT_POSTFIELDSIZE, 0L);
    }
    break;
  case HttpRequest::Method::PUT:
    curl_easy_setopt(aCurl.get(), CURLOPT_CUSTOMREQUEST, "PUT");
    if (!aRequest.body.empty()) {
      curl_easy_setopt(aCurl.get(), CURLOPT_POSTFIELDS, aRequest.body.c_str());
      curl_easy_setopt(aCurl.get(), CURLOPT_POSTFIELDSIZE, (long)aRequest.body.size());
    }
    break;
  case HttpRequest::Method::DELETE:
    curl_easy_setopt(aCurl.get(), CURLOPT_CUSTOMREQUEST, "DELETE");
    break;
  case HttpRequest::Method::PATCH:
    curl_easy_setopt(aCurl.get(), CURLOPT_CUSTOMREQUEST, "PATCH");
    if (!aRequest.body.empty()) {
      curl_easy_setopt(aCurl.get(), CURLOPT_POSTFIELDS, aRequest.body.c_str());
      curl_easy_setopt(aCurl.get(), CURLOPT_POSTFIELDSIZE, (long)aRequest.body.size());
    }
    break;
  case HttpRequest::Method::HEAD:
    curl_easy_setopt(aCurl.get(), CURLOPT_NOBODY, 1L);
    break;
  case HttpRequest::Method::OPTIONS:
    curl_easy_setopt(aCurl.get(), CURLOPT_CUSTOMREQUEST, "OPTIONS");
    break;
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

CurlHttpClient::AutoCleanupCurl CurlHttpClient::SetupEasyCurl(const std::string &aUrl) {
  // Curl Easy Init
  auto curlEasyCleanup = [](CURL *aFinishedCurl) {
    curl_easy_cleanup(aFinishedCurl);
  };
  CurlHttpClient::AutoCleanupCurl easyCurl(curl_easy_init(), curlEasyCleanup);
  curl_easy_setopt(easyCurl.get(), CURLOPT_URL, aUrl.c_str());
  return easyCurl;
}
