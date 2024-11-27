#ifndef __COMMON_H__
#define __COMMON_H__

#include <Arduino.h>

//typedef e struct
typedef enum {tabellone, orologio, OTA} Mode;
typedef enum {stop, run} Stato;

typedef struct Valori {
  byte val[9];
  Stato stato;
  Mode mode;
  bool modeImpostata;
  Valori();
  Valori& operator=(const Valori &val2);
  void print(bool whitConf = false)const;
  void println(bool whitConf = false)const;
  bool operator==(const Valori &val2);
} Valori;

typedef struct Comandi{
  bool state[17];
  Comandi();
  void print()const;
  void println()const;
} Comandi;

#endif
