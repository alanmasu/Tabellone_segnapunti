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

//Ininizializzazioni
void initSerial(String str);
void initESPNOW(esp_now_peer_info_t* peerInfo);
void sendOnNow();

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

// Callback when data is received
//void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
//  memcpy(&recv, incomingData, sizeof(recv));
//  for(int i = 0; i<16;i++){
//    Serial.print(recv.state[i] + ".");
//  }
//  Serial.println(recv.state[16]);
//}

//Core
void evaulateSerial(String data);
String splitString(String str, char sep, int index);
#endif
