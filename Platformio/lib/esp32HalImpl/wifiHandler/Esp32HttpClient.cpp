#include "Esp32HttpClient.hpp"
#include <cstring>
#include <esp_log.h>

static const char *TAG = "Esp32HttpClient";

// Event data structure for handling responses
struct HttpEventData {
  std::string response_body;
  std::map<std::string, std::string> response_headers;
  int response_code = 0;
};

Esp32HttpClient::Esp32HttpClient(size_t num_tasks)
    : shutdown_requested_(false), initialized_(false), num_tasks_(num_tasks) {
  // Initialization happens in initialize()
}

Esp32HttpClient::~Esp32HttpClient() {
  shutdown();
}

std::shared_ptr<HttpFuture> Esp32HttpClient::executeAsync(const HttpRequest &request) {
  auto promise = std::make_shared<std::promise<HttpResponse>>();
  auto future = promise->get_future();

  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    request_queue_.push(request);
    promise_queue_.push(std::move(*promise));
  }

  queue_cv_.notify_one();

  auto http_future = std::make_shared<HttpFuture>(std::move(future));
  return http_future;
}

std::shared_ptr<HttpFuture> Esp32HttpClient::getAsync(const std::string &url, int timeout_ms) {
  HttpRequest request(HttpRequest::Method::Get, url);
  request.timeout_ms = timeout_ms;
  return executeAsync(request);
}

std::shared_ptr<HttpFuture> Esp32HttpClient::postAsync(
    const std::string &url,
    const std::string &body,
    int timeout_ms) {
  HttpRequest request(HttpRequest::Method::Post, url, body);
  request.timeout_ms = timeout_ms;
  return executeAsync(request);
}

std::shared_ptr<HttpFuture> Esp32HttpClient::putAsync(
    const std::string &url,
    const std::string &body,
    int timeout_ms) {
  HttpRequest request(HttpRequest::Method::Put, url, body);
  request.timeout_ms = timeout_ms;
  return executeAsync(request);
}

std::shared_ptr<HttpFuture> Esp32HttpClient::deleteAsync(
    const std::string &url,
    int timeout_ms) {
  HttpRequest request(HttpRequest::Method::Delete, url);
  request.timeout_ms = timeout_ms;
  return executeAsync(request);
}

void Esp32HttpClient::setDefaultHeader(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> lock(headers_mutex_);
  default_headers_[key] = value;
}

void Esp32HttpClient::clearDefaultHeaders() {
  std::lock_guard<std::mutex> lock(headers_mutex_);
  default_headers_.clear();
}

bool Esp32HttpClient::isReady() const {
  return initialized_ && !shutdown_requested_;
}

bool Esp32HttpClient::initialize() {
  if (initialized_) {
    return true;
  }

  // Create worker tasks
  for (size_t i = 0; i < num_tasks_; ++i) {
    TaskHandle_t task_handle = nullptr;
    BaseType_t result = xTaskCreate(
        &Esp32HttpClient::workerTask,
        "HttpWorker",
        4096,                 // Stack size
        this,                 // Parameter
        tskIDLE_PRIORITY + 1, // Priority
        &task_handle);

    if (result != pdPASS) {
      ESP_LOGE(TAG, "Failed to create worker task %zu", i);
      shutdown();
      return false;
    }

    worker_tasks_.push_back(task_handle);
  }

  initialized_ = true;
  return true;
}

void Esp32HttpClient::shutdown() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    shutdown_requested_ = true;
  }
  queue_cv_.notify_all();

  // Wait for all worker tasks to finish
  for (auto task_handle : worker_tasks_) {
    if (task_handle != nullptr) {
      // Give tasks a chance to exit gracefully
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  worker_tasks_.clear();
}

void Esp32HttpClient::workerTask(void *pvParameters) {
  Esp32HttpClient *pThis = static_cast<Esp32HttpClient *>(pvParameters);
  pThis->workerTaskImpl();

  // Task should not reach here, but if it does, delete itself
  vTaskDelete(nullptr);
}

void Esp32HttpClient::workerTaskImpl() {
  while (true) {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // Wait for a request or shutdown signal
    queue_cv_.wait(lock, [this]() {
      return !request_queue_.empty() || shutdown_requested_;
    });

    if (shutdown_requested_ && request_queue_.empty()) {
      break;
    }

    if (request_queue_.empty()) {
      continue;
    }

    // Get the next request and promise pair
    HttpRequest request = std::move(request_queue_.front());
    request_queue_.pop();
    std::promise<HttpResponse> promise = std::move(promise_queue_.front());
    promise_queue_.pop();
    lock.unlock();

    // Execute the request
    // try {
    HttpResponse response = executeSyncRequest(request);
    promise.set_value(response);
    // } catch (const std::exception &e) {
    //   HttpResponse error_response(
    //       -1,
    //       "",
    //       std::string("Error: ") + e.what());
    //   error_response.success = false;
    //   promise.set_value(error_response);
    // }
  }
}

esp_err_t Esp32HttpClient::httpEventHandler(esp_http_client_event_t *evt) {
  if (evt->user_data == nullptr) {
    return ESP_OK;
  }

  HttpEventData *event_data = static_cast<HttpEventData *>(evt->user_data);

  switch (evt->event_id) {
  case HTTP_EVENT_ON_HEADER:
    // Parse header
    if (evt->header_key && evt->header_value) {
      std::string key(evt->header_key);
      std::string value(evt->header_value);
      event_data->response_headers[key] = value;
    }
    break;

  case HTTP_EVENT_ON_DATA:
    // Accumulate response body
    if (!esp_http_client_is_chunked_response(evt->client)) {
      event_data->response_body.append((char *)evt->data, evt->data_len);
    }
    break;

  case HTTP_EVENT_REDIRECT:
    ESP_LOGD(TAG, "HTTP redirect occurred");
    break;

  case HTTP_EVENT_ERROR:
    ESP_LOGE(TAG, "HTTP request error");
    break;

  default:
    break;
  }

  return ESP_OK;
}

HttpResponse Esp32HttpClient::executeSyncRequest(const HttpRequest &request) {
  HttpEventData event_data;

  // Configure HTTP client
  esp_http_client_config_t config = {};
  config.url = request.url.c_str();
  config.timeout_ms = request.timeout_ms;
  config.event_handler = &Esp32HttpClient::httpEventHandler;
  config.user_data = &event_data;
  config.method = methodToEspMethod(request.method);

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) {
    HttpResponse error_response(-1, "", "Failed to initialize HTTP client");
    error_response.success = false;
    return error_response;
  }

  // try {
  // Set request body if present
  if (!request.body.empty()) {
    esp_http_client_set_post_field(client,
                                   request.body.c_str(),
                                   request.body.length());
  }

  // Set request headers
  for (const auto &[key, value] : request.headers) {
    esp_http_client_set_header(client, key.c_str(), value.c_str());
  }

  // Perform the request
  esp_err_t err = esp_http_client_perform(client);

  HttpResponse response;

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    response.status_code = -1;
    response.error_message = std::string("HTTP error: ") + esp_err_to_name(err);
    response.success = false;
  } else {
    response.status_code = (int)esp_http_client_get_status_code(client);
    response.body = event_data.response_body;
    response.headers = event_data.response_headers;
    response.success = (response.status_code >= 200 && response.status_code < 300);
  }

  esp_http_client_cleanup(client);
  return response;

  // } catch (const std::exception &e) {
  //   ESP_LOGE(TAG, "Exception during HTTP request: %s", e.what());
  //   esp_http_client_cleanup(client);

  //   HttpResponse error_response(-1, "", std::string("Exception: ") + e.what());
  //   error_response.success = false;
  //   return error_response;
  // }
}

esp_http_client_method_t Esp32HttpClient::methodToEspMethod(HttpRequest::Method method) {
  switch (method) {
  case HttpRequest::Method::Get:
    return HTTP_METHOD_GET;
  case HttpRequest::Method::Post:
    return HTTP_METHOD_POST;
  case HttpRequest::Method::Put:
    return HTTP_METHOD_PUT;
  case HttpRequest::Method::Delete:
    return HTTP_METHOD_DELETE;
  case HttpRequest::Method::Patch:
    return HTTP_METHOD_PATCH;
  case HttpRequest::Method::Head:
    return HTTP_METHOD_HEAD;
  case HttpRequest::Method::Options:
    return HTTP_METHOD_OPTIONS;
  default:
    return HTTP_METHOD_GET;
  }
}
