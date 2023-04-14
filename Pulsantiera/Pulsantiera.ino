/* Creato il 22/05/2020
    da Alan Masutti

   Note
    - Comprende già le modifiche fatte: falli e time-out
    - Inserto RTC

   Ultima modifica il:
    25/05/2020

*/


#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_MCP23017.h>
#include <DS3231.h>

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
int val[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
byte state[17]; //PT1+,PT1-,PT2+,PT2-,PTR,PER+,PER-,PERr,MIN+,MIN-,SEC+,SEC-,TR,P,S,R,SHIFT
byte state_p[17];
bool stato = false;

//Modulo I/O
Adafruit_MCP23017 mcp;

//Per comando seriale
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

//Time
DS3231 Clock;
byte ore;
byte minuti;
bool h12;
bool PM;

//Per avanzamento veloce
long time_p;
byte tasto_p;

void setup() {
    initMCPs();
  initPins();
  initSerial();
  initWiFi();
initRTC();
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

void initRTC() {

  Wire.begin();
  Clock.setClockMode(false);
  minuti = Clock.getMinute();
  ore = Clock.getHour(h12, PM);
  Serial.println(getTime());
}

void loop() {
  readSerial();
  if(serial){
    readVirtualButtons();
  }else{
    readButtons();
  }
  if (checkConnection()) {
    String toSend = formact();
    Serial.println(toSend);
    sendClient(toSend);
    dataFromServer = readClient();
    deComp(dataFromServer);
    client.stop();
    client.flush();
      } else {
    reconnect();
  }
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
uint32_t t0 = 0;
void readVirtualButtons(){
  if (serial == true) {
    // for (byte i = 0; i < 16; i++) {
    //   state[i] = 0;
    // }
    // if(millis()-t0 > 1000){
    //   Serial.print("dataFromSerial: ");
    //   Serial.println(dataFromSerial);
    //   t0 = millis();
    // }
    if (dataFromSerial != ""){
      //Legge in seriale i valori
      
      for (byte i = 0; i < 17; i++) {
        state[i] = splitString(dataFromSerial, '.', i).toInt();
      }
      dataFromSerial = "";
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
if ( val[9] == 0 && val[10] == 1) {
    if (state[16] == 1) { //Shift
      if (state[5] == 1) {
        if ( state[5] != state_p[5]) { //+
          time_p = millis();
          tasto_p = 5;
          if (minuti == 59) {
            ore = ore == 24 ? 0 : ore + 1;
          }
          minuti = minuti == 59 ? 0 : minuti + 1;
          impostaOra(minuti, ore);
        } else {
          if (millis() - time_p > 3000 && tasto_p == 5 && millis() - time_p < 20000) {
            if ((minuti + 5) == 59 || (minuti + 5) > 59) {
              ore = ore == 24 || ore > 24 ? 0 : ore + 1;
            }
            minuti = (minuti + 5) == 59 || (minuti + 5) > 59 ? 0 : minuti + 5;
            impostaOra(minuti, ore);
            delay(800);
          } else if (millis() - time_p > 20000 && tasto_p == 5) {
            ore = ore == 24 || ore > 24 ? 0 : ore + 1;
            impostaOra(minuti, ore);
            delay(800);
          }
        }
      }
      if (state[6] == 1) {
        if (state[6] != state_p[6]) { //-
          time_p = millis();
          tasto_p = 6;
          if (minuti == 0) {
            ore = ore == 0 ? 24 : ore - 1;
          }
          minuti = minuti == 0 ? 59 : minuti - 1;
          Serial.println("min -; min: " + String(minuti) + " ore: " + String(ore));
          impostaOra(minuti, ore);
        } else {
          if (millis() - time_p > 3000 && tasto_p == 6 && millis() - time_p < 20000) {
            if ((minuti - 5) == 0 || (minuti - 5) < 0) {
              ore = ore == 0 || ore < 0 ? 24 : ore - 1;
            }
            minuti = (minuti - 5) == 0 || (minuti - 5) < 0 ? 59 : minuti - 5;
            impostaOra(minuti, ore);
            delay(800);
          } else if (millis() - time_p > 20000 && tasto_p == 6) {
            ore = ore == 24 || ore > 24 ? 0 : ore - 1;
            impostaOra(minuti, ore);
            delay(800);
          }
        }
      }
    }
  }
  for (int i = 0; i < 16; i++) {
    state_p[i] = state[i];
  }
}


void impostaOra(byte minuti, byte ore) {
  Clock.setSecond(0);//Set the second
  Clock.setMinute(minuti);//Set the minute
  Clock.setHour(ore); //Set the hour
}

String formact() {
  //Prendi i valori dal globale e trasformali in una stringa
  String text = "";
  int i;
  for (i = 0; i < 17; i++ ) {
    text += String(state[i]) + ".";
  }
  text += getTime() + "\r";
  return text;
}

String getTime() {
  return String(Clock.getHour(h12, PM)) + ":" + String(Clock.getMinute());
}

bool deComp(String data) {
  //Decompone i valori letti dal server
  bool shift = digitalRead(shiftPin);
  if (data != "") {
    for (int i = 0; i < 11; i++) {
      val[i] = splitString(data, '.', i).toInt();
    }
      } else {
    return false;
  }
  if (!shift) {
          digitalWrite(startLed, !val[9]);
      digitalWrite(resetLed, !val[9]);
      digitalWrite(stopLed, val[9]);
    } else {
            digitalWrite(startLed, !val[10]);
      digitalWrite(resetLed, !val[10]);
      digitalWrite(stopLed, val[10]);
      }
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
    } else {
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
      return data;
    } else {
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
