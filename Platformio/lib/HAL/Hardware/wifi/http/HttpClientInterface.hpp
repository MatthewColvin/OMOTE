#pragma once

#include "HttpTypes.hpp"

/**
 * @brief Abstract HTTP client interface
 *
 * Provides asynchronous HTTP operations returning custom HttpFuture objects
 * that support fluent, monadic callback chaining.
 */
class HttpClientInterface {
public:
  virtual ~HttpClientInterface() = default;

  /**
   * @brief Execute an HTTP request asynchronously
   *
   * @param request The HTTP request configuration
   * @return std::shared_ptr<HttpFuture> A custom future for chaining callbacks
   */
  virtual std::shared_ptr<HttpFuture> executeAsync(const HttpRequest &request) = 0;

  /**
   * @brief Execute an HTTP GET request asynchronously
   *
   * @param url The URL to request
   * @param timeout_ms Timeout in milliseconds
   * @return std::shared_ptr<HttpFuture> A custom future for chaining callbacks
   */
  virtual std::shared_ptr<HttpFuture> getAsync(
      const std::string &url,
      int timeout_ms = 5000) = 0;

  /**
   * @brief Execute an HTTP POST request asynchronously
   *
   * @param url The URL to request
   * @param body The request body
   * @param timeout_ms Timeout in milliseconds
   * @return std::shared_ptr<HttpFuture> A custom future for chaining callbacks
   */
  virtual std::shared_ptr<HttpFuture> postAsync(
      const std::string &url,
      const std::string &body,
      int timeout_ms = 5000) = 0;

  /**
   * @brief Execute an HTTP PUT request asynchronously
   *
   * @param url The URL to request
   * @param body The request body
   * @param timeout_ms Timeout in milliseconds
   * @return std::shared_ptr<HttpFuture> A custom future for chaining callbacks
   */
  virtual std::shared_ptr<HttpFuture> putAsync(
      const std::string &url,
      const std::string &body,
      int timeout_ms = 5000) = 0;

  /**
   * @brief Execute an HTTP DELETE request asynchronously
   *
   * @param url The URL to request
   * @param timeout_ms Timeout in milliseconds
   * @return std::shared_ptr<HttpFuture> A custom future for chaining callbacks
   */
  virtual std::shared_ptr<HttpFuture> deleteAsync(
      const std::string &url,
      int timeout_ms = 5000) = 0;

  /**
   * @brief Set a default header that will be added to all requests
   *
   * @param key The header key
   * @param value The header value
   */
  virtual void setDefaultHeader(const std::string &key, const std::string &value) = 0;

  /**
   * @brief Clear all default headers
   */
  virtual void clearDefaultHeaders() = 0;

  /**
   * @brief Check if the HTTP client is initialized and ready
   *
   * @return true if ready, false otherwise
   */
  virtual bool isReady() const = 0;

  /**
   * @brief Initialize the HTTP client
   *
   * @return true if initialization was successful
   */
  virtual bool initialize() = 0;

  /**
   * @brief Shutdown the HTTP client
   */
  virtual void shutdown() = 0;
};
