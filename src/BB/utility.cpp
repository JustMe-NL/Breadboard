#include <breadboard.h>

// void enableGPIO()
// void disableGPIO()
// void enableUSB2Serial()
// void disableSlaveModes()
// void writeToExpander()
// void readFromExpander()
// void digitalExRead()
// void digitalExWrite()
// void pulseStepper()
// void calculateVoltage()
// void changePin()
// void showlogicPins(uint8_t myx, uint8_t curvalue, bool showPin3)
// uint8_t calculateLogic(uint8_t myblocks, uint8_t myvalue, uint8_t myx)
// void processLogic()
// void oneWirePullUp(bool OnOff)
// void i2cPullUp(bool OnOff)

//------------------------------------------------------------------------------ Enable GPIO
void enableGPIO() {
#ifdef debug
  Serial.println("enable GPIO");
#endif
  if (!control.gpioRelay) {
    control.gpioRelay = true;
    writeToExpander(bytePins, true);
    delay(30);
  }
    pinMode(GPIO1, INPUT_PULLUP);
    pinMode(GPIO2, INPUT_PULLUP);
}

//------------------------------------------------------------------------------ Turn off GPIO
void disableGPIO() {
#ifdef debug
  Serial.println("disable GPIO");
#endif
  oneWirePullUp(false);
  i2cPullUp(false);
  if (control.gpioRelay) {
    control.gpioRelay = false;
    writeToExpander(bytePins, true);
    delay(30);
  }
    changePin(GPIO1, false);
    changePin(GPIO2, false);
}

//------------------------------------------------------------------------------ Change piping mode PD
void enableUSB2Serial(uint8_t mode) {
  SPI_Out.push(EXITCOMMAND);
  houseKeeping();
  control.serialRelay = 1;
  SPI_Out.push(SET_BAUDRATE);
  SPI_Out.push(baudRate);
  houseKeeping();
  writeToExpander(writeByte, true);
  SPI_Out.push(SERIALPIPING);
  SPI_Out.push(mode);
  houseKeeping();
}

//------------------------------------------------------------------------------ Disable all piping
void disableSlaveModes() {
  SPI_Out.push(EXITCOMMAND);
  houseKeeping();
  control.serialRelay = 0;
  writeToExpander(writeByte, true);
}

//------------------------------------------------------------------------------ write to IO expander
void writeToExpander(uint8_t IOValue, bool update_all) {
  Wire.beginTransmission(PCF_ADDRESS);
  Wire.write(byte(IOValue));
  Wire.endTransmission();

  if (update_all) {
    Wire.beginTransmission(PCF_ADDRESS2);
    Wire.write(byte(control.all));
    Wire.endTransmission();
  }
}

//------------------------------------------------------------------------------ write to IO expander
void readFromExpander() {
  Wire.requestFrom(PCF_ADDRESS, 1);
  while(Wire.available()) {
    bytePins = Wire.read();
  }
}

//------------------------------------------------------------------------------ read IO expander pin
uint8_t digitalExRead(const uint8_t pin)
{
  uint8_t readByte;
  if (pin > 7) { return 0; }
  readFromExpander();
  readByte = bytePins;
  return (readByte & (1 << pin)) > 0;
}

//------------------------------------------------------------------------------ write IO expander pin
void digitalExWrite(const uint8_t pin, const uint8_t value)
{
  if (pin > 7) { return; }
  if (value == LOW) { 
    writeByte &= ~(1 << pin); 
  } else {
    writeByte |= (1 << pin);
  }
  writeToExpander(writeByte);
}

//------------------------------------------------------------------------------ send pulses to A4988
void pulseStepper(bool CCW) {
  // pulse the stepper CW or CCW for valuestep times
  // max speed = 100 rpm @ 200 steps/rotation = 50us so delay(1) is more then enough
#ifdef debug
  Serial.print("Move ");
  Serial.print(valuestep);
  Serial.print("steps ");
  if (CCW) { Serial.print(" C"); }
  Serial.println("CW");
#endif
  if (CCW) {
    control.stepper_direction = HIGH;
  } else {
    control.stepper_direction = LOW;
  }
  writeToExpander(0x00, true);
  for (uint8_t i = 0; i < valuestep; i++) {
    control.stepper_step = HIGH;
    writeToExpander(0x00, true);
    control.stepper_step = LOW;
    writeToExpander(0x00, true);
    delay(1);                                       // limit to 1000 steps/sec (5rpm@200steps)
  }
}

//------------------------------------------------------------------------------ calculate measured voltage using correction table
float calculateVoltage(int measuredvoltage) {
  float calculatedvoltage;
  if (measuredvoltage == 0) {
    return voltageTable[0][1] / 10;
  }
  if (measuredvoltage ==1023) {
    return voltageTable[7][1] / 10;
  }
  for (uint8_t i = 0; i < 7; i++) {
    if (voltageTable[i][0] > measuredvoltage) {
      calculatedvoltage = (float) map(measuredvoltage, voltageTable[i - 1][0], voltageTable[i][0], voltageTable[i - 1][1], voltageTable[i][1]);
      return calculatedvoltage / 10;
      break;
    }
  }
  return 0;                                     // We should never come this far
}

//------------------------------------------------------------------------------ set Pins to High or High-Z
void changePin(uint8_t pin, bool state) {
  if (state) {
    digitalWrite(pin, HIGH);
  } else {
    switch(pin) {
      case 0:
        CORE_PIN0_CONFIG = 0;
        break;
      case 1:
        CORE_PIN1_CONFIG = 0;
        break;
      case 2:
        CORE_PIN2_CONFIG = 0;
        break;
      case 3:
        CORE_PIN3_CONFIG = 0;
        break;
      case 4:
        CORE_PIN4_CONFIG = 0;
        break;
      case 5:
        CORE_PIN5_CONFIG = 0;
        break;
      case 6:
        CORE_PIN6_CONFIG = 0;
        break;
      case 7:
        CORE_PIN7_CONFIG = 0;
        break;
      case 8:
        CORE_PIN8_CONFIG = 0;
        break;
      case 9:
        CORE_PIN9_CONFIG = 0;
        break;
      case 10:
        CORE_PIN10_CONFIG = 0;
        break;
      case 11:
        CORE_PIN11_CONFIG = 0;
        break;
      case 12:
        CORE_PIN12_CONFIG = 0;
        break;
      case 13:
        CORE_PIN13_CONFIG = 0;
        break;
      case 14:
        CORE_PIN14_CONFIG = 0;
        break;
      case 15:
        CORE_PIN15_CONFIG = 0;
        break;
      case 16:
        CORE_PIN16_CONFIG = 0;
        break;
      case 17:
        CORE_PIN17_CONFIG = 0;
        break;
      case 18:
        CORE_PIN18_CONFIG = 0;
        break;
      case 19:
        CORE_PIN19_CONFIG = 0;
        break;    
      case 20:
        CORE_PIN20_CONFIG = 0;
        break;
      case 21:
        CORE_PIN21_CONFIG = 0;
        break;
      case 22:
        CORE_PIN22_CONFIG = 0;
        break;
      case 23:
        CORE_PIN23_CONFIG = 0;
        break;       
    }
  }
}

void showlogicPins(uint8_t myx, uint8_t curvalue, bool showPin3) {
  ByteBits_t value;
  value.all = curvalue;
    if (value.bit0) { oled.fillRect(myx + 3, 18, 5, 5, SSD1306_WHITE); }
    if (value.bit1) { oled.fillRect(myx + 3, 37, 5, 5, SSD1306_WHITE); }
    if (value.bit2 && showPin3) { oled.fillRect(myx + 3, 28, 5, 5, SSD1306_WHITE); }
    if (value.bit3) { oled.fillRect(myx + 56, 27, 5, 5, SSD1306_WHITE); }
  if (myx == 0) {
    oled.drawBitmap(myx + 14, 22, bmap1, 3, 5, SSD1306_WHITE);
    oled.drawBitmap(myx + 14, 41, bmap2, 3, 5, SSD1306_WHITE);
    if (showPin3) { oled.drawBitmap(myx + 14, 32, bmap3, 3, 5, SSD1306_WHITE); }
    oled.drawBitmap(myx + 50, 34, bmap4, 3, 5, SSD1306_WHITE);
  } else {
    oled.drawBitmap(myx + 14, 22, bmap5, 3, 5, SSD1306_WHITE);
    oled.drawBitmap(myx + 14, 41, bmap6, 3, 5, SSD1306_WHITE);
    if (showPin3) { oled.drawBitmap(myx + 14, 32, bmap7, 3, 5, SSD1306_WHITE); }
    oled.drawBitmap(myx + 50, 34, bmap8, 3, 5, SSD1306_WHITE);
  }
}

//------------------------------------------------------------------------------ emulate logic block
uint8_t calculateLogic(uint8_t myblocks, uint8_t myvalue, uint8_t myx) {
  ByteBits_t logic;
  logic.all = myvalue;
  switch(myblocks) {
    case INVERT: // 0 & 2 =IN, 1 & 3 = OUT
      logic.bit1 = !logic.bit0;
      logic.bit3 = !logic.bit2;
      break;
    case AND2:   // 3 = OUT
      oled.drawBitmap(myx + 0, 16, bmapAnd, 64, 32, SSD1306_WHITE);       // Base AND
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);  
      logic.bit3 = (logic.bit0 & logic.bit1);
      showlogicPins(myx, logic.all, false);
      break;
    case AND3:
      oled.drawBitmap(myx + 0, 16, bmapAnd, 64, 32, SSD1306_WHITE);       // Base AND
      logic.bit3 = (logic.bit0 & logic.bit1 & logic.bit2);
      showlogicPins(myx, logic.all, true);
      break;
    case NAND2:
      oled.drawBitmap(myx + 0, 16, bmapAnd, 64, 32, SSD1306_WHITE);       // Base AND
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);                   // No 3rd pin AND
      oled.drawRect(myx + 46, 29, 5, 1, SSD1306_BLACK);                   // clear 4th pin
      oled.drawCircle(myx + 48, 29, 3, SSD1306_WHITE);                    // add invert
      logic.bit3 = ~(logic.bit0 & logic.bit1);
      showlogicPins(myx, logic.all, false);
      break;
    case NAND3:
      oled.drawBitmap(myx + 0, 16, bmapAnd, 64, 32, SSD1306_WHITE);       // Base AND
      oled.drawRect(myx + 46, 29, 5, 1, SSD1306_BLACK);                   // clear 4th pin
      oled.drawCircle(myx + 48, 29, 3, SSD1306_WHITE);                    // add invert
      logic.bit3 = ~(logic.bit0 & logic.bit1 & logic.bit2);
      showlogicPins(myx, logic.all, true);
      break;
    case OR2:
      oled.drawBitmap(myx + 0, 16, bmapOr, 64, 32, SSD1306_WHITE);        // Base OR
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);                   // No 3rd pin OR
      oled.fillRect(myx + 19, 30, 7, 1, SSD1306_BLACK); 
      logic.bit3 = (logic.bit0 | logic.bit1);
      showlogicPins(myx, logic.all, false);
      break;
    case OR3:
      oled.drawBitmap(myx + 0, 16, bmapOr, 64, 32, SSD1306_WHITE);   
      logic.bit3 = (logic.bit0 | logic.bit1 | logic.bit2);
      showlogicPins(myx, logic.all, true);
      break;
    case NOR2:
      oled.drawBitmap(myx + 0, 16, bmapOr, 64, 32, SSD1306_WHITE);        // Base OR
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);                   // No 3rd pin OR
      oled.fillRect(myx + 19, 30, 7, 1, SSD1306_BLACK);                   // No 3rd pin OR part 2
      oled.drawRect(myx + 46, 29, 5, 1, SSD1306_BLACK);                   // clear 4th pin
      oled.drawCircle(myx + 48, 29, 3, SSD1306_WHITE);                    // add invert
      logic.bit3 = ~(logic.bit0 | logic.bit1);
      showlogicPins(myx, logic.all, false);
      break;
    case NOR3:
      oled.drawBitmap(myx + 0, 16, bmapOr, 64, 32, SSD1306_WHITE);        // Base OR
      oled.drawRect(myx + 46, 29, 5, 1, SSD1306_BLACK);                   // clear 4th pin
      oled.drawCircle(myx + 48, 29, 3, SSD1306_WHITE);                    // add invert
      logic.bit3 = ~(logic.bit0 | logic.bit1 | logic.bit2);
      showlogicPins(myx, logic.all, true);
      break;
    case XOR2:
      oled.drawBitmap(myx + 0, 16, bmapXor, 64, 32, SSD1306_WHITE);       // Base OR
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);                   // No 3rd pin OR
      oled.fillRect(myx + 19, 30, 7, 1, SSD1306_BLACK);
      logic.bit3 = (logic.bit0 ^ logic.bit1);
      showlogicPins(myx, logic.all, false);
      break;
    case XOR3:
      oled.drawBitmap(myx + 0, 16, bmapXor, 64, 32, SSD1306_WHITE);       // Base OR
      logic.bit3 = (logic.bit0 ^ logic.bit1 ^ logic.bit2);
      showlogicPins(myx, logic.all, true);
      break;
    case SRFF:  // 0=S, 1=R, 2=Q, 3=^Q
      oled.drawBitmap(myx + 0, 16, bmapFF, 64, 32, SSD1306_WHITE);       // Base FF
      oled.drawBitmap(myx + 14, 22, bmapS, 3, 5, SSD1306_WHITE);         // Add S
      oled.drawBitmap(myx + 14, 41, bmapR, 3, 5, SSD1306_WHITE);         // Add R
      oled.fillRect(myx + 20, 26, 14, 10, SSD1306_BLACK);                // Remove C
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);                   // No 3rd pin FF
      if (logic.bit0 && !logic.bit1) { logic.bit2 = true;}
      if (!logic.bit0 && logic.bit1) { logic.bit2 = false;}
      logic.bit3 = !logic.bit2;
      if (logic.bit0) { oled.fillRect(myx + 3, 18, 5, 5, SSD1306_WHITE); }
      if (logic.bit1) { oled.fillRect(myx + 3, 37, 5, 5, SSD1306_WHITE); }
      if (logic.bit2) { oled.fillRect(myx + 56, 18, 5, 5, SSD1306_WHITE); }
      if (logic.bit3) { oled.fillRect(myx + 56, 37, 5, 5, SSD1306_WHITE); }
      break;
    case JKFF:  // 0=J, 1=K, 2=C, 3=Q, 4=^Q, 7=Old C
      oled.drawBitmap(myx + 0, 16, bmapFF, 64, 32, SSD1306_WHITE);       // Base FF
      oled.drawBitmap(myx + 14, 22, bmapJ, 3, 5, SSD1306_WHITE);         // Add S
      oled.drawBitmap(myx + 14, 41, bmapK, 3, 5, SSD1306_WHITE);         // Add R
      if (!lastclock && logic.bit2) {
        lastclock = logic.bit2;
        if (logic.bit0 && !logic.bit1) { logic.bit3 = true; }
        if (!logic.bit0 && logic.bit1) { logic.bit3 = false; }
        if (logic.bit0 && logic.bit1) { logic.bit3 = !logic.bit3; }
        logic.bit4 = !logic.bit3;
      } else {
        lastclock = logic.bit2;
      }
      if (logic.bit0) { oled.fillRect(myx + 3, 18, 5, 5, SSD1306_WHITE); }
      if (logic.bit1) { oled.fillRect(myx + 3, 37, 5, 5, SSD1306_WHITE); }
      if (logic.bit2) { oled.fillRect(myx + 3, 28, 5, 5, SSD1306_WHITE); }
      if (logic.bit3) { oled.fillRect(myx + 56, 18, 5, 5, SSD1306_WHITE); } 
      if (logic.bit4) { oled.fillRect(myx + 56, 37, 5, 5, SSD1306_WHITE); }
      break;
  }
  logic.all |= 0x07;      // Make sure the inputs stay inputs
  return logic.all;
}

//------------------------------------------------------------------------------ process logic blocks
void processLogic() {
  uint8_t templogic, result;
  oled.fillRect(0, 16, 128, 32, SSD1306_BLACK);
  templogic = logicSet & 0x0F;
  writeByte = 0;
  if (templogic > 0) { 
    result = calculateLogic(templogic, bytePins, 0); 
    writeByte = result;
  }
  if (templogic < JKFF) {
    templogic = (logicSet & 0xF0) >> 4;
    result = (bytePins & 0xF0) >> 4;
    if (templogic > 0) { 
      result = calculateLogic(templogic, result, 64); 
      writeByte |= (result << 4);
    }
  }
  writeToExpander(writeByte);
}

//------------------------------------------------------------------------------ OnwWire pullup
void oneWirePullUp(bool OnOff) {
  if (OnOff) {
    digitalWrite(PULLUPSDA, HIGH);
    owPullUpActive = true;
    i2cPullUp(false);
  } else {
    digitalWrite(PULLUPSDA, LOW);
    owPullUpActive = false;
  }
}

//------------------------------------------------------------------------------ I2C pullup
void i2cPullUp(bool OnOff) {
  if (OnOff) {
    digitalWrite(PULLUPSDA, HIGH);
    digitalWrite(PULLUPSCL, HIGH);
    i2cPullUpActive = true;
    oneWirePullUp(false);
  } else {
    digitalWrite(PULLUPSDA, LOW);
    digitalWrite(PULLUPSCL, LOW);
    i2cPullUpActive = false;
  }
}