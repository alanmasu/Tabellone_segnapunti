//Pulsantiera
//SUO MAC: 7c:9e:bd:ee:8b:7c
//MAC a cui inviare: ac:67:b2:3f:54:9c
#include <pulsantiera.h>

#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

//Lettura delle variabili dal main
extern uint8_t broadcastAddress[];
extern Comandi comandi;

esp_now_peer_info_t peerInfo;

void setup() {
  // Init Serial Monitor
  initSerial(__FILE__);
  initESPNOW(&peerInfo);
//  WiFi.mode(WIFI_STA);
//  esp_wifi_set_ps(WIFI_PS_NONE);
//  
//  //Setto il canale 
//  //Configurazione canale WiFi
//  int32_t channel = 1;//getWiFiChannel(WIFI_SSID);
//  esp_wifi_set_promiscuous(true);
//  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
//  esp_wifi_set_promiscuous(false);
//  
//  // Init ESP-NOW
//  if (esp_now_init() != ESP_OK) {
//    Serial.println("Error initializing ESP-NOW");
//    return;
//  }
//  esp_now_register_send_cb(OnDataSent);
//  esp_now_peer_info_t peerInfo;
//  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
//  peerInfo.channel = 0;  
//  peerInfo.encrypt = false;
//  if (esp_now_add_peer(&peerInfo) != ESP_OK){
//    Serial.println("Failed to add peer");
//    return;
//  }
  //esp_now_register_recv_cb(OnDataRecv);

}

void loop() {
  String str = "";
  while(Serial.available()>0){
    str = Serial.readStringUntil('\n');
  }
  evaulateSerial(str);
  Serial.println(formact());
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &comandi, sizeof(comandi));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  
  delay(1000);
}
