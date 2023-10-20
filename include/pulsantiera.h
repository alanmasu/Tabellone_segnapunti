#ifndef __PULSANTIERA_H__ 
#define __PULSANTIERA_H__
#include <esp_now.h>
#include <WiFi.h>

//Definizioni delle costanti
#define CONNECTION_LED_PIN 2

//Struct e typedef
typedef struct Comandi {
  bool state[17];
} Stati;


//Testate funzioni
//Ininizializzazioni
void initSerial(String str);
void initESPNOW(esp_now_peer_info_t* peerInfo);
void initPins();

//ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

//Core
void evaulateSerial(String data);
String splitString(String str, char sep, int index);
String formact();

#endif
