#include <Arduino.h>
#include <SimpleFTPServer.h>
#include "ftp.hpp"
#include "Esp32Logger.hpp"

FtpServer ftpSrv;

void _callback(FtpOperation ftpOperation, unsigned int freeSpace, unsigned int totalSpace){
  auto logger = std::make_unique<LoggingInterface>();
  logger->setLogModule(LogModule::WiFi);

  switch (ftpOperation) {
    case FTP_CONNECT:
      logger->debug("FTP: Client connected");
      break;
    case FTP_DISCONNECT:
      logger->debug("FTP: Client disconnected");
      break;
    case FTP_FREE_SPACE_CHANGE:
      logger->debug("FTP: Free space change, free " + std::to_string(freeSpace) + " of %u!\n" + std::to_string(totalSpace));
      break;
    default:
      break;
  }
}

void _transferCallback(FtpTransferOperation ftpOperation, const char* name, unsigned int transferredSize){
  auto logger = std::make_unique<LoggingInterface>();
  logger->setLogModule(LogModule::WiFi);
  switch (ftpOperation) {
    case FTP_UPLOAD_START:
      logger->debug("FTP: Starting upload ");
      break;
    case FTP_UPLOAD:
      logger->debug("FTP: Upload of file " + std::string(name) + " bytes:" + std::to_string(transferredSize));
      break;
    case FTP_TRANSFER_STOP:
      logger->debug("FTP: Finished transfer");
      break;
    case FTP_TRANSFER_ERROR:
      logger->debug("FTP: Transfer error");
      break;
    default:
      break;
  }
}

void ftp::begin(const char *_user, const char *_pass) {
  ftpSrv.setCallback(_callback);
  ftpSrv.setTransferCallback(_transferCallback);
  ftpSrv.begin(_user, _pass);
}

void ftp::end() {
  ftpSrv.end();
}

void ftp::sync() {
  ftpSrv.handleFTP();
}