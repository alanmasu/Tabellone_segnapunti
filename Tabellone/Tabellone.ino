/*
   ultima modifica fatta il 17/10/19

   da: Alan Masutti

   Problemi riscontrati:


   Note:


*/
#include <WiFi.h>
#include <Ticker.h>
#include <SPI.h>
#include <setteSeg.h>

//Display
setteSeg pt1;
setteSeg pt2;
setteSeg c_m;
setteSeg c_s;
//
digit pt1_1(1);
digit pt1_2(9);
//digit pt1_3();
digit pt2_1(1);
digit pt2_2(9);
//digit pt2_3(9);
digit tempo(1);
digit c_m1(1);
digit c_m2(9);
digit c_s1(1);
digit c_s2(9);

Adafruit_MCP23017 mcp0;
Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;
Adafruit_MCP23017 mcp3;
Adafruit_MCP23017 mcp4;



//Timer
Ticker crono; //Timer tiker per gestione del cronometro
Ticker microSec; //Timer tiker per gestione del cronometro

//WiFi
char ssid[] = "Tabellone";
char pass[] = "tabellone";
WiFiServer server(80);

IPAddress ip(192, 168, 0, 80);
IPAddress gateway(192, 168, 0, 80);
IPAddress subnet(255, 255, 255, 0);

//Controllo
int val[5] = {0, 0, 0, 0, 0};
int valPrec[5] = {0, 0, 0, 0, 0};
String state;

//CONTROLLO ERRORI
int myIndex = 0;
int clientIndex = 0;
String buff[5];

//Routines
void tik();
void finishTime();
String splitString(String str, char sep, int index);
bool timeFinished = false;


void setup() {
  Serial.begin(115200); //COM5
  //WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(ssid, pass);
  server.begin();
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println("HTTP server started");
  pinMode(2, OUTPUT);
  pinMode(23, OUTPUT);
  digitalWrite(2, 1);

  //Impoistazioni cifre
  //MCP23017
  mcp0.begin(0);
  mcp1.begin(1);
  mcp2.begin(2);
  mcp3.begin(3);
  mcp4.begin(4);
  //Digit
  //    pt1_1.begin('k', mcp0);
  //    pt1_2.begin('k', mcp0);
  //    pt2_1.begin('k', mcp1);
  //    pt2_2.begin('k', mcp1);
  //    c_m1.begin('k', mcp2);
  //    c_m2.begin('k', mcp2);
  //    c_s1.begin('k', mcp3);
  //    c_s2.begin('k', mcp3);

  tempo.begin('a', mcp4);
  pt1_1.begin('a', mcp0);
  pt1_2.begin('a', mcp0);
  pt2_1.begin('a', mcp1);
  pt2_2.begin('a', mcp1);
  c_m1.begin('a', mcp2);
  c_m2.begin('a', mcp2);
  c_s1.begin('a', mcp3);
  c_s2.begin('a', mcp3);
  tempo.begin('a', mcp4);
  //setteSeg
  pt1 = setteSeg(pt1_1, pt1_2);
  pt2 = setteSeg(pt2_1, pt2_2);
  c_m = setteSeg(c_m1, c_m2);
  c_s = setteSeg(c_s1, c_s2);

  pt1.begin('1');
  pt2.begin('1');
  c_m.begin('1');
  c_s.begin('1');

  //Start Signal

  pt1.test();
  pt2.test();
  c_m.test();
  c_s.test();
  tempo.test();
  delay(500);
  pt1.clear();
  pt2.clear();
  c_m.clear();
  c_s.clear();
  tempo.clear();
}


void loop() {
  String data;
  String data1;
  String data2;
  String data3;
  bool modificato = false;
  WiFiClient client = server.available();
  while (Serial.available() > 0) {
    data = Serial.readStringUntil('\n');
  }
  if (data == "Sei Arduino?") {
    Serial.print("Si sono Arduino!\n");
  }
  if (client) {
    if (client.connected()) {
      data = client.readStringUntil('\r');
      if (data != "") {  //PROTOCOLLO: PT1.PT2.TP.MM:SS.ST.CI
        //        Serial.print("data: "); Serial.println(data);
        val[0] = splitString(data, '.', 0).toInt();
        //        Serial.print("splitString: "); Serial.println(splitString(data, '.', 0).toInt());
        //        Serial.print("val[0]: "); Serial.println(val[0]);
        val[1] = splitString(data, '.', 1).toInt();
        val[2] = splitString(data, '.', 2).toInt();
        String timeStr = splitString(data, '.', 3);
        val[3] = splitString(timeStr, ':', 0).toInt();
        val[4] = splitString(timeStr, ':', 1).toInt();
        state = splitString(data, '.', 4);
        clientIndex = splitString(data, '.', 5).toInt();
        //CONTROLLO DEL PACCHETTO
        for (int i = 0; i < 5 ; i++) {
          //          Serial.print("val["+String(i)+"]: "); Serial.println(val[i]);
          //          Serial.print("valPrec["+String(i)+"]: "); Serial.println(valPrec[i]);
          if (val[i] != valPrec[i]) {
            //            Serial.print("if: ");
            String toSendSerial = String(val[0]);
            for (int c = 1; c < 5; c++) {
              toSendSerial = toSendSerial + "." + String(val[c]);
            }
            Serial.println(toSendSerial);


            myIndex = (myIndex + 1) % 5;
            pt1.write(val[0]);
            pt2.write(val[1]);
            c_m.write(val[3]);
            c_s.write(val[4]);
            tempo.write(val[2]);
          }
          String toSendClient = String(val[0]) + "." + String(val[1]) + "." + String(val[2]) + "." + String(val[3]) + ":" + String(val[4]) + "." + String(state) + "." + String(myIndex) + String('|');
          //Serial.print("toSendClient: ");
          //Serial.println(toSendClient);
          client.println(toSendClient);
        }
      }
      client.stop();
      client.flush();
    }
  }
}
String splitString(String str, char sep, int index) {
  /* str è la variabile di tipo String che contiene il valore da splittare
     sep è ia variabile di tipo char che contiene il separatore (bisoga usare l'apostrofo: splitString(xx, 'xxx', yy)
     index è la variabile di tipo int che contiene il campo splittato: str = "11111:22222:33333" se index= 0;
                                                                       la funzione restituirà: "11111"
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

void tik() {
  if (val[4] == 0) {
    val[4] = 59;
    val[3] --;
    Serial.println("crono." + String(val[3]) + "." + String(val[4]));
  } else {
    val[4]--;
    Serial.println("crono_sec.write." + String(val[4]));
  }
  if (val[3] == 0 && val[4] == 0) finishTime();
  c_m.write(val[4]);
  c_s.write(val[3]);
}
void finishTime() {
  //All'evento tempo finito esegui:
  crono.detach();
  state = "s";
}
