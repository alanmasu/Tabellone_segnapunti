/* Creato il 05/12/2021
    da Alan Masutti

   Note
    - Comprende già le modifiche fatte: falli e time-out

   PRIMA PROVA DI ESP-NOW
   TO DO LIST:
    - Cambio di protocollo [TO DO]
    - OTA                  [TO DO]

   Ultima modifica il:
    05/12/2021

*/

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_MCP23017.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <git_revision.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <hardware.h>

#define CONNECTION_LED 2
#define ESP_NOW_MAX_TIMEOUT 10 *1000UL

//Funzioni
String splitString(String str, char sep, int index); //Funzione: splitta le stringhe

//Costanti pin
byte pins[16] = {1, 0, 3, 2, 4, 6, 5, 7, 9, 8, 11, 10, 12, 13, 14, 15};

byte shiftPin = 36;
int shiftLed = 13;
int startLed = 12;
int stopLed = 14;
int resetLed = 27;

//Valori
int val[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte state[17]; //PT1+,PT1-,PT2+,PT2-,PTR,PER+,PER-,PERr,MIN+,MIN-,SEC+,SEC-,TR,P,S,R,SHIFT
byte state_p[17];
bool stato = false;

//Modulo I/O
Adafruit_MCP23017 mcp;

//Per comando seriale
bool serial = true;

//WiFi
IPAddress server(192, 168, 4, 1); //indirizzo del Server
IPAddress ip(192, 168, 4, 2);
IPAddress gateway(192, 168, 4, 1);
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

//Per avanzamento veloce
long time_p;
byte tasto_p;

//Time
String timeString;

BluetoothSerial BT;

//Struct e typedef
typedef struct Comandi {
  bool state[17];
} Stati;

Comandi comandi;
Comandi comandi_p;

//--------------------------ESP-NOW
#ifndef TABELLONE_MAC_ADDRESS
  uint8_t broadcastAddress[] = {0x7C, 0x9E, 0xBD, 0xEE, 0x8B, 0x7C}; //7c:9e:bd:ee:8b:7c
#else
  uint8_t broadcastAddress[] = TABELLONE_MAC_ADDRESS;
#endif

uint32_t lastMessageDelivery = 0;
bool ESP_NOWState = true;


void initMCPs() {
  //inizializzo gli ingressi
  const byte a = 0;
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
  pinMode(CONNECTION_LED, OUTPUT);
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
  Serial.printf("Git commit hash: %s", __GIT_COMMIT__);
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

//--------------------------ESP-NOW
int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  Serial.println("WiFi channel: " + String(WiFi.channel()));
  if (status == 0) {
    lastMessageDelivery = millis();
    digitalWrite(CONNECTION_LED, 1);
  }
  else {
    digitalWrite(CONNECTION_LED, 0);
  }
}

bool checkNOWConnection() {
  return millis() - lastMessageDelivery > ESP_NOW_MAX_TIMEOUT ? false : true;
}

bool initESP_NOW() {
  // Impostazione Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Configurazione canale WiFi
  int32_t channel = getWiFiChannel("Tabellone");
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore inizializazione ESP-NOW");
    return false;
  }

  // Settagio calback in scrittura
  esp_now_register_send_cb(OnDataSent);

  // Registrazione peer
  //  esp_now_peer_info_t peerInfo;
  //  peerInfo.channel = 0;
  //  peerInfo.encrypt = false;
  //
  //  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  //
  //  // Aggiunta peer
  //  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
  //    Serial.println("Aggiunta peer fallita. WiFi channel: " + String(WiFi.channel()));
  //    return false;
  //  }
  return true;
}

void sendMessageViaNOW() {
  // Send message via ESP-NOW
  if (ESP_NOWState) {
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &comandi, sizeof(Comandi));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
}

//--------------------------ESP-NOW

bool checkConnection() {
  //Controlla di essere connesso al server
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(2, HIGH);
    return true;
  } else {
    return false;
  }
}

void setup() {
  //  initMCPs();
  initPins();
  initSerial();
  //ESP_NOWState = initESP_NOW();//&& addPeerESP_NOW();
  
  // Impostazione Wi-Fi Station
  WiFi.mode(WIFI_STA);
  //Configurazione canale WiFi
  int32_t channel = getWiFiChannel("Tabellone");
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore inizializazione ESP-NOW");
    return;
  }

  // Settagio calback in scrittura
  esp_now_register_send_cb(OnDataSent);
  //  // Registrazione peer
  esp_now_peer_info_t peerInfo;
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);

  // Aggiunta peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Aggiunta peer fallita... Reboot in 5sec...");
    delay(5000);
    ESP.restart();
  }
  //  initWiFi();
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
  } else if (dataFromSerial != ""){
    BT.println(dataFromSerial);
  }
}

void readVirtualButtons(){
  if (serial == true) {
    if (dataFromSerial != ""){
      //Legge in seriale i valori
      
      for (byte i = 0; i < 17; i++) {
        comandi.state[i] = splitString(dataFromSerial, '.', i).toInt();
      }
      dataFromSerial = "";
    }
  }
}

// void evaulateSerial(String data) {
//   if (data == "Sei Arduino?") {
//     Serial.println("Si Sono Arduino!\n\r");
//   }
//   else {
//     String cmd1 = splitString(data, '.', 0);
//     int shift = splitString(data, '.', 2).toInt();
//     for (byte i = 0; i < 16; i++) {
//       if (nomi[i] == cmd1) {
//         comandi.state[i] = 1;
//       } else {
//         comandi.state[i] = 0;
//       }
//       if (shift == 1) {
//         comandi.state[16] = 1;
//       } else if (shift == 0) {
//         comandi.state[16] = 0;
//       }
//     }
//     //Serial.println(formact());
//   }

// }

void readButtons() {
  //Legge i pulsanti
  //initMCPs();
  int i;
  for (i = 0; i < 13; i++) {
    comandi.state[i] = !mcp.digitalRead(pins[i]);
  }
  comandi.state[13] = mcp.digitalRead(13);
  comandi.state[14] = mcp.digitalRead(14);
  comandi.state[15] = mcp.digitalRead(15);

  comandi.state[16] = digitalRead(shiftPin);
  digitalWrite(shiftLed, state[16]);

  for (int i = 0; i < 16; i++) {
    comandi_p.state[i] = comandi.state[i];
  }
}

String formact() {
  //Prendi i valori dal globale e trasformali in una stringa
  String text = "";
  int i;
  for (i = 0; i < 16; i++ ) {
    text += String(comandi.state[i]) + ".";
  }
  text += String(comandi.state[16]);
  text += "\r";
  return text;
}

bool deComp(String data) {
  //Decompone i valori letti dal server
  bool shift = digitalRead(shiftPin);
  if (data != "") {
    for (int i = 0; i < 11; i++) {
      val[i] = splitString(data, '.', i).toInt();
    }
    timeString = splitString(data, '.', 11);
  } else {
    return false;
  }
  if (!shift) {
    if (val[10] == 0) {
      digitalWrite(startLed, !val[9]);
      digitalWrite(resetLed, !val[9]);
      digitalWrite(stopLed, val[9]);
    } else {
      digitalWrite(startLed, 0);
      digitalWrite(resetLed, 0);
      digitalWrite(stopLed, 0);
    }
  } else {
    if (val[9] == 0) {
      digitalWrite(startLed, !val[10]);
      digitalWrite(resetLed, !val[10]);
      digitalWrite(stopLed, val[10]);
    } else {
      digitalWrite(startLed, 0);
      digitalWrite(resetLed, 0);
      digitalWrite(stopLed, 0);
    }
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

void loop() {
  readSerial();
  if(serial){
    readVirtualButtons();
  }else{
    readButtons();
  }
  if (checkNOWConnection() && ESP_NOWState) {
    String toSend = formact();
    Serial.println(toSend);
    sendMessageViaNOW();
    //    sendClient(toSend);
    //    dataFromServer = readClient();
    //    deComp(dataFromServer);
    //    client.stop();
    //    client.flush();
    delay(125);
  } else {
    //    //reconnect();
    Serial.println("Error to connect ESPNOW!!... rebooting...");
    delay(5000);
    ESP.restart();
  }
  //Serial.println("LoopTime: " + String(millis() - loopTime));
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
