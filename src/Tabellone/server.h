#ifndef __SERVER_H__
#define __SERVER_H__

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <esp-fs-webserver.h>

extern FSWebServer myWebServer;

void setFileName(String name);
void initOTA();
void initServer();
void startFilesystem();

bool getExitingOtaMode();
void setExitingOtaMode(bool value);

void enteringOtaMode();
void exitOtaMode();

inline void serverLoop(){
    myWebServer.run();
}

inline void OTALoop(){
    ArduinoOTA.handle();
}

void handleGetVersion();

#endif
