#include <Arduino.h>
#include <WiFi.h>
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
volatile int val[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile bool stato = false;  //Stato della modalita' di gioco: true - countdown in corso || false - countdown non in corso
volatile bool mode = 0;       //Modalita di finzionamento: 0 - Tabellone || 1 - Orologio (o RTC o dall'accensione)
bool modeImpostata = false;   //Per ripristino automantico della mod. tabellone alla riconnessione
const long time_o = 30 * 1000;//Timeout per passaggio automantico alla mod. orologio
byte state[17];               //PT1+,PT1-,PT2+,PT2-,PTR,PER+,PER-,PERr,MIN+,MIN-,SEC+,SEC-,TR,P,S,R,SHIFT
byte state_p[17];             //Stati vecchi dei pulsanti per fronte
unsigned long time_c;         //Tempo dall'ultima connessione della pulsantiera

//Per avanzamento veloce
unsigned long time_p;         //Tempo dalla pressione del tasto

//ESP-NOW
uint32_t lastMessageFromNOW = 0;  //Ultimo messaggio ricevuto
bool ESP_NOWState = 0;            //Stato di ESP-NOW

//Time
RTC_DS3231 Clock;   //Clock di sistema collegato in I2C
byte minuti;        //Minuti
byte ore;           //Ore
bool RTC;           //Stato di configurazione RTC


//Definizioni delle funzioni
void initSerial(String str) {
  Serial.begin(115200); // COM5
  Serial.printf("Git commit hash: %s, File: %s\n", __GIT_COMMIT__, str.c_str());
}

bool initEEPROM() {
  //EEPROM
  return EEPROM.begin(10);
}

void rsBackup() {
  //Ripristino dati dell'ultima sessione
  for (byte i = 0; i < 9; i++) {
    val[i] = EEPROM.readInt(i);
  }
  stato = EEPROM.readInt(9);
}

bool EEPROMSave() {
  for (int i = 0; i < 9; i++) {
    EEPROM.write(i, val[i]);
  }
  EEPROM.write(9, stato);
  if (EEPROM.commit()) {
    return true;
  } else {
    return false;
  }
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
  pt1.write(val[0]);
  pt2.write(val[1]);
  periodo.write(val[2]);
  c_m.write(val[3]);
  c_s.write(val[4]);
  falli1.write(val[5]);
  falli2.write(val[6]);
}

bool initESP_NOW() {
  pinMode(CONNECTION_LED, OUTPUT);
  //Set device as a Wi-Fi Station
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
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&comandi, incomingData, sizeof(comandi));
  lastMessageFromNOW = millis();
  // for(int i = 0; i < 17; i++){
  //   Serial.print(comandi.state[i]);
  // }
  // Serial.println();
}

bool checkNOWConnection() {
  bool nowState = millis() - lastMessageFromNOW > ESP_NOW_MAX_TIMEOUT ? false : true;
  if(nowState){
    digitalWrite(CONNECTION_LED, HIGH);
  }else{
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
  if (val[4] == 0) {
    val[4] = 59;
    val[3] --;
  } else {
    val[4]--;
  }
  if (val[3] == 0 && val[4] == 0) {
    finishTime();
  }
}

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
void readSerial(String &str){
  str = "";
  while(Serial.available()>0){
    str = Serial.readStringUntil('\n');
  }
}

void restoreTabMode() {
  if (modeImpostata == false && mode == 1) {
    mode = 0;
    stateP = HIGH;
    timer2p.detach();
    displayWrite();
    delay(25);
  }
}

void mainProcess() {
  for (byte i = 0; i < 16; i++) {
    if (comandi.state[i] == 1 ) {
      if (comandi.state[i] != comandi_p.state[i]) {
        time_p = millis();
        if (mode == 0) {
          if (comandi.state[16] == 0) {//Shift non premuto in mod tab
            switch (i) {
              case 0:
                val[0] = val[0] == 199 ? 0 : val[0] + 1;
                break;
              case 1:
                val[0] = val[0] == 0 ? 199 : val[0] - 1;
                break;
              case 2:
                val[1] = val[1] == 199 ? 0 : val[1] + 1;
                break;
              case 3:
                val[1] = val[1] == 0 ? 199 : val[1] - 1;
                break;
              case 4:
                if (stato == false) {
                  val[0] = 0;
                  val[1] = 0;
                }
                break;
              case 5:
                val[2] = val[2] == 9 ? 0 : val[2] + 1;
                break;
              case 6:
                val[2] = val[2] == 0 ? 9 : val[2] - 1;
                break;
              case 7:
                if (stato == false) {
                  val[2] = 0;
                }
                break;
              case 8:
                val[3] = val[3] == 99 ? 0 : val[3] + 1;
                break;
              case 9:
                val[3] = val[3] == 0 ? 99 : val[3] - 1;
                break;
              case 10:
                if (stato == false) {
                  if (val[4] == 59) {
                    val[3] = val[3] == 99 ? 0 : val[3] + 1;
                  }
                  val[4] = val[4] == 59 ? 0 : val[4] + 1;
                }
                break;
              case 11:
                if (stato == false) {
                  if (val[4] == 0) {
                    val[3] = val[3] == 0 ? 99 : val[3] - 1;
                  }
                  val[4] = val[4] == 0 ? 59 : val[4] - 1;
                }
                break;
              case 12:
                if (stato == false) {
                  val[3] = 0;
                  val[4] = 0;
                }
                break;
              case 13://P
                if (val[3] != 0 || val[4] != 0) {
                  crono.attach(1, tik);
                  timer2p.attach(0.5, duePunti);
                  stato = true;
                }
                break;
              case 14://S
                crono.detach();
                timer2p.detach();
                stateP = true;
                stato = false;
                break;
              case 15:
                if (stato == false) {
                  for (byte i = 0; i < 9; i++) {
                    val[i] = 0;
                  }
                }
                break;
            }
          } else { //Shift Premuto in mod. tab
            switch (i) {
              case 0: //Falli 1
                val[5] = val[5] == 5 ? 0 : val[5] + 1;
                break;
              case 1:
                val[5] = val[5] == 0 ? 5 : val[5] - 1;
                break;
              case 2: //Falli 2
                val[6] = val[6] == 5 ? 0 : val[6] + 1;
                break;
              case 3:
                val[6] = val[6] == 0 ? 5 : val[6] - 1;
                break;
              case 4: //Falli reset
                if (stato == false) {
                  val[5] = 0;
                  val[6] = 0;
                }
                break;
              case 5:
                val[2] = val[2] == 9 ? 0 : val[2] + 1;
                break;
              case 6:
                val[2] = val[2] == 0 ? 9 : val[2] - 1;
                break;
              case 7:
                if (stato == false) {
                  val[2] = 0;
                }
                break;
              case 8:
                val[7] = val[7] == 3 ? 0 : val[7] + 1;
                crono.detach();
                timer2p.detach();
                stato = false;
                stateP = 1;
                break;
              case 9:
                val[7] = val[7] == 0 ? 3 : val[7] - 1;
                crono.detach();
                timer2p.detach();
                stato = false;
                stateP = 1;
                break;
              case 10:
                val[8] = val[8] == 3 ? 0 : val[8] + 1;
                crono.detach();
                timer2p.detach();
                stato = false;
                stateP = 1;
                break;
              case 11:
                val[8] = val[8] == 0 ? 3 : val[8] - 1;
                crono.detach();
                timer2p.detach();
                stato = false;
                stateP = 1;
                break;
              case 12:
                if (stato == false) {
                  val[7] = 0;
                  val[8] = 0;
                }
                break;
              case 13://Orologio
                if (stato == false && mode != 1) {
                  mode = 1;
                  clearTab();
                  modeImpostata = true;
                  timer2p.attach(0.5, duePunti);
                }
                break;
              case 14://Tabellone
                mode = 0;
                modeImpostata = false;
                timer2p.detach();
                stateP = true;
                displayWrite();
                break;
              case 15:
                if (stato == false) {
                  for (byte i = 0; i < 9; i++) {
                    val[i] = 0;
                  }
                }
                break;
            }
          }
        } else if (mode == 1) { //Modalità orologio
          if (RTC) {
            DateTime now = Clock.now();
            minuti = now.minute();
            ore = now.hour();
          }
          Serial.print("OROLOGIO i: "); Serial.print(i);
          Serial.print("\tShift: "); Serial.println(comandi.state[16]);
          if (comandi.state[16] == 1) { //Shift premuto in mod Orologio
            Serial.print("SHIFT IN OROLOGIO e i: "); Serial.println(i);
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
                mode = 0;
                Serial.println("STOP + SHIFT IN OROLOGIO");
                stateP = true;
                //                timer2p.detach();
                //                displayWrite();
                displayPrintOnSerial();
                break;
            }
          }
        }
      } else {
        if (millis() - time_p > 2000 ) {  // PASSATI 2 SECONDI DALLA PRESSIONE SI SALE DI 5 ALLA VOLTA
          if (mode == 0) {                //Modalità tabellone
            if (comandi.state[16] == 0) {         //Shift non premuto in mod tabellone
              switch (i) {
                case 0:
                  val[0] = (val[0] + 5) >= 199 ? 0 : val[0] + 5;
                  break;
                case 1:
                  val[0] = (val[0] - 5) <= 0 ? 199 : val[0] - 5;
                  break;
                case 2:
                  val[1] = (val[1] + 5) >= 199 ? 0 : val[1] + 5;
                  break;
                case 3:
                  val[1] = (val[1] - 5) <= 0 ? 199 : val[1] - 5;
                  break;
                case 8:
                  val[3] = (val[3] + 5) >= 99 ? 0 : val[3] + 5;
                  break;
                case 9:
                  val[3] = (val[3] - 5) <= 0 ? 99 : val[3] - 5;
                  break;
                case 10:
                  if (stato == false) {
                    if ((val[4] + 5) >= 59) {
                      val[3] = val[3] == 99 ? 0 : val[3] + 1;
                    }
                    val[4] = val[4] == 59 ? 0 : val[4] + 5;
                  }
                  break;
                case 11:
                  if (stato == false) {
                    if ((val[4] - 5) <= 0) {
                      val[3] = (val[3] + 5) >= 0 ? 99 : val[3] - 1;
                    }
                    val[4] = val[4] == 0 ? 59 : val[4] - 5;
                  }
                  break;
              }
            }
          } else if (mode == 1) { //Modalità orologio
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
                  minuti = minuti >= 59 ? 0 : minuti + 5;
                  impostaOra(minuti, ore);
                  break;
                case 11:
                  minuti = minuti <= 0 ? 59 : minuti - 5;
                  impostaOra(minuti, ore);
                  break;
                case 14://S
                  mode = 0;
                  Serial.println("STOP + SHIFT IN OROLOGIO");
                  //                  timer2p.detach();
                  //                  displayWrite();
                  displayPrintOnSerial();
                  break;
              }
            }
          }
          delay(500);
        }
      }
    }
  }
  for (byte i = 0; i < 17; i++) {
    comandi_p.state[i] = comandi.state[i];
  }
}

void automaticMode() {
  if (millis() - time_c > time_o && mode != 1) {
    mode = 1;
    timer2p.attach(0.5, duePunti);
    clearTab();
  }
}

void displayPrint() {
  if (pt1.read() != val[0]) {
    pt1.write(val[0]);
  }
  if (pt2.read() != val[1]) {
    pt2.write(val[1]);
  }
  if (periodo.read() != val[2]) {
    periodo.write(val[2]);
  }
  if (c_m.read() != val[3]) {
    c_m.write(val[3]);
  }
  if (c_s.read() != val[4]) {
    c_s.write(val[4]);
  }
  if (falli1.read() != val[5]) {
    falli1.write(val[5]);
  }
  if (falli2.read() != val[6]) {
    falli2.write(val[6]);
  }
}

void timeOutWrite() {
  switch (val[7]) {
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
  switch (val[8]) {
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
    toSendSerial += String(val[i]) + ".";
  }
  toSendSerial += String(val[8]);
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
    toSendSerial += String((int)(millis() * 2.7E-7)) + ".";
    toSendSerial += String((int)(millis() * 16.6E-6));
    for (int i = 0; i < 4; i++) {
      toSendSerial += ".-";
    }
    Serial.println(toSendSerial);
  }
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
  stato = false;
}
