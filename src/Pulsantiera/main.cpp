/*
       [TABELLONE SEGNAPUNTI WI-FI]
              (pulsantiera)

          Creato il: 06/12/2021
      Modificato il: 08/12/2021

      Versione 5.21_p

      Hardware:
       - SUO MAC: ac:67:b2:3f:54:9c
       - MAC a cui inviare: 7c:9e:bd:ee:8b:7c

      Note:
       - Utilizzare nuovo protocollo ESP-NOW            [WORKING] [DONE]
       - Prima prova con file di implementazione        [WORKING] [DONE]
          - Capire perche la peer non va nelle          
            funzioni                                    [FIXED]
       - Funziona con la versione 4.21 del tabellone    [VERSION COMPATIBILITY]

      TO DO:
       - MANCA L'AGGIORNAMENTO DEL LED DI CONNESSIONE   [DONE]
       - Passaggio all'indietro dei dati                [WORKING]
          - Costruire il tipo per passaggi all'ind.     [DONE]
          - Implementare la funzione di rielaborazione  [DONE]
          - Controllare i LED                           [TO DO] [HW]
       - WDT                                            [TO DO]
          - Implementare l'inizializzazione             [TO DO]
       - OTA                                            [TO DO]
          - Implementare funzioni di connessione        [TO DO]
          - Implementatr funzione di inizializzazione   [TO DO]
       - Controllare le letture dei pulsanti            [TO DO] [HW]

*/

#include <Arduino.h>
#include <pulsantiera.h>
#include <esp_now.h>

esp_now_peer_info_t peerInfo;

void setup() {
  // Init Serial Monitor
  String title = __FILE__;
  initSerial(title);
  initESPNOW(&peerInfo);  
  initMCPs();
  initPins();
  initWDT();
}

void loop() {
  String serialData;
  readSerial(serialData);
  //evaulateSerial(serialData);
  if (serialData != "") {
    evaulateSerial(serialData);
  } else {
    // readButtons();
  }
  sendViaNow();
  if (checkNowConnection()) {
    evaluateData();
  } else {
    connectionErrorHandle();
  }
  delay(150);
}
