/*
   ultima modifica fatta il 08/11/19

   da Alan Masutti

   Descrizione:
     programma che, caricato su una scheda ESP32 Wroom, si connetterà ad una rete WiFi creata da un'altra scheda
     uguale al fine di comunicare con essa i dati acquisiti dalla pusantiera fisica alla quale è collegata.
     Il formato dei dati è: punti1;punti2;tempoDiGiogo:minutiDiGioco;secondiDiGioco;stato;index\r\n
     Forma fisica del tabellone
  ____________________________________________________
  |      punti 1                             punti 2   |
  |        __    __                        __    __    |
  |      /__ / /__ /                     /__ / /__ /   |
  |     /__ / /__ /                     /__ / /__ /    |
  |                   tempo di gioco                   |
  |                        __                          |
  |                      /__ /                         |
  |                     /__ /                          |
  |                                                    |
  |               minuti         secondi               |
  |              __    __        __    __              |
  |            /__ / /__ /  .  /__ / /__ /             |
  |           /__ / /__ /  .  /__ / /__ /              |
  |____________________________________________________|


   Problemi riscontrati:


   Note:
    per il debug, ho creato una comunicazione seriale che grazie a componenti programmati in VBA si sostituiscono
    alla pulsantiera fisica

    l'array 'nomi' contiene tutte le possibilità.

*/

#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_MCP23017.h>

char ssid[] = "Tabellone";
char pass[] = "tabellone";

int myIndex = 0; //indice dei comandi: contiene il numero del comando che ha inviato al Server
int serverIndex = 0; //Indide dei comandi: contiene il numero del comando eseguito dal Server che viene ricevuto
String buff[50]; //buffer comandi

String splitString(String str, char sep, int index); //Funzione: splitta le stringhe

int pins[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}; //NON UTILIZZATO PER ORA:
//Sono i pin dei pulsanti fisici

int val[5] = { 0, 0, 0, 0, 0 };
String attivo = "s";

Adafruit_MCP23017 mcp;

//per la prova
String nomi[16] = { "pt1_p", "pt1_m", "pt2_p", "pt2_m", "pt_r", "t_p", "t_m", "t_r", "min_p", "min_m", "sec_p", "sec_m", "sec_r", "p", "s", "r" };
//\per la prova

IPAddress server(192, 168, 0, 80); //indirizzo del Server
IPAddress ip(192, 168, 0, 81);
IPAddress gateway(192, 168, 0, 80);
IPAddress subnet(255, 255, 255, 0);
WiFiClient client;

String dataToServer;
String data;

bool events = false;
bool connesso = false;

void setup() {
  //mcp.begin(0);
  Serial.begin(115200); // COM5
  Serial.println("");
  pinMode(2, OUTPUT);
  pinMode(1, INPUT_PULLUP);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, LOW);
    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, pass);
    for (int i = 0; i <= 10; i++) {
      Serial.print(".");
      delay(500);
    }
    Serial.print("\n");
  }
  digitalWrite(2, HIGH);

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  String data1;
  String data2;
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(2, HIGH);
    //Serial.print("WiFi.status(): "); Serial.println(WiFi.status());
  }
  else {
    Serial.println("Disconnect!");
    digitalWrite(2, LOW);
    WiFi.disconnect();
    while (WiFi.status() != WL_CONNECTED) {
      Serial.println(" Try to reconect");
      WiFi.begin(ssid, pass);
      for (int i = 0; i <= 10; i++) {
        Serial.print(".");
        delay(500);
      }
      Serial.println("");
    }
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  byte state[16];

  while (Serial.available() > 0) {
    data = Serial.readStringUntil('\n');
  }
  if (data == "Sei Arduino?") {
    connesso = true;
    Serial.print("Si sono Arduino!\n");
    delay(100);
    data = "";
  }

  if (data != "") {
    data1 = splitString(data, '.', 0);
    data2 = splitString(data, '.', 1);
    data = "";
  }
  for (byte i = 0; i <= 15; i++) state[i] = 0;
  /////// PER LA PROVA //////
  //il ciclo for va sostituito con : for(int i = 0; i <= 15; i++) state[i] = mcp.digitalRead(i);
  for (byte i = 0; i <= 15; i++) {
    if (data1 == nomi[i]) {
      state[i] = 1;
    }
  }
  ///////\PER LA PROVA //////

  while (client.connected() == 0) {
    if (WiFi.status() == WL_CONNECTED) {
      client.connect(server, 80);
    } else {
      return;
    }
  }

  //Serial.println("Conncetion sucessful");
  if (client) {
    if (client.connected()) {
      for (int i = 0; i <= 15; i++) {
        if (state[i] == 1) {
          switch (i) {
            case 0:
              val[0] ++;
              break;
            case 1:
              val[0] --;
              break;
            case 2:
              val[1] ++;
              break;
            case 3:
              val[1] --;
              break;
            case 4:
              if (attivo == "s") {
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
              if (attivo == "s") {
                val[2] = 0;
              }
              break;
            case 8:
              val[3] ++;
              break;
            case 9:
              val[3] --;
              break;
            case 10:
              val[4] ++;
              break;
            case 11:
              val[4] --;
              break;
            case 12:
              if (attivo == "s") {
                val[3] = 0;
                val[4] = 0;
              }
              break;
            case 13:
              if (attivo == "s") {
                attivo = "p";
              }
              break;
            case 14:
              if (attivo == "p") {
                attivo = "s";
              }
              break;
            case 15:
              if (attivo == "s") {
                for (byte i = 0; i <= 4; i++) {
                  val[i] = 0;
                }
              }
              break;
          }
        }
      }

      String toSend = String(val[0]) + "." + String(val[1]) + "." + String(val[2]) + "." + String(val[3])
                      + ":" + String(val[4]) + "."  + attivo + "." + String(myIndex) + "\r";
      buff[myIndex] = toSend;
      //      Serial.print("toSend: "); Serial.println(toSend);

      //      if (connesso) {
      //        if (buff[myIndex] != "") {
      //          Serial.print("myIndex_buff[" + String(myIndex) + "]: "); Serial.println(buff[myIndex]);
      //        }else{
      //          Serial.println("myIndex_buff[" + String(myIndex) + "]: VUOTO");
      //        }
      //        if (buff[serverIndex] != ""){
      //          Serial.print("serverIndex_buff[" + String(serverIndex) + "]: "); Serial.println(buff[serverIndex]);
      //        }else{
      //          Serial.println("serverIndex_buff[" + String(serverIndex) + "]: VUOTO");
      //        }
      //      }
      if (myIndex != serverIndex) {
        serverIndex = (serverIndex + 1) % 50;
      }
      if (buff[serverIndex] != "") {
        if (connesso) Serial.println(buff[serverIndex]);//Serial.print("sended: ");
        client.print(buff[serverIndex]);
        myIndex = (myIndex + 1) % 50;
      }
      dataToServer = client.readStringUntil('\r');
      if (dataToServer != "") {
        //        if (connesso) Serial.print("dataToServer: "); Serial.println(dataToServer);
        serverIndex = splitString(dataToServer, '.', 5).toInt();
        attivo = splitString(dataToServer, '.', 4);
        if(attivo == "p"){
          String timeStr = splitString(dataToServer, '.', 3);
          val[3] = splitString(timeStr, ':', 0).toInt();
          val[4] = splitString(timeStr, ':', 1).toInt();
        }
      }
    }
  }
  client.stop();
  client.flush();
  delay(2000);
}



String splitString(String str, char sep, int index) {
  /* str � la variabile di tipo String che contiene il valore da splittare
     sep � ia variabile di tipo char che contiene il separatore (bisoga usare l'apostrofo: splitString(xx, 'xxx', yy)
     index � la variabile di tipo int che contiene il campo splittato: str = "11111:22222:33333" se index= 0;
                                                                       la funzione restituir�: "11111"
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
