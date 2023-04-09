/*
   ultima modifica fatta il 18/11/19

   da: Alan Masutti

   Problemi riscontrati:


   Note: inserzione comunicazione stato e gestione del timer
   e powerFail
    PowerFail da completare

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
Ticker powerDetector; //Timer tiker per campionamento presenza tensione
String timeStrClient = "";
String timeStr = "STOP";
bool attivo = false;

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


//CONTROLLO ERRORI
int myIndex = 0;
int clientIndex = 0;
String buff[50];
//Seriale
bool debug = false;
bool connesso = false;
String toSendSerial = "";
String serialPrec = "";

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
  WiFiClient client = server.available();
  while (Serial.available() > 0) {
    data = Serial.readStringUntil('\n');
  }
  if (data == "Sei Arduino?") {
    Serial.print("Si sono Arduino!\n");
    connesso = true;
    delay(250);
  }
  if(data == "Disconesso"){
    connesso = false;
  }
  if (data == "debug.1") {
    debug = true;
  }
  if (data == "debug.0") {
    debug = false;
  }
  if (data == "?ip") {
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  }
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
  //  if (connesso == true && serialPrec != toSendSerial) {
  //    Serial.println(toSendSerial);
  //  }
  if (client) {
    if (client.connected()) {
      data = client.readStringUntil('\r');
      if (data != "") {  //PROTOCOLLO: PT1.PT2.TP.MM:SS.ST.CI
        if(debug){
          Serial.print("data: ");Serial.println(data);
        }
        //myIndex = (myIndex + 1) % 10;
        val[0] = splitString(data, '.', 0).toInt();
        val[1] = splitString(data, '.', 1).toInt();
        val[2] = splitString(data, '.', 2).toInt();
        String timeStrClient = splitString(data, '.', 3);
        if (debug) {
          Serial.println("----------");
          //Serial.print("data: "); Serial.println(data);
          Serial.print("timeStrClient: "); Serial.println(timeStrClient);
          //delay(5000);
        }
        if (timeStrClient == "STOPPED") {
          if(debug) Serial.println("crono detached");
          crono.detach();
          timeStr = "STOP";
          attivo = false;
        } else if (timeStrClient == "PLAYED") {
          if (attivo == false) {
            if(debug) Serial.println("crono attached");
            crono.attach(1, tik);
            timeStr = "PLAY";
            attivo = true;
          }
        } else if (timeStrClient != "PLAY"){
          val[3] = splitString(timeStrClient, ':', 0).toInt();
          val[4] = splitString(timeStrClient, ':', 1).toInt();
          //          if(connesso){
          //            Serial.print("val[3]: "); Serial.println(val[3]);
          //            Serial.print("val[4]: "); Serial.println(val[4]);
          //          }
        }
        //          val[3] = splitString(timeStr, ':', 0).toInt();
        //          val[4] = splitString(timeStr, ':', 1).toInt();
        //          state = splitString(data, '.', 4);
        //        }else if( timeStr == "MOD"){
        //          state = splitString(data, '.', 4);
        //        }else{
        //          state = "p";
        //        }
        clientIndex = splitString(data, '.', 4).toInt();
        //CONTROLLO DEL PACCHETTO
        //        if (state == "p") {
        //          crono.attach(1, tik);
        //        } else if (state == "s") {
        //          crono.detach();
        //        }
        toSendSerial = String(val[0]);
        for (int i = 1; i < 5; i++) {
          toSendSerial = toSendSerial + "." + String(val[i]);
        }
        pt1.write(val[0]);
        pt2.write(val[1]);
        c_m.write(val[3]);
        c_s.write(val[4]);
        tempo.write(val[2]);

        String toSendClient;
        if (debug) {
          Serial.print("timeStr: "); Serial.println(timeStr);
          //delay(5000);
        }
        if (attivo == false) {
          if (timeStr == "STOP") {
            toSendClient = String(val[0]) + "." + String(val[1]) + "." + String(val[2]) + "."
                           + String(val[3]) + ":" + String(val[4]) + "." + String(myIndex) + String('\r');
            if(debug) Serial.println("timeStr = STOP");
          } else if (timeStr == "STOPPED") {
            if(timeStrClient == "PLAY"){
              toSendClient = String(val[0]) + "." + String(val[1]) + "." + String(val[2])
                           + ".STOPPED." + String(myIndex) + String('\r');
              if(debug) Serial.println("timeStr = STOPPED");
            }else{
               toSendClient = String(val[0]) + "." + String(val[1]) + "." + String(val[2]) + "."
                           + String(val[3]) + ":" + String(val[4]) + "." + String(myIndex) + String('\r');
            }
            timeStr = "STOP";
          }
        } else {
          toSendClient = String(val[0]) + "." + String(val[1]) + "." + String(val[2]) + "."
                         + String(val[3]) + ":" + String(val[4]) + "." + String(myIndex) + String('\r');
          if(debug) Serial.println("timeStr = ELSE");
        }
        if (debug){
          Serial.print("toSendClient: "); Serial.println(toSendClient);
          Serial.print("attivo: "); Serial.println(attivo);
        }
        client.println(toSendClient);
        myIndex = clientIndex + 1;
      }
      client.stop();
      //client.flush();
    }
  }
    Serial.println(toSendSerial);
    delay(100);


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
  } else {
    val[4]--;
  }
  if (val[3] == 0 && val[4] == 0) {
    if(debug) Serial.println("stop");
    timeStr = "STOPPED";
    
    attivo = false;
    
    //    finishTime();
    //    c_m.write(val[4]);
    //    c_s.write(val[3]);
    crono.detach();
    
    
  }
  //  byte c = 0;
  //  String SendSerial = String(val[0]);
  //  for (int c = 1; c < 5; c++) {
  //    SendSerial = SendSerial + "." + String(val[c]);
  //  }
  //  Serial.println(SendSerial);
}
void finishTime() {
  //All'evento tempo finito esegui:
  crono.detach();

}
