#ifndef __SERVER_H__
#define __SERVER_H__

#include <Arduino.h>
//#include <esp-fs-webserver.h>


void startFilesystem();
void initServer();

inline void serverLoop();



#endif
