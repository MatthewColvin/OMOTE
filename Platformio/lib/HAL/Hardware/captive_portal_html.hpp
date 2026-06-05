#pragma once

namespace captive_portal_html {

inline const char *page(bool simMode) {
  if (simMode) {
    return R"(<!DOCTYPE html><html><head><meta name=viewport content="width=device-width,initial-scale=1">
<title>OMOTE WiFi Setup (Simulator)</title><style>body{font-family:sans-serif;max-width:400px;margin:2em auto;padding:1em}
input,button{width:100%;padding:12px;margin:8px 0;box-sizing:border-box}button{background:#3366cc;color:#fff;border:0}</style></head>
<body><h1>OMOTE WiFi (Sim)</h1><p>Enter WiFi credentials for the simulator. No reboot — the sim applies them immediately.</p>
<form method=POST action=/save>
<label>Network name (SSID)</label><input name=ssid required autocomplete=off>
<label>Password</label><input name=password type=password autocomplete=off>
<button type=submit>Save</button></form></body></html>)";
  }
  return R"(<!DOCTYPE html><html><head><meta name=viewport content="width=device-width,initial-scale=1">
<title>OMOTE WiFi Setup</title><style>body{font-family:sans-serif;max-width:400px;margin:2em auto;padding:1em}
input,button{width:100%;padding:12px;margin:8px 0;box-sizing:border-box}button{background:#3366cc;color:#fff;border:0}</style></head>
<body><h1>OMOTE WiFi</h1><p>Connect your phone to this network, then enter your home WiFi details.</p>
<form method=POST action=/save>
<label>Network name (SSID)</label><input name=ssid required autocomplete=off>
<label>Password</label><input name=password type=password autocomplete=off>
<button type=submit>Save &amp; reboot</button></form></body></html>)";
}

inline const char *savedPage(bool simMode) {
  if (simMode)
    return "<html><body><h2>Saved. Simulator will connect using these credentials.</h2></body></html>";
  return "<html><body><h2>Saved. Rebooting OMOTE...</h2></body></html>";
}

} // namespace captive_portal_html
