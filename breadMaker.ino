#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>

AsyncWebServer server(80);

// Define EEPROM addresses for SSID, password, and hostname storage
#define EEPROM_SIZE 128
#define SSID_STA_ADDR 0
#define PASS_STA_ADDR 32
#define HOSTNAME_ADDR 60
#define SSID_AP_ADDR 16
#define PASS_AP_ADDR 48

// Default settings for AP mode
const char* default_ssid_ap = "ESP32-AP";
const char* default_password_ap = "password";

// Default hostname for STA mode
const char* default_hostname = "breadMaker";

// Function prototypes
String readStringFromEEPROM(int addr);
void writeStringToEEPROM(int addr, const String& str);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Read stored SSID, password, and hostname settings from EEPROM
  String ssid_sta = readStringFromEEPROM(SSID_STA_ADDR);
  String pass_sta = readStringFromEEPROM(PASS_STA_ADDR);
  String hostname = readStringFromEEPROM(HOSTNAME_ADDR);
  String ssid_ap = readStringFromEEPROM(SSID_AP_ADDR);
  String pass_ap = readStringFromEEPROM(PASS_AP_ADDR);

  // Set default settings if not configured
  if (hostname.isEmpty()) {
    hostname = default_hostname;
    writeStringToEEPROM(HOSTNAME_ADDR, hostname);
    EEPROM.commit();
  }
  if (ssid_ap.isEmpty() || pass_ap.isEmpty()) {
    ssid_ap = default_ssid_ap;
    pass_ap = default_password_ap;
    writeStringToEEPROM(SSID_AP_ADDR, ssid_ap);
    writeStringToEEPROM(PASS_AP_ADDR, pass_ap);
    EEPROM.commit();
  }

  // Set up WiFi in Station (STA) mode
  WiFi.begin(ssid_sta.c_str(), pass_sta.c_str());
  WiFi.setHostname(hostname.c_str());

  // Set up WiFi in Access Point (AP) mode
  WiFi.softAP(ssid_ap.c_str(), pass_ap.c_str());

  // Route for root / page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String ssid_sta_value = readStringFromEEPROM(SSID_STA_ADDR);
    String pass_sta_value = readStringFromEEPROM(PASS_STA_ADDR);
    String hostname_value = readStringFromEEPROM(HOSTNAME_ADDR);
    String ssid_ap_value = readStringFromEEPROM(SSID_AP_ADDR);
    String pass_ap_value = readStringFromEEPROM(PASS_AP_ADDR);

    String html = "<form action='/config' method='post'>";
    html += "Station Mode:<br>";
    html += "SSID:<br><input type='text' name='ssid_sta' value='" + ssid_sta_value + "'><br>";
    html += "Password:<br><input type='text' name='password_sta' value='" + pass_sta_value + "'><br>";
    html += "Hostname:<br><input type='text' name='hostname' value='" + hostname_value + "'><br><br>";
    html += "Access Point Mode:<br>";
    html += "SSID:<br><input type='text' name='ssid_ap' value='" + ssid_ap_value + "'><br>";
    html += "Password:<br><input type='text' name='password_ap' value='" + pass_ap_value + "'><br><br>";
    html += "<input type='submit' value='Submit'></form>";

    request->send(200, "text/html", html);
  });

  // Route to handle form submission
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request){
    // Get parameters from POST request
    String ssid_sta = request->arg("ssid_sta");
    String password_sta = request->arg("password_sta");
    String hostname = request->arg("hostname");
    String ssid_ap = request->arg("ssid_ap");
    String password_ap = request->arg("password_ap");

    // Store the new settings in EEPROM
    writeStringToEEPROM(SSID_STA_ADDR, ssid_sta);
    writeStringToEEPROM(PASS_STA_ADDR, password_sta);
    writeStringToEEPROM(HOSTNAME_ADDR, hostname);
    writeStringToEEPROM(SSID_AP_ADDR, ssid_ap);
    writeStringToEEPROM(PASS_AP_ADDR, password_ap);
    EEPROM.commit(); // Save changes to EEPROM

    // Configure ESP32 WiFi with the new settings
    WiFi.begin(ssid_sta.c_str(), password_sta.c_str());
    WiFi.setHostname(hostname.c_str());
    WiFi.softAP(ssid_ap.c_str(), password_ap.c_str());

    // Respond with success message
    request->send(200, "text/plain", "Configuration updated successfully. Restart device to apply changes.");
  });

  // Start server
  server.begin();
}

void loop() {
  // Handle web server events
  // The AsyncWebServer runs in the background, so no explicit handling in the loop is required
}

// Helper function to read a string from EEPROM
String readStringFromEEPROM(int addr) {
  String result = "";
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    char c = EEPROM.read(addr + i);
    if (c == '\0') {
      break;
    }
    result += c;
  }
  return result;
}

// Helper function to write a string to EEPROM
void writeStringToEEPROM(int addr, const String& str) {
  for (int i = 0; i < str.length(); ++i) {
    EEPROM.write(addr + i, str[i]);
  }
  EEPROM.write(addr + str.length(), '\0'); // Null-terminate the string
}
