#include "IProcessMessage.hpp"

#include <algorithm>

#include "IChunkProcessor.hpp"

namespace Json {

IProcessMessage::ProcessResult::ProcessResult(ProcessResult::StatusCode aStatus,
                                              rapidjson::ParseResult aResult)
    : mStatus(aStatus), mParseResult(aResult) {}

IProcessMessage::ProcessResult::ProcessResult(rapidjson::ParseResult aResult)
    : mParseResult(aResult) {
  mStatus = mParseResult.IsError() ? ProcessResult::StatusCode::ParseError
                                   : ProcessResult::StatusCode::Success;
}

IProcessMessage::ProcessResult::ProcessResult(rapidjson::ParseErrorCode aError)
    : mStatus(StatusCode::ParseError), mParseResult(aError, 0) {}

IProcessMessage::ProcessResult::operator bool() const {
  using Result = ProcessResult::StatusCode;
  return mStatus == Result::Success ||
         mStatus == Result::SuccessFinishedChunkParse ||
         mStatus == Result::SuccessWaitingForNextChunk;
}

IProcessMessage::IProcessMessage() : mChunkStream(mUnprocessedBuffer.data()) {};

IProcessMessage::IProcessMessage(
    DocumentProccessor aDocProcessor,
    std::unique_ptr<IChunkProcessor> aChunkProcessor)
    : mDocProcessor(aDocProcessor),
      mChunkProcessor(std::move(aChunkProcessor)),
      mChunkStream(mUnprocessedBuffer.data()) {}

IProcessMessage::~IProcessMessage() = default;

bool IProcessMessage::ProcessDocument(
    const MemConciousDocument &aRecievedDocument) {
  if (mDocProcessor) {
    return mDocProcessor(aRecievedDocument);
  }
  return false;
}

bool IProcessMessage::HasChunkProcessor() { return mChunkProcessor != nullptr; }

// MatthewColvin/OMOTE#6 pass final size in here and track currently processed
// byte to allow for for calling status update callback
IProcessMessage::ProcessResult IProcessMessage::ProcessChunk(
    const std::string &aJsonChunk, const size_t aTotalJsonSize) {
  if (!mChunkProcessor) {
    return {ProcessResult::StatusCode::MissingChunkProcessor};
  }
  if (!IsProcessingChunks()) {
    mChunkReader.IterativeParseInit();
    mCurrentChunkBasedTotalJsonSize = aTotalJsonSize;
    mMaxBufferSize =
        mMaxBufferSize == 0 ? DefaultMaxBufferSize : mMaxBufferSize;
    mUnprocessedBuffer.reserve(mMaxBufferSize);
  } else {
    if (aTotalJsonSize != mCurrentChunkBasedTotalJsonSize) {
      return {
          ProcessResult::StatusCode::FailedChunkProcessingMessageSizeChanged};
    }
  }

  if (mUnprocessedBuffer.size() + aJsonChunk.length() > mMaxBufferSize) {
    return {ProcessResult::StatusCode::FailedChunkProcessorMemoryLimitMet};
  }

  mUnprocessedBuffer.insert(mUnprocessedBuffer.end(), aJsonChunk.begin(),
                            aJsonChunk.end());

  mChunkStream = {mUnprocessedBuffer.data()};
  auto lastSuccessfulReadIndex = 0;

  while (!mChunkReader.IterativeParseComplete()) {
    if (IsChunkBufferToSmallForProcessing()) {
      return {ProcessResult::StatusCode::SuccessWaitingForNextChunk};
    }
    auto IsChunkParseSuccess =
        mChunkReader.IterativeParseNext<rapidjson::kParseIterativeFlag>(
            mChunkStream, *mChunkProcessor);
    if (IsChunkParseSuccess) {
      UpdateBufferAndMetaData();
    } else {
      UpdateBufferAndMetaData();
      if (mOffsetIntoChunkBasedJson == aTotalJsonSize) {
        break;
      }
      ProcessResult result = {
          ProcessResult::StatusCode::ParseError,
          {mChunkReader.GetParseErrorCode(), mOffsetIntoChunkBasedJson}};
      EndChunkProcessing(result);
      return result;
    }
  }
  ProcessResult result = {ProcessResult::StatusCode::SuccessFinishedChunkParse};
  EndChunkProcessing(result);
  return result;
}

bool IProcessMessage::IsProcessingChunks() const {
  return mCurrentChunkBasedTotalJsonSize != ChunkMessageSizeNotProcessing;
}
void IProcessMessage::EndChunkProcessing(
    const ProcessResult &aResultToEndWith) {
  mCurrentChunkBasedTotalJsonSize = ChunkMessageSizeNotProcessing;
  mOffsetIntoChunkBasedJson = 0;
  if (mChunkProcessor) {
    mChunkProcessor->Completed(aResultToEndWith);
  }
  // Clear the buffer
  mUnprocessedBuffer.clear();
  mUnprocessedBuffer.shrink_to_fit();
}

bool IProcessMessage::IsChunkBufferToSmallForProcessing() const {
  if (mUnprocessedBuffer.size() == 0) {
    return true;
  }
  // TODO: Consider this lookAheadsize abit more
  const auto lookAheadBytes = mMaxBufferSize * .5;
  auto bytesLeftToProcess = mUnprocessedBuffer.size() - mChunkStream.Tell();
  auto isBufferTooSmall = bytesLeftToProcess < lookAheadBytes;

  auto isLastChunkRecieved =
      mOffsetIntoChunkBasedJson + mUnprocessedBuffer.size() ==
      mCurrentChunkBasedTotalJsonSize;

  return isBufferTooSmall && !isLastChunkRecieved;
}

void IProcessMessage::UpdateBufferAndMetaData() {
  auto bytesProcesssed = mChunkStream.Tell();
  auto unProcessedBytes = mUnprocessedBuffer.size() - bytesProcesssed;
  // Update our offset since we processed all data in to where tell is at
  mOffsetIntoChunkBasedJson += bytesProcesssed;
  // Throw out already processed data with rotation as to not cause realloc
  std::rotate(mUnprocessedBuffer.begin(),
              mUnprocessedBuffer.begin() + bytesProcesssed,
              mUnprocessedBuffer.end());
  mUnprocessedBuffer.resize(unProcessedBytes);
  // Setup Stream for next parse
  mChunkStream = {mUnprocessedBuffer.data()};
  // Notify Processor we processed some data
  mChunkProcessor->UpdateProgress(mOffsetIntoChunkBasedJson,
                                  mCurrentChunkBasedTotalJsonSize);
}

IProcessMessage::ProcessResult IProcessMessage::ProcessJsonAsDoc(
    const std::string &aJsonString) {
  MemConciousDocument aDoc;
  aDoc.Parse(aJsonString.data());
  if (aDoc.HasParseError()) {
    return {aDoc.GetParseError()};
  }
  if (!ProcessDocument(aDoc)) {
    return {ProcessResult::StatusCode::DocProcessorFailed};
  }
  return {ProcessResult::StatusCode::Success};
}

bool IProcessMessage::IsChunkProcessingPrefered() {
  // Only processor we have is chunk probably use it.
  return mChunkProcessor && !mDocProcessor;
}

void IProcessMessage::SetMaxProcessBufferSize(size_t aProcessBufferSize) {
  mMaxBufferSize = aProcessBufferSize;
  mUnprocessedBuffer.shrink_to_fit();
  mUnprocessedBuffer.reserve(mMaxBufferSize);
}

size_t IProcessMessage::GetMaxProcessBufferSize() const {
  return mMaxBufferSize;
}

size_t IProcessMessage::GetUnProcessedBufferCapacity() {
  return mUnprocessedBuffer.capacity();
}

} // namespace Json
