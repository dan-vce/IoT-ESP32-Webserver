#include "wifi_manager.h"
#include "html_templates.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

static constexpr const char* AP_SSID = "ESP32-Setup";
static constexpr const char* NVS_NS = "wifi";
static constexpr const char* NVS_KEY_SSID = "ssid";
static constexpr const char* NVS_KEY_PASS = "pass";
static constexpr uint8_t DNS_PORT = 53;
static constexpr int CONNECT_TIMEOUT_MS = 15000;

static WebServer s_server(80);
static DNSServer s_dns;
static Preferences s_prefs;
static String s_statusMsg;

// internal helpers
static String buildSsidOptions() {
    String options;
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        options += "<option value=\"" + ssid + "\">" + ssid + "</option>\n";
    }
    return options;
}

static void handleRoot() {
    String ssidOptions = buildSsidOptions();

    const TemplateVar vars[] = {
        { "SSID_OPTIONS", ssidOptions.c_str() },
        { "STATUS_MSG", s_statusMsg.c_str()  }
    };
    String html = renderTemplate(PROVISIONING_PAGE_TMPL, vars, 2);
    s_statusMsg = "";  // clear after display

    s_server.send(200, "text/html", html);
}

static void handleConnect() {
    String ssid = s_server.arg("ssid");
    String password = s_server.arg("password");

    // send the connecting page immediately so the browser has feedback
    const TemplateVar connectingVars[] = {
        { "TARGET_SSID", ssid.c_str()},
        { "STATUS_MSG", "Connecting&hellip;" }
    };
    String connectingHtml = renderTemplate(CONNECTING_PAGE_TMPL, connectingVars, 2);
    s_server.send(200, "text/html", connectingHtml);

    // attempt connection
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= CONNECT_TIMEOUT_MS) {
            Serial.println("Provisioning: connect timed out");
            s_statusMsg = "Could not connect to &ldquo;" + ssid +
                          "&rdquo;. Check the password and try again.";
            WiFi.disconnect(false);
            return;
        }
        delay(200);
    }

    Serial.print("Provisioning: connected to ");
    Serial.println(ssid);
    wifiManagerSaveCredentials(ssid, password);
    delay(500);  //allow time for HTTP response to be sent before restarting
    ESP.restart();
}

// redirect everything to the portal, triggers captive-portal overlay on mobile
static void handleNotFound() {
    s_server.sendHeader("Location", "http://192.168.4.1/", true);
    s_server.send(302, "text/plain", "");
}

static void registerRoutes() {
    s_server.on("/", HTTP_GET, handleRoot);
    s_server.on("/connect", HTTP_POST, handleConnect);
    s_server.onNotFound(handleNotFound);
}

//Public API
void wifiManagerInit() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);
}

WifiStatus wifiManagerHasCredentials() {
    s_prefs.begin(NVS_NS, true);
    String ssid = s_prefs.getString(NVS_KEY_SSID, "");
    s_prefs.end();
    return ssid.isEmpty() ? WifiStatus::NO_CREDENTIALS : WifiStatus::HAS_CREDENTIALS;
}

WifiStatus wifiManagerConnectSaved() {
    s_prefs.begin(NVS_NS, true);
    String ssid = s_prefs.getString(NVS_KEY_SSID, "");
    String pass = s_prefs.getString(NVS_KEY_PASS, "");
    s_prefs.end();

    Serial.print("Connecting to saved network: ");
    Serial.println(ssid);

    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= CONNECT_TIMEOUT_MS) {
            Serial.println("Saved credentials failed");
            return WifiStatus::CONNECT_FAILED;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());
    return WifiStatus::CONNECTED;
}

void wifiManagerStartAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    delay(100);

    s_dns.start(DNS_PORT, "*", WiFi.softAPIP());

    registerRoutes();
    s_server.begin();

    Serial.print("AP started: ");
    Serial.print(AP_SSID);
    Serial.print("  IP: ");
    Serial.println(WiFi.softAPIP());
}

void wifiManagerHandleAP() {
    s_dns.processNextRequest();
    s_server.handleClient();
}

bool wifiManagerSaveCredentials(const String& ssid, const String& password) {
    s_prefs.begin(NVS_NS, false);
    bool ok = s_prefs.putString(NVS_KEY_SSID, ssid) &&
              s_prefs.putString(NVS_KEY_PASS, password);
    s_prefs.end();
    return ok;
}

void wifiManagerClearCredentials() {
    s_prefs.begin(NVS_NS, false);
    s_prefs.clear();
    s_prefs.end();
}
