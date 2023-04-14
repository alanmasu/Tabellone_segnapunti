/*
    Creato il 22/05/2020
     da Alan Masutti

    Note
     - Comprende già le modifiche fatte: falli e time-out
     - Da controllare gli indirizzi I2C
     - PowerFail detector e EEPROM
     - Aggiunte le sleep modes
     - Inserito RTC

    Ultima modifica il:
     22/05/2020

     SISTEMARE SOLO TERZE CIFRE E RIFIERMINETO CIRCOLARE

*/

#include <Ticker.h>
#include <WiFi.h>
#include <TimeLib.h>
#include <SPI.h>
#include <setteSeg.h>
#include <EEPROM.h>
#include <esp_bt_main.h>
#include <esp_bt.h>
#include <esp_wifi.h>
#include <esp_system.h>


//Display
setteSeg pt1;
setteSeg pt2;
setteSeg c_m;
setteSeg c_s;

//Cifre
digit pt1_1(0, 1, 2, 3, 5, 4, 6);
digit pt1_2(8, 9, 10, 11, 13, 12, 14);
//digit pt1_3();
digit pt2_1(0, 1, 2, 3, 5, 4, 6);
digit pt2_2(8, 9, 10, 11, 13, 12, 14);
//digit pt2_3();
digit periodo(8, 9, 10, 11, 13, 12, 14);
digit min_1(0, 1, 2, 3, 5, 4, 6);
digit min_2(0, 1, 2, 3, 5, 4, 6);
digit sec_1(8, 9, 10, 11, 13, 12, 14);
digit sec_2(0, 1, 2, 3, 5, 4, 6);
digit falli1(8, 9, 10, 11, 13, 12, 14);
digit falli2(0, 1, 2, 3, 5, 4, 6);

//Pin per controllo timeout
int f1_1 = 8;
int f1_2 = 9;
int f1_3 = 10;
int f2_1 = 11;
int f2_2 = 12;
int f2_3 = 13;
int mcpTimeout = 4;

//Moduli I/O
Adafruit_MCP23017 mcp[6];

//WiFi
char ssid[] = "Tabellone";
char pass[] = "tabellone";
WiFiServer server(80);
IPAddress ip(192, 168, 0, 80);
IPAddress gateway(192, 168, 0, 80);
IPAddress subnet(255, 255, 255, 0);
WiFiClient client;

//Timer
Ticker crono;
Ticker timer2p;

//Funzioni
String splitString(String str, char sep, int index);

//Power Fail
long time_s = 0;
bool powerFail_state = false;
bool powerFail_state0 = false;
volatile bool powerFail_event = false;
TaskHandle_t powerFail_t;

//Valori
volatile int val[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile bool stato = false;
byte state[18]; //PT1+,PT1-,PT2+,PT2-,PTR,PER+,PER-,PERr,MIN+,MIN-,SEC+,SEC-,TR,P,S,R,SHIFT
byte state_p[18];
byte mode = 0;  // Se 0 --> tabellone | Se 1 --> orologio
long time_c;

//Comunication
String dataFromClient = "";
String dataFromSerial = "";

//Per avanzamento veloce
long time_p;
byte tasto_p;

void setup() {
  initSerial();
  if (initEEPROM()) {
    rsBackup();
  }
  initMCP();
  initDigits();
  initDisplays();
  initFalli();
  initDuePunti();
  displayPrint();
  initWiFi();
  initPowerFail();
}

void initSerial() {
  //Seriale
  Serial.begin(115200); // COM5
}

void initWiFi() {
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(ssid, pass);
  server.begin();
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println("HTTP server started");
  pinMode(2, OUTPUT);
  digitalWrite(2, 1);
}

void initMCP() {
  for (int i = 0; i < 6; i++) {
    mcp[i].begin(i);
  }
}

void initDigits() {
  //da vedere
  pt1_1.begin('k', mcp[0]);
  pt1_2.begin('k', mcp[0]);
  pt2_1.begin('k', mcp[1]);
  pt2_2.begin('k', mcp[1]);
  min_1.begin('k', mcp[2]);
  min_2.begin('k', mcp[3]);
  sec_1.begin('k', mcp[2]);
  sec_2.begin('k', mcp[4]);
  falli1.begin('k', mcp[5]);
  falli2.begin('k', mcp[5]);
  periodo.begin('k', mcp[3]);
}

void initDisplays() {
  pt1 = setteSeg(pt1_2, pt1_1);
  pt2 = setteSeg(pt2_2, pt2_1);
  c_m = setteSeg(min_2, min_1);
  c_s = setteSeg(sec_2, sec_1);
  pt1.begin('1');
  pt2.begin('1');
  c_m.begin('1');
  c_s.begin('1');
}

void initFalli() {
  mcp[mcpTimeout].pinMode(f1_1, OUTPUT);
}

void initDuePunti() {
  mcp[2].pinMode(7, OUTPUT);
  mcp[2].pinMode(15, OUTPUT);
  //timer2p.attach(0.7, duePunti);
}

void duePunti() {
  byte stateP = !stateP;
  if (stateP) {
    mcp[2].digitalWrite(7, HIGH);
    mcp[2].digitalWrite(15, HIGH);
  } else {
    mcp[2].digitalWrite(7, LOW);
    mcp[2].digitalWrite(15, LOW);
  }
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

void initPowerFail() {
  pinMode(15, INPUT);
  xTaskCreatePinnedToCore(
    powerFailTaskRoutine,   /* Task function. */
    "POWERFAIL_T",          /* name of task. */
    10000,                  /* Stack size of task */
    NULL,                   /* parameter of the task */
    1,                      /* priority of the task */
    &powerFail_t,           /* Task handle to keep track of created task */
    0);                     /* pin task to core 0 */
}

void powerFailTaskRoutine(void * pvParameters) {
  Serial.println("POWERFAIL DETECTOR IS RUNNIG");
  while (1) {
    powerFail_state = digitalRead(15);
    if (!powerFail_state && powerFail_state0 != powerFail_state) {
      Serial.println("POWERFAIL DETECTOR WAS TRIGGERED");
      //vTaskSuspend(loopTaskHandle);
      powerFail_event = true;
      Serial.println("STOPPING CONNECTIONS");
      esp_wifi_stop();
      esp_bluedroid_disable();
      esp_bt_controller_disable();
      digitalWrite(2, 0);
      Serial.println("SAVING DATA");
      time_s = millis();
      if (EEPROMSave()) {
        Serial.println("SAVING SUCCESFUL");
        Serial.println("TIME FROM POWERFAIL TRIGGERING: " + String(millis() - time_s));
      }
    } else if (powerFail_state && powerFail_state0 != powerFail_state) {
      if ( powerFail_event) {
        //vTaskResume(loopTaskHandle);
        Serial.println("POWERFAIL RETURNED");
        initWiFi();
        powerFail_event = false;
      }
    }
    powerFail_state0 = powerFail_state;
    vTaskDelay(10);
  }
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

void powerFailReset() {
  for (int c = 0; c < 6; c++) {
    for (int i = 0; i < 16; i++) {
      mcp[c].digitalWrite(i, 0);
    }
  }
}

void reset() {
  pt1.write(0);
  pt2.write(0);
  periodo.write(0);
  c_m.write(0);
  c_s.write(0);
  falli1.write(0);
  falli2.write(0);
}

void loop() {
  if (!powerFail_event) {
    readSerial();
    if (checkConnection()) {
      dataFromClient = readClient();
      deComp(dataFromClient);
      String toSend = formact();
      sendClient(toSend);
    } else {
      if (millis() - time_c > 30000) {
        Serial.print("Mod orologio auto; state: ");
        Serial.println(String(stato));
        if (stato == false) {
          mode = 1;
        }
      } else {
        if (mode == 0 && millis() - time_c > 1000) {
          Serial.println("Tempo prima della mod orologio: " + String(30000 - (millis() - time_c)));
        }
      }
    }
    if (mode == 0) {
      displayPrint();
      delay(50);
      displayPrintOnSerial();
    } else {
      oraPrint();
      delay(50);
      oraPrintSerial();
    }
  } else {
    delay(100);
  }
}


bool checkConnection() {
  if (WiFi.softAPIP() != ip) {
    Serial.println("WiFi ERROR");
    WiFi.softAPConfig(ip, gateway, subnet);
    WiFi.softAP(ssid, pass);
    server.begin();
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    Serial.println("HTTP server started");
    delay(5000);
  }
  client = server.available();
  if (client) {
    if (client.connected()) {
      time_c = millis();
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }

}
void readSerial() {
  while (Serial.available() > 0) {
    dataFromSerial = Serial.readStringUntil('\n');
  }
}
String readClient() {
  String data = client.readStringUntil('\r');
  return data;
}

void deComp(String data) {
  if (data != "") {
    for (int i = 0; i < 18; i++) {
      state[i] = splitString(data, '.', i).toInt();
    }
    String timeStr = splitString(data, '.', 17);
    if ( timeStr != "" && timeStatus() == timeNotSet) {
      impostaOra(timeStr);
    }

    for (int i = 0; i < 16; i++) {
      if (state[i] == 1) {
        if (state[i] != state_p[i]) {
          time_p = millis();
          tasto_p = i;
          if (state[16] == 0) {//Shift non premuto
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
                if (val[4] == 59) {
                  val[3] = val[3] == 99 ? 0 : val[3] + 1;
                }
                val[4] = val[4] == 59 ? 0 : val[4] + 1;
                break;
              case 11:
                if (val[4] == 0) {
                  val[3] = val[3] == 0 ? 99 : val[3] - 1;
                }
                val[4] = val[4] == 0 ? 59 : val[4] - 1;
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
                  stato = true;
                }
                break;
              case 14://S
                crono.detach();
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
          } else { //Shift premuto
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
                if (timeStr != "" && timeStr.length() >= 3) {
                  impostaOra(timeStr);
                }
                oraPrint();
                oraPrintSerial();
                break;
              case 6:
                if (timeStr != "" && timeStr.length() >= 3) {
                  impostaOra(timeStr);
                }
                oraPrint();
                oraPrintSerial();
                break;
              case 7:
                if (stato == false) {
                  val[2] = 0;
                }
                break;
              case 8:
                val[7] = val[7] == 3 ? 0 : val[7] + 1;
                break;
              case 9:
                val[7] = val[7] == 0 ? 3 : val[7] - 1;
                break;
              case 10:
                val[8] = val[8] == 3 ? 0 : val[8] + 1;
                break;
              case 11:
                val[8] = val[8] == 0 ? 3 : val[8] - 1;
                break;
              case 12:
                if (stato == false) {
                  val[7] = 0;
                  val[8] = 0;
                }
                break;
              case 13:  //Orologio
                if (stato == false) {
                  mode = 1;
                }
                break;
              case 14:  //Tabellone
                mode = 0;
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
        } else {
          if (millis() - time_p > 3000  && i == tasto_p) { // PASSATI 5 SECONDI DALLA PRESSIONE SI SALE DE 5 ALLA VOLTA
            if (state[16] == 0) {//Shift non premuto
              switch (i) {
                case 0:
                  val[0] = val[0] == 199 ? 0 : val[0] + 5;
                  break;
                case 1:
                  val[0] = val[0] == 0 ? 199 : val[0] - 5;
                  break;
                case 2:
                  val[1] = val[1] == 199 ? 0 : val[1] + 5;
                  break;
                case 3:
                  val[1] = val[1] == 0 ? 199 : val[1] - 5;
                  break;
                case 5:
                  val[2] = val[2] == 9 ? 0 : val[2] + 5;
                  break;
                case 6:
                  val[2] = val[2] == 0 ? 9 : val[2] - 5;
                  break;
                case 8:
                  val[3] = val[3] == 99 ? 0 : val[3] + 5;
                  break;
                case 9:
                  val[3] = val[3] == 0 ? 99 : val[3] - 5;
                  break;
                case 10:
                  if (val[4] == 59) {
                    val[3] = val[3] == 99 ? 0 : val[3] + 5;
                  }
                  val[4] = val[4] == 59 ? 0 : val[4] + 5;
                  break;
                case 11:
                  if (val[4] == 0) {
                    val[3] = val[3] == 0 ? 99 : val[3] - 5;
                  }
                  val[4] = val[4] == 0 ? 59 : val[4] - 5;
                  break;
              }
            } else { //Shift premuto
              switch (i) {
                case 5:
                  if (timeStr != "" && timeStr.length() >= 3) {
                    impostaOra(timeStr);
                  }
                  break;
                case 6:
                  if (timeStr != "" && timeStr.length() >= 3) {
                    impostaOra(timeStr);
                  }
                  break;
              }
            }
            delay(500);
          }
        }
      }
    }
    for (int i = 0; i < 18; i++) {
      state_p[i] = state[i];
    }
  }
}

void displayPrint() {
  pt1.write(val[0]);
  pt2.write(val[1]);
  periodo.write(val[2]);
  c_m.write(val[3]);
  c_s.write(val[4]);
  falli1.write(val[5]);
  falli2.write(val[6]);
  printTimeOut();
}
void clearTab() {
  pt1.clear();
  pt2.clear();
  periodo.clear();
  falli1.clear();
  falli2.clear();
}

void oraPrint() {
  clearTab();
  int minInt = minute(now());
  int hourInt = hour(now());
  c_m.write(hourInt);
  c_s.write(minInt);
}

void oraPrintSerial() {
  String toSendSerial = "";
  int minInt = minute(now());
  int hourInt = hour(now());

  for (int i = 0; i < 3; i++) {
    toSendSerial += "-.";
  }
  toSendSerial += String(hourInt) + ".";
  toSendSerial += String(minInt);
  for (int i = 0; i < 4; i++) {
    toSendSerial += ".-";
  }
  Serial.println(toSendSerial);
}

void displayPrintOnSerial() {
  String toSendSerial = "";
  for (int i = 0; i < 8; i++) {
    toSendSerial += String(val[i]) + ".";
  }
  toSendSerial += String(val[8]);
  Serial.println(toSendSerial);
}

void printTimeOut() {
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

String formact() { //PT1.PT2.TP.MM.SS.F1.F2.TO1.TO2.STATE
  String str = "";
  for (int i = 0; i < 9; i++) {
    str += String(val[i]) + ".";
  }
  str += String(stato) + ".";
  str += String(mode) + "\r";
  return str;
}

String sendClient(String text) {
  if (client) {
    if (client.connected()) {
      client.println(text);
    }
  }
}

void impostaOra(String timeStr) {
  int Hours, Minutes;
  if (timeStr != "") {
    Hours = splitString(timeStr, ':', 0).toInt();
    Minutes = splitString(timeStr, ':', 1).toInt();
    setTime(Hours, Minutes, 1, 1, 1, 2020);
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

void finishTime() {
  //All'evento tempo finito esegui:
  crono.detach();
  stato = false;

}
String splitString(String str, char sep, int index) {
  /* str e' la variabile di tipo String che contiene il valore da splittare
     sep e' ia variabile di tipo char che contiene il separatore (bisoga usare l'apostrofo: splitString(xx, 'xxx', yy)
     index e' la variabile di tipo int che contiene il campo splittato: str = "11111:22222:33333" se index= 0;
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
