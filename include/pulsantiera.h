#ifndef __PULSANTIERA_H__
#define __PULSANTIERA_H__

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <esp_now.h>
#include <common.h>

#define CONNECTION_LED_PIN 2
#define ESP_NOW_TIMEOUT 10 * 1000UL //10 secondi
#define ESP_T_WDT_TIMEOUT 10 //10 SECONTI PER RESETTARE IL T WDT


//Testate funzioni
//Inizializzazioni
void initSerial(const String &title);
void initMCPs();
void initPins();
void initESPNOW(esp_now_peer_info_t* peerInfo);
void initWDT();
void initWiFi();
void initOTA();

//WiFi
bool checkWiFiConnection();
void reconnectWiFi();

//Utility
String splitString(String str, char sep, int index);

//ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void sendViaNow();
bool checkNowConnection();

//OTA
inline void serverLoop() {
  ArduinoOTA.handle();
}

//Core
void resetWDT();
void readSerial(String &str);
void evaulateSerial(const String &data);
bool serialMode();
void readButtons();
void evaluateData();
void connectionErrorHandle();

#endif
