#pragma once
#include <string>

class IAction {
public:
  IAction(const std::string &aName);
  virtual ~IAction() = default;

  virtual void execute() = 0;

  std::string GetName();

private:
  std::string mName;
};

inline IAction::IAction(const std::string &aName) : mName(aName) {
}

inline std::string IAction::GetName() {
  return mName;
}