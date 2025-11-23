#pragma once

#include "Hardware/wifi/http/HttpClientInterface.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

/**
 * @brief Curl-based HTTP client for desktop simulator
 *
 * This implementation uses libcurl to provide HTTP functionality
 * on desktop platforms. Requests are executed asynchronously using
 * a thread pool pattern with custom HttpFuture for fluent callbacks.
 */
class CurlHttpClient : public HttpClientInterface {
public:
  /**
   * @brief Constructor
   *
   * @param num_threads Number of worker threads for async requests (default: 2)
   */
  explicit CurlHttpClient(size_t num_threads = 2);

  /**
   * @brief Destructor - ensures proper cleanup
   */
  ~CurlHttpClient() override;

  // Prevent copy operations
  CurlHttpClient(const CurlHttpClient &) = delete;
  CurlHttpClient &operator=(const CurlHttpClient &) = delete;

  // Allow move operations
  CurlHttpClient(CurlHttpClient &&) = default;
  CurlHttpClient &operator=(CurlHttpClient &&) = default;

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
   * @brief Worker thread function
   */
  void workerThread();

  /**
   * @brief Execute a request synchronously (called by worker threads)
   */
  static HttpResponse executeSyncRequest(const HttpRequest &request);

  /**
   * @brief Convert HTTP method enum to string
   */
  static std::string methodToString(HttpRequest::Method method);

  // Thread pool management
  std::vector<std::thread> worker_threads_;
  std::queue<HttpRequest> request_queue_;
  std::queue<std::promise<HttpResponse>> promise_queue_;
  mutable std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  bool shutdown_requested_ = false;

  // Default headers
  std::map<std::string, std::string> default_headers_;
  mutable std::mutex headers_mutex_;

  // State
  bool initialized_ = false;
};
