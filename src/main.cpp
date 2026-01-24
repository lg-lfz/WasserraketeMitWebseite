#include "main.h"

Adafruit_BME280 bme;
bool bmpReadyStatus = false;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;
unsigned long lastSerialPrint = 0;

File file;
String index_html = "";
ApplicationState appState;

#ifdef ARDUINO_ESP8266_NODEMCU_ESP12E
  const char* BOARD_TYPE = "NodeMCU ESP-12E";
#elif defined(ARDUINO_ESP8266_NODEMCU)
  const char* BOARD_TYPE = "NodeMCU";
#elif defined(ARDUINO_ESP8266_WEMOS_D1MINI)
  const char* BOARD_TYPE = "D1 Mini";
#elif defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO)
  const char* BOARD_TYPE = "D1 Mini Pro";
#elif defined(ARDUINO_ESP8266_WEMOS_D1MINILITE)
  const char* BOARD_TYPE = "D1 Mini Lite";
#else
  const char* BOARD_TYPE = "ESP8266";
#endif

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  setlocale(LC_ALL, "de_DE.UTF-8");
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  bmpReadyStatus = checkIfBmeIsReady(I2CADDRESS, &bme);

  if (!LittleFS.begin())
  {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }
  Serial.println("LittleFS.begin() works successfully...");
  initAccessPoint(IPAddress(10, 10, 10, 10), IPAddress(10, 10, 10, 10), IPAddress(255, 255, 255, 0), "Rocket-" + WiFi.macAddress());

  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/script.js", LittleFS, "/script.js");
  server.serveStatic("/favicon.ico", LittleFS, "/favicon.ico");
  server.serveStatic("/10-10-10-10-qr-code.svg", LittleFS, "/10-10-10-10-qr-code.svg");

  initWebSocket();
  server.begin();
  yield();
  Serial.println("------ Board Info ------");
  Serial.printf("Board: %s\n", BOARD_TYPE);
  Serial.printf("Chip ID: %08X\n", ESP.getChipId());
  Serial.printf("Flash Size: %d KB\n", ESP.getFlashChipSize() / 1024);
  Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println("------------------------");
}

void loop()
{
  if (ESP.getFreeHeap() < 5000)
  {
    Serial.println("Low memory!!");
    // ESP.restart();
  }
  yield();
  dnsServer.processNextRequest();
  yield();
  ws.cleanupClients();
  yield();
  if (ws.count() == 0)
  {
    delay(500);
    return;
  }
  yield();

  SensorData data = {};
  data.realData = bmpReadyStatus;
  if (bmpReadyStatus)
  {
    data.pressure = bme.readPressure() / 100.0;             // pressure in hPa
    data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA); // in m
    data.temperature = bme.readTemperature();               // in °C
  }
  else
  { // Faked data
    data.temperature = 21 + random(-50, 50) / 10.0;
    data.pressure = 1013.25 + sin(millis() / 10000.0) * 5;
    data.altitude = 450.0 + random(-50, 50) / 10.0;
  }

  sendDataToClient(data);
  yield();
  storeData(data, appState);
  yield();
  if (millis() - lastSerialPrint > HEARTBEAT_INTERVALL)
  {
    lastSerialPrint = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.printf("Heap: %d | Clients: %d | Recording: %s\n",
                  ESP.getFreeHeap(),
                  ws.count(),
                  appState.dataRecording ? "yes" : "no");
  }
  yield();
  delay(DELAY);
}

void loadHTMLPage(String fileName)
{
  File htmlFile = LittleFS.open(fileName, "r");

  if (htmlFile)
  {
    index_html = htmlFile.readString();
    htmlFile.close();
  }
  else
  {
    Serial.println("Reading the HTML file failed...");
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char *)data, "toggle") == 0)
    {
      appState.dataRecording = !appState.dataRecording;

      if (!appState.dataRecording)
      {
        if (file)
        {
          file.close();
        }
        Serial.println("File closed");
      }
      else
      {
        file = LittleFS.open("data.csv", "w");
        if (file)
        {
          file.print("Altitude; Pressure; Temp; RealData\n");
          Serial.println("File opened");
        }
        else
        {
          Serial.println("Failed to open file for writing");
          appState.dataRecording = false;
        }
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
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
  case WS_EVT_PING:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Limit to 2 Clients for stability
  // ws.setMaxClients(2);  // Optional

  // Apple Captive Portal Check
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>"); });

  // Android Captive Portal Check (both variants)
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(204); });

  // Windows Captive Portal Check
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Microsoft Connect Test"); });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html", false, processor); });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html", false, processor); });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      
      if (appState.dataRecording)
      {
        appState.dataRecording = false;
      }
      
      if (file)
      {
        file.close();
      }
      request->send(LittleFS, "data.csv", String(), true); });

  // Catch-All: Redirect all unknown requests to homepage
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->redirect("http://10.10.10.10/"); });
}

String processor(const String &var)
{
  if (var == "MAC")
  {
    return WiFi.macAddress();
  }
  if (var == "BOARD")
  {
    return String(BOARD_TYPE);
  }
  if (var == "CHIP_ID")
  {
    char buf[16];
    sprintf(buf, "%08X", ESP.getChipId());
    return String(buf);
  }
  if (var == "FLASH_SIZE")
  {
    return String(ESP.getFlashChipSize() / 1024) + " KB";
  }
  if (var == "CPU_FREQ")
  {
    return String(ESP.getCpuFreqMHz()) + " MHz";
  }
  return String();
}

void sendDataToClient(const SensorData &data)
{
  if (ws.count() == 0 || ws.getClients().empty())
  {
    return;
  }

  char buffer[150];
  int len = snprintf(buffer, sizeof(buffer),
                     "{\"altitude\":%.2f,\"pressure\":%.2f,\"temperature\":%.2f,\"realData\":%s,\"recording\":%s,\"clients\":%d}",
                     data.altitude, data.pressure, data.temperature,
                     data.realData ? "true" : "false",
                     appState.dataRecording ? "true" : "false",
                     ws.count());

  if (len <= 0 || len >= (int)sizeof(buffer))
  {
    return; // Buffer overflow protection
  }

  for (auto &client : ws.getClients())
  {
    if (client.status() == WS_CONNECTED && client.canSend())
    {
      client.text(buffer, len);
      yield();
    }
  }
}

void storeData(const SensorData &data, const ApplicationState &appState)
{
  if (appState.dataRecording && file)
  {
    if (!file.printf("%0.2f; %0.2f; %0.2f; %s\n", data.altitude, data.pressure, data.temperature, data.realData ? "true" : "false"))
    {
      Serial.println("Write failed");
    }
  }
}

void initAccessPoint(const IPAddress &localIP, const IPAddress &gatewayIP, const IPAddress &netmask, const String &ssid)
{
  if (!WiFi.softAPConfig(localIP, gatewayIP, netmask))
  {
    Serial.println("WiFi configuration failed.");
  }
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
  WiFi.softAP(ssid);

  // Start DNS server - redirects all domain requests to the ESP32
  dnsServer.start(53, "*", localIP);
  Serial.println("DNS server started");
}

bool checkIfBmeIsReady(uint8_t addr, Adafruit_BME280 *bme)
{
  Serial.printf("Checking for sensor at addr: 0x%x \n", addr);
  bool status = bme->begin(I2CADDRESS, &Wire);
  if (!status)
  {
    Serial.println("No sensor found! Check wiring!");
    return false;
  }
  return true;
}
