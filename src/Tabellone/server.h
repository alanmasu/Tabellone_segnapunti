#ifndef __SERVER_H__
#define __SERVER_H__

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <esp-fs-webserver.h>

extern FSWebServer myWebServer;

void initOTA();
void initServer();
void startFilesystem();

void enteringOtaMode();
void exitOtaMode();

inline void serverLoop(){
    myWebServer.run();
}

inline void OTALoop(){
    ArduinoOTA.handle();
}



#endif
