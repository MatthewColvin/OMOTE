#pragma once

#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

/**
 * @brief HTTP response object containing status and data
 */
struct HttpResponse {
  int status_code;                            // HTTP status code (e.g., 200, 404, 500)
  std::string body;                           // Response body content
  std::map<std::string, std::string> headers; // Response headers
  bool success;                               // Whether the request completed successfully
  std::string error_message;                  // Error message if any

  HttpResponse();
  HttpResponse(int aCode, const std::string &aResponseBody, bool aIsSuccess = true);
  HttpResponse(int aCode, const std::string &aResponseBody, const std::string &aError);
};

/**
 * @brief HTTP request configuration
 */
struct HttpRequest {
  enum class Method {
    Get,
    Post,
    Put,
    Delete,
    Patch,
    Head,
    Options
  };

  Method method;
  std::string url;
  std::map<std::string, std::string> headers;
  std::string body;
  int timeout_ms; // Timeout in milliseconds

  HttpRequest();
  HttpRequest(Method aMethod, const std::string &aUrl);
  HttpRequest(Method aMethod, const std::string &aUrl, const std::string &aBody);
};

class HttpFuture : public std::enable_shared_from_this<HttpFuture> {
public:
  using ResponseCallback = std::function<void(const HttpResponse &)>;

  explicit HttpFuture(std::future<HttpResponse> aFuture);

  /**
   * @brief Register a callback for a specific HTTP status code
   *
   * @param aCode The HTTP status code to match (e.g., 200, 404, 500)
   * @param aCallback Function to call when this status code is received
   * @return Shared pointer to this HttpFuture for chaining
   *
   * @example
   * client->getAsync("https://api.example.com/data")
   *     ->onReturnCode(200, [](const HttpResponse& resp) {
   *         std::cout << "Success!" << std::endl;
   *     })
   *     ->onReturnCode(404, [](const HttpResponse& resp) {
   *         std::cout << "Not found!" << std::endl;
   *     });
   */
  std::shared_ptr<HttpFuture> onReturnCode(int aCode, ResponseCallback aCallback);

  /**
   * @brief Register a callback for multiple specific HTTP status codes
   *
   * @param aCodes Vector of HTTP status codes to match
   * @param aCallback Function to call when any of these codes are received
   * @return Shared pointer to this HttpFuture for chaining
   *
   * @example
   * client->getAsync("https://api.example.com/data")
   *     ->onReturnCode({404, 410}, [](const HttpResponse& resp) {
   *         std::cout << "Resource gone!" << std::endl;
   *     });
   */
  std::shared_ptr<HttpFuture> onReturnCode(const std::vector<int> &aCodes, ResponseCallback aCallback);

  /**
   * @brief Alias for onReturnCodeDefault() - register a callback for any unmatched HTTP status codes
   *
   * This is an alternative name for the catch-all handler. Invoked for status codes
   * that don't have specific handlers registered via onReturnCode().
   *
   * @param aCallback Function to call for unmatched status codes
   * @return Shared pointer to this HttpFuture for chaining
   *
   * @example
   * client->getAsync("https://api.example.com/data")
   *     ->onReturnCode(200, [](const HttpResponse& resp) {
   *         std::cout << "Success!" << std::endl;
   *     })
   *     ->onOtherReturnCode([](const HttpResponse& resp) {
   *         std::cout << "Unexpected status: " << resp.status_code << std::endl;
   *     });
   */
  std::shared_ptr<HttpFuture> onOtherReturnCode(ResponseCallback aCallback);

  /**
   * @brief Block and get the response (synchronous)
   *
   * @return The response when ready
   */
  HttpResponse get();

  /**
   * @brief Wait for the response with a timeout
   *
   * @param aTimeout How long to wait
   * @return true if response is ready, false if timed out
   */
  template <typename Rep, typename Period>
  bool waitFor(const std::chrono::duration<Rep, Period> &aTimeout) {
    if (!mFuture.valid())
      return false;
    return mFuture.wait_for(aTimeout) == std::future_status::ready;
  }

  /**
   * @brief Check if the response is ready without blocking
   *
   * @return true if response is ready
   */
  bool IsReady() const;

  /**
   * @brief Register a callback to invoke when response is ready (regardless of success/error)
   *
   * @param aCallback Function to call with the response
   * @return Shared pointer to this HttpFuture for chaining
   */
  std::shared_ptr<HttpFuture> always(std::function<void(const HttpResponse &)> aCallback);

private:
  std::future<HttpResponse> mFuture;

  mutable std::mutex mCallbackMutex;
  std::map<int, ResponseCallback> mReturnCodeCallbacks;
  ResponseCallback mReturnCodeDefaultCallback;
  ResponseCallback mAlwaysCallback;

  /**
   * @brief Check if future is ready and invoke appropriate callback
   */
  void checkAndInvoke();
};