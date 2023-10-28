#include <Arduino.h>
#include <server.h>
#include <WiFi.h>
#include <esp-fs-webserver.h>
#include <FS.h>
#include <LITTLEFS.h>

#define FILESYSTEM LITTLEFS
WebServer server(80);
FSWebServer myWebServer(FILESYSTEM, server);

//WiFi
char ssid[] = "Tabellone";
char pass[] = "Tabellone";

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

////////////////////////////////  Server  /////////////////////////////////////////
void initServer(){
 // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to flash stored SSID, start AP if fails after timeout
  myWebServer.setAPmode(ssid, pass);

  // Add custom page handlers to webserver
  //myWebServer.addHandler("/led", HTTP_GET, handleLed);

  // Start webserver
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(WiFi.softAPIP());
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
    Serial.println(F("Open /update page to upload firmware and filesystem updates"));
  }
}

void serverLoop() {
  myWebServer.run();
}