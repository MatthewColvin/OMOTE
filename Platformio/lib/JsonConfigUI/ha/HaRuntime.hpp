#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace UI::Page {
class JsonPage;
}

namespace HaRuntime {

void init();
void tick();

/** Reload HaSettings.json from LittleFS (call after editor deploy). */
void reloadSettingsFromDisk();

bool configured();
bool callService(const std::string &domain, const std::string &service, const std::string &entityId);
bool callServiceWithData(const std::string &domain, const std::string &service, const std::string &entityId,
                         const std::string &serviceDataJson);

bool getCachedState(const std::string &entityId, std::string &stateOut);
bool getCachedAttributes(const std::string &entityId, std::string &attributesJsonOut);
bool fetchEntityState(const std::string &entityId);
bool stateIsOn(const std::string &entityId, const std::string &state);

/** Only poll / subscribe to entities on the visible JsonPage tab. */
void setActivePage(UI::Page::JsonPage *page, const std::vector<std::string> &entityIds);
void requestRefresh();

} // namespace HaRuntime
