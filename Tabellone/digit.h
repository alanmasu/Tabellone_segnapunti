/*
 * Libreria di classi digit
 * 
 * Scritta da Alan Masutti
 * 
 * ultima modifica il: 25/05/2020
 * 
 * Dettagli: libreria che funziona 
 *           
 */
#pragma once
#include "Adafruit_MCP23017.h"
class digit{
public:
  digit();
	digit(int first_pin); //Costruttore per collegamento sequenziale
	digit(int b, int c);
	digit(int a, int b, int c, int d, int e, int f, int g);
	digit(int a, int b, int c, int d, int e, int f, int g, int decimalp);
	void begin(char mode, Adafruit_MCP23017 _mcp);
	void write(int n);
	void clear();
	void test();
	int read();
private:
	Adafruit_MCP23017 mcp;
	void init();
	char _mode;
	int _number;
	int _add;
	int _a;
	int _b;
	int _c;
	int _d;
	int _e;
	int _f;
	int _g;
	int _dp;

};
