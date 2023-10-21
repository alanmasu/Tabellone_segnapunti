/*
      [TABELLONE SEGNAPUNTI WI-FI]
              (tabellone)

      Creato il 09/12/2021
      Modificato il 09/12/2021

      Versione 6.23_t

      Note:
       - Utilizzare nuovo protocollo ESP-NOW            [WORKING] [DONE]
       - Prima prova con file di implementazione        [WORKING]
       - Funziona con la versione 5.21 della puls.      [VERSION COMPATIBILITY]
       
      TO DO:
       - MANCA L'AGGIORNAMENTO DEL LED DI CONNESSIONE   [DONE]
       - Non passa ancora i parametri all'indietro      [WORKING]
          - Costruire il tipo per passaggi all'ind.     [DONE]
          - Modificare le mod. di BACKUP                [DONE]
             - Ripristino di default                    [ONLY TO TRY]
          - Implementare la funzione di invio dei dati  [DONE]
       - OTA                                            [TO DO]
          - Implementare funzioni di connessione        [TO DO]
          - Implementare funzione di inizializzazione   [TO DO]
          
*/

#include <tabellone.h>
esp_now_peer_info_t peerInfo;
extern unsigned long time_c;

void setup() {
  //Initialize Serial Monitor
  uint32_t time_l = millis();
  String title = __FILE__;
  initSerial(title);
  if (initEEPROM()) {
    rsBackup();
  }
  Serial.println("Time to restore data: " + String(millis()-time_l));
  initMCP();
  initDigits();
  initDisplays();
  initFalli();
  initDuePunti();
  testTab();
  displayWrite();
  initWiFi();
  initOTA();
  initESP_NOW(&peerInfo);
  initPowerFail();
  initRTC();
}

void loop() {
  String serialData;
  readSerial(serialData);                           //Leggi la seriale
  if (checkNowConnection()) {                       //Controlli la connessione
    restoreTabMode();                               //Se ti trovi in mod. Orologio allora ti re-imposti a mod. Tab
    time_c = millis();                              //Salvi il timestamp per il passaggio auto da una mod all'altra
    mainProcess();                                  //Elabori i comandi ricevuti
    sendViaNow();                                   //Invii i dati alla pulsantiera
  } else {
    automaticMode();                                //Se non sei connesso da almeno time_o ms allora entri in auto nella mod Orologio
  }
  if (getMode() == tabellone) {                     //Modalita' Tabellone
    displayPrint();                                 //Scrivi i punteggi sui display
    displayPrintOnSerial();                         //Scrivi i punteggi sui display seriali (tool Visual Basic)
    timeOutWrite();                                 //Scrivi i timeout
  } else {                                          //Modalita' Orologio
    oraPrint();                                     //Scrivi l'ora sui display
    oraPrintOnSerial();                             //Scrivi l'ora sui display seriali (tool Visual Basic)
  }
  duePuntiWrite();                                  //Scrivi i due punti
  serverLoop();                                     //Loop del WebServer
  delay(50);
}
