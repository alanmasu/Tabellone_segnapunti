/*
       [TABELLONE SEGNAPUNTI WI-FI]
              (pulsantiera)

          Creato il: 09/12/2021

      Hardware:
       - SUO MAC:           ac:67:b2:3f:54:9c
       - MAC a cui inviare: 7c:9e:bd:ee:8b:7c

      Note:
       - Utilizzare nuovo protocollo ESP-NOW            [WORKING] [DONE]
       - Prima prova con file di implementazione        [WORKING] [DONE]
          - Capire perche la peer non va nelle          
            funzioni                                    [FIXED]

      TO DO:
       - CONTROLLARE IL REBOOOT                         [IMPORTANTEEEE!!!!!!!!!!!!!!!!!!!!!!!]
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
          - Implementare l'OTA                          [WORKING IN PROGRESS]
          - Spostare il codice file                     [TO DO]
      
       - Controllare le letture dei pulsanti            [TO TRY] [HW]
       - Sistemare gesitone mod. seriale                [WORKING]

*/

#include <Arduino.h>
#include <pulsantiera.h>
#include <esp_now.h>
#include <common.h>



void setup() {
  // Init Serial Monitor
  String title = __FILE__;
  initSerial(title);
  initESPNOW();  
  initMCPs();
  initPins();
  initWDT();
}

void loop() {
  String serialData;
  resetWDT();
  readSerial(serialData);         //Leggi la seriale
  evaulateSerial(serialData);     //Leggi i pulsanti virtuali (Tool VisualBasic)
  if (!serialMode()) {            //Se la modalità seriale non è attiva
    readButtons();                //Leggi i pulsanti hardwere
  }
  if(getMode() != OTA){           //Se il tabellone non e' in modalita' OTA
    sendViaNow();                 //Invii i dati letti al Tabellone
    if (checkNowConnection()) {   //Se ti sono arrivati dati da poco
      evaluateData();             //Intrepreti i dati ricevuti
    } else {                      //Altrimenti
      connectionErrorHandle();    //Gestisci l'errore di connessione
    }
    delay(150);
  }else{                          //Se il tabellone e' in modalita' OTA
    //Serial.println("OTA MODE"); //FOR DEBUG
    if(!getWifiInitialized()){    //Se il WiFi non e' inizializzato
      initWiFi();                 //Inizializza il WiFi
      initOTA();                  //Inizializza l'OTA
    }
    if(checkWiFiConnection()){    //Se il WiFi e' connesso
      OTALoop();                  //Gestisci la connessione OTA
    }else{                        //Altrimenti
      reconnectWiFi();            //Riconnetti il WiFi
    }
    evaluateData();               //Se sono stati ricevuti dati da ESP-NOW intrepretali
    handleWiFiLed();
  }
}
