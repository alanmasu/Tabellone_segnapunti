#pragma once
#include "digit.h"
#include "Adafruit_MCP23017.h"
#include <Arduino.h>
digit::digit(){}

digit::digit(int first_pin) {
  _a = first_pin;
  _b = first_pin + 1;
  _c = first_pin + 2;
  _d = first_pin + 3;
  _e = first_pin + 4;
  _f = first_pin + 5;
  _g = first_pin + 6;
  _dp = 129;
}

digit::digit(int b, int c) {
  _a = -1;
  _b = b;
  _c = c;
  _dp = 129;
}

digit::digit(int a, int b, int c, int d, int e, int f, int g) {
  _a = a;
  _b = b;
  _c = c;
  _d = d;
  _e = e;
  _f = f;
  _g = g;
  _dp = 129;
}

digit::digit(int a, int b, int c, int d, int e, int f, int g, int decimalp) {
  _a = a;
  _b = b;
  _c = c;
  _d = d;
  _e = e;
  _f = f;
  _g = g;
  _dp = decimalp;
}

void digit::begin(char mode, Adafruit_MCP23017 _mcp) {
  _mode = mode;
  mcp = _mcp;
  init();
}

void digit::clear() {
  int Off;
  if (_mode == 'k') {
    Off = 0;
  }
  else if (_mode == 'a') {
    Off = 1;
  }
  if ( _a >= 0) {
    mcp.digitalWrite(_a, Off);
    mcp.digitalWrite(_b, Off);
    mcp.digitalWrite(_c, Off);
    mcp.digitalWrite(_d, Off);
    mcp.digitalWrite(_e, Off);
    mcp.digitalWrite(_f, Off);
    mcp.digitalWrite(_g, Off);
  } else {
    mcp.digitalWrite(_b, Off);
    mcp.digitalWrite(_c, Off);
  }
  if (_dp != 129) {
    mcp.digitalWrite(_dp, Off);
  }
}
void digit::write(int n) {
  int On;
  if (_mode == 'k') On = 1;
  else if (_mode == 'a')On = 0;
  if (n < 0) {
    return;
  }
  if (n > 9) {
    return;
  }
  _number = n;
  if (_a >= 0) {
    switch (n) {
      case 0:
        mcp.digitalWrite(_a, On);
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        mcp.digitalWrite(_d, On);
        mcp.digitalWrite(_e, On);
        mcp.digitalWrite(_f, On);
        mcp.digitalWrite(_g, !On);
        break;
      case 1:
        mcp.digitalWrite(_a, !On);
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        mcp.digitalWrite(_d, !On);
        mcp.digitalWrite(_e, !On);
        mcp.digitalWrite(_f, !On);
        mcp.digitalWrite(_g, !On);
        break;
      case 2:
        mcp.digitalWrite(_a, On);
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, !On);
        mcp.digitalWrite(_d, On);
        mcp.digitalWrite(_e, On);
        mcp.digitalWrite(_f, !On);
        mcp.digitalWrite(_g, On);
        break;
      case 3:
        mcp.digitalWrite(_a, On);
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        mcp.digitalWrite(_d, On);
        mcp.digitalWrite(_e, !On);
        mcp.digitalWrite(_f, !On);
        mcp.digitalWrite(_g, On);
        break;
      case 4:
        mcp.digitalWrite(_a, !On);
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        mcp.digitalWrite(_d, !On);
        mcp.digitalWrite(_e, !On);
        mcp.digitalWrite(_f, On);
        mcp.digitalWrite(_g, On);
        break;
      case 5:
        mcp.digitalWrite(_a, On);
        mcp.digitalWrite(_b, !On);
        mcp.digitalWrite(_c, On);
        mcp.digitalWrite(_d, On);
        mcp.digitalWrite(_e, !On);
        mcp.digitalWrite(_f, On);
        mcp.digitalWrite(_g, On);
        break;
      case 6:
        mcp.digitalWrite(_a, On);
        mcp.digitalWrite(_b, !On);
        mcp.digitalWrite(_c, On);
        mcp.digitalWrite(_d, On);
        mcp.digitalWrite(_e, On);
        mcp.digitalWrite(_f, On);
        mcp.digitalWrite(_g, On);
        break;
      case 7:
        mcp.digitalWrite(_a, On);
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        mcp.digitalWrite(_d, !On);
        mcp.digitalWrite(_e, !On);
        mcp.digitalWrite(_f, !On);
        mcp.digitalWrite(_g, !On);
        break;
      case 8:
        test();
        break;
      case 9:
        mcp.digitalWrite(_a, On);
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        mcp.digitalWrite(_d, On);
        mcp.digitalWrite(_e, !On);
        mcp.digitalWrite(_f, On);
        mcp.digitalWrite(_g, On);
        break;
    }
  } else {
    switch (n) {
      case 0:
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
      case 1:
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        break;
      case 2:
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, !On);
      case 3:
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        break;
      case 4:
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        break;
      case 5:
        mcp.digitalWrite(_b, !On);
        mcp.digitalWrite(_c, On);
        break;
      case 6:
        mcp.digitalWrite(_b, !On);
        mcp.digitalWrite(_c, On);
        break;
      case 7:
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        break;
      case 8:
        test();
        break;
      case 9:
        mcp.digitalWrite(_b, On);
        mcp.digitalWrite(_c, On);
        break;
    }
  }
}
int digit::read() {
  return _number;
}


void digit::init() {
  if (_a >= 0) {
    mcp.pinMode(_a, OUTPUT);
    mcp.pinMode(_b, OUTPUT);
    mcp.pinMode(_c, OUTPUT);
    mcp.pinMode(_d, OUTPUT);
    mcp.pinMode(_e, OUTPUT);
    mcp.pinMode(_f, OUTPUT);
    mcp.pinMode(_g, OUTPUT);
  } else {
    mcp.pinMode(_b, OUTPUT);
    mcp.pinMode(_c, OUTPUT);

  }
  if (_dp != 129) {
    mcp.pinMode(_dp, OUTPUT);
  }
}

void digit::test() {
  int On;
  if (_mode == 'k') {
    On = 1;
  }
  else if (_mode == 'a') {
    On = 0;
  }
  if (_a >= 0) {
    mcp.digitalWrite(_a, On);
    mcp.digitalWrite(_b, On);
    mcp.digitalWrite(_c, On);
    mcp.digitalWrite(_d, On);
    mcp.digitalWrite(_e, On);
    mcp.digitalWrite(_f, On);
    mcp.digitalWrite(_g, On);
  }else{
    mcp.digitalWrite(_b, On);
    mcp.digitalWrite(_c, On);
  }
  if (_dp != 129) {
    mcp.digitalWrite(_dp, On);
  }
}
