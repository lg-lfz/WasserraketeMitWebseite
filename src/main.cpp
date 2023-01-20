#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Adafruit_BME280.h>
#include <stdio.h>

// Folgende Pins gelten für den ESP8266 (NodeMCU)
#define SCL_PIN D1
#define SDA_PIN D2
#define I2CADDRESS 0x76

#define SEALEVELPRESSURE_HPA 1013.25
#define DELAY 50

Adafruit_BME280 bme;

float pressure = 0.0;
float altitude = 0.0;
float temperature = 0.0;
bool dataRecording = false;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

File file;


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>Wasserrakete</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <style>
      html {
        font-family: Arial, Helvetica, sans-serif;
        text-align: center;
      }
      h1 {
        font-size: 1.8rem;
        color: white;
      }
      h2 {
        font-size: 1.5rem;
        font-weight: bold;
        color: #143642;
      }
      .topnav {
        overflow: hidden;
        background-color: #143642;
      }
      body {
        margin: 0;
      }
      .content {
        padding: 30px;
        max-width: 600px;
        margin: 0 auto;
      }
      .card {
        background-color: #f8f7f9;
        box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, 0.5);
        padding-top: 10px;
        padding-bottom: 20px;
      }
      .button {
        padding: 15px 50px;
        font-size: 24px;
        text-align: center;
        outline: none;
        color: #fff;
        background-color: #0f8b8d;
        border: none;
        border-radius: 5px;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0, 0, 0, 0);
      }
      /*.button:hover {background-color: #0f8b8d}*/
      .button:active {
        background-color: #0f8b8d;
        box-shadow: 2 2px #cdcdcd;
        transform: translateY(2px);
      }
      .state {
        font-size: 1.5rem;
        color: #8c8c8c;
        font-weight: bold;
      }
    </style>
    <title>Wasserrakete</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
  </head>
  <body>
    <div class="topnav">
      <h1>Wasserrakete</h1>
    </div>
    <div class="content">
      <div class="card">
        <h2 class="pressure">Luftdruck: <span id="pressure">0.0</span> hPa</h2>
        <h2 class="altitude">H&ouml;he: <span id="altitude">0.0</span>  m</h2>
        <h2 class="temperature">Temperatur: <span id="temperature">0.0</span> &deg;C</h2>
        <p class="state">state: <span id="state">no recording...</span></p>
        <p><button id="Startbutton" class="button">Start</button></p>
      </div>
    </div>
    <script>
      var gateway = `ws://${window.location.hostname}/ws`;
      var websocket;
      window.addEventListener('load', onLoad);
      function initWebSocket() {
        console.log('Trying to open a WebSocket connection...');
        websocket = new WebSocket(gateway);
        websocket.onopen    = onOpen;
        websocket.onclose   = onClose;
        websocket.onmessage = onMessage;
      }
      function onOpen(event) {
        console.log('Connection opened');
      }
      function onClose(event) {
        console.log('Connection closed');
        setTimeout(initWebSocket, 2000);
      }
      function onMessage(event) {
        const data = JSON.parse(event.data);

        if(data.state){
          if(data.state == "start"){
            document.getElementById("Startbutton").innerHTML = "Stop";
            document.getElementById("state").innerHTML = "RECORDING!";
          }
          else if(data.state == "stop"){
            document.getElementById("Startbutton").innerHTML = "Start";
            document.getElementById("state").innerHTML = "no recording...";
            window.location.href = 'download';
          }
        }
        else{
          document.getElementById("temperature").innerHTML = data.temperature.toFixed(2);
          document.getElementById("altitude").innerHTML = data.altitude.toFixed(2);
          document.getElementById("pressure").innerHTML = data.pressure.toFixed(2);
        }
      }
      
      function onLoad(event) {
        initWebSocket();
        initButton();
      }
      function initButton() {
        document.getElementById('Startbutton').addEventListener('click', toggle);
      }
      function toggle(){
        websocket.send('toggle');
      }
    </script>
  </body>
</html>
)rawliteral";


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      dataRecording = !dataRecording;
      //invert dataRecording
      
      if(!dataRecording){
        Serial.println("File closed");
        ws.textAll("{\"state\":\"stop\"}");
      }
      else{
        file = LittleFS.open("data.csv", "w");
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
}

String processor(const String& var){
  Serial.println(var);
  return String();
}

void sendDataToClient(){
  ws.printfAll("{\"altitude\": %0.2f, \"pressure\": %0.2f, \"temperature\": %0.2f}", altitude, pressure, temperature);
}

void storeData(){
  if(dataRecording)
  {
    if(file.printf("%0.2f; %0.2f; %0.2f\n", altitude, pressure, temperature))
    {}
    else {
      Serial.println("- Schreiben fehlgeschlagen");
    }
  }
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("BME280 Test");

  bool status = bme.begin(I2CADDRESS, &Wire);  
  if (!status) {
      Serial.println("Sensor nicht gefunden. Verkabelung ok?");
      while (1);
  }

  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  file = LittleFS.open("data.csv", "w");
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }


  if (!WiFi.softAPConfig(IPAddress(1,1,1,1), IPAddress(1,1,1,1), IPAddress(255,255,255,0)))
  {
    Serial.println("WLAN Konfiguration nicht erfolgreich.");
  }
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
  WiFi.softAP("Wasserrakete");

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
      file.close();
      request->send(LittleFS, "data.csv", String(), true);
  });

  // Start server
  server.begin();
}

void loop() {
  pressure = bme.readPressure() / 100.0; //pressure in hPa
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA); //in m
  temperature = bme.readTemperature(); // in °C
  sendDataToClient();
  storeData();
  ws.cleanupClients();
  delay(DELAY);
}
