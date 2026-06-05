#if !defined(IS_SIMULATOR)
#include "esp32WebSocket.hpp"

#include "HardwareFactory.hpp"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "nvs_flash.h"

esp32WebSocket::esp32WebSocket(std::shared_ptr<wifiHandler> aWifiHandler, std::unique_ptr<LoggingInterface> aLogger = nullptr)
    : mWifiHandler(aWifiHandler),
      mLogger(std::move(aLogger)),
      client(nullptr),
      mIncomingMessage(),
      mWifiStatusUpdateHandler(mWifiHandler->WifiStatusNotification()) {
  mLogger->setLogModule(LogModule::WebSocket);
  mWifiStatusUpdateHandler =
      [this](wifiHandlerInterface::wifiStatus aWifiStatus) {
        if (aWifiStatus.isConnected) {
          connect(mUri);
        }
      };
}

void esp32WebSocket::connect(const std::string &url) {
  mUri = url;
  if (client && !isConnected()) {
    disconnect();
  }
  if (mWifiHandler->GetStatus().isConnected) {
    mConfig.uri = mUri.c_str();

    client = esp_websocket_client_init(&mConfig);
    if (!client) {
      mLogger->error("Failed to init Client");
      return;
    }

    auto err = esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY,
                                             websocket_event_handler, this);
    if (err != ESP_OK) {
      mLogger->error("Failed to register event handler");
      return;
    }
    err = esp_websocket_client_start(client);
    if (err != ESP_OK) {
      mLogger->error("Failed to start Client");
      return;
    }

  } else {
    mLogger->error("Cannot Connect Websocket Without wifi connected");
  }
}

void esp32WebSocket::disconnect() {
  if (client) {
    esp_websocket_client_stop(client);
    esp_websocket_client_destroy(client);
    client = nullptr;
  }
}

void esp32WebSocket::sendMessage(const std::string &message) {
  if (isConnected() && client) {
    if (mLogger && mLogger->debug("Sending message")) {
      mLogStream << "Sending message: " << message;
      mLogger->debug(mLogStream);
    }

    esp_websocket_client_send_text(client, message.c_str(), message.length(),
                                   portMAX_DELAY);
  }
}

void esp32WebSocket::setMessageCallback(MessageCallback callback) {
  messageCallback = callback;
}

void esp32WebSocket::proccessEventData(esp_websocket_event_data_t *aEventData) {
  // Todo is this a standard timeout message?
  if (aEventData->op_code == 0x08 && aEventData->data_len == 2) {
    if (mLogger && mLogger->debug("Received close message")) {
      mLogStream << "code=" << 256 * aEventData->data_ptr[0] + aEventData->data_ptr[1];
      mLogger->debug(mLogStream);
    }
    return;
  }
  bool IsStartOfMessage = aEventData->payload_offset == 0;
  if (IsStartOfMessage) {
    mPartialProcessingFailed = false;
  }

  if (aEventData->data_len < 0) {
    return;
  }
  auto dataLength = static_cast<uint>(aEventData->data_len);
  auto nextStep = getNextStep(aEventData);
  printDebugInfo(aEventData, nextStep);
  using Step = ProcessingStep;
  switch (nextStep) {
  case Step::Reserve:
    mIncomingMessage.reserve(aEventData->payload_len);
    [[fallthrough]];
  case Step::Append:
    mIncomingMessage += {aEventData->data_ptr, dataLength};
    break;
  case Step::Partial:
    if (mJsonHandler) {
      if (IsStartOfMessage) {
        // Grab 10% of current heap to process chunkwise message on
        auto freeHeap = esp_get_free_heap_size();
        auto buffSize = freeHeap * .10;
        mJsonHandler->SetMaxProcessBufferSize(buffSize);
      }
      mPartialProcessingFailed = !mJsonHandler->ProcessChunk(
          {aEventData->data_ptr, dataLength}, aEventData->payload_len);
    }
    break;
  case Step::Drop:
    break;
  default:
    break;
  }

  auto isMessageGathered = aEventData->payload_len == mIncomingMessage.length();
  if (isMessageGathered) {
    processStoredMessage();
    mIncomingMessage.clear();
    mIncomingMessage.shrink_to_fit();
  }
}

void esp32WebSocket::processStoredMessage() {
  // Give JsonHandler a crack at it and
  // if not pass it down to the string handler
  if (mJsonHandler && mJsonHandler->ProcessJsonAsDoc(mIncomingMessage)) {
    return;
  }
  if (messageCallback) {
    messageCallback(mIncomingMessage);
  }
}

esp32WebSocket::ProcessingStep esp32WebSocket::getNextStep(
    esp_websocket_event_data_t *aEventData) const {
  if (aEventData->data_len == 0) {
    return ProcessingStep::Drop;
  }

  if (mJsonHandler && mJsonHandler->IsChunkProcessingPrefered()) {
    return ProcessingStep::Partial;
  }

  if (mIncomingMessage.empty() &&
      aEventData->payload_len == aEventData->data_len) {
    return ProcessingStep::Reserve;
  }

  const auto IsStartOfMultiEventData =
      aEventData->payload_len > aEventData->data_len &&
      aEventData->payload_offset == 0;
  if (IsStartOfMultiEventData) {
    return getStartStep(aEventData);
  }

  auto inMiddleOfPartialProcMessage =
      aEventData->payload_offset > 0 && mIncomingMessage.empty();

  if (inMiddleOfPartialProcMessage && !mPartialProcessingFailed) {
    return ProcessingStep::Partial;
  }

  return !mIncomingMessage.empty() ? ProcessingStep::Append
                                   : ProcessingStep::Drop;
}

esp32WebSocket::ProcessingStep esp32WebSocket::getStartStep(
    esp_websocket_event_data_t *aEventData) const {
  auto step = ProcessingStep::Drop;

  auto freeHeap = esp_get_free_heap_size();
  auto isProccessInMemory = aEventData->payload_len < (freeHeap / 2);
  if (isProccessInMemory) {
    step = ProcessingStep::Reserve;
  } else {
    if (mJsonHandler && mJsonHandler->HasChunkProcessor()) {
      step = ProcessingStep::Partial;
    } else {
      if (mLogger->error("Cannot Process message no partial handler")) {
        mLogStream << " size:"
                   << aEventData->payload_len
                   << " FreeHeap:" << freeHeap;
        mLogger->error(mLogStream);
      }
    }
  }
  return step;
}

void esp32WebSocket::printDebugInfo(esp_websocket_event_data_t *aEventData,
                                    ProcessingStep aNextStep) {
  if (!aEventData) {
    return;
  }
  if (mLogger && mLogger->debug("Received aEventData")) {
    mLogStream << "Received aEventData: opcode=" << aEventData->op_code
               << ", payload_len=" << aEventData->payload_len
               << ", data_len=" << aEventData->data_len
               << ", data_offset=" << aEventData->payload_offset
               << ", NextStep =" << static_cast<int>(aNextStep);
    mLogger->debug(mLogStream);
  }
}

void esp32WebSocket::websocket_event_handler(void *handler_args,
                                             esp_event_base_t base,
                                             int32_t event_id,
                                             void *event_data) {
  esp32WebSocket *self = static_cast<esp32WebSocket *>(handler_args);

  switch (event_id) {
  case WEBSOCKET_EVENT_CONNECTED:
    self->mLogger->info("WebSocket connected");
    self->Connected();
    break;
  case WEBSOCKET_EVENT_DISCONNECTED:
    self->mLogger->info("WebSocket disconnected");
    self->Disconnected();
    break;
  case WEBSOCKET_EVENT_ERROR:
    self->mLogger->error("WebSocket error");
    break;
  case WEBSOCKET_EVENT_DATA:
    self->proccessEventData(
        static_cast<esp_websocket_event_data_t *>(event_data));
    break;
  default:
    break;
  }
}
#endif // !IS_SIMULATOR

