#include <unity.h>
#include <Arduino.h>
#include <setteSeg.h>
#include <Adafruit_MCP23017.h>
#include <Wire.h>

void setUp(void) {
    // Procedure di setup
}

void tearDown(void) {
    // Procedure di pulizia
}

void test_MCP(void) {
  Wire.begin();
  byte error;
  char mess[20] = "";
  for(int i = 0; i < 6; ++i) {
    Wire.beginTransmission(0x20 | i);
    error = Wire.endTransmission();
    snprintf(mess, 20, "Errore MCP: i was %d", i);
    TEST_ASSERT_EQUAL_MESSAGE(0, error, mess);
    delay(500);
  }

}

void test_displays(void) {
  Adafruit_MCP23017 mcp;
  digit d1(0, 1, 2, 3, 5, 4, 6);
  digit d2(8, 9, 10, 11, 13, 12, 14);
  digit d3(7, 15);
  setteSeg s(d1, d2, d3);

  for(int i = 0; i < 6; ++i){
    mcp.begin(i);
    d1.begin('k', mcp);
    d2.begin('k', mcp);
    d3.begin('k', mcp); 
    for(int n = 0; n < 10; ++n){
      d1.write(n);
      d2.write(n);
      if(n == 1) {
        d3.write(1);
      } else {
        d3.clear();
      }
      TEST_ASSERT_EQUAL(n, d1.read());
      TEST_ASSERT_EQUAL(n, d2.read());
      delay(1000);
    }
  }
  
}


void setup() {
  delay(5000);
//   while(!Serial);
  UNITY_BEGIN();
  RUN_TEST(test_MCP);
  RUN_TEST(test_displays);
  UNITY_END();
}

void loop() {
    // Zona in loop, poco utilizzata in test su piattaforma Embedded
}