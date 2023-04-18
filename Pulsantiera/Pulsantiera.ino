/* Creato il 09/05/2020
    da Alan Masutti

   Note
    - Comprende già le modifiche fatte: falli e time-out

   Ultima modifica il:
    25/05/2020

*/


#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_MCP23017.h>

//Funzioni
String splitString(String str, char sep, int index); //Funzione: splitta le stringhe


//Costanti pin
byte pins[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

byte shiftPin = 36;
int shiftLed = 13;
int startLed = 12;
int stopLed = 14;
int resetLed = 27;



//Valori
int val[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
byte state[17]; //PT1+,PT1-,PT2+,PT2-,PTR,PER+,PER-,PERr,MIN+,MIN-,SEC+,SEC-,TR,P,S,R,SHIFT
bool stato = false;

//Modulo I/O
Adafruit_MCP23017 mcp;

//Per comando seriale
String nomi[16] = { "pt1_p", "pt1_m", "pt2_p", "pt2_m", "pt_r", "t_p", "t_m", "t_r", "min_p", "min_m", "sec_p", "sec_m", "sec_r", "p", "s", "r" };
bool serial = true;

//WiFi
IPAddress server(192, 168, 0, 80); //indirizzo del Server
IPAddress ip(192, 168, 0, 81);
IPAddress gateway(192, 168, 0, 80);
IPAddress subnet(255, 255, 255, 0);
WiFiClient client;
char ssid[] = "Tabellone";
char pass[] = "tabellone";

//Comuniction
String dataFromServer;
String dataFromSerial;

//Serial comunication
bool connesso = false;
bool debug1 = false;

void setup() {
  initMCPs();
  initPins();
  initSerial();
  initWiFi();
}
void initMCPs() {
  //inizializzo gli ingressi
  const int a = 0;
  mcp.begin(a);
  for (byte i = 0; i < 13; i++) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);
  }
  mcp.pinMode(13, INPUT);
  mcp.pinMode(14, INPUT);
  mcp.pinMode(14, INPUT);
}
void initPins() {
  pinMode(2, OUTPUT);
  //Shift pin
  pinMode(shiftPin, INPUT);
  pinMode(shiftLed, OUTPUT);
  pinMode(startLed, OUTPUT);
  pinMode(stopLed, OUTPUT);
  pinMode(resetLed, OUTPUT);

}
void initSerial() {
  //Seriale
  Serial.begin(115200); // COM5
  Serial.println("");
}
void initWiFi() {
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

long istant = 0;

void loop() {
  istant = millis();
  readSerial();
  if(serial){
    readVirtualButtons();
  }else{
    readButtons();
  }
  
  // Serial.println("Controllo connessione");
  if (checkConnection()) {
    //Serial.println("Connesso");
    String toSend = formact();
    Serial.println(toSend);
    bool result = sendClient(toSend);
    // Serial.println("Client Scritto: " + String(result));
    dataFromServer = readClient();
    // Serial.println("Client letto");
    deComp(dataFromServer);
    // Serial.println("Dati elaborati");
    client.stop();
    client.flush();
  } else {
    reconnect();
  }
  //Serial.println("Loop time: " + String(millis() - istant));
  delay(50);
}

bool checkConnection() {
  //Controlla di essere connesso al server
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(2, HIGH);
    return true;
  } else {
    return false;
  }
}

void readSerial() {
  String data1 = "", data2 = "", data3 = "";
  while (Serial.available() > 0) {
    dataFromSerial = Serial.readStringUntil('\n');
  }
  data1 = splitString(dataFromSerial, '.', 0);
  data2 = splitString(dataFromSerial, '.', 1);
  data3 = splitString(dataFromSerial, '.', 2);
  if (dataFromSerial == "Sei Arduino?") {
    connesso = true;
    Serial.print("Si sono Arduino!\n");
    delay(100);
    dataFromSerial = "";
  }else if (data1 == "debug1") {
    debug1 = data2.toInt();
    Serial.print("debug1: "); Serial.println(debug1);
    dataFromSerial = "";
  } 
}

void readVirtualButtons(){
  String data1 = "", data2 = "", data3 = "";
  //Azzero l'array degli stati
  if (serial == true) {
    for (byte i = 0; i < 16; i++) {
      state[i] = 0;
    }
    if (dataFromSerial != ""){
      //Legge in seriale i valori
      data1 = splitString(dataFromSerial, '.', 0);
      data2 = splitString(dataFromSerial, '.', 1);
      data3 = splitString(dataFromSerial, '.', 2);
      dataFromSerial = "";
      for (byte i = 0; i < 16; i++) {
        if (data1 == nomi[i]) {
          state[i] = 1;
        }
      }
      state[16] = data3.toInt();
    }
  }
}

void readButtons() {
  //Legge i pulsanti
  initMCPs();
  int i;
  for (i = 0; i < 13; i++) {
    state[i] = !mcp.digitalRead(i);
  }
  state[13] = mcp.digitalRead(13);
  state[14] = mcp.digitalRead(14);
  state[15] = mcp.digitalRead(15);
  state[16] = digitalRead(shiftPin);
  digitalWrite(shiftLed, state[16]);
}

String formact() {
  //Prendi i valori dal globale e trasformali in una stringa
  String text = "";
  int i;
  for (i = 0; i < 17; i++ ) {
    text += String(state[i]) + ".";
  }
  text += String(state[i + 1]) + "\r";
  return text;
}

bool deComp(String data) {
  //Decompone i valori letti dal server
  if (data != "") {
    if (debug1) {
      Serial.print("data: "); Serial.println(data);
      Serial.print("debug: ");
    }
    for (int i = 0; i < 10; i++) {
      val[i] = splitString(data, '.', i).toInt();
      if (debug1) {
        Serial.print("val[" + String(i) + "]: "); Serial.println(String(val[i]) + ".");
      }
    }
    if (debug1) {
      Serial.println();
    }
  } else {
    return false;
  }
  digitalWrite(startLed, !val[9]);
  digitalWrite(resetLed, !val[9]);
  digitalWrite(stopLed, val[9]);
  return true;
}


bool sendClient(String text) {
  //Invia la stringa formattata
  if (!client.connected()) {
    if (WiFi.status() == WL_CONNECTED) {
      client.connect(server, 80);
    } else {
      return false;
    }
  }
  if (client) {
    if (client.connected()) {
      client.print(text);
      return true;
      client.stop();
      client.flush();
    } else {
      client.stop();
      client.flush();
      return false;
    }
  } else {
    return false;
  }
}
String readClient() {
  String data;
  if (!client.connected()) {
    if (WiFi.status() == WL_CONNECTED) {
      client.connect(server, 80);
    } else {
      return "";
    }
  }
  if (client) {
    if (client.connected()) {
      data = client.readStringUntil('\r');
      client.stop();
      client.flush();
      return data;
    } else {
      client.stop();
      client.flush();
      return "";
    }
  } else {
    return "";
  }
}

void reconnect() {
  //Verifica della connessione WiFi
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(2, HIGH);
  } else {
    Serial.println("Disconnect!");
    digitalWrite(2, LOW);
    digitalWrite(startLed, LOW);
    digitalWrite(resetLed, LOW);
    digitalWrite(stopLed, LOW);
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
