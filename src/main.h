#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Adafruit_BME280.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#define SCL_PIN D1
#define SDA_PIN D2
#define I2CADDRESS 0x76

#define SEALEVELPRESSURE_HPA 1013.25
#define DELAY 50

struct SensorData{
  float pressure;
  float altitude;
  float temperature;
};

struct ApplicationState{
  bool dataRecording;
};


void initWebSocket();
void initAccessPoint(const IPAddress& localIP, const IPAddress& gatewayIP, const IPAddress& netmask, const String& ssid);


int checkI2CStatus(uint8_t addr, Adafruit_BME280 *bme){
  Serial.printf("Checking for sensor at addr: 0x%x \n", addr);
  bool status = bme->begin(I2CADDRESS, &Wire);  
  if (!status) {
    Serial.println("No sensor found! Check wiring!");
    return -1;  
  }
  return 0;
}

String processor(const String& var);

void sendDataToClient(const SensorData& data);

void storeData(const SensorData& data, const ApplicationState& appState);

void loadHTMLPage(String fileName);

