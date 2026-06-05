#pragma once

namespace editor_sync_mode {

bool isActive();
/** @param showOverlay When true, JsonUI shows the full-screen sync popup (device menu only). */
bool enter(bool showOverlay = false);
bool overlayRequested();
void exit(bool reboot);

} // namespace editor_sync_mode
