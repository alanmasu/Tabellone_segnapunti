#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <pulsantiera.h>
#include <git_revision.h>
#include <Adafruit_MCP23017.h>
#include <hardware.h>

// REPLACE WITH THE MAC Address of your receiver 
// uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x3F, 0x54, 0x9C};
#ifndef TABELLONE_MAC_ADDRESS
  uint8_t broadcastAddress[] = {0x7C, 0x9E, 0xBD, 0xEE, 0x8B, 0x7C}; //7c:9e:bd:ee:8b:7c
#else
  uint8_t broadcastAddress[] = TABELLONE_MAC_ADDRESS;
#endif

bool ESP_NOWState = false;
bool ESP_NOWConnection = false;
unsigned long time_c = 0;

//Pin
const byte pins[16] = {1, 0, 3, 2, 4, 6, 5, 7, 9, 8, 11, 10, 12, 13, 14, 15};
const byte shiftPin = 36;
const byte shiftLed = 13;
const byte startLed = 12;
const byte stopLed = 14;
const byte resetLed = 27;

//Struct per comunicazione comandi
Comandi comandi;  //Invio dei comandi
Valori recv;     //DA SISTEMARE IL TIPO

//Modulo I/O
Adafruit_MCP23017 mcp;

//Implementazione di metodi di struct
//Comandi
Comandi::Comandi() {
  for (byte i = 0; i < 17; i++) {
    state[i] = 0;
  }
}
void Comandi::print()const {
  for (byte i = 0; i < 16; i++) {
    Serial.print(state[i]); Serial.print(".");
  }
  Serial.print(state[16]);
}
void Comandi::println()const {
  print();
  Serial.println();
}

//Valori
Valori::Valori() {
  for (byte i = 0; i < 9; i++) {
    val[i] = 0;
  }
  stato = stop;
  mode = tabellone;
  modeImpostata = false;
}

void Valori::print(bool whitConf)const {
  for (byte i = 0; i < 8; i++) {
    Serial.print(val[i]); Serial.print(".");
  }
  Serial.print(val[8]);
  if (whitConf) {
    String str = "\tStato:";
    switch (stato) {
      case stop:
        str += "stop\t";
        break;
      case run:
        str += "run\t";
        break;
    }
    str += "Modalita': ";
    switch (mode) {
      case tabellone:
        str += "tabellone\t";
        break;
      case orologio:
        str += "orologio\t";
        break;
    }
    str += "Mode Impostata: ";
    str += modeImpostata;
    Serial.print(str);
  }
}
void Valori::println(bool whitConf)const {
  print(whitConf);
  Serial.println();
}
bool Valori::operator==(const Valori &val2) {
  for (byte i = 0; i < 9; i++) {
    if (val[i] != val2.val[i]) {
      return false;
    }
  }
  if (stato != val2.stato) {
    return false;
  }
  if (mode != val2.mode) {
    return false;
  }
  if (modeImpostata != val2.modeImpostata) {
    return false;
  }
  return true;
}

//Dichiarazioni delle funizioni
//Inizializzazione

// void initSerial(String &title);
// void initMCPs();
// void initPins();
// void initESPNOW();
// void initWDT(); //DA IMPLEMENTARE DA ZERO

void initSerial(const String &title) {
  Serial.begin(115200); // COM5
  Serial.printf("Git commit hash: %s, File: %s\n", __GIT_COMMIT__, title.c_str());
}

void initMCPs() {
  //inizializzo gli ingressi
  mcp.begin((uint8_t)0);
  for (byte i = 0; i < 13; i++) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);
  }
  mcp.pinMode(13, INPUT);
  mcp.pinMode(14, INPUT);
  mcp.pinMode(14, INPUT);
}

void initPins() {
  //Shift pin
  pinMode(shiftPin, INPUT);
  pinMode(shiftLed, OUTPUT);
  pinMode(startLed, OUTPUT);
  pinMode(stopLed, OUTPUT);
  pinMode(resetLed, OUTPUT);
}

void initESPNOW(esp_now_peer_info_t* peerInfo) {
  WiFi.mode(WIFI_STA);
  pinMode(CONNECTION_LED_PIN, OUTPUT);
    // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    ESP_NOWState = false;
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo->peer_addr, broadcastAddress, 6);
  peerInfo->channel = 0;
  peerInfo->encrypt = false;
  esp_err_t peer = esp_now_add_peer(peerInfo);

  if (peer != ESP_OK) {
    Serial.print("Failed to add peer: ");
    switch (peer) {
      case ESP_ERR_ESPNOW_NOT_INIT:
        Serial.println("ESP_ERR_ESPNOW_NOT_INIT");
        break;
      case ESP_ERR_ESPNOW_ARG:
        Serial.println("ESP_ERR_ESPNOW_ARG");
        break;
      case ESP_ERR_ESPNOW_NOT_FOUND:
        Serial.println("ESP_ERR_ESPNOW_NOT_FOUND");
        break;
    }
    ESP_NOWState = false;
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  ESP_NOWState = true;
}


void initWDT() {

}

//Utility
// String splitString(String str, char sep, int index);

String splitString(String str, char sep, int index) {
  /* str a' la variabile di tipo String che contiene il valore da splittare
     sep a' ia variabile di tipo char che contiene il separatore (bisoga usare l'apostrofo: splitString(xx, 'xxx', yy)
     index a' la variabile di tipo int che contiene il campo splittato: str = "11111:22222:33333" se index= 0;
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


//ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    time_c = millis();
    Serial.print("Data sent: ");
    comandi.println();
    ESP_NOWConnection = true;
    digitalWrite(CONNECTION_LED_PIN, HIGH);
  }  else {
    //    Serial.print("Data NOT sent: ");
    //    comandi.println();
    ESP_NOWConnection = false;
    digitalWrite(CONNECTION_LED_PIN, LOW);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&recv, incomingData, sizeof(recv));
  Serial.print("Dati ricevuti: "); recv.println(true);
}

void sendViaNow() {
  if (ESP_NOWState) {
    //comandi.println();
    esp_now_send(broadcastAddress, (uint8_t *) &comandi, sizeof(comandi));
  }
}

bool checkNowConnection() {
  return ESP_NOWConnection;
}

//Core
// void readSerial(String &str);
// void evaulateSerial(String &data);
// void readButtons();
// evaluateData();              //DA IMPLEMENTARE DA ZERO
// void connectionErrorHandle();

void readSerial(String &str) {
  str = "";
  while (Serial.available() > 0) {
    str = Serial.readStringUntil('\n');
  }
}

void evaulateSerial(const String &data) {
  if (data != "") {
    if (data == "Sei Arduino?") {
      Serial.println("Si Sono Arduino!\n\r");
    } else if (data != "") {
      for (byte i = 0; i < 17; i++) {
        comandi.state[i] = splitString(data, '.', i).toInt();
      }
    }
  } else {
    for (int i = 0; i < 17; i++) {
      comandi.state[i] = 0;
    }
  }
}

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
  digitalWrite(shiftLed, comandi.state[16]);

}

void evaluateData() {
  bool shift = digitalRead(shiftPin);
  bool stato = recv.stato == run ? true : false;
  bool mode = recv.mode == tabellone ? true : false;
  if (!shift) {
    if (recv.mode == tabellone) {
      digitalWrite(startLed, !stato);
      digitalWrite(resetLed, !stato);
      digitalWrite(stopLed, stato);
    } else if (recv.mode == tabellone){
      digitalWrite(startLed, 0);
      digitalWrite(resetLed, 0);
      digitalWrite(stopLed, 0);
    }
  } else {
    if (stato == 0) {
      digitalWrite(startLed, !mode);
      digitalWrite(resetLed, !mode);
      digitalWrite(stopLed, mode);
    } else {
      digitalWrite(startLed, 0);
      digitalWrite(resetLed, 0);
      digitalWrite(stopLed, 0);
    }
  }
}

void connectionErrorHandle() {
  if (millis() - time_c > ESP_NOW_TIMEOUT) {
    Serial.println("ERRORE di CONNESSIONE.... REBOOT IN 5 SECONDI!!");
    delay(5000);
    ESP.restart();
  }
}
