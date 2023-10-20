#ifndef __TABELLONE_H__ 
#define __TABELLONE_H__
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

//DEFINIZIONE COSTANTI
#define CONNECTION_LED 2

//typedef e struct
typedef struct Comandi {
  bool state[17];
} Stati;


//Variabili globali
extern Comandi comandi; //Struct ricezione dati

//Testate funzioni 
//Ininizializzazioni
void initSerial();
bool initESP_NOW();
//ESP-NOW
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);

#endif
