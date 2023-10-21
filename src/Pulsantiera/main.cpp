/*    
 *    [TABELLONE SEGNAPUNTI WI-FI] 
 *            (pulsantiera)
 *    
 *    Creato il 06/12/2021
 *    Modificato il 08/12/2021
 *
 *    Versione 3.21 
 *   
 *    Hardware:
 *     - SUO MAC: ac:67:b2:3f:54:9c
 *     - MAC a cui inviare: 7c:9e:bd:ee:8b:7c
 *     
 *    Note:
 *     - Utilizzare nuovo protocollo ESP-NOW            [WORKING] [DONE]
 *     - Prima prova con file di implementazione        [TO DO]
 *     
 *    TO DO:
 *     - MANCA L'AGGIORNAMENTO DEL LED DI CONNESSIONE   [DONE]
 *     - Non passa ancora i parametri all'indietro      [TO DO]
 *     - OTA                                            [TO DO]
 *     
*/

#include <esp_now.h>
#include <pulsantiera.h>
#include <WiFi.h>

// REPLACE WITH THE MAC Address of your receiver 
//extern uint8_t broadcastAddress[];

extern Comandi comandi;
extern Comandi recv;

bool shift;

esp_now_peer_info_t peerInfo;

//Testate funzioni
void evaulateSerial(String data);
String splitString(String str, char sep, int index);

void setup() {
  // Init Serial Monitor
  initSerial(__FILE__);
  initESPNOW(&peerInfo);
}
 
void loop() {
  String str = "";
  while(Serial.available()>0){
    str = Serial.readStringUntil('\n');
  }
  evaulateSerial(str);
  sendOnNow();
  
  delay(150);
}