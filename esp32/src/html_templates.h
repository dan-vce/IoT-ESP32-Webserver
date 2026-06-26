#pragma once
#include <Arduino.h>

// Template variable 
// A key/value pair used to substitute {{KEY}} placeholders in a template.
struct TemplateVar {
    const char* key;
    const char* value;
};

// renderTemplate
// Replaces every {{key}} occurrence in tmpl with the corresponding value.
// Returns the fully substituted String. Unknown placeholders are left as-is.
inline String renderTemplate(const char* tmpl, const TemplateVar vars[], int count) {
    String result = String(tmpl);
    for (int i = 0; i < count; i++) {
        String token = String("{{") + vars[i].key + "}}";
        result.replace(token, vars[i].value);
    }
    return result;
}

// PROVISIONING PAGE
// Placeholders:
//   {{SSID_OPTIONS}} one <option value="...">...</option> per scanned network
//   {{STATUS_MSG}} error feedback from a failed connect attempt (may be "")
constexpr const char* PROVISIONING_PAGE_TMPL = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>WiFi Setup</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      max-width: 420px;
      margin: 48px auto;
      padding: 0 20px;
      color: #1a1a1a;
      background: #f5f5f5;
    }
    .card {
      background: #fff;
      border-radius: 10px;
      padding: 28px 24px;
      box-shadow: 0 2px 8px rgba(0,0,0,.1);
    }
    h1 { font-size: 1.3rem; margin: 0 0 20px; }
    label { display: block; font-size: .85rem; font-weight: 600; margin-bottom: 4px; }
    select, input[type="password"] {
      display: block; width: 100%; padding: 10px 12px;
      margin-bottom: 16px; border: 1px solid #ccc;
      border-radius: 6px; font-size: 1rem;
    }
    button {
      display: block; width: 100%; padding: 11px;
      background: #0066cc; color: #fff; border: none;
      border-radius: 6px; font-size: 1rem; cursor: pointer;
    }
    button:hover { background: #0052a3; }
    .error { color: #c00; font-size: .9rem; margin-top: 14px; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Connect to WiFi</h1>
    <form method="POST" action="/connect">
      <label for="ssid">Network</label>
      <select id="ssid" name="ssid">
        {{SSID_OPTIONS}}
      </select>
      <label for="pass">Password</label>
      <input type="password" id="pass" name="password" placeholder="Enter WiFi password">
      <button type="submit">Connect</button>
    </form>
    <p class="error">{{STATUS_MSG}}</p>
  </div>
</body>
</html>
)rawhtml";

// CONNECTING PAGE
// Shown immediately after the form is submitted while the ESP32 tries to join.
// Meta-refresh retries the root page after 5 seconds
// Placeholders:
//   {{TARGET_SSID}} — the network name the user selected
//   {{STATUS_MSG}} — "Connecting…" or an error message on failure
constexpr const char* CONNECTING_PAGE_TMPL = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="5;url=/">
  <title>Connecting…</title>
  <style>
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      max-width: 420px; margin: 48px auto; padding: 0 20px;
      color: #1a1a1a; background: #f5f5f5;
    }
    .card {
      background: #fff; border-radius: 10px; padding: 28px 24px;
      box-shadow: 0 2px 8px rgba(0,0,0,.1);
    }
    h1 { font-size: 1.3rem; margin: 0 0 12px; }
    p  { color: #555; font-size: .95rem; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Connecting to {{TARGET_SSID}}</h1>
    <p>{{STATUS_MSG}}</p>
    <p>This page will refresh in 5 seconds.</p>
  </div>
</body>
</html>
)rawhtml";
