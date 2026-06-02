#pragma once

#include "Hardware/wifi/http/HttpClientInterface.hpp"
#include <condition_variable>
#include <esp_http_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory>
#include <mutex>
#include <queue>

/**
 * @brief ESP32 HTTP client using esp_http_client
 *
 * This implementation uses the ESP-IDF esp_http_client library
 * to provide HTTP functionality on ESP32 devices. Requests are
 * executed asynchronously using a task pool pattern with custom
 * HttpFuture for fluent callbacks.
 */
class Esp32HttpClient : public HttpClientInterface {
public:
  /**
   * @brief Constructor
   *
   * @param num_tasks Number of FreeRTOS tasks for async requests (default: 2)
   */
  explicit Esp32HttpClient(size_t num_tasks = 2);

  /**
   * @brief Destructor - ensures proper cleanup
   */
  ~Esp32HttpClient() override;

  // Prevent copy operations
  Esp32HttpClient(const Esp32HttpClient &) = delete;
  Esp32HttpClient &operator=(const Esp32HttpClient &) = delete;

  // Allow move operations
  Esp32HttpClient(Esp32HttpClient &&) = default;
  Esp32HttpClient &operator=(Esp32HttpClient &&) = default;

  // HttpClientInterface implementation
  std::shared_ptr<HttpFuture> executeAsync(const HttpRequest &request) override;

  std::shared_ptr<HttpFuture> getAsync(
      const std::string &url,
      int timeout_ms = 5000) override;

  std::shared_ptr<HttpFuture> postAsync(
      const std::string &url,
      const std::string &body,
      int timeout_ms = 5000) override;

  std::shared_ptr<HttpFuture> putAsync(
      const std::string &url,
      const std::string &body,
      int timeout_ms = 5000) override;

  std::shared_ptr<HttpFuture> deleteAsync(
      const std::string &url,
      int timeout_ms = 5000) override;

  void setDefaultHeader(const std::string &key, const std::string &value) override;
  void clearDefaultHeaders() override;

  bool isReady() const override;
  bool initialize() override;
  void shutdown() override;

private:
  /**
   * @brief Worker task function (FreeRTOS task)
   * @note This is a static function that wraps the instance method
   */
  static void workerTask(void *pvParameters);

  /**
   * @brief Instance worker method
   */
  void workerTaskImpl();

  /**
   * @brief Execute a request synchronously
   */
  static HttpResponse executeSyncRequest(const HttpRequest &request);

  /**
   * @brief Callback for esp_http_client event handling
   */
  static esp_err_t httpEventHandler(esp_http_client_event_t *evt);

  /**
   * @brief Convert HTTP method enum to esp_http_client_method_t
   */
  static esp_http_client_method_t methodToEspMethod(HttpRequest::Method method);

  // Task management
  std::vector<TaskHandle_t> worker_tasks_;
  std::queue<HttpRequest> request_queue_;
  std::queue<std::promise<HttpResponse>> promise_queue_;
  mutable std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  bool shutdown_requested_ = false;
  size_t num_tasks_ = 0;

  // Default headers
  std::map<std::string, std::string> default_headers_;
  mutable std::mutex headers_mutex_;

  // State
  bool initialized_ = false;
};

using Esp32HttpClientPtr = std::shared_ptr<Esp32HttpClient>;
