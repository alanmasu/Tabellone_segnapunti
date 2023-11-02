#include <Arduino.h>
#include <common.h>

//Implementazione di metodi di struct
//Valori
Valori::Valori() {
  for (byte i = 0; i < 9; i++) {
    val[i] = 0;
  }
  stato = stop;
  mode = tabellone;
  modeImpostata = false;
}

Valori& Valori::operator=(const Valori &val2){
  for (byte i = 0; i < 9; i++) {
    val[i] = val2.val[i];
  }
  stato = val2.stato;
  mode = val2.mode;
  modeImpostata = val2.modeImpostata;
  return *this;
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
      case OTA:
        str += "OTA\t";
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