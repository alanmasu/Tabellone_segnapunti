/*
   ultima modifica fatta il 12/02/2020 ore 07:26

   da Alan Masutti


   Problemi riscontrati:
     riferimeti circolare e indici in caso di reset, ma funziona lo stesso (parecchio instabile)

   Note:
    - Prova di inserimento della EEPROM, e powerFail da completare
    - Prova nuovo algoritmo; MANCA IL TIMER

   Note EEPROM:
   Address Parameter
      0     pt1
      1     pt2
      2     t_g
      3     min
      4     sec
      5     state
      6     myIndex
      7     serverIndex

*/

#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_MCP23017.h>
#include <EEPROM.h>
#include <Ticker.h>

//Gestione errori
int myIndex = 0; //indice dei comandi: contiene il numero del comando che ha inviato al Server
int serverIndex = 0; //Indide dei comandi: contiene il numero del comando eseguito dal Server che viene ricevuto
String buff[10]; //buffer comandi

Ticker timer;

//Funzioni
String splitString(String str, char sep, int index); //Funzione: splitta le stringhe
//void pwISR();

//Costanti pin
int pins[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}; //NON UTILIZZATO PER ORA:
int shiftPin = 3;
//Sono i pin dei pulsanti fisici

//Valori
int val[5] = { 0, 0, 0, 0, 0 };
bool stato = false;

Adafruit_MCP23017 mcp;

//per la prova
String nomi[16] = { "pt1_p", "pt1_m", "pt2_p", "pt2_m", "pt_r", "t_p", "t_m", "t_r", "min_p", "min_m", "sec_p", "sec_m", "sec_r", "p", "s", "r" };
bool shift;
bool serial = false;
//\per la prova

//WiFi
IPAddress server(192, 168, 0, 80); //indirizzo del Server
IPAddress ip(192, 168, 0, 81);
IPAddress gateway(192, 168, 0, 80);
IPAddress subnet(255, 255, 255, 0);
WiFiClient client;
char ssid[] = "Tabellone";
char pass[] = "tabellone";

//Comuniction
String dataToServer;
String data;
String toSendClient = "";
String toSendClient_prec = "";

//Serial comunication
bool connesso = false;
bool debug1 = false;


void setup() {
  //Ingressi
  mcp.begin(0);
  for (byte i = 0; i < 16; i++) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);
  }
  //Seriale
  Serial.begin(115200); // COM5
  Serial.println("");

  //Pin remotaggio
  pinMode(2, OUTPUT);
  pinMode(1, INPUT_PULLUP);
  //Connessione WiFi
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

  //PowerFail
  //attachInterrupt(digitalPinToInterrupt(2), pwISR, FALLING);

  //EEPROM
  EEPROM.begin(9);

  //Ripristino dati dell'ultima sessione
  //  for (byte i = 0; i < 5; i++) {
  //    val[i] = EEPROM.read(i);
  //  }
  //  state = bool(EEPROM.readInt(5));
  //  timeStr = EEPROM.readString(6);
  //  myIndex = EEPROM.readInt(7);
  //  serverIndex = EEPROM.readInt(8);
  delay(7000);
}

void loop() {
  //Comandi
  String data1;
  String data2;
  String data3;

  //Stati
  byte state[16];

  //Verifica della connessione WiFi
  checkWiFi();

  //Lettura della seriale
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
    data3 = splitString(data, '.', 2);
    data = "";
  }
  if (data1 == "debug1") {
    debug1 = data2.toInt();
    Serial.print("debug1: "); Serial.println(debug1);
    data = "";
  }

  //Azzero l'array degli stati
  for (byte i = 0; i < 16; i++) {
    state[i] = 0;
  }
  if (serial == true) {
    for (byte i = 0; i < 16; i++) {
      if (data1 == nomi[i]) {
        state[i] = 1;
      }
    }
    shift = data3.toInt();
  } else {
    //Leggo gli stati dei pulsanti
    for (int i = 0; i <= 15; i++) {
      state[i] = !mcp.digitalRead(i);
    }
    shift = !mcp.digitalRead(shiftPin);
  }



  while (client.connected() == 0) {
    if (WiFi.status() == WL_CONNECTED) {
      client.connect(server, 80);
    } else {
      return;
    }
  }

  //Operazioni
  if (client) {
    if (client.connected()) {
      decomp();
      toSendClient = String(val[0]) + "." + String(val[1]) + "." + String(val[2]) + "."
                     + String(val[3]) + "." + String(val[4]);
      if (myIndex == serverIndex) {
        if (debug1 == true) {
          //          Serial.print("toSend: "); Serial.println(toSend);
          Serial.print("buff[my]: "); Serial.println(buff[myIndex]);
        }
        if (toSendClient_prec != toSendClient) {
          buff[myIndex] = toSendClient;
          myIndex = (myIndex + 1) % 10;
          client.print(buff[myIndex] + + "." + String(myIndex) + String('\r'));
          if (debug1 == true) {
            Serial.print("client.print1(): "); Serial.println(buff[myIndex] + "." + String(myIndex) + String('\r'));
            Serial.println("uguali");
            Serial.print("serverIndex: "); Serial.println(serverIndex);
            Serial.print("myIndex: "); Serial.println(myIndex);
          }
        }
      } else {
        if (debug1) {
          //          Serial.print("toSend: "); Serial.println(toSend);
          Serial.print("buff[Server]: "); Serial.println(buff[serverIndex] + "." + String(myIndex) + String('\r'));
          Serial.println("diversi");
          Serial.print("serverIndex: "); Serial.println(serverIndex);
          Serial.print("myIndex: "); Serial.println(myIndex);
        }
        client.print(buff[serverIndex] + "." + String(myIndex) + String('\r'));
        if (debug1 == true) {
          Serial.print("client.print2(): "); Serial.println(buff[serverIndex] + "." + String(myIndex) + String('\r'));
        }

      }
      toSendClient_prec = toSendClient;
      dataToServer = client.readStringUntil('\r');
      if (debug1) {
        Serial.print("dataToServer: "); Serial.println(dataToServer);
        Serial.print("myIndex: "); Serial.println(myIndex);
        //          Serial.print("buff[my]: "); Serial.println(buff[myIndex]);
        //          Serial.print("buff[my]: "); Serial.println(buff[myIndex]);
      }
      if (dataToServer != "") {
        serverIndex = splitString(dataToServer, '.', 5).toInt();
        if (debug1) {
          Serial.print("dataToServer: "); Serial.println(dataToServer);
          //          Serial.print("buff[my]: "); Serial.println(buff[myIndex]);
          //          Serial.print("buff[my]: "); Serial.println(buff[myIndex]);
          //          Serial.print("buff[my]: "); Serial.println(buff[myIndex]);
        }
      }
    }
  }
  client.stop();
  client.flush();
  delay(150);
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

void checkWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(2, HIGH);
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
}

void serialAndPuls() {

}

void decomp() {
  for (int i = 0; i <= 15; i++) {
    if (state[i] == 1) {
      if (shift == false) {
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
            stato = true;
            break;
          case 14://S
            stato = false;
            break;
          case 15:
            if (stato == false) {
              for (byte i = 0; i <= 4; i++) {
                val[i] = 0;
              }
            }
            break;
        }
      } else {

      }
    }
  }
}
