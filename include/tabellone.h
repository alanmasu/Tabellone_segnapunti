#ifndef __TABELLONE_H__
#define __TABELLONE_H__
#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include <SPI.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <esp_now.h>
#include <esp_bt_main.h>
#include <esp_bt.h>
#include <esp_system.h>
#include <setteSeg.h> 

//DEFINIZIONE COSTANTI
#define CONNECTION_LED 2
#define ESP_NOW_MAX_TIMEOUT 3 * 1000UL //3 secondi

//typedef e struct
typedef struct Comandi {
  bool state[17];
} Stati;

//Testate funzioni
//Ininizializzazioni
void initSerial(String str);
//EEPROM
bool initEEPROM();
void rsBackup();
bool EEPROMSave();
//TAB
void initMCP();
void initDigits();
void initDisplays();
void initFalli();
void initDuePunti();
void IRAM_ATTR duePunti();
void reset();
void clearTab();
void testTab();
void displayWrite();

//void initWiFi();
bool initESP_NOW();
void initPowerFail();
void initRTC();

//ESP-NOW
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
bool checkNOWConnection();

//PowerFail
void powerFailTaskRoutine(void * pvParameters);

//Utility
String splitString(String str, char sep, int index);
void impostaOra(byte minInt, byte oraInt);
String getTime();
void tik();

//Core
void readSerial(String &str);
void restoreTabMode();
void mainProcess();
void automaticMode();
void displayPrint();
void timeOutWrite();
void displayPrintOnSerial();
void oraPrint();
void oraPrintOnSerial();
void duePuntiWrite();
void finishTime();


#endif
