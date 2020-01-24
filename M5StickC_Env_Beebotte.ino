#include <M5StickC.h>
#include "esp_deep_sleep.h"
#include "DHT12.h"
#include <Wire.h>
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

Preferences preferences;

char ssid[32] = {};
char pass[32] = {};

bool WifiConnect() {
  Serial.printf("Attempting to connect to WPA SSID: %s\r\n", ssid);
  WiFi.begin(ssid, pass);
  for (int i=0; i<10; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    delay(1000);
    Serial.print(".");
    
    if (i == 9) {
      Serial.printf("Can not connect to WPA SSID: %s\r\n", ssid);
      M5.Lcd.printf("Conn failed!\r\n");
      return false;
    }
  }
  Serial.println(""); 
  Serial.print("WiFi connected\r\nIP address: "); Serial.println(WiFi.localIP());

  return true;
}

bool WifiDisconnect() {
  WiFi.disconnect();
  Serial.println("Disonnected!");
  delay(100);
}

char key[64]      = {};
char pattern[128] = {};

HTTPClient http;

boolean beebotteSend(char* channel, float value) {
  char json[64] = {};
  sprintf(json, "{\"data\": %f}", value);
  Serial.printf("json=%s\r\n", json);
  char url[256] = {};
  sprintf(url, pattern, channel);
  Serial.printf("url=%s\r\n", url);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int status = http.POST((uint8_t*)json, strlen(json));
  String line = http.getString();
  Serial.printf("Status: %d", status); Serial.println(" " + line);
  M5.Lcd.printf("Status: %d", status); M5.Lcd.println(" " + line);
  http.end();
  return status == 200;
}

DHT12 dht12;
Adafruit_BMP280 bme;

void setup() {
  M5.begin();
  M5.Axp.begin();
  M5.Axp.ScreenBreath(8);
    
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);

  Serial.println("enter setup");
  
  Wire.begin(0, 26);
  
  while (!bme.begin(0x76)){  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    M5.Lcd.println("Could not find a valid BMP280 sensor, check wiring!");
  }

  float bat_power     = M5.Axp.GetBatVoltage();
  float temperature   = dht12.readTemperature();
  float humidity      = dht12.readHumidity();
  float pressure      = bme.readPressure();

  Serial.printf("Batt Power[V]:      %1.2f\r\n", bat_power);
  Serial.printf("Temperature[degC]: %2.2f\r\n", temperature);
  Serial.printf("Humidity[%%]:       %2.2f\r\n", humidity);
  Serial.printf("Pressure[Pa]:  %0.2f\r\n", pressure);

  M5.Lcd.printf("Tempe: %2.1f`C\r\n",  temperature);
  M5.Lcd.printf("Humid: %2.1f%%\r\n",  humidity);
  M5.Lcd.printf("Press:%4.0fhPa\r\n", pressure/100);
  M5.Lcd.printf("VBatt:  %1.2fV\r\n",   bat_power);
  
  preferences.begin("wi-fi", true);
  preferences.getString("ssid", ssid, sizeof(ssid));
  preferences.getString("pass", pass, sizeof(pass));
  preferences.end();
  
  if (WifiConnect()) {  
    preferences.begin("beebotte", true);
    preferences.getString("key",     key,     sizeof(key));
    preferences.getString("pattern", pattern, sizeof(pattern));
    preferences.end();
    
    beebotteSend("temperature",     temperature);
    beebotteSend("humidity",        humidity);
    beebotteSend("pressure",        pressure);
    beebotteSend("battery_voltage", bat_power);
    WifiDisconnect();
  }
  
  esp_sleep_enable_timer_wakeup(20* 60 * 1000 * 1000);

  Serial.println("exit setup");    
}

void loop() {
  Serial.println("enter loop()");

  uint8_t deadTime = 20;
  Serial.printf("Going to sleep in %d seconds\r\n", deadTime);
  delay(1000 * deadTime);

//  M5.Axp.ScreenBreath(0);
//  M5.Lcd.fillScreen(BLACK);
  esp_deep_sleep_start();
}
