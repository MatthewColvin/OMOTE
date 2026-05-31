#include "HttpTypes.hpp"

// HttpResponse constructors
HttpResponse::HttpResponse() : status_code(0), body(""), success(false), error_message("") {}

HttpResponse::HttpResponse(int aCode, const std::string &aResponseBody, bool aIsSuccess)
    : status_code(aCode), body(aResponseBody), success(aIsSuccess), error_message("") {}

HttpResponse::HttpResponse(int aCode, const std::string &aResponseBody, const std::string &aError)
    : status_code(aCode), body(aResponseBody), success(false), error_message(aError) {}

// HttpRequest constructors
HttpRequest::HttpRequest() : method(Method::Get), url(""), timeout_ms(5000) {}

HttpRequest::HttpRequest(Method aMethod, const std::string &aUrl)
    : method(aMethod), url(aUrl), timeout_ms(5000) {}

HttpRequest::HttpRequest(Method aMethod, const std::string &aUrl, const std::string &aBody)
    : method(aMethod), url(aUrl), body(aBody), timeout_ms(5000) {}

// HttpFuture implementation
HttpFuture::HttpFuture(std::future<HttpResponse> aFuture)
    : mFuture(std::move(aFuture)),
      mReturnCodeDefaultCallback(nullptr) {}

std::shared_ptr<HttpFuture> HttpFuture::onReturnCode(int aCode, ResponseCallback aCallback) {
  mReturnCodeCallbacks[aCode] = aCallback;
  return shared_from_this();
}

std::shared_ptr<HttpFuture> HttpFuture::onReturnCode(const std::vector<int> &aCodes,
                                                     ResponseCallback aCallback) {
  for (int aCode : aCodes) {
    mReturnCodeCallbacks[aCode] = aCallback;
  }
  return shared_from_this();
}

std::shared_ptr<HttpFuture> HttpFuture::onOtherReturnCode(ResponseCallback aCallback) {
  mReturnCodeDefaultCallback = aCallback;
  return shared_from_this();
}

HttpResponse HttpFuture::get() {
  if (mFuture.valid()) {
    return mFuture.get();
  }
  HttpResponse aError;
  aError.status_code = -1;
  aError.error_message = "Future is not valid";
  aError.success = false;
  return aError;
}

bool HttpFuture::IsReady() const {
  if (!mFuture.valid())
    return false;
  return mFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void HttpFuture::checkAndInvoke() {
  if (!IsReady()) {
    return;
  }

  HttpResponse aResponse = mFuture.get();

  std::lock_guard<std::mutex> aLock(mCallbackMutex);

  // Call the always handler if it exists
  if (mAlwaysCallback) {
    mAlwaysCallback(aResponse);
  }

  // Check for specific return code handler
  auto aIt = mReturnCodeCallbacks.find(aResponse.status_code);
  if (aIt != mReturnCodeCallbacks.end()) {
    aIt->second(aResponse);
    return;
  }

  // Check for default return code handler
  if (mReturnCodeDefaultCallback) {
    mReturnCodeDefaultCallback(aResponse);
    return;
  }
}

std::shared_ptr<HttpFuture> HttpFuture::always(std::function<void(const HttpResponse &)> aCallback) {
  // Add the callback to be called whenever we get a response
  std::lock_guard<std::mutex> aLock(mCallbackMutex);
  mAlwaysCallback = aCallback;
  return shared_from_this();
}
