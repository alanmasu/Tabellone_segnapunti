/* Creato il 18/02/2020
 *  da Alan Masutti
 * 
 * Note
 *  - Comprende già le modifiche fatte: falli e time-out
 *  - Da controllare gli indirizzi I2C
 *  
 * Ultima modifica il:
 *  18/02/2020
 * 
 */

#include <Ticker.h>
#include <WiFi.h>
#include <SPI.h>
#include <setteSeg.h>

//Display
setteSeg pt1;
setteSeg pt2;
setteSeg c_m;
setteSeg c_s;

//Cifre
digit pt1_1(0);
digit pt1_2(8);
//digit pt1_3();
digit pt2_1(0);
digit pt2_2(8);
//digit pt2_3(9);
digit periodo(0);
digit min_1(0);
digit min_2(8);
digit sec_1(0);
digit sec_2(8);
digit falli1;
digit falli2;

//Pin per controllo falli
int f1_1;
int f1_2;
int f1_3;
int f2_1;
int f2_2;
int f2_3;

//Moduli I/O
Adafruit_MCP23017 mcp0;
Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;
Adafruit_MCP23017 mcp3;
Adafruit_MCP23017 mcp4;
Adafruit_MCP23017 mcp5;

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

//Funzioni
String splitString(String str, char sep, int index);


//Valori
int val[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool stato = false;
byte state[17]; //PT1+,PT1-,PT2+,PT2-,PTR,PER+,PER-,PERr,MIN+,MIN-,SEC+,SEC-,TR,P,S,R,SHIFT
byte state_p[17];

//Comunication
String dataFromClient = "";
String dataFromSerial = "";

void setup() {
  initSerial();
  initWiFi();
  initMCP();
  initDigits();
  initDisplays();
  reset();
}

void initSerial() {
  //Seriale
  Serial.begin(115200); // COM5
  Serial.println("");
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
  mcp0.begin(0);
  mcp1.begin(1);
  mcp2.begin(2);
  mcp3.begin(3);
  mcp4.begin(4);
  mcp5.begin(5);
}

void initDigits() {
  //da vedere
  pt1_1.begin('k', mcp0);
  pt1_2.begin('k', mcp0);
  pt2_1.begin('k', mcp1);
  pt2_2.begin('k', mcp1);
  min_1.begin('k', mcp2);
  min_2.begin('k', mcp2);
  sec_1.begin('k', mcp3);
  sec_2.begin('k', mcp3);
  falli1.begin('k', mcp4);
  falli2.begin('k', mcp4);
  periodo.begin('k', mcp5);
}
void initDisplays() {
  pt1 = setteSeg(pt1_1, pt1_2);
  pt2 = setteSeg(pt2_1, pt2_2);
  c_m = setteSeg(min_1, min_2);
  c_s = setteSeg(sec_1, sec_2);
  pt1.begin('1');
  pt2.begin('1');
  c_m.begin('1');
  c_s.begin('1');
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
  readSerial();
  if (checkConnection()) {
    dataFromClient = readClient();
    deComp(dataFromClient);
//    displayPrint();
    delay(50);
    displayPrintOnSerial();
    String toSend = formact();
    sendClient("prova");
    client.stop();
    client.flush();
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
    for (int i = 0; i < 17; i++) {
      state[i] = splitString(data, '.', i).toInt();
    }
    for (int i = 0; i < 16; i++) {
      if (state[i] == 1 && state[i] != state_p[i]) {
        if (state[16] == 0) {//Shift non premuto
          switch (i) {
            case 0:
              val[0] ++;
              Serial.println("val[0] ++");
              break;
            case 1:
              Serial.println("val[0] --");
              val[0] --;
              break;
            case 2:
              val[1] ++;
              break;
            case 3:
              val[1] --;
              break;
            case 4:
              if (stato == false) {
                val[0] = 0;
                val[1] = 0;
              }
              break;
            case 5:
              val[2] ++;
              break;
            case 6:
              val[2] --;
              break;
            case 7:
              if (stato == false) {
                val[2] = 0;
              }
              break;
            case 8:
              val[3] = (val[3] + 1) % 60;
              break;
            case 9:
              val[3] = (val[3] - 1) % 60;
              break;
            case 10:
              if (val[4] == 59) {
                val[3] = (val[3] + 1) % 60;
              }
              val[4] = (val[4] + 1) % 60;
              break;
            case 11:
              if (val[4] == 0) {
                val[3] = (val[3] - 1) % 60;
              }
              val[4] = (val[4] - 1) % 60;
              break;
            case 12:
              if (stato == false) {
                val[3] = 0;
                val[4] = 0;
              }
              break;
            case 13://P
              if(val[3] != 0 || val[4] != 0){
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
              val[5] = (val[5] + 1) % 6;
              break;
            case 1:
              val[5] = (val[5] - 1) % 6;
              break;
            case 2: //Falli 2
              val[6] = (val[6] + 1) % 6;
              break;
            case 3:
              val[6] = (val[6] - 1) % 6;
              break;
            case 4: //Falli reset
              if (stato == false) {
                val[5] = 0;
                val[6] = 0;
              }
              break;
            case 5:
              val[2] ++;
              break;
            case 6:
              val[2] --;
              break;
            case 7:
              if (stato == false) {
                val[2] = 0;
              }
              break;
            case 8:
              val[7] = (val[7] + 1) % 4;
              break;
            case 9:
              val[7] = (val[7] - 1) % 4;
              break;
            case 10:
              val[8] = (val[8] + 1) % 4;
              break;
            case 11:
              val[8] = (val[8] - 1) % 4;
              break;
            case 12:
              if (stato == false) {
                val[7] = 0;
                val[8] = 0;
              }
              break;
            case 13://P
              if(val[3] != 0 || val[4] != 0){
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
        }
      }
    }
    for (int i = 0; i < 17; i++) {
      state_p[i] = state[i];
    }
  }
}

void displayPrint() {
//  pt1.write(val[0]);
//  pt2.write(val[1]);
//  periodo.write(val[2]);
//  c_m.write(val[3]);
//  c_s.write(val[4]);
//  falli1.write(val[5]);
//  falli2.write(val[6]);
//  printTimeOut();
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
      mcp5.digitalWrite(f1_1, 0);
      mcp5.digitalWrite(f1_2, 0);
      mcp5.digitalWrite(f1_3, 0);
      break;
    case 1:
      mcp5.digitalWrite(f1_1, 1);
      mcp5.digitalWrite(f1_2, 0);
      mcp5.digitalWrite(f1_3, 0);
      break;
    case 2:
      mcp5.digitalWrite(f1_1, 1);
      mcp5.digitalWrite(f1_2, 1);
      mcp5.digitalWrite(f1_3, 0);
      break;
    case 3:
      mcp5.digitalWrite(f1_1, 1);
      mcp5.digitalWrite(f1_2, 1);
      mcp5.digitalWrite(f1_3, 1);
      break;
  }
  switch (val[8]) {
    case 0:
      mcp5.digitalWrite(f2_1, 0);
      mcp5.digitalWrite(f2_2, 0);
      mcp5.digitalWrite(f2_3, 0);
      break;
    case 1:
      mcp5.digitalWrite(f2_1, 1);
      mcp5.digitalWrite(f2_2, 0);
      mcp5.digitalWrite(f2_3, 0);
      break;
    case 2:
      mcp5.digitalWrite(f2_1, 1);
      mcp5.digitalWrite(f2_2, 1);
      mcp5.digitalWrite(f2_3, 0);
      break;
    case 3:
      mcp5.digitalWrite(f2_1, 1);
      mcp5.digitalWrite(f2_2, 1);
      mcp5.digitalWrite(f2_3, 1);
      break;
  }
}

String formact() { //PT1.PT2.TP.MM.SS.F1.F2.TO1.TO2.STATE
  String str = "";
  for (int i = 0; i < 9; i++) {
    str += String(val[i]) + ".";
  }
  str += String(stato) + "\r";
  return str;
}

String sendClient(String text) {
  client.println(text);
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
