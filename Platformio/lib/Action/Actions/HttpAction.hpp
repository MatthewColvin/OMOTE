#pragma once
#include "IAction.hpp"
#include <memory>
#include <string>
#include <map>

class HttpClientInterface;
class HttpResponse;

class HttpAction : public IAction {
public:
  HttpAction(const std::string &aName, const std::string &aUrl, const std::string &aMethod, const std::string &aBody, const std::map<std::string, std::string> &aHeaders);
  
  void execute() override;

private:
  std::string mUrl;
  std::string mMethod;
  std::string mBody;
  std::map<std::string, std::string> mHeaders;
};