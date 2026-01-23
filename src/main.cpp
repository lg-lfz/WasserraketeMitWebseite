#include "main.h"

Adafruit_BME280 bme;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

File file;
String index_html = "";
ApplicationState appState;

void setup()
{
  setlocale(LC_ALL, "de_DE.UTF-8");
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  checkI2CStatus(I2CADDRESS, &bme);

  if (!LittleFS.begin())
  {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }
  else
  {
    Serial.println("LittleFS.begin() works successfully...");
  }

  file = LittleFS.open("data.csv", "w");
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  else
  {
    Serial.println("Open file for writing works successfully...");
  }

  initAccessPoint(IPAddress(10, 10, 10, 10), IPAddress(10, 10, 10, 10), IPAddress(255, 255, 255, 0), "Rocket-" + WiFi.macAddress());

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  initWebSocket();
  // Start server
  server.begin();
}

void loop()
{
  dnsServer.processNextRequest();

  SensorData data = {};
  data.pressure = bme.readPressure() / 100.0;             // pressure in hPa
  data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA); // in m
  data.temperature = bme.readTemperature();               // in °C
  sendDataToClient(data);
  storeData(data, appState);
  ws.cleanupClients();
  delay(DELAY);
}

//--------------------------------------------------------------------

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
        Serial.println("File closed");
        ws.textAll("{\"state\":\"stop\"}");
      }
      else
      {
        file = LittleFS.open("data.csv", "w");
        file.print("Altitude; Pressure; Temp\n");
        Serial.println("File opened");
        ws.textAll("{\"state\":\"start\"}");
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
            { request->send_P(200, "text/html", index_html.c_str(), processor); });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      file.close();
      request->send(LittleFS, "data.csv", String(), true); });

  // Catch-All: Redirect all unknown requests to homepage
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->redirect("http://10.10.10.10/"); });
}

String processor(const String &var)
{
  Serial.println(var);
  return String();
}

void sendDataToClient(const SensorData &data)
{
  ws.printfAll("{\"altitude\": %0.2f, \"pressure\": %0.2f, \"temperature\": %0.2f}", data.altitude, data.pressure, data.temperature);
}

void storeData(const SensorData &data, const ApplicationState &appState)
{
  if (appState.dataRecording)
  {
    if (file.printf("%0.2f; %0.2f; %0.2f\n", data.altitude, data.pressure, data.temperature))
    {
    }
    else
    {
      Serial.println("- Write failed");
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

int checkI2CStatus(uint8_t addr, Adafruit_BME280 *bme)
{
  Serial.printf("Checking for sensor at addr: 0x%x \n", addr);
  bool status = bme->begin(I2CADDRESS, &Wire);
  if (!status)
  {
    Serial.println("No sensor found! Check wiring!");
    return -1;
  }
  return 0;
}
