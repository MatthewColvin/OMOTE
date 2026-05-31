#pragma once

namespace captive_portal {

void start(const char *apName = "OMOTE-Setup");
void loop();
void stop();
bool isActive();
const char *statusText();

} // namespace captive_portal
