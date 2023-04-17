#pragma once
#include "setteSeg.h"
#include <Arduino.h>

setteSeg::setteSeg() {

}
setteSeg::setteSeg(digit digit2, digit digit1) {
  _digit1 = digit1;
  _digit2 = digit2;
  cifre = 2;
}
setteSeg::setteSeg(digit digit3, digit digit2, digit digit1) {
  _digit1 = digit1;
  _digit2 = digit2;
  _digit3 = digit3;
  cifre = 3;
}
void setteSeg::begin(char digitsNotUsed) {
  _digitsNotUsed = digitsNotUsed;
}

void setteSeg::write(int n) {
  if (n >= 0 && n < 10) {
    _digit1.write(n);
    if (_digitsNotUsed == '1') {
      if(cifre > 1){
        _digit2.write(0);
      }
      if(cifre == 3){
        _digit3.write(0);
      }
    } else if (_digitsNotUsed == '0') {
      if(cifre > 1){
        _digit2.clear();
      }
      if(cifre == 3){
        _digit3.clear();
      }
    }else if (_digitsNotUsed == '2') {
      if(cifre > 1){
        _digit2.write(0);
      }
      if(cifre == 3){
        _digit3.clear();
      }
    }
  } else if (n >= 10 && n < 100) {
    _digit1.write(n % 10);
    _digit2.write(int(n / 10));
    if (_digitsNotUsed == '1') {
      if(cifre == 3){
        _digit3.write(0);
      }
    } else if (_digitsNotUsed == '0') {
      if(cifre == 3){
        _digit3.clear();
      }
    }else if (_digitsNotUsed == '2') {
      if(cifre == 3){
        _digit3.clear();
      }
    }
    
  } else if (n >= 100 && n < 1000) {
    _digit3.write(n / 100);
    _digit2.write(int((n % 100) / 10));
    _digit1.write(int((n % 100) % 10));
  }
}
void setteSeg::clear() {
  _digit1.clear();
  delay(20);
  _digit2.clear();
  delay(20);
  _digit3.clear();
}
void setteSeg::test() {
  _digit1.test();
  delay(20);
  _digit2.test();
  delay(20);
  _digit3.test();
}
int setteSeg::read() {
  String uni = String(_digit1.read());
  String dec = String(_digit2.read());
  String cen = String(_digit3.read());
  String num = cen + dec + uni;
  return num.toInt();
}
