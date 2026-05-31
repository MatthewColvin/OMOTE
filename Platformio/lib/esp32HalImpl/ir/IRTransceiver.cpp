#include "IRTransceiver.hpp"
#include "magic_enum.hpp"
#include "omoteconfig.h"
#include <IRutils.h>
#include <sstream>

struct callbackStruct {
  MessageBufferHandle_t IRSendHandle;
  IRTransceiver *thisPtr;
};

static callbackStruct CallbackData;

IRTransceiver::IRTransceiver(std::unique_ptr<LoggingInterface> aLogger)
    :
#ifdef OMOTE_KEYBRD_3661
      IRsend(IR_LED, false), // IR driver not inverted on 3661 hardware
#else
      IRsend(IR_LED, true),
#endif
      IRrecv(IR_RX, 1024, 50, true),
      mLog(std::move(aLogger)) {

  if (mLog) {
    mLog->setLogModule(LogModule::IR);
  }

  IRsend::begin();
  // calibrateTx();  //slows boot slightly so include only if default IR period is not correct

  digitalWrite(IR_VCC, HIGH); // Turn on IR receiver

  CallbackData.thisPtr = this;
  CallbackData.IRSendHandle = xMessageBufferCreate(4096);
  xTaskCreate(IRSendTask, "IRSendTask", 4096, &CallbackData, configMAX_PRIORITIES - 1, &mIRSendTask);
}

IRTransceiver::~IRTransceiver() {
  digitalWrite(IR_VCC, LOW); // Turn off IR receiver
}

void IRTransceiver::IRSendTask(void *aStruct) {
  callbackStruct *cs = (callbackStruct *)aStruct;

  char RxMessBuff[MAX_MESSAGE_SIZE];
  while (true) {
    auto rxSize = xMessageBufferReceive(cs->IRSendHandle, RxMessBuff, MAX_MESSAGE_SIZE, portMAX_DELAY);
    // Serial.print("IR Send, ");
    if (rxSize > 1) {
      // extract protocol from first byte
      auto protocol = magic_enum::enum_cast<IRInterface::protocol>(RxMessBuff[0]);
      if (protocol.has_value()) {
        std::string command;
        command.assign(&RxMessBuff[1], &RxMessBuff[0] + rxSize);
        if (cs->thisPtr->mLog && cs->thisPtr->mLog->isPrintWanted(LogLevel::Debug)) {
          std::stringstream info;
          info << "IRSendTask, send with protocol:" << magic_enum::enum_name(protocol.value()) << " with command:" << command;
          cs->thisPtr->mLog->debug(info);
        }

        uint16_t repeat = 0;
        auto pos = command.find(':');
        if (pos != std::string::npos)
          repeat = std::stoul(command.substr(pos + 1));

        if (IRInterface::protocol::DELAY == protocol.value()) {
          uint32_t msec = std::stoul(command, nullptr, 0);
          if (cs->thisPtr->mLog && cs->thisPtr->mLog->isPrintWanted(LogLevel::Debug)) {
            std::stringstream info;
            info << "IRSendTask: Sending Delay: " << msec << "ms";
            cs->thisPtr->mLog->debug(info);
          }
          if (msec < 10000)
            vTaskDelay(msec / portTICK_PERIOD_MS);
        } else {
          auto intVal = magic_enum::enum_integer(protocol.value());

          if (magic_enum::enum_contains<IRInterface::constInt64SendTypes>(intVal)) {
            if (cs->thisPtr->mLog)
              cs->thisPtr->mLog->debug("IRSendTask, constInt64SendTypes");
            cs->thisPtr->send((IRInterface::constInt64SendTypes)intVal, std::stoull(command, nullptr, 0));
          } else if (magic_enum::enum_contains<IRInterface::int64SendTypes>(intVal)) {
            if (cs->thisPtr->mLog)
              cs->thisPtr->mLog->debug("IRSendTask, int64SendTypes");
            cs->thisPtr->send((IRInterface::int64SendTypes)intVal, std::stoull(command, nullptr, 0));
          } else if (magic_enum::enum_contains<IRInterface::int16SendTypes>(intVal)) {
            if (cs->thisPtr->mLog)
              cs->thisPtr->mLog->debug("IRSendTask, int16SendTypes");
            std::vector<uint16_t> dataArray;
            std::stringstream ss(command);
            while (ss.good()) {
              std::string values;
              std::getline(ss, values, ',');
              dataArray.push_back(std::stoul(values, nullptr, 16));
            }
            std::stringstream info;
            info << "Repeat: " << repeat;
            cs->thisPtr->mLog->debug(info);
            cs->thisPtr->send((IRInterface::int16SendTypes)intVal, dataArray, repeat);
          } else if (magic_enum::enum_contains<IRInterface::charArrSendType>(intVal)) {
            if (cs->thisPtr->mLog)
              cs->thisPtr->mLog->debug("IRSendTask, charArrSendType");
            std::vector<uint8_t> dataArray;
            std::stringstream ss(command);
            while (ss.good()) {
              std::string values;
              std::getline(ss, values, ',');
              dataArray.push_back(std::stoul(values, nullptr, 0));
            }
            cs->thisPtr->send((IRInterface::charArrSendType)intVal, dataArray.data());
          }
          vTaskDelay(50 / portTICK_PERIOD_MS); // don't process another command for 50ms
        }
      }
    }
  }
}

void IRTransceiver::sendBackground(std::string aProtocol, std::vector<std::string> data) {
  auto protocol = magic_enum::enum_cast<IRInterface::protocol>(aProtocol);
  // Serial.println("Sending IR");
  if (protocol.has_value() && (data.size() > 0)) {
    int8_t protocolAsInt = magic_enum::enum_integer(protocol.value());
    std::string commInclProtocol = data[0];
    commInclProtocol.insert(0, 1, protocolAsInt);
    if (commInclProtocol.size() < MAX_MESSAGE_SIZE) {
      size_t numBytes = xMessageBufferSend(CallbackData.IRSendHandle, commInclProtocol.data(), commInclProtocol.size(), 0); // don't block
    }
  }
}

void IRTransceiver::send(int16SendTypes protocol, std::vector<uint16_t> &data, uint16_t repeat) {
  if (mLog && mLog->isPrintWanted(LogLevel::Info)) {
    std::stringstream info;
    info << "IR Send int16:" << magic_enum::enum_name(protocol) << ", repeat of:" << repeat << " with " << data.size() << " words of data:";
    for (auto x : data)
      info << x << ", ";
    mLog->info(info);
  }
  if (mIsRxEnabled) {
    IRrecv::pause();
  }
  maxOutTaskPriority();
  switch (protocol) {
  case int16SendTypes::Pronto:
    return sendPronto(data.data(), data.size(), repeat);
  }
}

void IRTransceiver::send(int64SendTypes protocol, uint64_t data) {
  if (mLog && mLog->info("IR Send int64:")) {
    std::stringstream info;
    info << magic_enum::enum_name(protocol) << " with data " << data;
    mLog->info(info);
  }
  if (mIsRxEnabled) {
    IRrecv::pause();
  }
  maxOutTaskPriority();
  switch (protocol) {
  case int64SendTypes::NEC:
    return sendNEC(data);
  case int64SendTypes::Sherwood:
    return sendSherwood(data);
  case int64SendTypes::LG:
    return sendLG(data);
  case int64SendTypes::LG2:
    return sendLG2(data);
  case int64SendTypes::JVC:
    return sendJVC(data);
  case int64SendTypes::Denon:
    return sendDenon(data);
  case int64SendTypes::DISH:
    return sendDISH(data);
  case int64SendTypes::RCMM:
    return sendRCMM(data);
  case int64SendTypes::Mitsubishi:
    return sendMitsubishi(data);
  case int64SendTypes::Mitsubishi2:
    return sendMitsubishi2(data);
  case int64SendTypes::AiwaRCT501:
    return sendAiwaRCT501(data);
  case int64SendTypes::Nikai:
    return sendNikai(data);
  case int64SendTypes::Midea:
    return sendMidea(data);
  case int64SendTypes::Lasertag:
    return sendLasertag(data);
  case int64SendTypes::CarrierAC:
    return sendCarrierAC(data);
  case int64SendTypes::CarrierAC40:
    return sendCarrierAC40(data);
  case int64SendTypes::CarrierAC64:
    return sendCarrierAC64(data);
  case int64SendTypes::GICable:
    return sendGICable(data);
  case int64SendTypes::Lutron:
    return sendLutron(data);
  case int64SendTypes::Epson:
    return sendEpson(data);
  case int64SendTypes::Symphony:
    return sendSymphony(data);
  case int64SendTypes::Airwell:
    return sendAirwell(data);
  case int64SendTypes::DelonghiAc:
    return sendDelonghiAc(data);
  case int64SendTypes::TechnibelAc:
    return sendTechnibelAc(data);
  }
  restoreTaskPriority();
  if (mIsRxEnabled) {
    IRrecv::resume();
  }
};
void IRTransceiver::send(constInt64SendTypes protocol, const uint64_t data) {
  if (mLog && mLog->info("IR Send constint64:")) {
    std::stringstream info;
    info << magic_enum::enum_name(protocol) << " with data " << data;
    mLog->info(info);
  }
  if (mIsRxEnabled) {
    IRrecv::pause();
  }
  maxOutTaskPriority();
  switch (protocol) {
  case constInt64SendTypes::Sony:
    return sendSony(data);
  case constInt64SendTypes::Sony15:
    // TODO: What is 15, 4?
    return sendSony(data, 15, 4);
  case constInt64SendTypes::Sony38:
    return sendSony38(data);
  case constInt64SendTypes::SAMSUNG:
    return sendSAMSUNG(data);
  case constInt64SendTypes::Samsung36:
    return sendSamsung36(data);
  case constInt64SendTypes::SharpRaw:
    return sendSharpRaw(data);
  case constInt64SendTypes::SanyoLC7461:
    return sendSanyoLC7461(data);
  case constInt64SendTypes::Panasonic64:
    return sendPanasonic64(data);
  case constInt64SendTypes::RC5:
    return sendRC5(data);
  case constInt64SendTypes::RC6:
    return sendRC6(data);
  case constInt64SendTypes::COOLIX:
    return sendCOOLIX(data);
  case constInt64SendTypes::Coolix48:
    return sendCoolix48(data);
  case constInt64SendTypes::Whynter:
    return sendWhynter(data);
  case constInt64SendTypes::Inax:
    return sendInax(data);
  case constInt64SendTypes::Daikin64:
    return sendDaikin64(data);
  case constInt64SendTypes::Gree:
    return sendGree(data);
  case constInt64SendTypes::Goodweather:
    return sendGoodweather(data);
  case constInt64SendTypes::Gorenje:
    return sendGorenje(data);
  case constInt64SendTypes::Midea24:
    return sendMidea24(data);
  case constInt64SendTypes::MagiQuest:
    return sendMagiQuest(data);
  case constInt64SendTypes::PanasonicAC32:
    return sendPanasonicAC32(data);
  case constInt64SendTypes::Pioneer:
    return sendPioneer(data);
  case constInt64SendTypes::VestelAc:
    return sendVestelAc(data);
  case constInt64SendTypes::Teco:
    return sendTeco(data);
  case constInt64SendTypes::LegoPf:
    return sendLegoPf(data);
  case constInt64SendTypes::Doshisha:
    return sendDoshisha(data);
  case constInt64SendTypes::Multibrackets:
    return sendMultibrackets(data);
  case constInt64SendTypes::Zepeal:
    return sendZepeal(data);
  case constInt64SendTypes::Metz:
    return sendMetz(data);
  case constInt64SendTypes::Transcold:
    return sendTranscold(data);
  case constInt64SendTypes::Elitescreens:
    return sendElitescreens(data);
  case constInt64SendTypes::Milestag2:
    return sendMilestag2(data);
  case constInt64SendTypes::Ecoclim:
    return sendEcoclim(data);
  case constInt64SendTypes::Xmp:
    return sendXmp(data);
  case constInt64SendTypes::Truma:
    return sendTruma(data);
  case constInt64SendTypes::Kelon:
    return sendKelon(data);
  case constInt64SendTypes::Bose:
    return sendBose(data);
  case constInt64SendTypes::Arris:
    return sendArris(data);
  case constInt64SendTypes::Airton:
    return sendAirton(data);
  case constInt64SendTypes::Toto:
    return sendToto(data);
  case constInt64SendTypes::ClimaButler:
    return sendClimaButler(data);
  case constInt64SendTypes::Wowwee:
    return sendWowwee(data);
  }
  restoreTaskPriority();
  if (mIsRxEnabled) {
    IRrecv::resume();
  }
};
void IRTransceiver::send(charArrSendType protocol, const unsigned char data[]) {
  if (mLog && mLog->info("IR Send charArr:")) {
    std::stringstream info;
    info << magic_enum::enum_name(protocol) << " with data " << data;
    mLog->info(info);
  }
  if (mIsRxEnabled) {
    IRrecv::pause();
  }
  maxOutTaskPriority();
  switch (protocol) {
  case charArrSendType::SamsungAC:
    return sendSamsungAC(data);
  case charArrSendType::SharpAc:
    return sendSharpAc(data);
  case charArrSendType::Mirage:
    return sendMirage(data);
  case charArrSendType::Mitsubishi136:
    return sendMitsubishi136(data);
  case charArrSendType::Mitsubishi112:
    return sendMitsubishi112(data);
  case charArrSendType::MitsubishiAC:
    return sendMitsubishiAC(data);
  // case charArrSendType::FujitsuAC: return sendFujitsuAC(data); add other args
  // check lib version
  case charArrSendType::Kelvinator:
    return sendKelvinator(data);
  case charArrSendType::Daikin:
    return sendDaikin(data);
  case charArrSendType::Daikin128:
    return sendDaikin128(data);
  case charArrSendType::Daikin152:
    return sendDaikin152(data);
  case charArrSendType::Daikin160:
    return sendDaikin160(data);
  case charArrSendType::Daikin176:
    return sendDaikin176(data);
  case charArrSendType::Daikin2:
    return sendDaikin2(data);
  case charArrSendType::Daikin200:
    return sendDaikin200(data);
  case charArrSendType::Daikin216:
    return sendDaikin216(data);
  case charArrSendType::Daikin312:
    return sendDaikin312(data);
  case charArrSendType::Argo:
    return sendArgo(data);
    // Can't Rx it?
    //   case charArrSendType::ArgoWREM3:
    //     return sendArgoWREM3(data);
  case charArrSendType::Trotec:
    return sendTrotec(data);
  case charArrSendType::Trotec3550:
    return sendTrotec3550(data);
  case charArrSendType::HaierAC:
    return sendHaierAC(data);
  case charArrSendType::HaierACYRW02:
    return sendHaierACYRW02(data);
  case charArrSendType::HaierAC160:
    return sendHaierAC160(data);
  case charArrSendType::HaierAC176:
    return sendHaierAC176(data);
  case charArrSendType::HitachiAC:
    return sendHitachiAC(data);
  case charArrSendType::HitachiAC1:
    return sendHitachiAC1(data);
  case charArrSendType::HitachiAC2:
    return sendHitachiAC2(data);
  // case charArrSendType::HitachiAc3: return sendHitachiAc3(data); Add other
  // args
  case charArrSendType::HitachiAc264:
    return sendHitachiAc264(data);
  case charArrSendType::HitachiAc296:
    return sendHitachiAc296(data);
  case charArrSendType::HitachiAc344:
    return sendHitachiAc344(data);
  case charArrSendType::HitachiAc424:
    return sendHitachiAc424(data);
  case charArrSendType::WhirlpoolAC:
    return sendWhirlpoolAC(data);
  case charArrSendType::ElectraAC:
    return sendElectraAC(data);
  case charArrSendType::PanasonicAC:
    return sendPanasonicAC(data);
  // case charArrSendType::MWM: return sendMWM(data); TODO find good way to add
  // other args
  case charArrSendType::Tcl96Ac:
    return sendTcl96Ac(data);
  case charArrSendType::Tcl112Ac:
    return sendTcl112Ac(data);
  case charArrSendType::Neoclima:
    return sendNeoclima(data);
  case charArrSendType::Amcor:
    return sendAmcor(data);
  case charArrSendType::Voltas:
    return sendVoltas(data);
  case charArrSendType::Teknopoint:
    return sendTeknopoint(data);
  case charArrSendType::Kelon168:
    return sendKelon168(data);
  case charArrSendType::Rhoss:
    return sendRhoss(data);
  case charArrSendType::Bosch144:
    return sendBosch144(data);
  case charArrSendType::York:
    return sendYork(data);
  }
  restoreTaskPriority();
  if (mIsRxEnabled) {
    IRrecv::resume();
  }
};

void IRTransceiver::send(IRInterface::RawIR aRawIR) {
  if (mIsRxEnabled) {
    IRrecv::pause();
  }
  // Serial.print(aRawIR.data.size());
  // Serial.print("Sending:");
  // for (auto x : aRawIR.data) {
  //   Serial.print(x);
  //   Serial.print(":");
  // }
  // Serial.println("");
  maxOutTaskPriority();
  sendRaw(aRawIR.data.data(), aRawIR.data.size(), 38);
  restoreTaskPriority();
  if (mIsRxEnabled) {
    IRrecv::resume();
  }
}

void IRTransceiver::enableRx() {
  if (!mIsRxEnabled)
    enableIRIn();
  mIsRxEnabled = true;
}

void IRTransceiver::disableRx() {
  // Note: IR library crashes if disableIRIn() called before enableIRIn()
  if (mIsRxEnabled)
    disableIRIn();
  mIsRxEnabled = false;
}

void IRTransceiver::loopHandleRx() {
  if (decode(&mCurrentResults)) {
    IRInterface::RawIR received;
    std::string humanReadable(
        resultToHumanReadableBasic(&mCurrentResults).c_str());

    mLastCapture.valid = true;
    mLastCapture.protocol = std::string(typeToString(mCurrentResults.decode_type, true).c_str());
    mLastCapture.human = humanReadable;
    {
      std::stringstream hex;
      hex << "0x" << std::uppercase << std::hex << mCurrentResults.value;
      mLastCapture.dataHex = hex.str();
    }

    // Store protocol
    received.mprotocol =
        static_cast<IRInterface::protocol>(mCurrentResults.decode_type);

    // Reserve room in Vector
    uint16_t rawLength = getCorrectedRawLength(&mCurrentResults);
    received.data.reserve(rawLength);

    // Get Data and copy into vector then free
    uint16_t *rawData = resultToRawArray(&mCurrentResults);
    received.data.assign(rawData, rawData + rawLength);
    delete[] (rawData);
    mIRReceived->notify(received, humanReadable);
  }
}

void IRTransceiver::maxOutTaskPriority() {
  mPreSendPriority = uxTaskPriorityGet(nullptr);
  vTaskPrioritySet(nullptr, configMAX_PRIORITIES - 1);
}
void IRTransceiver::restoreTaskPriority() {
  vTaskPrioritySet(nullptr, mPreSendPriority);
}