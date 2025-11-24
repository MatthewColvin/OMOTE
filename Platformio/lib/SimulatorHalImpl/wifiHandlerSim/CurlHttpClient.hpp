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
 * Uses a thread pool and queue to handle asynchronous HTTP requests
 * via libcurl. Implements HttpClientInterface with custom HttpFuture
 * for fluent callback handling.
 */
class CurlHttpClient : public HttpClientInterface {
public:
  using QueueRequestItemType = std::pair<HttpRequest, std::weak_ptr<std::promise<HttpResponse>>>;

  explicit CurlHttpClient(size_t aNumWorkerThreads = 2);
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

  // Thread pool for executing requests
  std::vector<std::thread> mWorkerThreads;

  // Queue for thread-safe non-blocking request handling
  mutable std::mutex mQueueMutex;
  std::condition_variable mQueueCv;
  std::queue<QueueRequestItemType> mRequestQueue;

  // Default headers
  std::map<std::string, std::string> default_headers_;
  mutable std::mutex mHeadersMutex;

  // State
  bool mInitialized = false;
  bool mShutdownRequested = false;
};
