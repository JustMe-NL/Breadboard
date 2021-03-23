#include <breadboard.h>

// void serialdebug()
// void dumpBuffer()

#ifdef debug
//------------------------------------------------------------------------------ Debug vals
void serialdebug(int debugStep) {
  Serial.print(debugStep);
  Serial.print(" UPS-PmRsRt");
  Serial.print(encoder.up);
  Serial.print(encoder.down);
  Serial.print(encoder.sw);
  Serial.print("-");
  Serial.print(pinmonOn);
  Serial.print(readSet);
  Serial.print(readTrigger);
  Serial.print(", bp:");
  Serial.print(bytePins);
  Serial.print(", O: ");
  Serial.print(options);
  Serial.print(", mD: ");
  Serial.print(maxDigits);
  Serial.print("mV: ");
  Serial.print(maxValue);
  Serial.print(", eV: ");
  Serial.print(editValue);
  Serial.print(", sys: ");
  Serial.print(sysState);
  Serial.print(", iT:");
  Serial.print(inputType);
  Serial.print(", mV: ");
  Serial.print(", sV: ");
  Serial.print(setValue);  
  Serial.println(myValue);
}

//------------------------------------------------------------------------------ Dump buffer 4 debug
void dumpBuffer() {
  char temp[3];
  Serial.println("Buffer: ");
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print("0x");
    Serial.print(i);
    Serial.print("0: ");
    for (uint8_t j = 0; j < 0x10; j++) {
      snprintf(temp, 10, "%04X", buffer[(i * 0x10) + j]);
      Serial.print(temp);
      if (j == 0x07) {
        Serial.print(" : ");
      } else { Serial.print(" "); }
    }
    Serial.println();
  }
}
#endif