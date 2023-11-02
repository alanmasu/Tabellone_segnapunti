#include <Arduino.h>
#include "server.h"
#include <common.h>
#include <tabellone.h>
#include <WiFi.h>
#include <esp-fs-webserver.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <LITTLEFS.h>

#define FILESYSTEM LITTLEFS
WebServer server(80);
FSWebServer myWebServer(FILESYSTEM, server);

//WiFi
char ssid[] = "Tabellone";
char pass[] = "Tabellone";

void initOTA() {
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("Tabellone");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin();
}

////////////////////////////////  Server  /////////////////////////////////////////
void initServer(){
 // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to flash stored SSID, start AP if fails after timeout
  myWebServer.setAPmode(ssid, pass);

  // Add custom page handlers to webserver
  //myWebServer.addHandler("/led", HTTP_GET, handleLed);
  myWebServer.addHandler("/exitOtaMode", HTTP_GET, exitOtaMode);

  // Start webserver
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(WiFi.softAPIP());
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
    Serial.println(F("Open /update page to upload firmware and filesystem updates"));
  }
}

////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem(){
  // FILESYSTEM INIT
  if ( FILESYSTEM.begin()){
    File root = FILESYSTEM.open("/", "r");
    File file = root.openNextFile();
    while (file){
      const char* fileName = file.name();
      size_t fileSize = file.size();
      Serial.printf("FS File: %s, size: %lu\n", fileName, (long unsigned)fileSize);
      file = root.openNextFile();
    }
    Serial.println();
  }
  else {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
}

void enteringOtaMode(){
  Serial.println("Entering OTA mode");
  initOTA();
  initServer();
}

void exitOtaMode(){
  Serial.println("Exiting OTA mode");
  ArduinoOTA.end();
  WiFi.softAPdisconnect();
  setMode(tabellone);
}