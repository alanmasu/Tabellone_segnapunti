/*
      [TABELLONE SEGNAPUNTI WI-FI]
              (tabellone)

      Creato il 09/12/2021

      Note:
        - Utilizzare nuovo protocollo ESP-NOW             [WORKING] [DONE]
        - Prima prova con file di implementazione         [WORKING] [DONE]                     
        - Tolto un delay da 500ms che non so perchè       [DONE] 
          era stato messo  
        
      TO DO:
        - MANCA L'AGGIORNAMENTO DEL LED DI CONNESSIONE    [DONE]
        - Non passa ancora i parametri all'indietro       [WORKING]
            - Costruire il tipo per passaggi all'ind.     [DONE]
            - Modificare le mod. di BACKUP                [DONE]
              - Ripristino di default                     [ONLY TO TRY]
            - Implementare la funzione di invio dei dati  [DONE]
            - CORREGGERE AVAZAMENTO VELOCE DEI SECONDI    [TO DO] [!!!IMPORTANT!!!]
        - Libreria setteSeg                               [TO DO] [NEW LIBRARY]
            - Modificare le librerie per scrivere char    [TO DO] 
              sui display anche char
        - OTA                                             [WORK IN PROGRESS]
            - Implementare funzioni di connessione        [DONE]
            - Implementare funzione di inizializzazione   [DONE]
            - Inserire le funzioni di hadle per l'OTA     [DONE]
            - Inserire la combinazione di tasti per l'OTA [TO DO]
            - Inserire il WebServer da FileSystem         [TO DO]   
            - Scrivere sui display la modalita'           [NEED NEW LIBRARY]                     
        - Modalita CRONOMETRO                             [TO DO]
            - Decidere la combinazione di tasti           [TO DO]
            - Implementare la funzione di cronometro      [TO DO]
            - Scrivere sui display la modalita'           [NEED NEW LIBRARY]                     
*/

#include <tabellone.h>
#include <common.h>

#include "server.h"
esp_now_peer_info_t peerInfo;
extern unsigned long time_c;

uint32_t dt = 0;

void setup() {
  String fileName = __FILE__;
  initSerial(fileName);
  setFileName(fileName);
  if (initEEPROM()) {
    uint32_t time_l = millis();
    rsBackup();
    dt = millis() - time_l;
  }
  Serial.printf("Time to restore data: %d ms\n", dt);
  initMCP();
  initDigits();
  initDisplays();
  initFalli();
  initDuePunti();
  testTab();
  displayWrite();
  initESP_NOW(&peerInfo);
  initPowerFail();
  initRTC();
}

uint32_t serialTimer = 0;

void loop() {
  String serialData;
  readSerial(serialData);                           //Leggi la seriale
  bool nowConnection = checkNowConnection();        //Controlli la connessione
  bool exitingOtaMode = getExitingOtaMode();        //Controlli se stai uscendo da mod. OTA
  if(nowConnection || exitingOtaMode){              //Se sei connesso o stai uscendo da mod. OTA...
    restoreTabMode();                               //Se ti trovi in mod. Orologio allora ti re-imposti a mod. Tab
    time_c = millis();                              //Salvi il timestamp per il passaggio auto da una mod all'altra
    mainProcess();                                  //Elabori i comandi ricevuti
    sendViaNow();                                   //Invii i dati alla pulsantiera
    
    //DEBUG
    // if(exitingOtaMode){
    //   if(millis() - serialTimer > 1000){
    //     Serial.println("Sending data after exiting OTA mode!");
    //     Serial.printf("Mode: %d\n", getMode());
    //     serialTimer = millis();
    //   }
    // }
    //END DEBUG

    if(nowConnection && exitingOtaMode){            //Se sei connesso e stai uscendo da mod. OTA...
      setExitingOtaMode(false);                     //... allora reimposti la variabile di uscita da mod. OTA
    }
  } else if(getMode() != OTA) {                     //Se non sei in mod. OTA...
    automaticMode();                                //... e non sei connesso da almeno time_o ms allora entri in automatico in mod Orologio
  }
  switch (getMode()) {                     
    case tabellone:                                 //Modalita' Tabellone
      displayPrint();                                 //Scrivi i punteggi sui display
      displayPrintOnSerial();                         //Scrivi i punteggi sui display seriali (tool Visual Basic)
      timeOutWrite();                                 //Scrivi i timeout
      break;
    case orologio:                                  //Modalita' Orologio
      oraPrint();                                     //Scrivi l'ora sui display
      oraPrintOnSerial();                             //Scrivi l'ora sui display seriali (tool Visual Basic)
      break;
    case OTA:                                       //Modalita' OTA
      serverLoop();                                   //Loop del WebServer
      OTALoop();                                      //Loop dell'OTA Updater
      OTAPrintOnSerial();
      break;
  }
  duePuntiWrite();                                  //Scrivi i due punti
  delay(50);
}
