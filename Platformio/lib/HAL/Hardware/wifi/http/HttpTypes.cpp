#include "HttpTypes.hpp"

// HttpResponse constructors
HttpResponse::HttpResponse() : status_code(0), body(""), success(false), error_message("") {}

HttpResponse::HttpResponse(int aCode, const std::string &aResponseBody, bool aIsSuccess)
    : status_code(aCode), body(aResponseBody), success(aIsSuccess), error_message("") {}

HttpResponse::HttpResponse(int aCode, const std::string &aResponseBody, const std::string &aError)
    : status_code(aCode), body(aResponseBody), success(false), error_message(aError) {}

// HttpRequest constructors
HttpRequest::HttpRequest() : method(Method::GET), url(""), timeout_ms(5000) {}

HttpRequest::HttpRequest(Method aMethod, const std::string &aUrl)
    : method(aMethod), url(aUrl), timeout_ms(5000) {}

HttpRequest::HttpRequest(Method aMethod, const std::string &aUrl, const std::string &aBody)
    : method(aMethod), url(aUrl), body(aBody), timeout_ms(5000) {}

// HttpFuture implementation
HttpFuture::HttpFuture(std::future<HttpResponse> aFuture)
    : mFuture(std::move(aFuture)),
      mResponseCallback(nullptr),
      mErrorCallback(nullptr),
      mIsBlocking(false),
      mReturnCodeDefaultCallback(nullptr) {}

std::shared_ptr<HttpFuture> HttpFuture::onReturnCode(int aCode, ResponseCallback aCallback) {
  mReturnCodeCallbacks[aCode] = aCallback;
  checkAndInvoke();
  return shared_from_this();
}

std::shared_ptr<HttpFuture> HttpFuture::onReturnCode(const std::vector<int> &aCodes,
                                                     ResponseCallback aCallback) {
  for (int aCode : aCodes) {
    mReturnCodeCallbacks[aCode] = aCallback;
  }
  checkAndInvoke();
  return shared_from_this();
}

std::shared_ptr<HttpFuture> HttpFuture::onOtherReturnCode(ResponseCallback aCallback) {
  mReturnCodeDefaultCallback = aCallback;
  checkAndInvoke();
  return shared_from_this();
}

std::shared_ptr<HttpFuture> HttpFuture::onResponse(ResponseCallback aCallback) {
  mResponseCallback = aCallback;
  checkAndInvoke();
  return shared_from_this();
}

std::shared_ptr<HttpFuture> HttpFuture::onError(ErrorCallback aCallback) {
  mErrorCallback = aCallback;
  checkAndInvoke();
  return shared_from_this();
}

std::shared_ptr<HttpFuture> HttpFuture::then(ResponseCallback aOnResponse, ErrorCallback aOnError) {
  mResponseCallback = aOnResponse;
  mErrorCallback = aOnError;
  checkAndInvoke();
  return shared_from_this();
}

std::shared_ptr<HttpFuture> HttpFuture::flatMap(TransformCallback aTransform) {
  // If future is ready, transform immediately
  if (mFuture.valid() &&
      mFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    // Note: exceptions are not used here. If the transform callable throws
    // and exceptions are disabled, the program will terminate. Callers
    // should avoid throwing from transform when building without
    // exceptions (ESP builds).
    HttpResponse aResp = mFuture.get();
    return aTransform(aResp);
  }

  // Otherwise, chain the transformation
  std::shared_ptr<HttpFuture> aSelf = shared_from_this();
  onResponse([aSelf, aTransform](const HttpResponse &aResp) {
    auto aNextFuture = aTransform(aResp);
    if (aNextFuture) {
      // Propagate callbacks to next future if needed
      // (callbacks are already set on the returned future)
    }
  });

  return shared_from_this();
}

std::shared_ptr<HttpFuture> HttpFuture::map(
    std::function<HttpResponse(const HttpResponse &)> aTransform) {
  if (mFuture.valid() &&
      mFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    // Note: do not throw in transform when building without exceptions.
    HttpResponse aResp = mFuture.get();
    HttpResponse aTransformed = aTransform(aResp);
    auto aNewFuture = std::make_shared<HttpFuture>(
        std::async(std::launch::deferred, [aTransformed]() { return aTransformed; }));
    return aNewFuture;
  }

  std::shared_ptr<HttpFuture> aSelf = shared_from_this();
  auto aMappedPromise = std::make_shared<std::promise<HttpResponse>>();

  onResponse([aSelf, aTransform, aMappedPromise](const HttpResponse &aResp) {
    // If transform throws and exceptions are disabled the program will
    // terminate. Prefer not to throw in callbacks when building for ESP.
    HttpResponse aTransformed = aTransform(aResp);
    aMappedPromise->set_value(aTransformed);
  });

  onError([aMappedPromise](const HttpResponse &aErr) { aMappedPromise->set_value(aErr); });

  auto aNewFuture = std::make_shared<HttpFuture>(aMappedPromise->get_future());
  return aNewFuture;
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

bool HttpFuture::ready() const {
  if (!mFuture.valid())
    return false;
  return mFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

std::shared_ptr<HttpFuture> HttpFuture::always(std::function<void(const HttpResponse &)> aCallback) {
  auto aCb = [aCallback](const HttpResponse &aResp) { aCallback(aResp); };

  // Register for both success and error
  if (!mResponseCallback) {
    mResponseCallback = aCb;
  } else {
    auto aExisting = mResponseCallback;
    mResponseCallback = [aExisting, aCb](const HttpResponse &aResp) {
      aExisting(aResp);
      aCb(aResp);
    };
  }

  if (!mErrorCallback) {
    mErrorCallback = aCb;
  } else {
    auto aExisting = mErrorCallback;
    mErrorCallback = [aExisting, aCb](const HttpResponse &aResp) {
      aExisting(aResp);
      aCb(aResp);
    };
  }

  checkAndInvoke();
  return shared_from_this();
}

void HttpFuture::checkAndInvoke() {
  if (!mFuture.valid())
    return;

  if (mFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    // Note: avoid throwing inside futures/callbacks when building without
    // exceptions. mFuture.get() should return a valid HttpResponse value
    // (implementations should set an error HttpResponse instead of
    // throwing). If an exception does escape and exceptions are disabled,
    // the program will terminate.
    HttpResponse aResponse = mFuture.get();

    std::lock_guard<std::mutex> aLock(mCallbackMutex);

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

    // Fall back to success/error handlers
    if (aResponse.success && mResponseCallback) {
      mResponseCallback(aResponse);
    } else if (!aResponse.success && mErrorCallback) {
      mErrorCallback(aResponse);
    }
  }
}
