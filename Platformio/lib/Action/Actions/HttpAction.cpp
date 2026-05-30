#include "HttpAction.hpp"
#include "ActionFactory.hpp"
#include "ObjectSchemaBuilder.hpp"
#include <iostream>

static constexpr auto headersSchema = OMOTE::JSON::ObjectSchema()
                                          .Optional("headers", "object");

static constexpr auto httpDataSchema =
    OMOTE::JSON::ObjectSchema()
        .Require("url", "string")
        .Optional("method", "string")
        .Optional("body", "string")
        .Optional("headers", headersSchema)
        .Build();

const auto mHttpActionRegistered = ActionFactory::Register(
    ActionTypes::HttpAction,
    std::string_view(httpDataSchema.data(), httpDataSchema.size()),
    [](const std::string &aActionName, const rapidjson::Value &aDataJson) {
      std::string url = aDataJson["url"].GetString();
      std::string method = "GET";
      if (aDataJson.HasMember("method")) {
        method = aDataJson["method"].GetString();
      }
      std::string body = "";
      if (aDataJson.HasMember("body")) {
        body = aDataJson["body"].GetString();
      }
      // Parse headers if they exist
      std::map<std::string, std::string> headers;
      if (aDataJson.HasMember("headers") && aDataJson["headers"].IsObject()) {
        for (auto it = aDataJson["headers"].MemberBegin(); it != aDataJson["headers"].MemberEnd(); ++it) {
          headers[it->name.GetString()] = it->value.GetString();
        }
      }

      return std::make_unique<HttpAction>(aActionName, url, method, body, headers);
    });

HttpAction::HttpAction(const std::string &aName, const std::string &aUrl, const std::string &aMethod, const std::string &aBody, const std::map<std::string, std::string> &aHeaders)
    : IAction(aName), mUrl(aUrl), mMethod(aMethod), mBody(aBody), mHeaders(aHeaders) {}

void HttpAction::execute() {
  auto &hardware = HardwareFactory::getAbstract();
  auto httpClient = hardware.wifi()->getHttpClient();

  if (!httpClient || !httpClient->isReady()) {
    std::cerr << "HTTP client not available or not ready" << std::endl;
    return;
  }

  HttpRequest request;
  request.url = mUrl;
  request.body = mBody;
  request.timeout_ms = 5000; // Default timeout

  // Set method
  if (mMethod == "POST") {
    request.method = HttpRequest::Method::POST;
  } else if (mMethod == "PUT") {
    request.method = HttpRequest::Method::PUT;
  } else if (mMethod == "DELETE") {
    request.method = HttpRequest::Method::DELETE;
  } else {
    request.method = HttpRequest::Method::GET;
  }

  // Set headers
  for (const auto &header : mHeaders) {
    request.headers[header.first] = header.second;
  }

  std::cout << "Executing HTTP " << mMethod << " request to: " << mUrl << std::endl;

  auto future = httpClient->executeAsync(request);

  // We won't wait for the response, as this is just a fire-and-forget execution
  // The result would be handled if we want to chain callbacks, but for simple action execution,
  // we're executing the request and moving on

  future->always([](const HttpResponse &response) {
    if (response.success) {
      std::cout << "HTTP Request succeeded with status: " << response.status_code << std::endl;
    } else {
      std::cerr << "HTTP Request failed with error: " << response.error_message << std::endl;
    }
  });
}