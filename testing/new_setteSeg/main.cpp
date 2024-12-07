#include <Arduino.h>
#include <setteSeg.h> // ////////////////////////////////////////////// <<<<<--------------- MODIFICAREEEE

#warning "Compiling new_setteSeg.cpp"

//Display
setteSeg pt1;
setteSeg pt2;
setteSeg c_m;
setteSeg c_s;

//Cifre
digit pt1_1(0, 1, 2, 3, 5, 4, 6);
digit pt1_2(8, 9, 10, 11, 13, 12, 14);
digit pt1_3(7, 15);
digit pt2_1(0, 1, 2, 3, 5, 4, 6);
digit pt2_2(8, 9, 10, 11, 13, 12, 14);
digit pt2_3(7, 15);
digit periodo(8, 9, 10, 11, 13, 12, 14);
digit min_1(0, 1, 2, 3, 5, 4, 6);
digit min_2(0, 1, 2, 3, 5, 4, 6);
digit sec_1(8, 9, 10, 11, 13, 12, 14);
digit sec_2(0, 1, 2, 3, 5, 4, 6);
digit falli1(0, 1, 2, 3, 5, 4, 6);
digit falli2(8, 9, 10, 11, 13, 12, 14);

//Moduli I/O
Adafruit_MCP23017 mcp[6]; //Moduli MCP23017


void initMCP() {
  for (int i = 0; i < 6; i++) {
    mcp[i].begin(i);
  }
}

void initDigits() {
  pt1_1.begin('k', mcp[0]);
  pt1_2.begin('k', mcp[0]);
  pt1_3.begin('k', mcp[0]);
  pt2_1.begin('k', mcp[1]);
  pt2_2.begin('k', mcp[1]);
  pt2_3.begin('k', mcp[1]);
  min_1.begin('k', mcp[2]);
  min_2.begin('k', mcp[3]);
  sec_1.begin('k', mcp[2]);
  sec_2.begin('k', mcp[4]);
  falli1.begin('k', mcp[5]);
  falli2.begin('k', mcp[5]);
  periodo.begin('k', mcp[3]);
}

void initDisplays() {
  pt1 = setteSeg(pt1_3, pt1_2, pt1_1);
  pt2 = setteSeg(pt2_3, pt2_2, pt2_1);
  c_m = setteSeg(min_2, min_1);
  c_s = setteSeg(sec_2, sec_1);
  pt1.begin('2');
  pt2.begin('2');
  c_m.begin('1');
  c_s.begin('1');
}

void setup(){
    initMCP();
    initDigits();
    initDisplays();
}

void loop(){
    pt1.print("ti");
    pt2.print("Me");
    delay(5000);
    pt1.print("cr");
    pt2.print("No");
    delay(5000);
}
    