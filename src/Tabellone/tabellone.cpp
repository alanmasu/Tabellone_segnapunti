#include <tabellone.h>
#include <git_revision.h>
Comandi comandi;

//Definizioni delle funzioni 
void initSerial() {
  Serial.begin(115200); // COM5
  Serial.printf("Git commit hash: %s\n", __GIT_COMMIT__);
}

bool initESP_NOW() {
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return false;
  }
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
  return true;
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&comandi, incomingData, sizeof(comandi));
  for (int i = 0; i < 16; i++) {
    Serial.print(comandi.state[i]); Serial.print(".");
  }
  Serial.println(comandi.state[16]);
}