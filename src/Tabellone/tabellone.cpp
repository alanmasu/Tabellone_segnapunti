#include <Arduino.h>
#include <WiFi.h>
#include "server.h"
#include <hardware.h>
#include <Ticker.h>
#include <SPI.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_bt_main.h>
#include <esp_bt.h>
#include <esp_system.h>
#include <setteSeg.h> // ////////////////////////////////////////////// <<<<<--------------- MODIFICAREEEE
#include <tabellone.h>
#include <git_revision.h>
#include <common.h>
#include <hardware.h>

Comandi comandi;
Comandi comandi_p;

//Display
setteSeg pt1;
setteSeg pt2;
setteSeg c_m;
setteSeg c_s;

//Cifre
digit pt1_1(0, 1, 2, 3, 5, 4, 6);
digit pt1_2(8, 9, 10, 11, 13, 12, 14);
digit pt1_3(7, 15);
digit pt2_1(0, 1, 2, 3, 5, 4, 6);
digit pt2_2(8, 9, 10, 11, 13, 12, 14);
digit pt2_3(7, 15);
digit periodo(8, 9, 10, 11, 13, 12, 14);
digit min_1(0, 1, 2, 3, 5, 4, 6);
digit min_2(0, 1, 2, 3, 5, 4, 6);
digit sec_1(8, 9, 10, 11, 13, 12, 14);
digit sec_2(0, 1, 2, 3, 5, 4, 6);
digit falli1(0, 1, 2, 3, 5, 4, 6);
digit falli2(8, 9, 10, 11, 13, 12, 14);

//Pin per controllo falli
const byte f1_1 = 8;
const byte f1_2 = 9;
const byte f1_3 = 10;
const byte f2_1 = 11;
const byte f2_2 = 12;
const byte f2_3 = 13;
const byte mcpTimeout = 4;

//Due punti
volatile bool stateP = 1; //Stato dei due punti
Ticker timer2p;           //Blinker due punti

//Moduli I/O
Adafruit_MCP23017 mcp[6]; //Moduli MCP23017

//Timer
Ticker crono;

//Power Fail
long time_s = 0;                          //Cronometro per la routine di salvatggio (Only for humans)
bool powerFail_state = false;             //Stato del pin
bool powerFail_state0 = false;            //Stato precedente del pin
volatile bool powerFail_event = false;    //PowerFail in corso
extern TaskHandle_t loopTaskHandle;       //Task handle del loop
TaskHandle_t powerFail_t;                 //Task handle del PowerFail

//Valori
//volatile int val[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
//volatile bool stato = false;  //Stato della modalita' di gioco: true - countdown in corso || false - countdown non in corso
//volatile bool mode = 0;       //Modalita di finzionamento: 0 - Tabellone || 1 - Orologio (o RTC o dall'accensione)
//bool modeImpostata = false;   //Per ripristino automantico della mod. tabellone alla riconnessione
Valori valori;                  //Struttura che contiene i valore del tabellone
const long time_o = 30 * 1000;  //Timeout per passaggio automantico alla mod. orologio
unsigned long time_c;           //Tempo dall'ultima connessione della pulsantiera

//Per avanzamento veloce
unsigned long time_p;           //Tempo dalla pressione del tasto

//ESP-NOW
#ifndef PULSANTEIRA_MAC_ADDRESS
  uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x3F, 0x54, 0x9C}; //7c:9e:bd:ee:8b:7c
#else
  uint8_t broadcastAddress[] = PULSANTEIRA_MAC_ADDRESS;
#endif
uint32_t lastMessageFromNOW = 0;  //Ultimo messaggio ricevuto
bool ESP_NOWState = 0;            //Stato di ESP-NOW
esp_now_peer_info_t peerInfo;

//Time
RTC_DS3231 Clock;   //Clock di sistema collegato in I2C
byte minuti;        //Minuti
byte ore;           //Ore
bool RTC;           //Stato di configurazione RTC

// //WiFi
// char ssid[] = "Tabellone";
// char pass[] = "Tabellone";


//Definizioni delle funzioni
void initSerial(String &title) {
  Serial.begin(115200); // COM5
  Serial.printf("Git commit hash: %s, File: %s\n", __GIT_COMMIT__, title.c_str());
}

bool initEEPROM() {
  //EEPROM
  return EEPROM.begin(512);
}

void rsBackup() {
  byte address = 0;
  EEPROM.get(address, valori);
  if( valori.stato != stop || valori.stato != run){
    valori.stato = stop;
    EEPROMSave();
  }
  if( valori.mode != tabellone || valori.mode != orologio){
    valori.mode = tabellone;
    EEPROMSave();
  }
  address += sizeof(valori);

}

bool EEPROMSave() {
  byte address = 0;
  //  for (int i = 0; i < 9; i++) {
  //    EEPROM.write(i, valori.val[i]);
  //  }
  //  EEPROM.write(9, stato);
  EEPROM.put(address, valori);
  address += sizeof(valori);
  if (EEPROM.commit()) {
    return true;
  } else {
    return false;
  }
  return true;
}

void initMCP() {
  for (int i = 0; i < 6; i++) {
    mcp[i].begin(i);
  }
}

void initDigits() {
  pt1_1.begin('k', mcp[0]);
  pt1_2.begin('k', mcp[0]);
  pt1_3.begin('k', mcp[0]);
  pt2_1.begin('k', mcp[1]);
  pt2_2.begin('k', mcp[1]);
  pt2_3.begin('k', mcp[1]);
  min_1.begin('k', mcp[2]);
  min_2.begin('k', mcp[3]);
  sec_1.begin('k', mcp[2]);
  sec_2.begin('k', mcp[4]);
  falli1.begin('k', mcp[5]);
  falli2.begin('k', mcp[5]);
  periodo.begin('k', mcp[3]);
}

void initDisplays() {
  pt1 = setteSeg(pt1_3, pt1_2, pt1_1);
  pt2 = setteSeg(pt2_3, pt2_2, pt2_1);
  c_m = setteSeg(min_2, min_1);
  c_s = setteSeg(sec_2, sec_1);
  pt1.begin('2');
  pt2.begin('2');
  c_m.begin('1');
  c_s.begin('1');
}

void initFalli() {
  mcp[mcpTimeout].pinMode(f1_1, OUTPUT);
  mcp[mcpTimeout].pinMode(f1_2, OUTPUT);
  mcp[mcpTimeout].pinMode(f1_3, OUTPUT);
  mcp[mcpTimeout].pinMode(f2_1, OUTPUT);
  mcp[mcpTimeout].pinMode(f2_2, OUTPUT);
  mcp[mcpTimeout].pinMode(f2_3, OUTPUT);
}

void initDuePunti() {
  mcp[2].pinMode(7, OUTPUT);
  mcp[2].pinMode(15, OUTPUT);
}

void IRAM_ATTR duePunti() {
  stateP = !stateP;
}

void reset() {
  pt1.write(0);
  pt2.write(0);
  periodo.write(0);
  c_m.write(0);
  c_s.write(0);
  falli1.write(0);
  falli2.write(0);
  mcp[mcpTimeout].digitalWrite(f1_1, 0);
  mcp[mcpTimeout].digitalWrite(f1_2, 0);
  mcp[mcpTimeout].digitalWrite(f1_3, 0);
  mcp[mcpTimeout].digitalWrite(f2_1, 0);
  mcp[mcpTimeout].digitalWrite(f2_2, 0);
  mcp[mcpTimeout].digitalWrite(f2_3, 0);
}

void clearTab() {
  pt1.clear();
  pt2.clear();
  periodo.clear();
  c_m.clear();
  c_s.clear();
  falli1.clear();
  falli2.clear();
  mcp[mcpTimeout].digitalWrite(f1_1, 0);
  mcp[mcpTimeout].digitalWrite(f1_2, 0);
  mcp[mcpTimeout].digitalWrite(f1_3, 0);
  mcp[mcpTimeout].digitalWrite(f2_1, 0);
  mcp[mcpTimeout].digitalWrite(f2_2, 0);
  mcp[mcpTimeout].digitalWrite(f2_3, 0);
}

void testTab() {
  pt1.test();
  pt2.test();
  periodo.test();
  c_m.test();
  c_s.test();
  falli1.test();
  falli2.test();
  mcp[mcpTimeout].digitalWrite(f1_1, 1);
  mcp[mcpTimeout].digitalWrite(f1_2, 1);
  mcp[mcpTimeout].digitalWrite(f1_3, 1);
  mcp[mcpTimeout].digitalWrite(f2_1, 1);
  mcp[mcpTimeout].digitalWrite(f2_2, 1);
  mcp[mcpTimeout].digitalWrite(f2_3, 1);
  mcp[2].digitalWrite(7, 1);
  mcp[2].digitalWrite(15, 1);
  delay(2000);
  reset();
}

void displayWrite() {
  pt1.write(valori.val[0]);
  pt2.write(valori.val[1]);
  periodo.write(valori.val[2]);
  c_m.write(valori.val[3]);
  c_s.write(valori.val[4]);
  falli1.write(valori.val[5]);
  falli2.write(valori.val[6]);
}

bool initESP_NOW() {
  pinMode(CONNECTION_LED, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  WiFi.mode(WIFI_AP_STA);
  
  //Disabilito il controllo potenza WiFi
  esp_wifi_set_ps(WIFI_PS_NONE);

  //Setto il canale
  //Configurazione canale WiFi
  int32_t channel = 1;//getWiFiChannel(WIFI_SSID);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    ESP_NOWState = false;
    return false;
  }
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_err_t peer = esp_now_add_peer(&peerInfo);

  if (peer != ESP_OK) {
    Serial.print("Failed to add peer: ");
    switch (peer) {
      case ESP_ERR_ESPNOW_NOT_INIT:
        Serial.println("ESP_ERR_ESPNOW_NOT_INIT");
        break;
      case ESP_ERR_ESPNOW_ARG:
        Serial.println("ESP_ERR_ESPNOW_ARG");
        break;
      case ESP_ERR_ESPNOW_NOT_FOUND:
        Serial.println("ESP_ERR_ESPNOW_NOT_FOUND");
        break;
    }
  }
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
  ESP_NOWState = true;
  return true;
}

void initPowerFail() {
  pinMode(15, INPUT);
  xTaskCreate(
    powerFailTaskRoutine,   /* Task function. */
    "POWERFAIL_T",          /* name of task. ONLY FOR HUMANS*/
    10000,                  /* Stack size of task */
    NULL,                   /* parameter of the task */
    1,                      /* priority of the task */
    &powerFail_t);          /* Task handle to keep track of created task */
}

void initRTC() {
  RTC = Clock.begin();
  if (RTC) {
    DateTime now = Clock.now();
    minuti = now.minute();
    ore = now.hour();
    Serial.println(getTime());
  }
}

//---------------------------------------------------------------------------------------------ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    digitalWrite(13,HIGH);
  } else {
    digitalWrite(13,LOW);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&comandi, incomingData, sizeof(comandi));
  lastMessageFromNOW = millis();
}

void sendViaNow() {
  if (ESP_NOWState) {
    esp_now_send(broadcastAddress, (uint8_t *) &valori, sizeof(valori));
  }
}

bool checkNowConnection() {
  bool nowState = millis() - lastMessageFromNOW > ESP_NOW_MAX_TIMEOUT ? false : true;
  if (nowState) {
    digitalWrite(CONNECTION_LED, HIGH);
  } else {
    digitalWrite(CONNECTION_LED, LOW);
  }
  return nowState;
}

//PowerFail
void powerFailTaskRoutine(void * pvParameters) {
  Serial.println("POWERFAIL DETECTOR IS RUNNING");
  while (1) {
    powerFail_state = digitalRead(15);
    if (!powerFail_state && powerFail_state0 != powerFail_state) {
      time_s = millis();
      Serial.println("POWERFAIL DETECTOR WAS TRIGGERED");
      vTaskSuspend(loopTaskHandle);
      powerFail_event = true;
      Serial.println("STOPPING CONNECTIONS");
      esp_wifi_stop();
      esp_bluedroid_disable();
      esp_bt_controller_disable();
      //powerFailReset();
      digitalWrite(2, 0);
      Serial.println("SAVING DATA");
      if (EEPROMSave()) {
        Serial.println("SAVING SUCCESFUL");
        Serial.println("TIME FROM POWERFAIL TRIGGERING: " + String(millis() - time_s));
      }
    } else if (powerFail_state && powerFail_state0 != powerFail_state) {
      if (powerFail_event) {
        Serial.println("POWERFAIL RETURNED");
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        vTaskResume(loopTaskHandle);
        powerFail_event = false;
      }
    }
    powerFail_state0 = powerFail_state;
    vTaskDelay(10);
  }
  vTaskDelete(powerFail_t);
}

//-----------------------------------------------------------------------------------------UILITY
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

void impostaOra(byte minInt, byte oraInt) {
  if (RTC) {
    Clock.adjust(DateTime(2020, 5, 26, oraInt, minInt, 0));
    minuti = minInt;
    ore = oraInt;
    //    Serial.println("Ora: " + getTime());
  }
}

String getTime() {
  if (RTC) {
    DateTime now = Clock.now();
    return String(now.hour()) + ":" + String(now.minute());
  } else {
    return "ERR";
  }
}

void tik() {
  if (valori.val[4] == 0) {
    valori.val[4] = 59;
    valori.val[3] --;
  } else {
    valori.val[4]--;
  }
  if (valori.val[3] == 0 && valori.val[4] == 0) {
    finishTime();
  }
}

//----------------------------------------------------------------------------------- OTA
// void serverLoop() {
//   // ArduinoOTA.handle();
// }

//----------------------------------------------------------------------------------- CORE
// String readSerial(String &str);
// void restoreTabMode();
// void mainProcess();
// void automaticMode();
// void displayPrint();
// void timeOutWrite();
// void displayPrintOnSerial();
// void oraPrint();
// void oraPrintOnSerial();
// void duePuntiWrite();
// void finishTime();



//CORE
void readSerial(String & str) {
  str = "";
  while (Serial.available() > 0) {
    str = Serial.readStringUntil('\n');
  }
}

void restoreTabMode() {
  if (valori.modeImpostata  == false && valori.mode == orologio) {
    valori.mode = tabellone;
    stateP = HIGH;
    timer2p.detach();
    displayWrite();
    delay(25);
  }
}

void mainProcess() {
  bool isOTAcmd = comandi.state[13] && comandi.state[14] && comandi.state[15];
  for (byte i = 0; i < 16; i++) {
    if (comandi.state[i] == 1 ) {
      if (comandi.state[i] != comandi_p.state[i]) {
        time_p = millis();
        if (valori.mode == tabellone) {
          if (comandi.state[16] == 0) {//Shift non premuto in mod tab
            switch (i) {
              case 0:
                valori.val[0] = valori.val[0] == 199 ? 0 : valori.val[0] + 1;
                break;
              case 1:
                valori.val[0] = valori.val[0] == 0 ? 199 : valori.val[0] - 1;
                break;
              case 2:
                valori.val[1] = valori.val[1] == 199 ? 0 : valori.val[1] + 1;
                break;
              case 3:
                valori.val[1] = valori.val[1] == 0 ? 199 : valori.val[1] - 1;
                break;
              case 4:
                if (valori.stato  == stop) {
                  valori.val[0] = 0;
                  valori.val[1] = 0;
                }
                break;
              case 5:
                valori.val[2] = valori.val[2] == 9 ? 0 : valori.val[2] + 1;
                break;
              case 6:
                valori.val[2] = valori.val[2] == 0 ? 9 : valori.val[2] - 1;
                break;
              case 7:
                if (valori.stato  == stop) {
                  valori.val[2] = 0;
                }
                break;
              case 8:
                valori.val[3] = valori.val[3] == 99 ? 0 : valori.val[3] + 1;
                break;
              case 9:
                valori.val[3] = valori.val[3] == 0 ? 99 : valori.val[3] - 1;
                break;
              case 10:
                if (valori.stato  == stop) {
                  if (valori.val[4] == 59) {
                    valori.val[3] = valori.val[3] == 99 ? 0 : valori.val[3] + 1;
                  }
                  valori.val[4] = valori.val[4] == 59 ? 0 : valori.val[4] + 1;
                }
                break;
              case 11:
                if (valori.stato  == stop) {
                  if (valori.val[4] == 0) {
                    valori.val[3] = valori.val[3] == 0 ? 99 : valori.val[3] - 1;
                  }
                  valori.val[4] = valori.val[4] == 0 ? 59 : valori.val[4] - 1;
                }
                break;
              case 12:
                if (valori.stato  == stop) {
                  valori.val[3] = 0;
                  valori.val[4] = 0;
                }
                break;
              case 13://P
                if (valori.val[3] != 0 || valori.val[4] != 0) {
                  crono.attach(1, tik);
                  timer2p.attach(0.5, duePunti);
                  valori.stato = run;
                }
                break;
              case 14://S
                crono.detach();
                timer2p.detach();
                stateP = true;
                valori.stato = stop;
                break;
              case 15:
                if (valori.stato == stop) {
                  for (byte i = 0; i < 9; i++) {
                    valori.val[i] = 0;
                  }
                }
                break;
            }
          } else { //Shift Premuto in mod. tab
            switch (i) {
              case 0: //Falli 1
                valori.val[5] = valori.val[5] == 5 ? 0 : valori.val[5] + 1;
                break;
              case 1:
                valori.val[5] = valori.val[5] == 0 ? 5 : valori.val[5] - 1;
                break;
              case 2: //Falli 2
                valori.val[6] = valori.val[6] == 5 ? 0 : valori.val[6] + 1;
                break;
              case 3:
                valori.val[6] = valori.val[6] == 0 ? 5 : valori.val[6] - 1;
                break;
              case 4: //Falli reset
                if (valori.stato  == stop) {
                  valori.val[5] = 0;
                  valori.val[6] = 0;
                }
                break;
              case 5:
                valori.val[2] = valori.val[2] == 9 ? 0 : valori.val[2] + 1;
                break;
              case 6:
                valori.val[2] = valori.val[2] == 0 ? 9 : valori.val[2] - 1;
                break;
              case 7:
                if (valori.stato  == stop) {
                  valori.val[2] = 0;
                }
                break;
              case 8:
                valori.val[7] = valori.val[7] == 3 ? 0 : valori.val[7] + 1;
                crono.detach();
                timer2p.detach();
                valori.stato  = stop;
                stateP = 1;
                break;
              case 9:
                valori.val[7] = valori.val[7] == 0 ? 3 : valori.val[7] - 1;
                crono.detach();
                timer2p.detach();
                valori.stato  = stop;
                stateP = 1;
                break;
              case 10:
                valori.val[8] = valori.val[8] == 3 ? 0 : valori.val[8] + 1;
                crono.detach();
                timer2p.detach();
                valori.stato  = stop;
                stateP = 1;
                break;
              case 11:
                valori.val[8] = valori.val[8] == 0 ? 3 : valori.val[8] - 1;
                crono.detach();
                timer2p.detach();
                valori.stato  = stop;
                stateP = 1;
                break;
              case 12:
                if (valori.stato  == stop) {
                  valori.val[7] = 0;
                  valori.val[8] = 0;
                }
                break;
              case 13://Orologio
                if (valori.stato  == stop && valori.mode != orologio) {
                  valori.mode = orologio;
                  clearTab();
                  valori.modeImpostata  = true;
                  timer2p.attach(0.5, duePunti);
                }
                break;
              case 14://Tabellone
                valori.mode = tabellone;
                valori.modeImpostata  = false;
                timer2p.detach();
                stateP = true;
                displayWrite();
                break;
              case 15:
                if (valori.stato  == stop) {
                  for (byte i = 0; i < 9; i++) {
                    valori.val[i] = 0;
                  }
                }
                break;
            }
          }
        } else if (valori.mode == orologio) { //Modalità orologio
          if (RTC) {
            DateTime now = Clock.now();
            minuti = now.minute();
            ore = now.hour();
          }
          if (comandi.state[16] == 1) { //Shift premuto in mod Orologio
            switch (i) {
              case 8:
                ore = ore >= 24 ? 0 : ore + 1;
                impostaOra(minuti, ore);
                break;
              case 9:
                ore = ore <= 0 ? 24 : ore - 1;
                impostaOra(minuti, ore);
                break;
              case 10:
                minuti = minuti >= 59 ? 0 : minuti + 1;
                impostaOra(minuti, ore);
                break;
              case 11:
                minuti = minuti <= 0 ? 59 : minuti - 1;
                impostaOra(minuti, ore);
                break;
              case 14://S
                valori.mode = tabellone;
                Serial.println("STOP + SHIFT IN OROLOGIO");
                stateP = true;
                valori.modeImpostata = false;
                // timer2p.detach();
                // displayWrite();
                displayPrintOnSerial();
                break;
            }
          }
        }
      } else {
        if (millis() - time_p > 1000 ) {          // PASSATI 1 SECONDI DALLA PRESSIONE SI SALE DI 5 ALLA VOLTA
          if (valori.mode == tabellone) {         // Modalità tabellone
            if (isOTAcmd && valori.stato != run) {
              enteringOtaMode();
              valori.mode = OTA;
            } else if (comandi.state[16] == 0) {  // Shift non premuto in mod tabellone
              switch (i) {
                case 0:
                  valori.val[0] = (valori.val[0] + 5) >= 199 ? 0 : valori.val[0] + 5;
                  break;
                case 1:
                  valori.val[0] = (valori.val[0] - 5) <= 0 ? 199 : valori.val[0] - 5;
                  break;
                case 2:
                  valori.val[1] = (valori.val[1] + 5) >= 199 ? 0 : valori.val[1] + 5;
                  break;
                case 3:
                  valori.val[1] = (valori.val[1] - 5) <= 0 ? 199 : valori.val[1] - 5;
                  break;
                case 8:
                  valori.val[3] = (valori.val[3] + 5) >= 99 ? 0 : valori.val[3] + 5;
                  break;
                case 9:
                  valori.val[3] = (valori.val[3] - 5) <= 0 ? 99 : valori.val[3] - 5;
                  break;
                case 10:
                  if (valori.stato  == stop) {
                    if ((valori.val[4] + 5) >= 59) {
                      valori.val[3] = valori.val[3] == 99 ? 0 : valori.val[3] + 5;
                    }
                    valori.val[4] = valori.val[4] == 59 ? 0 : valori.val[4] + 5;
                  }
                  break;
                case 11:
                  if (valori.stato  == stop) {
                    if ((valori.val[4] - 5) <= 0) {
                      valori.val[3] = (valori.val[3] + 5) >= 0 ? 99 : valori.val[3] - 5;
                    }
                    valori.val[4] = valori.val[4] == 0 ? 59 : valori.val[4] - 5;
                  }
                  break;
              }
            }
          } else if (valori.mode == orologio) { //Modalità orologio
            if (RTC) {
              DateTime now = Clock.now();
              minuti = now.minute();
              ore = now.hour();
            }
            if (comandi.state[16] == 1) {       //Shift premuto in mod Orologio
              switch (i) {
                case 8:
                  ore = ore >= 24 ? 0 : ore + 1;
                  impostaOra(minuti, ore);
                  break;
                case 9:
                  ore = ore <= 0 ? 24 : ore - 1;
                  impostaOra(minuti, ore);
                  break;
                case 10:
                  minuti = minuti >= 59 ? 0 : minuti + 5;
                  impostaOra(minuti, ore);
                  break;
                case 11:
                  minuti = minuti <= 0 ? 59 : minuti - 5;
                  impostaOra(minuti, ore);
                  break;
                case 14://S
                  valori.mode = tabellone;
                  Serial.println("STOP + SHIFT IN OROLOGIO");
                  // timer2p.detach();
                  // displayWrite();
                  displayPrintOnSerial();
                  break;
              }
            }
          }
          // delay(500);
        }
      }
    }
  }
  for (byte i = 0; i < 17; i++) {
    comandi_p.state[i] = comandi.state[i];
  }
}

void clearCommands(){
  Serial.println("Cleared commands!");
  comandi.state[13] = 0;
  comandi.state[14] = 0;
  comandi.state[15] = 0;
  comandi_p.state[13] = 0;
  comandi_p.state[14] = 0;
  comandi_p.state[15] = 0;
}

void automaticMode() {
  if (millis() - time_c > time_o && valori.mode != orologio && valori.stato != run) {
    valori.mode = orologio;
    timer2p.attach(0.5, duePunti);
    clearTab();
  }
}

Mode getMode() {
  return valori.mode;
}

void setMode(Mode mode) {
  valori.mode = mode;
  Serial.printf("Setted mode to %d\n", valori.mode);
}

void displayPrint() {
  if (pt1.read() != valori.val[0]) {
    pt1.write(valori.val[0]);
  }
  if (pt2.read() != valori.val[1]) {
    pt2.write(valori.val[1]);
  }
  if (periodo.read() != valori.val[2]) {
    periodo.write(valori.val[2]);
  }
  if (c_m.read() != valori.val[3]) {
    c_m.write(valori.val[3]);
  }
  if (c_s.read() != valori.val[4]) {
    c_s.write(valori.val[4]);
  }
  if (falli1.read() != valori.val[5]) {
    falli1.write(valori.val[5]);
  }
  if (falli2.read() != valori.val[6]) {
    falli2.write(valori.val[6]);
  }
}

void timeOutWrite() {
  switch (valori.val[7]) {
    case 0:
      mcp[mcpTimeout].digitalWrite(f1_1, 0);
      mcp[mcpTimeout].digitalWrite(f1_2, 0);
      mcp[mcpTimeout].digitalWrite(f1_3, 0);
      break;
    case 1:
      mcp[mcpTimeout].digitalWrite(f1_1, 1);
      mcp[mcpTimeout].digitalWrite(f1_2, 0);
      mcp[mcpTimeout].digitalWrite(f1_3, 0);
      break;
    case 2:
      mcp[mcpTimeout].digitalWrite(f1_1, 1);
      mcp[mcpTimeout].digitalWrite(f1_2, 1);
      mcp[mcpTimeout].digitalWrite(f1_3, 0);
      break;
    case 3:
      mcp[mcpTimeout].digitalWrite(f1_1, 1);
      mcp[mcpTimeout].digitalWrite(f1_2, 1);
      mcp[mcpTimeout].digitalWrite(f1_3, 1);
      break;
  }
  switch (valori.val[8]) {
    case 0:
      mcp[mcpTimeout].digitalWrite(f2_1, 0);
      mcp[mcpTimeout].digitalWrite(f2_2, 0);
      mcp[mcpTimeout].digitalWrite(f2_3, 0);
      break;
    case 1:
      mcp[mcpTimeout].digitalWrite(f2_1, 1);
      mcp[mcpTimeout].digitalWrite(f2_2, 0);
      mcp[mcpTimeout].digitalWrite(f2_3, 0);
      break;
    case 2:
      mcp[mcpTimeout].digitalWrite(f2_1, 1);
      mcp[mcpTimeout].digitalWrite(f2_2, 1);
      mcp[mcpTimeout].digitalWrite(f2_3, 0);
      break;
    case 3:
      mcp[mcpTimeout].digitalWrite(f2_1, 1);
      mcp[mcpTimeout].digitalWrite(f2_2, 1);
      mcp[mcpTimeout].digitalWrite(f2_3, 1);
      break;
  }
}

void displayPrintOnSerial() {
  String toSendSerial = "";
  for (int i = 0; i < 8; i++) {
    toSendSerial += String(valori.val[i]) + ".";
  }
  toSendSerial += String(valori.val[8]);
  Serial.println(toSendSerial);
}

void oraPrint() {
  if (RTC) {
    DateTime now = Clock.now();
    int minInt = now.minute();
    int hourInt = now.hour();
    if (c_m.read() != hourInt) {
      c_m.write(hourInt);
    }
    if (c_s.read() != minInt) {
      c_s.write(minInt);
    }
  }
}

void oraPrintOnSerial() {
  String toSendSerial = "";
  if (RTC) {
    DateTime now = Clock.now();
    int minInt = now.minute();
    int hourInt = now.hour();

    for (int i = 0; i < 3; i++) {
      toSendSerial += "-.";
    }
    toSendSerial += String(hourInt) + ".";
    toSendSerial += String(minInt);
    for (int i = 0; i < 4; i++) {
      toSendSerial += ".-";
    }
    Serial.println(toSendSerial);
  } else {
    for (int i = 0; i < 3; i++) {
      toSendSerial += "-.";
    }
    toSendSerial += String(((int)(millis() *  2.7E-7)) % 23) + ".";
    toSendSerial += String(((int)(millis() * 16.6E-6)) % 60);
    for (int i = 0; i < 4; i++) {
      toSendSerial += ".-";
    }
    Serial.println(toSendSerial);
  }
}

void OTAPrintOnSerial(){
  Serial.println("-.-.-.ot.a.-.-.-.-");
}

void duePuntiWrite() {
  mcp[2].digitalWrite(7, stateP);
  mcp[2].digitalWrite(15, stateP);
}

void finishTime() {
  //All'evento tempo finito esegui:
  stateP = true;
  crono.detach();
  timer2p.detach();
  valori.stato  = stop;
}
