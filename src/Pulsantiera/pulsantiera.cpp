#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <pulsantiera.h>
#include <git_revision.h>

// REPLACE WITH THE MAC Address of your receiver 
// uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x3F, 0x54, 0x9C};
uint8_t broadcastAddress[] = {0x7C, 0x9E, 0xBD, 0xEE, 0x8B, 0x7C};

// Variable to store if sending data was successful
String success;

Comandi comandi;

bool shift;


void initSerial(String str){
  Serial.begin(115200); // COM5
  Serial.printf("Git commit hash: %s\n", __GIT_COMMIT__);
}

void initESPNOW(esp_now_peer_info_t* peerInfo){
  //Set devie as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  
  //Setto il canale 
  //Configurazione canale WiFi
  int32_t channel = 1;//getWiFiChannel(WIFI_SSID);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo->peer_addr, broadcastAddress, 6);
  peerInfo->channel = 0;  
  peerInfo->encrypt = false;
  if (esp_now_add_peer(peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  //esp_now_register_recv_cb(OnDataRecv);
  return;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

void evaulateSerial(String data) {
  if (data == "Sei Arduino?") {
    Serial.println("Si Sono Arduino!\n\r");
  } else if (data != "") {
    for (byte i = 0; i < 17; i++) {
        comandi.state[i] = splitString(data, '.', i).toInt();
    }
  }
}

String formact() {
  //Prendi i valori dal globale e trasformali in una stringa
  String text = "";
  int i;
  for (i = 0; i < 16; i++ ) {
    text += String(comandi.state[i]) + ".";
  }
  text += String(comandi.state[16]);
  text += "\r";
  return text;
}

String splitString(String str, char sep, int index) {
  /* str a' la variabile di tipo String che contiene il valore da splittare
     sep a' ia variabile di tipo char che contiene il separatore (bisoga usare l'apostrofo: splitString(xx, 'xxx', yy)
     index a' la variabile di tipo int che contiene il campo splittato: str = "11111:22222:33333" se index= 0;
                                                                       la funzione restituira': "11111"
  */
  int found = 0;
  int strIdx[] = { 0, -1 };
  int maxIdx = str.length() - 1;

  for (int i = 0; i <= maxIdx && found <= index; i++) {
    if (str.charAt(i) == sep || i == maxIdx) {
      found++;
      strIdx[0] = strIdx[1] + 1;
      strIdx[1] = (i == maxIdx) ? i + 1 : i;
    }
  }
  return found > index ? str.substring(strIdx[0], strIdx[1]) : "";
}
