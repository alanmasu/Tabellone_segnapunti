/*
   ultima modifica fatta il 12/02/2020 ore 07:26

   da: Alan Masutti

   Problemi riscontrati:


   Note:
    - Prova di inserimento della EEPROM, e powerFail da completare
    - Prova nuovo algoritmo

   Note EEPROM:
   Address Parameter
      0     pt1
      1     pt2
      2     t_g
      3     min
      4     sec
      5     state
      6     myIndex
      7     ClientIndex
*/

#include <WiFi.h>
#include <SPI.h>
#include <setteSeg.h>
#include <EEPROM.h>

//Display
setteSeg pt1;
setteSeg pt2;
setteSeg c_m;
setteSeg c_s;
//
digit pt1_1(0);
digit pt1_2(9);
//digit pt1_3();
digit pt2_1(0);
digit pt2_2(9);
//digit pt2_3(9);
digit tempo(0);
digit c_m1(0);
digit c_m2(9);
digit c_s1(0);
digit c_s2(9);

Adafruit_MCP23017 mcp0;
Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;
Adafruit_MCP23017 mcp3;
Adafruit_MCP23017 mcp4;



//Timer
bool state = false;

//WiFi
char ssid[] = "Tabellone";
char pass[] = "tabellone";
WiFiServer server(80);

IPAddress ip(192, 168, 0, 80);
IPAddress gateway(192, 168, 0, 80);
IPAddress subnet(255, 255, 255, 0);

//Controllo
int val[5] = {0, 0, 0, 0, 0};

//Gestione errori
int myIndex = 0;            //indice dei comandi: contiene il numero del comando che ha inviato al Server
int serverIndex = 0;        //Indice dei comandi: contiene il numero del comando eseguito dal Server che viene ricevuto
String buff[10];            //buffer comandi

//Seriale
bool debug = false;
bool connesso = false;
String toSendSerial = "";

//Routines
String splitString(String str, char sep, int index);
void pwISR();

void setup() {
  Serial.begin(115200); //COM5
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(ssid, pass);
  server.begin();
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println("HTTP server started");
  pinMode(15, OUTPUT);
  digitalWrite(15, 1);
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
  if (data == "Disconesso") {
    connesso = false;
  }
  if (data == "debug1.1") {
    debug = true;
    Serial.println("degug 1 attivo");
  }
  if (data == "debug1.0") {
    debug = false;
    Serial.println("degug 1 disattivo");
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

  if (client) {
    if (client.connected()) {
      data = client.readStringUntil('\r');
      if(debug == true){
        Serial.print("data: "); Serial.println(data);
        Serial.print("myIndex: "); Serial.println(myIndex);
      }
      if (data != "") {  //PROTOCOLLO: PT1.PT2.TP.MM.SS.ST.CI
        if (debug) {
          Serial.print("data: "); Serial.println(data);
        }

        val[0] = splitString(data, '.', 0).toInt();
        val[1] = splitString(data, '.', 1).toInt();
        val[2] = splitString(data, '.', 2).toInt();
        val[3] = splitString(data, '.', 3).toInt();
        val[4] = splitString(data, '.', 4).toInt();

        toSendSerial = String(val[0]);
        for (int i = 1; i < 5; i++) {
          toSendSerial = toSendSerial + "." + String(val[i]);
        }

//        pt1.write(val[0]);
//        pt2.write(val[1]);
//        c_m.write(val[3]);
//        c_s.write(val[4]);
//        tempo.write(val[2]);

        myIndex = (myIndex + 1) % 10;
        if (debug) {
          Serial.print("myIndex: "); Serial.println(myIndex);
        }
        String toSendClient;
        String toSendSerial;

        toSendClient = String(val[0]) + "." + String(val[1]) + "." + String(val[2]) + "."
                       + String(val[3]) + "." + String(val[4]) + "." + String(myIndex) + String('\r');
        toSendSerial = String(val[0]) + "." + String(val[1]) + "." + String(val[2]) + "."
                       + String(val[3]) + "." + String(val[4]) + String(".0.0.0.0\r");
        if (debug==true) {
          Serial.print("toSendClient: "); Serial.println(toSendClient);
        }
        if (connesso == true) {
          Serial.println(toSendSerial);
        }
        client.println(toSendClient);
      }
      client.stop();
    }
  }
  delay(150);
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
