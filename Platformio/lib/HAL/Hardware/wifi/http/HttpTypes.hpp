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

  HttpResponse()
      : status_code(0), body(""), success(false), error_message("") {}

  HttpResponse(int code, const std::string &response_body, bool is_success = true)
      : status_code(code), body(response_body), success(is_success), error_message("") {}

  HttpResponse(int code, const std::string &response_body, const std::string &error)
      : status_code(code), body(response_body), success(false), error_message(error) {}
};

/**
 * @brief HTTP request configuration
 */
struct HttpRequest {
  enum class Method {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
  };

  Method method;
  std::string url;
  std::map<std::string, std::string> headers;
  std::string body;
  int timeout_ms; // Timeout in milliseconds

  HttpRequest()
      : method(Method::GET), url(""), timeout_ms(5000) {}

  HttpRequest(Method m, const std::string &u)
      : method(m), url(u), timeout_ms(5000) {}

  HttpRequest(Method m, const std::string &u, const std::string &b)
      : method(m), url(u), body(b), timeout_ms(5000) {}
};

/**
 * @brief Custom HTTP future with monadic/fluent API for callbacks
 *
 * Allows chaining callbacks for success and error cases:
 * @example
 * httpclient->getAsync("https://api.example.com/data")
 *     ->onResponse([](const HttpResponse& resp) {
 *         std::cout << resp.body << std::endl;
 *     })
 *     ->onError([](const HttpResponse& err) {
 *         std::cout << "Error: " << err.error_message << std::endl;
 *     });
 */
class HttpFuture : public std::enable_shared_from_this<HttpFuture> {
public:
  using ResponseCallback = std::function<void(const HttpResponse &)>;
  using ErrorCallback = std::function<void(const HttpResponse &)>;
  using TransformCallback = std::function<std::shared_ptr<HttpFuture>(const HttpResponse &)>;

  explicit HttpFuture(std::future<HttpResponse> future)
      : future_(std::move(future)),
        response_callback_(nullptr),
        error_callback_(nullptr),
        is_blocking_(false),
        return_code_default_callback_(nullptr) {}

  /**
   * @brief Register a callback for a specific HTTP status code
   *
   * @param code The HTTP status code to match (e.g., 200, 404, 500)
   * @param callback Function to call when this status code is received
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
  std::shared_ptr<HttpFuture> onReturnCode(int code, ResponseCallback callback) {
    return_code_callbacks_[code] = callback;
    checkAndInvoke();
    return shared_from_this();
  }

  /**
   * @brief Register a callback for multiple specific HTTP status codes
   *
   * @param codes Vector of HTTP status codes to match
   * @param callback Function to call when any of these codes are received
   * @return Shared pointer to this HttpFuture for chaining
   *
   * @example
   * client->getAsync("https://api.example.com/data")
   *     ->onReturnCode({404, 410}, [](const HttpResponse& resp) {
   *         std::cout << "Resource gone!" << std::endl;
   *     });
   */
  std::shared_ptr<HttpFuture> onReturnCode(const std::vector<int> &codes, ResponseCallback callback) {
    for (int code : codes) {
      return_code_callbacks_[code] = callback;
    }
    checkAndInvoke();
    return shared_from_this();
  }

  /**
   * @brief Register a default callback for any unmatched HTTP status codes
   *
   * This is invoked for status codes that don't have specific handlers
   * registered via onReturnCode(). It acts as a catch-all.
   *
   * @param callback Function to call for unmatched status codes
   * @return Shared pointer to this HttpFuture for chaining
   *
   * @example
   * client->getAsync("https://api.example.com/data")
   *     ->onReturnCode(200, [](const HttpResponse& resp) {
   *         std::cout << "Success!" << std::endl;
   *     })
   *     ->onReturnCode(404, [](const HttpResponse& resp) {
   *         std::cout << "Not found!" << std::endl;
   *     })
   *     ->onReturnCodeDefault([](const HttpResponse& resp) {
   *         std::cout << "Unexpected status: " << resp.status_code << std::endl;
   *     });
   */
  std::shared_ptr<HttpFuture> onReturnCodeDefault(ResponseCallback callback) {
    return_code_default_callback_ = callback;
    checkAndInvoke();
    return shared_from_this();
  }

  /**
   * @brief Alias for onReturnCodeDefault() - register a callback for any unmatched HTTP status codes
   *
   * This is an alternative name for the catch-all handler. Invoked for status codes
   * that don't have specific handlers registered via onReturnCode().
   *
   * @param callback Function to call for unmatched status codes
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
  std::shared_ptr<HttpFuture> onOtherReturnCode(ResponseCallback callback) {
    return onReturnCodeDefault(callback);
  }

  /**
   * @brief Register a callback to be invoked on successful response (status 200-299)
   *
   * @param callback Function to call with the response
   * @return Shared pointer to this HttpFuture for chaining
   */
  std::shared_ptr<HttpFuture> onResponse(ResponseCallback callback) {
    response_callback_ = callback;
    checkAndInvoke();
    return shared_from_this();
  }

  /**
   * @brief Register a callback to be invoked on error (non-2xx status or network error)
   *
   * @param callback Function to call with the error response
   * @return Shared pointer to this HttpFuture for chaining
   */
  std::shared_ptr<HttpFuture> onError(ErrorCallback callback) {
    error_callback_ = callback;
    checkAndInvoke();
    return shared_from_this();
  }

  /**
   * @brief Register callbacks for both success and error cases
   *
   * @param on_response Callback for successful responses
   * @param on_error Callback for errors
   * @return Shared pointer to this HttpFuture for chaining
   */
  std::shared_ptr<HttpFuture> then(ResponseCallback on_response, ErrorCallback on_error) {
    response_callback_ = on_response;
    error_callback_ = on_error;
    checkAndInvoke();
    return shared_from_this();
  }

  /**
   * @brief Transform the response into another HttpFuture (monadic bind)
   *
   * Allows chaining HTTP requests: first request result feeds into second request
   * @example
   * httpclient->getAsync("https://api.example.com/user/1")
   *     ->flatMap([httpclient](const HttpResponse& resp) {
   *         // Parse resp and make another request
   *         return httpclient->getAsync("https://api.example.com/user/1/posts");
   *     })
   *     ->onResponse([](const HttpResponse& resp) {
   *         std::cout << resp.body << std::endl;
   *     });
   */
  std::shared_ptr<HttpFuture> flatMap(TransformCallback transform) {
    // If future is ready, transform immediately
    if (future_.valid() &&
        future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      // Note: exceptions are not used here. If the transform callable throws
      // and exceptions are disabled, the program will terminate. Callers
      // should avoid throwing from transform when building without
      // exceptions (ESP builds).
      HttpResponse resp = future_.get();
      return transform(resp);
    }

    // Otherwise, chain the transformation
    std::shared_ptr<HttpFuture> self = shared_from_this();
    onResponse([self, transform](const HttpResponse &resp) {
      auto next_future = transform(resp);
      if (next_future) {
        // Propagate callbacks to next future if needed
        // (callbacks are already set on the returned future)
      }
    });

    return shared_from_this();
  }

  /**
   * @brief Map/transform the response into a new value
   *
   * @param transform Function to transform the response
   * @return Shared pointer to a new HttpFuture containing the transformed result
   */
  std::shared_ptr<HttpFuture> map(std::function<HttpResponse(const HttpResponse &)> transform) {
    if (future_.valid() &&
        future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      // Note: do not throw in transform when building without exceptions.
      HttpResponse resp = future_.get();
      HttpResponse transformed = transform(resp);
      auto new_future = std::make_shared<HttpFuture>(
          std::async(std::launch::deferred, [transformed]() { return transformed; }));
      return new_future;
    }

    std::shared_ptr<HttpFuture> self = shared_from_this();
    auto mapped_promise = std::make_shared<std::promise<HttpResponse>>();

    onResponse([self, transform, mapped_promise](const HttpResponse &resp) {
      // If transform throws and exceptions are disabled the program will
      // terminate. Prefer not to throw in callbacks when building for ESP.
      HttpResponse transformed = transform(resp);
      mapped_promise->set_value(transformed);
    });

    onError([mapped_promise](const HttpResponse &err) {
      mapped_promise->set_value(err);
    });

    auto new_future = std::make_shared<HttpFuture>(mapped_promise->get_future());
    return new_future;
  }

  /**
   * @brief Block and get the response (synchronous)
   *
   * @return The response when ready
   */
  HttpResponse get() {
    if (future_.valid()) {
      return future_.get();
    }
    HttpResponse error;
    error.status_code = -1;
    error.error_message = "Future is not valid";
    error.success = false;
    return error;
  }

  /**
   * @brief Wait for the response with a timeout
   *
   * @param timeout How long to wait
   * @return true if response is ready, false if timed out
   */
  template <typename Rep, typename Period>
  bool waitFor(const std::chrono::duration<Rep, Period> &timeout) {
    if (!future_.valid())
      return false;
    return future_.wait_for(timeout) == std::future_status::ready;
  }

  /**
   * @brief Check if the response is ready without blocking
   *
   * @return true if response is ready
   */
  bool ready() const {
    if (!future_.valid())
      return false;
    return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
  }

  /**
   * @brief Register a callback to invoke when response is ready (regardless of success/error)
   *
   * @param callback Function to call with the response
   * @return Shared pointer to this HttpFuture for chaining
   */
  std::shared_ptr<HttpFuture> always(std::function<void(const HttpResponse &)> callback) {
    auto cb = [callback](const HttpResponse &resp) { callback(resp); };

    // Register for both success and error
    if (!response_callback_) {
      response_callback_ = cb;
    } else {
      auto existing = response_callback_;
      response_callback_ = [existing, cb](const HttpResponse &resp) {
        existing(resp);
        cb(resp);
      };
    }

    if (!error_callback_) {
      error_callback_ = cb;
    } else {
      auto existing = error_callback_;
      error_callback_ = [existing, cb](const HttpResponse &resp) {
        existing(resp);
        cb(resp);
      };
    }

    checkAndInvoke();
    return shared_from_this();
  }

private:
  std::future<HttpResponse> future_;
  ResponseCallback response_callback_;
  ErrorCallback error_callback_;
  bool is_blocking_;
  mutable std::mutex callback_mutex_;
  std::map<int, ResponseCallback> return_code_callbacks_;
  ResponseCallback return_code_default_callback_;

  /**
   * @brief Check if future is ready and invoke appropriate callback
   */
  void checkAndInvoke() {
    if (!future_.valid())
      return;

    if (future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      // Note: avoid throwing inside futures/callbacks when building without
      // exceptions. future_.get() should return a valid HttpResponse value
      // (implementations should set an error HttpResponse instead of
      // throwing). If an exception does escape and exceptions are disabled,
      // the program will terminate.
      HttpResponse response = future_.get();

      std::lock_guard<std::mutex> lock(callback_mutex_);

      // Check for specific return code handler
      auto it = return_code_callbacks_.find(response.status_code);
      if (it != return_code_callbacks_.end()) {
        it->second(response);
        return;
      }

      // Check for default return code handler
      if (return_code_default_callback_) {
        return_code_default_callback_(response);
        return;
      }

      // Fall back to success/error handlers
      if (response.success && response_callback_) {
        response_callback_(response);
      } else if (!response.success && error_callback_) {
        error_callback_(response);
      }
    }
  }
};

// Convenience type aliases
using HttpFuturePtr = std::shared_ptr<HttpFuture>;
