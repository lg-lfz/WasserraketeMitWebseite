#include "main.h"

Adafruit_BME280 bme;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

File file;
String index_html = "";
ApplicationState appState;



void setup(){
  setlocale(LC_ALL, "de_DE.UTF-8");
  // Serial port for debugging purposes
  Serial.begin(9600);
  Serial.setDebugOutput(true);

  checkI2CStatus(I2CADDRESS, &bme);

  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  file = LittleFS.open("data.csv", "w");
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  loadHTMLPage("/index.html");

  initAccessPoint(IPAddress(10,10,10,10), IPAddress(10,10,10,10), IPAddress(255,255,255,0), "WasserRakete");

  initWebSocket();
  // Start server
  server.begin();
}

void loop() {
  SensorData data = {};
  data.pressure = bme.readPressure() / 100.0; //pressure in hPa
  data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA); //in m
  data.temperature = bme.readTemperature(); // in °C
  sendDataToClient(data);
  storeData(data, appState);
  ws.cleanupClients();
  delay(DELAY);
}


//--------------------------------------------------------------------


void loadHTMLPage(String fileName){
  File htmlFile = LittleFS.open(fileName, "r");
  
  if(htmlFile){
    index_html = htmlFile.readString();
    htmlFile.close();
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      appState.dataRecording = !appState.dataRecording;
      
      if(!appState.dataRecording){
        Serial.println("File closed");
        ws.textAll("{\"state\":\"stop\"}");
      }
      else{
        file = LittleFS.open("data.csv", "w");
        file.print("Hoehe; Druck; Temp\n");
        Serial.println("File opened");
        ws.textAll("{\"state\":\"start\"}");
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html.c_str(), processor);
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
      file.close();
      request->send(LittleFS, "data.csv", String(), true);
  });
}

String processor(const String& var){
  Serial.println(var);
  return String();
}

void sendDataToClient(const SensorData& data){
  ws.printfAll("{\"altitude\": %0.2f, \"pressure\": %0.2f, \"temperature\": %0.2f}", data.altitude, data.pressure, data.temperature);
}

void storeData(const SensorData& data, const ApplicationState& appState){
  if(appState.dataRecording)
  {
    if(file.printf("%0.2f; %0.2f; %0.2f\n", data.altitude, data.pressure, data.temperature))
    {}
    else {
      Serial.println("- Schreiben fehlgeschlagen");
    }
  }
}

void initAccessPoint(const IPAddress& localIP, const IPAddress& gatewayIP, const IPAddress& netmask, const String& ssid){
  if (!WiFi.softAPConfig(localIP, gatewayIP, netmask))
  {
    Serial.println("WLAN Konfiguration nicht erfolgreich.");
  }
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
  WiFi.softAP(ssid);
}

int checkI2CStatus(uint8_t addr, Adafruit_BME280 *bme){
  Serial.printf("Checking for sensor at addr: 0x%x \n", addr);
  bool status = bme->begin(I2CADDRESS, &Wire);  
  if (!status) {
    Serial.println("No sensor found! Check wiring!");
    return -1;  
  }
  return 0;
}
