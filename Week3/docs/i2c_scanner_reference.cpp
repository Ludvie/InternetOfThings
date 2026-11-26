#include <Arduino.h>
#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin(21,22);
  Serial.println("I2C Scanner");
  int found = 0;
  for (uint8_t address=1; address<127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission()==0) {
      Serial.printf("I2C device found at 0x%02X\n", address);
      found++;
    }
  }
  Serial.printf("Scan complete. Found: %d\n", found);
}
void loop() {}
