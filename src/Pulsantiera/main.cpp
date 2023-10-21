/*
       [TABELLONE SEGNAPUNTI WI-FI]
              (pulsantiera)

          Creato il: 09/12/2021
      Modificato il: 09/12/2021

      Versione 6.21_p

      Hardware:
       - SUO MAC:           ac:67:b2:3f:54:9c
       - MAC a cui inviare: 7c:9e:bd:ee:8b:7c

      Note:
       - Utilizzare nuovo protocollo ESP-NOW            [WORKING] [DONE]
       - Prima prova con file di implementazione        [WORKING] [DONE]
          - Capire perche la peer non va nelle          
            funzioni                                    [FIXED]
       - Accoppiata con la versione 5.21 del tabellone  [VERSION COMPATIBILITY]

      TO DO:
       - MANCA L'AGGIORNAMENTO DEL LED DI CONNESSIONE   [DONE]
       - Passaggio all'indietro dei dati                [WORKING]
          - Costruire il tipo per passaggi all'ind.     [DONE]
          - Implementare la funzione di rielaborazione  [DONE]
          - Controllare i LED                           [TO TRY] [HW]
          - Modificare l'accensione di START solo se
            il crono lo permette                        [TO TRY] [HW]
       - WDT                                            [WORKING]
          - Implementare l'inizializzazione             [DONE]
       - OTA                                            [TO DO]
          - Implementare funzioni di connessione        [TO DO]
          - Implementatr funzione di inizializzazione   [TO DO]
       - Controllare le letture dei pulsanti            [TO TRY] [HW]
       - Sistemare gesitone mod. seriale                [WORKING]

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
  initWiFi();
  initOTA();
}

void loop() {
  String serialData;
  resetWDT();
  readSerial(serialData);         //Leggi la seriale
  evaulateSerial(serialData);     //Leggi i pulsanti virtuali (Tool VisualBasic)
  if (!serialMode()) {            //Se la modalità seriale non è attiva
    readButtons();                //Leggi i pulsanti hardwere
  }
  sendViaNow();                   //Invii i dati letti al Tabellone
  if (checkNowConnection()) {     //Se ti sono arrivati dati da poco
    evaluateData();               //Intrepreti i dati ricevuti
  } else {                        //Altrimenti
    connectionErrorHandle();      //Gestisci l'errore di connessione
  }
  delay(150);
  if(checkWiFiConnection()){
    serverLoop();
  }else{
    reconnectWiFi();
  }
}
