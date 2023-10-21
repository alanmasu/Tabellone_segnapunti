#ifndef __PULSANTIERA_H__
#define __PULSANTIERA_H__

#define CONNECTION_LED_PIN 2
#define ESP_NOW_TIMEOUT 10 * 1000UL //10 secondi

typedef struct Comandi {
  bool state[17];
  void print()const;
  void println()const;
} Stati;

//Testate funzioni
//Inizializzazioni
void initSerial(const String &title);
void initMCPs();
void initPins();
void initESPNOW(esp_now_peer_info_t* peerInfo);
void addPeerESPNOW(const uint8_t address[]);
void initWDT();

//Utility
String splitString(String str, char sep, int index);

//ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void sendViaNow();
bool checkNowConnection();

//Core
void readSerial(String &str);
void evaulateSerial(const String &data);
void readButtons();
void evaluateData();
void connectionErrorHandle();

#endif
