#include <breadboard.h>

// void attachMainInterruots()
// void detachMainInterrupts()
// void enableGPIO()
// void disableGPIO()
// void enableUSB2Serial()
// void enableUSBKeyboard()
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
// void PullUp(bool OnOff)
// void checkScreen()
// void salaeProtocolExport()
// void I2CprotocolExport()
// void rawI2CExport()


//------------------------------------------------------------------------------ Enable interrupts
void attachMainInterrupts() {
  //bitSet(I2C0_C1, 6);                                           // I2C_C1_IICIE
  attachInterrupt(digitalPinToInterrupt(EXPINT), expanderInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(PDINT), pdInterrupt, CHANGE);
}

//------------------------------------------------------------------------------ Disable interrupts
void detachMainInterrupts() {
  bitClear(I2C0_C1, 6);                                           // I2C_C1_IICIE
  detachInterrupt(EXPINT);
  detachInterrupt(PDINT);
}

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
  PullUp(false);
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
  allSPIisData = false;
  SPI_Out.push(cmdESC_ExitCommand);
  houseKeeping();
  control.serialRelay = 1;
  writeToExpander(writeByte, true);
  if (mode == modeUSB_Serial || mode == modeSPI_Serial) {
    SPI_Out.push(cmdSetBaudRate);
    SPI_Out.push(baudRate);
    houseKeeping();
  }
  SPI_Out.push(cmdSerialPiping);
  SPI_Out.push(mode);
  houseKeeping();
}

//------------------------------------------------------------------------------ Change piping mode PD
void enableUSBKeyboard() {
  allSPIisData = false;
  SPI_Out.push(cmdESC_ExitCommand);
  houseKeeping();
  SPI_Out.push(cmdSerialPiping);
  SPI_Out.push(modeSPI_Keyboard);
  houseKeeping();
  delay(5000);
}

//------------------------------------------------------------------------------ Disable all piping
void disableSlaveModes() {
  allSPIisData = false;
  SPI_Out.push(cmdESC_ExitCommand);
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
  volatile uint32_t *config;
  if (state) {
    digitalWrite(pin, HIGH);
  } else {
    config = portConfigRegister(pin);
    *config = 0;
  }
}

//------------------------------------------------------------------------------ show logic pin statuses
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
    case INVERT: // 0->2 1->3
      oled.drawBitmap(myx + 0, 16, bmapNot, 64, 32, SSD1306_WHITE);       // Base Not
      oled.drawBitmap(myx + 14, 22, bmap1, 3, 5, SSD1306_WHITE);          // pin numbers
      oled.drawBitmap(myx + 14, 41, bmap2, 3, 5, SSD1306_WHITE);
      oled.drawBitmap(myx + 50, 22, bmap3, 3, 5, SSD1306_WHITE);
      oled.drawBitmap(myx + 50, 41, bmap4, 3, 5, SSD1306_WHITE);
      logic.bit2 = !logic.bit0;
      logic.bit3 = !logic.bit1;
      if (logic.bit0) { oled.fillRect(myx + 3, 18, 5, 5, SSD1306_WHITE); }  // pin statuses
      if (logic.bit1) { oled.fillRect(myx + 3, 37, 5, 5, SSD1306_WHITE); }
      if (logic.bit2) { oled.fillRect(myx + 56, 18, 5, 5, SSD1306_WHITE); }
      if (logic.bit3) { oled.fillRect(myx + 56, 37, 5, 5, SSD1306_WHITE); }
      logic.all |= 0b00000011;                                            // Make sure the inputs stay inputs
      break;
    case AND2:   // 3 = OUT
      oled.drawBitmap(myx + 0, 16, bmapAnd, 64, 32, SSD1306_WHITE);       // Base AND
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);  
      logic.bit3 = (logic.bit0 & logic.bit1);
      showlogicPins(myx, logic.all, false);
      logic.all |= 0b00000011;
      break;
    case AND3:
      oled.drawBitmap(myx + 0, 16, bmapAnd, 64, 32, SSD1306_WHITE);       // Base AND
      logic.bit3 = (logic.bit0 & logic.bit1 & logic.bit2);
      showlogicPins(myx, logic.all, true);
      logic.all |= 0b00000111;
      break;
    case NAND2:
      oled.drawBitmap(myx + 0, 16, bmapAnd, 64, 32, SSD1306_WHITE);       // Base AND
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);                   // No 3rd pin AND
      oled.drawRect(myx + 46, 29, 5, 1, SSD1306_BLACK);                   // clear 4th pin
      oled.drawCircle(myx + 48, 29, 3, SSD1306_WHITE);                    // add invert
      logic.bit3 = ~(logic.bit0 & logic.bit1);
      showlogicPins(myx, logic.all, false);
      logic.all |= 0b00000011;
      break;
    case NAND3:
      oled.drawBitmap(myx + 0, 16, bmapAnd, 64, 32, SSD1306_WHITE);       // Base AND
      oled.drawRect(myx + 46, 29, 5, 1, SSD1306_BLACK);                   // clear 4th pin
      oled.drawCircle(myx + 48, 29, 3, SSD1306_WHITE);                    // add invert
      logic.bit3 = ~(logic.bit0 & logic.bit1 & logic.bit2);
      showlogicPins(myx, logic.all, true);
      logic.all |= 0b00000111;
      break;
    case OR2:
      oled.drawBitmap(myx + 0, 16, bmapOr, 64, 32, SSD1306_WHITE);        // Base OR
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);                   // No 3rd pin OR
      oled.fillRect(myx + 19, 30, 7, 1, SSD1306_BLACK); 
      logic.bit3 = (logic.bit0 | logic.bit1);
      showlogicPins(myx, logic.all, false);
      logic.all |= 0b00000011;
      break;
    case OR3:
      oled.drawBitmap(myx + 0, 16, bmapOr, 64, 32, SSD1306_WHITE);   
      logic.bit3 = (logic.bit0 | logic.bit1 | logic.bit2);
      showlogicPins(myx, logic.all, true);
      logic.all |= 0b00000111;
      break;
    case NOR2:
      oled.drawBitmap(myx + 0, 16, bmapOr, 64, 32, SSD1306_WHITE);        // Base OR
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);                   // No 3rd pin OR
      oled.fillRect(myx + 19, 30, 7, 1, SSD1306_BLACK);                   // No 3rd pin OR part 2
      oled.drawRect(myx + 46, 29, 5, 1, SSD1306_BLACK);                   // clear 4th pin
      oled.drawCircle(myx + 48, 29, 3, SSD1306_WHITE);                    // add invert
      logic.bit3 = ~(logic.bit0 | logic.bit1);
      showlogicPins(myx, logic.all, false);
      logic.all |= 0b00000011;
      break;
    case NOR3:
      oled.drawBitmap(myx + 0, 16, bmapOr, 64, 32, SSD1306_WHITE);        // Base OR
      oled.drawRect(myx + 46, 29, 5, 1, SSD1306_BLACK);                   // clear 4th pin
      oled.drawCircle(myx + 48, 29, 3, SSD1306_WHITE);                    // add invert
      logic.bit3 = ~(logic.bit0 | logic.bit1 | logic.bit2);
      showlogicPins(myx, logic.all, true);
      logic.all |= 0b00000111;
      break;
    case XOR2:
      oled.drawBitmap(myx + 0, 16, bmapXor, 64, 32, SSD1306_WHITE);       // Base OR
      oled.fillRect(myx + 0, 28, 19, 9, SSD1306_BLACK);                   // No 3rd pin OR
      oled.fillRect(myx + 19, 30, 7, 1, SSD1306_BLACK);
      logic.bit3 = (logic.bit0 ^ logic.bit1);
      showlogicPins(myx, logic.all, false);
      logic.all |= 0b00000011;
      break;
    case XOR3:
      oled.drawBitmap(myx + 0, 16, bmapXor, 64, 32, SSD1306_WHITE);       // Base OR
      logic.bit3 = (logic.bit0 ^ logic.bit1 ^ logic.bit2);
      showlogicPins(myx, logic.all, true);
      logic.all |= 0b00000111;
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
      logic.all |= 0b00000011;
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
      logic.all |= 0b00000111;
      break;
  }   
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
Serial.print("Templogic highbyte: ");
Serial.print(templogic, HEX);
Serial.print(", highbyte pins:");
Serial.println(result, HEX);
    if (templogic > 0) { 
      result = calculateLogic(templogic, result, 64);
      writeByte |= (result << 4);
    }
  }
  writeToExpander(writeByte);
}

//------------------------------------------------------------------------------ OnwWire pullup
void PullUp(bool OnOff) {
  if (OnOff) {
    digitalWrite(PULLUPEN, HIGH);
    PullUpActive = true;
  } else {
    digitalWrite(PULLUPEN, LOW);
    PullUpActive = false;
  }
}

//------------------------------------------------------------------------------ dim/bright screen
void checkScreen() {
  timeoutTimer = 500;
  if (screenState > 0 ) {
    oled.dim(false);
  }
}

//------------------------------------------------------------------------------ print 2 Serial or SPI
void printTo(char bufferToPrint[], uint8_t sizeOfBuffer) {
  for (uint8_t i = 0; i < sizeOfBuffer; i++) {
    if (bufferToPrint[i] == 0x00) {
      break;
    }
    if (allSPIisData) {
      SPI_Out.push(bufferToPrint[i]);
      //Serial.write(bufferToPrint[i]);
    } else {
      Serial.write(bufferToPrint[i]);
    }
  }
}


//------------------------------------------------------------------------------ Export I2C capture in Salea format
void saleaeProtocolExport() {
// Time [s],Packet ID,Address,Data,Read/Write,ACK/NAK
// 9.85e-005,0,0xD0,0x72,Write,ACK
// 0.000299,1,0xD1,0x00,Read,ACK
// 0.0003905,1,0xD1,0x2A,Read,NAK
// 0.0005975,2,0xD0,0x3A,Write,ACK
I2CBufferStates dataState;
char buffer[80];
double elapsedTime, startTime;
uint16_t packetid;
uint8_t bitpointer, bitdata, address;
char protocolValue;
bool acknak, readwrite, firstdone;

  elapsedTime = 0;
  packetid = 0;
  bitpointer = 0;
  bitdata = 0;
  dataState = I2CIDLE;
  firstdone = false;
  readwrite = false;
  acknak = false;
  address = 0;
  strcpy (buffer, "Time [s],Packet ID,Address,Data,Read/Write,ACK/NAK\r\n");
  printTo(buffer, 52);
  houseKeeping();

  startTime = (double) (CaptureMinBuffer[0] * PIT_LDVAL0) + CaptureSecBuffer[0];
  for (uint16_t i = 1; i < sizeof(CaptureDataBuffer); i++) {
    protocolValue = ' ';
    if ((CaptureDataBuffer[i] & 0x81) == 0x81) { // Stop
      dataState = I2CSTOP;
      packetid++;
      if (!firstdone) {
        snprintf(buffer, 80, "%.9f,%d,0x%02X,", elapsedTime, packetid, address);
        printTo(buffer, sizeof(buffer));
        if (readwrite) {
          strcpy (buffer, "Read,");
          printTo(buffer, 5);
        } else {
          strcpy (buffer, "Write,");
          printTo(buffer, 6);            
        }
        if (acknak) {
          strcpy (buffer, "NAK\r\n");
          printTo(buffer, 5);
        } else {
          strcpy (buffer, "ACK\r\n");
          printTo(buffer, 5);          
        }
      }
      houseKeeping();
    }
    if ((CaptureDataBuffer[i] & 0x81) == 0x80) { protocolValue='1'; }
    if ((CaptureDataBuffer[i] & 0x81) == 0x00) { protocolValue='0'; }
    if ((CaptureDataBuffer[i] & 0x81) == 0x01) { // Start
      dataState = I2CSTART;
      elapsedTime = (double) (startTime - ((CaptureMinBuffer[i] * PIT_LDVAL0) + CaptureSecBuffer[i])) / F_BUS;
      bitpointer = 0;
      bitdata = 0;
      firstdone = false;
    }
    if (protocolValue == '0' || protocolValue == '1') {
      switch (dataState) {
        case I2CSTART:
          bitpointer = 1;
          bitdata = 0;
          if (protocolValue == '1') { bitdata |= 0x01; }
          dataState = I2CADDRESSBITS;
          break;
        case I2CADDRESSBITS:
          bitdata = bitdata << 1;
          if (protocolValue == '1') { bitdata |= 0x01; }
          if (bitpointer < 6) {
            bitpointer++;
          } else {                                                // I2C Address complete
            address = bitdata;
            dataState = I2CREADWRITE;
            bitpointer = 0;
            bitdata = 0;        
          }
          break;
        case I2CREADWRITE:                                        // Read/Write
          dataState = I2CADDRESSACKNAK;
          if (protocolValue == '1') {
            readwrite = true;                                     // R
          } else {
            readwrite = false;                                    // W
          }
          break;
        case I2CADDRESSACKNAK:
          acknak = false;
          if (protocolValue == '1') { acknak = true; };
          dataState = I2CDATABITS;   
          break;
        case I2CDATABITS:
          bitdata = bitdata << 1;
          if (protocolValue == '1') { bitdata |= 0x01; }
          if (bitpointer < 7) {
            bitpointer++;
          } else {                                                // Data complete
            dataState = I2CDATAACKNAK;
          }
          break;
        case I2CDATAACKNAK:
          acknak = false;
          if (protocolValue == '1') { acknak = true; };
          dataState = I2CDATABITS;
          firstdone = true;
          snprintf(buffer, 80, "%.9f,%d,0x%02X,0x%02X,", elapsedTime, packetid, address, bitdata);
          printTo(buffer, sizeof(buffer));
          if (readwrite) {
            strcpy (buffer, "Read,");
            printTo(buffer, 5);
          } else {
            strcpy (buffer, "Write,");
            printTo(buffer, 6);            
          }
          if (acknak) {
            strcpy (buffer, "NAK\r\n");
            printTo(buffer, 5);
          } else {
            strcpy (buffer, "ACK\r\n");
            printTo(buffer, 5);          
          }
          houseKeeping();         
          break;
        default:
          break;
      }
    }
  }
  houseKeeping();
}

//------------------------------------------------------------------------------ Export I2C capture in Human readable format
void I2CprotocolExport() {
// PacketID;TIME[us];START;0x10;R/W;A;0x00;A;0x00;A;...;0x00;N;STOP

I2CBufferStates dataState;
char buffer[80];
double elapsedTime, startTime;
uint16_t packetid;
uint8_t bitpointer, bitdata;
char protocolValue;
bool acknak;

  packetid = 0;
  bitpointer = 0;
  bitdata = 0;
  dataState = I2CIDLE;
  strcpy (buffer, "Time[us];START;Address;R/W;A/N;Data;A/N;Data;A/N;STOP\r\n");
  printTo(buffer, 55);
  houseKeeping();

  startTime = (double) (CaptureMinBuffer[0] * PIT_LDVAL0) + CaptureSecBuffer[0];
  for (uint16_t i = 1; i < CapturePointer; i++) {
    protocolValue = ' ';
    if ((CaptureDataBuffer[i] & 0x81) == 0x81) { // Stop
      dataState = I2CSTOP;
      bitpointer = 0;
      bitdata = 0;
      packetid++;
      strcpy (buffer, "STOP\r\n");
      printTo(buffer, 6);
      houseKeeping();
    }
    if ((CaptureDataBuffer[i] & 0x81) == 0x80) { protocolValue='1'; }
    if ((CaptureDataBuffer[i] & 0x81) == 0x00) { protocolValue='0'; }
    if ((CaptureDataBuffer[i] & 0x81) == 0x01) { // Start
      dataState = I2CSTART;
      elapsedTime = (double) (startTime - ((CaptureMinBuffer[i] * PIT_LDVAL0) + CaptureSecBuffer[i])) / F_BUS;
      snprintf(buffer, 15, "%.9f", elapsedTime);
      printTo(buffer, sizeof(buffer));       
      strcpy (buffer, ";START;");
      printTo(buffer, 7);
    }

    if (protocolValue == '0' || protocolValue == '1') {
      switch (dataState) {
        case I2CSTART:
          bitpointer = 1;
          bitdata = 0;
          if (protocolValue == '1') { bitdata |= 0x01; }
          dataState = I2CADDRESSBITS;
          break;
        case I2CADDRESSBITS:
          bitdata = bitdata << 1;
          if (protocolValue == '1') { bitdata |= 0x01; }
          if (bitpointer < 6) {
            bitpointer++;
          } else {
            snprintf(buffer, 15, "%02X;", bitdata);
            printTo(buffer, 3);
            dataState = I2CREADWRITE;
            bitpointer = 0;
            bitdata = 0;        
          }
          break;
        case I2CREADWRITE:
          dataState = I2CADDRESSACKNAK;
          if (protocolValue == '1') {
            strcpy (buffer, "R;");
          } else {
            strcpy (buffer, "W;");
          }
          printTo(buffer, 2);
          break;
        case I2CADDRESSACKNAK:
          acknak = false;
          if (protocolValue == '1') { acknak = true; };
          dataState = I2CDATABITS;
          if (acknak) {
            strcpy (buffer, "N;");
          } else {
            strcpy (buffer, "A;");
          }
          printTo(buffer, 2);     
          break;
        case I2CDATABITS:
          bitdata = bitdata << 1;
          if (protocolValue == '1') { bitdata |= 0x01; }
          if (bitpointer < 7) {
            bitpointer++;
          } else {
            dataState = I2CDATAACKNAK;
            snprintf(buffer, 15, "%2X;", bitdata);
            printTo(buffer, sizeof(buffer));
            bitpointer = 0;
            bitdata = 0;
          }
          break;
        case I2CDATAACKNAK:
          acknak = false;
          if (protocolValue == '1') { acknak = true; };
          dataState = I2CDATABITS;
          if (acknak) {
            strcpy (buffer, "N;");
          } else {
            strcpy (buffer, "A;");
          }
          printTo(buffer, 2);
          break;
        default:
          break;
      }
    }
  }
  strcpy (buffer, "\r\n");
  printTo(buffer, 2);
  houseKeeping();
}

void rawI2CExport(bool timinginfo) {
char buffer[20];
double elapsedTime, startTime;

  if (timinginfo) {
    strcpy (buffer, "t[s],SDA,SCL\r\n");
    printTo(buffer, 14);
  } else {
    strcpy (buffer, "SDA,SCL\r\n");
    printTo(buffer, 9);
  }
  houseKeeping();
  startTime = (double) (CaptureMinBuffer[0] * PIT_LDVAL0) + CaptureSecBuffer[0];
  for (uint16_t i = 0; i < CapturePointer; i++) {
    if (timinginfo) {
      elapsedTime = (double) (startTime - ((CaptureMinBuffer[i] * PIT_LDVAL0) + CaptureSecBuffer[i])) / F_BUS;
      snprintf(buffer, 20, "%.9f,", elapsedTime);
      printTo(buffer, sizeof(buffer));
    }
    if ((CaptureDataBuffer[i] & 0x80) == 0x80) {
      strcpy (buffer, "1,");
    } else {
      strcpy (buffer, "0,");
    }
    printTo(buffer, 2);
    if ((CaptureDataBuffer[i] & 0x10) == 0x10) {
      strcpy (buffer, "1\r\n");
    } else {
      strcpy (buffer, "0\r\n");
    }
    printTo(buffer, 3);
    houseKeeping();
  }
  strcpy (buffer, "\r\n");
  printTo(buffer, 2);
  houseKeeping();
}