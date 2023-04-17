/*
 * Libreria di classi setteSeg
 * 
 * Scritta da Alan Masutti
 * 
 * ultima modifica il: 25/05/2020
 * 
 * Dettagli: libreria che funziona
 * 
 */

#pragma once
#include "digit.h"
class setteSeg{
	public:
    setteSeg();
		setteSeg(digit digit2, digit digit1);
		setteSeg(digit digit3, digit digit2, digit digit1);
		void begin(char digitsNotUsed);
		void write(int n);
		void clear();
		void test();
		int read();
		int _dec;
	private:
		int _number;
		char _digitsNotUsed;
    int cifre;
		digit _digit1;
		digit _digit2;
		digit _digit3;
};
