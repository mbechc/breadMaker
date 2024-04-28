#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "WIFI";
const char* password = "PejsSkorstenSky";

// webServer pages
const char* PARAM_SSID = "inputSSID";
const char* PARAM_PW = "inputPW";
const char* PARAM_PID = "inputPID";

AsyncWebServer server(80);

// HTML web page to handle 3 input fields (inputSSID, inputPW, inputPID)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
    inputSSID (current value %inputSSID%): <input type="text" name="inputSSID">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputPW (current value %inputPW%): <input type="number " name="inputPW">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputPID (current value %inputPID%): <input type="number " name="inputPID">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "inputSSID"){
    return readFile(SPIFFS, "/inputSSID.txt");
  }
  else if(var == "inputPW"){
    return readFile(SPIFFS, "/inputPW.txt");
  }
  else if(var == "inputPID"){
    return readFile(SPIFFS, "/inputPID.txt");
  }
  return String();
}


void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}

void setup(){
  Serial.begin(115200);

  // Initialize SPIFFS
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif

  // delete old config
  WiFi.disconnect(true);

  delay(1000);

  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  /* Remove WiFi event
  Serial.print("WiFi Event ID: ");
  Serial.println(eventID);
  WiFi.removeEvent(eventID);*/

  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("breadMaker", "PejsSkorstenSky");  
  Serial.println();
  Serial.println();
  Serial.println("Wait for WiFi... ");


  // Start the web server
  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputSSID=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputSSID value on <ESP_IP>/get?inputSSID=<inputMessage>
    if (request->hasParam(PARAM_SSID)) {
      inputMessage = request->getParam(PARAM_SSID)->value();
      writeFile(SPIFFS, "/inputSSID.txt", inputMessage.c_str());
    }
    // GET inputPW value on <ESP_IP>/get?inputPW=<inputMessage>
    else if (request->hasParam(PARAM_PW)) {
      inputMessage = request->getParam(PARAM_PW)->value();
      writeFile(SPIFFS, "/inputPW.txt", inputMessage.c_str());
    }
    // GET inputPID value on <ESP_IP>/get?inputPID=<inputMessage>
    else if (request->hasParam(PARAM_PID)) {
      inputMessage = request->getParam(PARAM_PID)->value();
      writeFile(SPIFFS, "/inputPID.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  // To access your stored values on inputSSID, inputPW, inputPID
  String yourinputSSID = readFile(SPIFFS, "/inputSSID.txt");
  Serial.print("*** Your inputSSID: ");
  Serial.println(yourinputSSID);
  
  int yourinputPW = readFile(SPIFFS, "/inputPW.txt").toInt();
  Serial.print("*** Your inputPW: ");
  Serial.println(yourinputPW);
  
  float yourinputPID = readFile(SPIFFS, "/inputPID.txt").toFloat();
  Serial.print("*** Your inputPID: ");
  Serial.println(yourinputPID);
  delay(5000);
}

/*
https://github.com/me-no-dev/ESPAsyncWebServer
*/