#ifndef __TABELLONE_H__
#define __TABELLONE_H__
#include <Arduino.h>
#include <esp_now.h>
#include <common.h>


//DEFINIZIONE COSTANTI
#define CONNECTION_LED 2
#define ESP_NOW_MAX_TIMEOUT 3 * 1000UL //3 secondi

//Testate funzioni
//Ininizializzazioni
void initSerial(String &title);
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
bool initESP_NOW();
void initPowerFail();
void initRTC();

//ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void sendViaNow();
bool checkNowConnection();

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
void clearCommands();
void automaticMode();
Mode getMode();
void setMode(Mode mode);
void displayPrint();
void timeOutWrite();
void displayPrintOnSerial();
void oraPrint();
void oraPrintOnSerial();
void OTAPrint();
void OTAPrintOnSerial();
void duePuntiWrite();
void finishTime();

#endif
