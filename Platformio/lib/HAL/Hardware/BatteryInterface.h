#pragma once
#include "Notification.hpp"

enum calMode { direct,
               charge,
               discharge };

class BatteryInterface {
public:
  BatteryInterface() = default;
  virtual ~BatteryInterface() = default;
  virtual int getPercentage() = 0;
  virtual uint16_t getRawSOC() = 0;
  virtual int getVoltage() = 0;
  virtual bool isCharging() = 0;
  virtual void writeCustomModel() = 0;
  virtual void disableHibernate(bool disable) = 0;
  virtual void saveLinearisationData(bool charge, uint16_t *rawSOCs, uint16_t numVals, int startmV, int endmV) = 0;
  virtual void setCalMode(int mode) = 0;
  virtual int getCalMode() = 0;
  virtual std::vector<uint16_t> getCalData() = 0;
};