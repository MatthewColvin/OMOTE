#include "WebSocket/Session/Session.hpp"

#include "WebSocket/Message/MessageHandler.hpp"
#include "WebSocket/Request.hpp"

namespace HomeAssist::WebSocket {

Session::Session(std::unique_ptr<Request> aStartRequest,
                 std::unique_ptr<Request> aEndRequest,
                 std::shared_ptr<MessageHandler> aMessageHandler,
                 std::shared_ptr<Json::IChunkProcessor> aChunkProcessor)
    : mStartRequest(std::move(aStartRequest)),
      mEndRequest(std::move(aEndRequest)),
      mMessageHandler(aMessageHandler),
      mChunkProcessor(aChunkProcessor) {}

bool Session::ProcessMessage(const Message& aMessage) {
  if (auto handler = mMessageHandler.lock(); handler) {
    return handler->ProcessMessage(aMessage);
  }
  return false;
}
std::shared_ptr<Json::IChunkProcessor> Session::GetChunkProcessor() {
  return mChunkProcessor.lock();
}

bool Session::IsComplete() const {
  // Our message handler and chunk processor are no longer active so we are done
  auto isSomethingListening =
      mMessageHandler.use_count() == 0 && mChunkProcessor.use_count() == 0;
  auto isStartRequestSent = mStartRequest == nullptr;

  return isSomethingListening && isStartRequestSent;
}

std::unique_ptr<Request> Session::GetStartRequest() {
  return std::move(mStartRequest);
}

Request* Session::BorrowStartRequest() { return mStartRequest.get(); }

Request* Session::BorrowEndRequest() { return mEndRequest.get(); }

bool Session::IsPreferringChunkProcessing() {
  return mMessageHandler.expired() && !mChunkProcessor.expired();
}

}  // namespace HomeAssist::WebSocket