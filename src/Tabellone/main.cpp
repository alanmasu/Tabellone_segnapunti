/*    
 *    [TABELLONE SEGNAPUNTI WI-FI] 
 *            (tabellone)
 *    
 *    Creato il 06/12/2021
 *    Modificato il 08/12/2021
 *
 *    Versione 3.21 
 *    
 *    Note:
 *     - Utilizzare nuovo protocollo ESP-NOW            [DONE]
 *     - Prima prova con file di implementazione        [WORKING]
 *     
 *    TO DO:
 *     - MANCA L'AGGIORNAMENTO DEL LED DI CONNESSIONE   [DONE]
 *     - Non passa ancora i parametri all'indietro      [TO DO]
 *     - OTA                                            [TO DO]
 *     
*/

#include <setteSeg.h>

#include <tabellone.h>

extern volatile bool mode;
extern unsigned long time_c;

void setup() {
  //Initialize Serial Monitor
  initSerial(__FILE__);
  if (initEEPROM()) {
    rsBackup();
  }
  initESP_NOW();
  initMCP();
  initDigits();
  initDisplays();
  initFalli();
  initDuePunti();
  testTab();
  displayWrite();
  //  initWiFi();
  initESP_NOW();
  initPowerFail();
  initRTC();
}

void loop() {
  String serialData;
  readSerial(serialData);                           //Leggi la seriale
  if (checkNOWConnection()) {                       //Controlli la connessione
    restoreTabMode();                               //Se ti trovi in mod. Orologio allora ti re-imposti a mod. Tab
    time_c = millis();                              //Salvi il timestamp per il passaggio auto da una mod all'altra
    mainProcess();                                  //Elabori i comandi ricevuti
  } else {
    automaticMode();                                //Se non sei connesso da almeno time_o ms allora entri in auto nella mod Orologio
  }
  if (mode == 0) {                                  //Modalita' Tabellone
    displayPrint();                                 //Scrivi i punteggi sui display
    displayPrintOnSerial();                         //Scrivi i punteggi sui display seriali (tool Visual Basic)
    timeOutWrite();                                 //Scrivi i timeout
    delay(50);
  } else {                                          //Modalita' Orologio
    oraPrint();                                     //Scrivi l'ora sui display
    oraPrintOnSerial();                             //Scrivi l'ora sui display seriali (tool Visual Basic)
    delay(50);
  }
  duePuntiWrite();
}
