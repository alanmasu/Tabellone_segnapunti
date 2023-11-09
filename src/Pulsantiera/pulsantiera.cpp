#include <Arduino.h>
#include <esp_now.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <pulsantiera.h>
#include <git_revision.h>
#include <Adafruit_MCP23017.h>
#include <common.h>
#include <hardware.h>

// REPLACE WITH THE MAC Address of your receiver 
// uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x3F, 0x54, 0x9C};
#ifndef TABELLONE_MAC_ADDRESS
  uint8_t broadcastAddress[] = {0x7C, 0x9E, 0xBD, 0xEE, 0x8B, 0x7C}; //7c:9e:bd:ee:8b:7c
#else
  uint8_t broadcastAddress[] = TABELLONE_MAC_ADDRESS;
#endif

//ESP-NOW
bool ESP_NOWState = false;
bool ESP_NOWConnection = false;
unsigned long time_c = 0;
static esp_now_peer_info_t peerInfo;

//Pin
const byte pins[16] = {1, 0, 3, 2, 4, 6, 5, 7, 9, 8, 11, 10, 12, 13, 14, 15};
const byte shiftPin = 36;
const byte shiftLed = 13;
const byte startLed = 12;
const byte stopLed = 14;
const byte resetLed = 27;

//Struct per comunicazione comandi
Comandi comandi;  //Invio dei comandi
Valori recv;      //Ricezione dei valori
Valori tabStatus; //Vecchi valori ricevuti

//Modulo I/O
Adafruit_MCP23017 mcp;

//Modalità seriale
bool serialModeEnable = true;

//WiFi
char ssid[] = "Tabellone";
char pass[] = "Tabellone";
bool wifiInitialized = false;
uint32_t wifiReconnectTimer = 0;
uint32_t wifiLastConnect = 0;
const uint32_t WIFI_CONNECTION_INTERVAL = 5000;       //5 secondi tra una connessione e l'altra
const uint32_t WIFI_CONNECTION_TIMEOUT = 40 * 1000UL; //40 secondi di timeout per la riconnessione

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

void initESPNOW() {
  WiFi.mode(WIFI_STA);
  pinMode(CONNECTION_LED_PIN, OUTPUT);
    // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    ESP_NOWState = false;
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_err_t peer = esp_now_add_peer(&peerInfo);

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
  esp_int_wdt_init();                           //Inizializzo l'interrupt WDT
  esp_task_wdt_init(ESP_T_WDT_TIMEOUT, true);   //Inizializzo il task WDT
}

static void handleWiFiEvent(arduino_event_id_t event){
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               Serial.println("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           Serial.println("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           Serial.println("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            Serial.println("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:       Serial.println("Connected to access point"); break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:    Serial.println("Disconnected from WiFi access point"); break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: Serial.println("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      break;
    default:
      break;
  }
}

//WiFi
void initWiFi() {
  esp_now_deinit();
  WiFi.disconnect(true);
  WiFi.enableSTA(true);
  WiFi.onEvent(handleWiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.config(IPAddress(192, 168, 4, 10), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0), IPAddress(192, 168, 4, 1));
  WiFi.begin(ssid, pass);
  for(int i = 0; i < 10; i++){
    if(WiFi.status() == WL_CONNECTED) {
      break;
    }
    delay(250);
    digitalWrite(CONNECTION_LED_PIN, HIGH);
    delay(250);
    digitalWrite(CONNECTION_LED_PIN, LOW);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connesso con IP: "); Serial.println(WiFi.localIP());
  wifiInitialized = true;
  initESPNOW();
}
 
bool getWifiInitialized() {
  return wifiInitialized;
}

bool checkWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED){
    wifiLastConnect = millis();
    return true;
  }
  return false;
}

void reconnectWiFi() {
  if(millis() - wifiReconnectTimer > WIFI_CONNECTION_INTERVAL){
    wifiReconnectTimer = millis();
    Serial.println("Riconnessione WiFi...");
    WiFi.reconnect();
    if(WiFi.status() == WL_CONNECTED){  
      Serial.println();
      Serial.print("Connesso con IP: "); Serial.println(WiFi.localIP());
    }else if(millis() - wifiLastConnect > WIFI_CONNECTION_TIMEOUT){
      digitalWrite(CONNECTION_LED_PIN, LOW);
      Serial.println("ERRORE di CONNESSIONE.... REBOOT IN 5 SECONDI!!");
      delay(5000);
      ESP.restart();
    }
  }
}

static uint32_t blink_timer = 0;

void handleWiFiLed(){
  if(WiFi.status() != WL_CONNECTED){
    if(millis() - blink_timer > 250){
      blink_timer = millis();
      digitalWrite(CONNECTION_LED_PIN, !digitalRead(CONNECTION_LED_PIN));
    }
  }else{
    if(millis() - blink_timer > 750){
      blink_timer = millis();
      digitalWrite(CONNECTION_LED_PIN, !digitalRead(CONNECTION_LED_PIN));
    }
  }
}

//OTA
void initOTA() {
  ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("Pulsantiera");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
}

void OTALoop() {
  ArduinoOTA.handle();
}

void exitOtaMode(){
  ArduinoOTA.end();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  wifiInitialized = false;
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
    //    Serial.print("Data sent: ");
    //    comandi.println();
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
  //Serial.print("Dati ricevuti: "); recv.println(true);
}

void sendViaNow() {
  if (ESP_NOWState) {
    comandi.println();
    esp_now_send(broadcastAddress, (uint8_t *) &comandi, sizeof(comandi));
  }
}

bool checkNowConnection() {
  return ESP_NOWConnection;
}

//Core
// void resetWDT();
// void readSerial(String &str);
// void evaulateSerial(String &data);
// void readButtons();
// void evaluateData();              
// bool serialMode();
// void connectionErrorHandle();

void resetWDT(){
  esp_task_wdt_reset();
}

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
    } else if (data == "serialON") {
      serialModeEnable = true;
    } else if (data == "serialOFF") {
      serialModeEnable = false;
    } else {
      if (serialModeEnable) {
        for (byte i = 0; i < 17; i++) {
        comandi.state[i] = splitString(data, '.', i).toInt();
      }
      }
    }
  } else if(data == "" && serialModeEnable){
    for (int i = 0; i < 17; i++) {
      comandi.state[i] = 0;
    }
  }
}

bool serialMode(){
  return serialModeEnable;
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
      if (recv.val[3] != 0 || recv.val[4] != 0) {
        digitalWrite(startLed, !stato);
      } else {
        digitalWrite(startLed, LOW);
      }
      digitalWrite(resetLed, !stato);
      digitalWrite(stopLed, stato);
    } else if (recv.mode == tabellone) {
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
  if(recv.mode != OTA && recv.mode != tabStatus.mode){
    Serial.println("EXIT OTA MODE");
    delay(10000);
    exitOtaMode();
  }
  tabStatus = recv;
}

Mode getMode() {
  return recv.mode;
}

void connectionErrorHandle() {
  if (millis() - time_c > ESP_NOW_TIMEOUT) {
    Serial.println("ERRORE di CONNESSIONE.... REBOOT IN 5 SECONDI!!");
    delay(5000);
    ESP.restart();
  }
}

