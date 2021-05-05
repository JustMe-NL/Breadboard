#include <breadboard.h>

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
  screenTimeout = 0;
  if (screenState > 0 ) {
    oled.dim(false);
  }
}

void printTo(char bufferToPrint[], uint8_t sizeOfBuffer) {
  for (uint8_t i = 0; i < sizeOfBuffer; i++) {
    if (allSPIisData) {
      SPI_Out.push(bufferToPrint[i]);
    } else {
      Serial.write(bufferToPrint[i]);
    }
  }
  houseKeeping();
}

/*
//------------------------------------------------------------------------------ Export I2C capture in Salea format
void salaeProtocolExport() {
// Time [s],Packet ID,Address,Data,Read/Write,ACK/NAK
// 0.004328500000000,0,0xD0,0x75,Write,ACK

capture_struct capturedByte;
I2CBufferStates dataState;
char myBuffer[25];
uint16_t packetid;
unsigned long packettime, capstarttime;
uint8_t bitpointer, bitdata, addressbyte;
bool acknak, readwrite;

  packetid = 0;
  bitpointer = 0;
  bitdata = 0;
  dataState = I2CIDLE;
  
  capturedByte = CaptureBuf[0];
  capstarttime = capturedByte.time;
  strcpy (myBuffer, "Time [s],Packet ID,Address,Data,Read/Write,ACK/NAK\r\n");
  Serial.print(myBuffer);

  for (uint16_t i = 0; i < 4096; i++) {
    capturedByte = CaptureBuf[i];

    if (capturedByte.value == 0x91) {
      if (dataState != I2CDATAACKNAK || dataState != I2CDATABITS) {
        snprintf(myBuffer, 25, "%.15f,%u,", (float) packettime / 1000000, packetid);
        Serial.print(myBuffer);
        snprintf(myBuffer, 25, "0x%02X,", addressbyte);
        Serial.print(myBuffer);
        if (readwrite) {
          strcpy (myBuffer, "Read,");
        } else {
          strcpy (myBuffer, "Write,");
        }
        Serial.print(myBuffer);
        if (acknak) {
          strcpy (myBuffer, "NAK\r\n");
        } else {
          strcpy (myBuffer, "ACK\r\n");
        }
        Serial.print(myBuffer);
      }
      dataState = I2CSTOP;
      bitpointer = 0;
      bitdata = 0;
      addressbyte = 0;
      packetid++;
    }

    if (capturedByte.value == 0x11) {
      dataState = I2CSTART;
      packettime = capturedByte.time - capstarttime;
    }
    
    if (capturedByte.value == 0x12 || capturedByte.value == 0x92) {
      switch (dataState) {
        case I2CSTART:
          bitpointer = 1;
          bitdata = 0;
          if (capturedByte.value == 0x92) { bitdata |= 0x01; }
          dataState = I2CADDRESSBITS;
          break;
        case I2CADDRESSBITS:
          bitdata = bitdata << 1;
          if (capturedByte.value == 0x92) { bitdata |= 0x01; }
          if (bitpointer < 6) {
            bitpointer++;
          } else {
            dataState = I2CREADWRITE;
            addressbyte = bitdata;
            bitpointer = 0;
            bitdata = 0;
          }
          break;
        case I2CREADWRITE:
          readwrite = (bool) capturedByte.value & 0x90;
          dataState = I2CADDRESSACKNAK;
          break;
        case I2CADDRESSACKNAK:
          acknak = false;
          if (capturedByte.value == 0x92) { acknak = true; };
          dataState = I2CDATABITS;
          break;
        case I2CDATABITS:
          bitdata = bitdata << 1;
          if (capturedByte.value == 0x92) { bitdata |= 0x01; }
          if (bitpointer < 7) {
            bitpointer++;
          } else {
            dataState = I2CDATAACKNAK;
          }
          break;
        case I2CDATAACKNAK:
          acknak = false;
          if (capturedByte.value == 0x92) { acknak = true; };
          snprintf(myBuffer, 25, "%.15f,%u,", (float) packettime / 1000000, packetid);
          Serial.print(myBuffer);
          snprintf(myBuffer, 25, "0x%02X,", addressbyte);
          Serial.print(myBuffer);
          snprintf(myBuffer, 25, "0x%02X,", bitdata);
          Serial.print(myBuffer);
          if (readwrite) {
            strcpy (myBuffer, "Read,");
          } else {
            strcpy (myBuffer, "Write,");
          }
          Serial.print(myBuffer);
          if (acknak) {
            strcpy (myBuffer, "NAK\r\n");
          } else {
            strcpy (myBuffer, "ACK\r\n");
          }
          Serial.print(myBuffer);
          dataState = I2CDATABITS;
          bitpointer = 0;
          bitdata = 0;
          break;
        default:
          break;
      }
    }
  }
}
*/

//------------------------------------------------------------------------------ Export I2C capture in Human readable format
void I2CprotocolExport() {
// PacketID;TIME[us];START;0x10;R/W;A;0x00;A;0x00;A;...;0x00;N;STOP

I2CBufferStates dataState;
char buffer[80];
double elapsedTime;
uint16_t packetid;
uint8_t bitpointer, bitdata;
char protocolValue;
bool acknak;

  packetid = 0;
  bitpointer = 0;
  bitdata = 0;
  dataState = I2CIDLE;
  strcpy (buffer, "Time[us];START;Address;R/W;A/N;Data;A/N;Data;A/N;STOP\r\n");
  printTo(buffer, sizeof(buffer));

  for (uint16_t i = 1; i < sizeof(CaptureDataBuffer); i++) {
    protocolValue = ' ';
    if (CaptureDataBuffer[i] == 0x90) {
      if (CaptureDataBuffer[i-1] == 0x10) { // Stop
        dataState = I2CSTOP;
        bitpointer = 0;
        bitdata = 0;
        packetid++;
        strcpy (buffer, "STOP\r\n");
        printTo(buffer, sizeof(buffer));
      }
      if (CaptureDataBuffer[i-1] == 0x80) { protocolValue='1'; }
    }
    if (CaptureDataBuffer[i] == 0x10) {
      if (CaptureDataBuffer[i-1] == 0x00) { protocolValue='0'; }
      if (CaptureDataBuffer[i-1] == 0x90) { // Start
        dataState = I2CSTART;
        elapsedTime = (double) (CaptureTimeBuffer[0] - CaptureTimeBuffer[i]) / F_BUS;
        snprintf(buffer, 15, "%.9f", elapsedTime);
        printTo(buffer, sizeof(buffer));
        strcpy (buffer, ";START;");
        printTo(buffer, sizeof(buffer));
      }
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
            printTo(buffer, sizeof(buffer));
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
          printTo(buffer, sizeof(buffer));
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
          printTo(buffer, sizeof(buffer));     
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
          printTo(buffer, sizeof(buffer));
          break;
        default:
          break;
      }
    }
  }
}

/*
//------------------------------------------------------------------------------ Export RAW I2C capture
void rawI2CExport() {
char myBuffer[70];

  strcpy (myBuffer, "Packet ID;Timestamp;Value(SDA:0x80±1,SCL:0x10±2);I2C data\r\n");
  Serial.print(myBuffer);
  for (uint16_t i = 0; i < CaptureBuf.size(); i++) {
    curByte = CaptureBuf[i];
    if (curByte.value == 0x11 || curByte.value == 0x12 || curByte.value == 0x91 || curByte.value == 0x92) {
      snprintf(myBuffer, 70, "%04d;%010lu;%02X;", i, curByte.time, curByte.value);
      Serial.print(myBuffer);
      switch (curByte.value) {
        case 0x01:                                                   // SDA \ , SCL 0 (ignore)
          break;           
        case 0x11:                                                   // SDA \ , SCL 1 (start)
          strcpy (myBuffer, "start");
          Serial.print(myBuffer);
          break;
        case 0x12:                                                   // SDA 0 , SCL / (0)
          Serial.print("0");
          break;
        case 0x81:                                                   // SDA / , SCL 0 (ignore)
          break;
        case 0x91:                                                   // SDA / , SCL 1 (stop)
          strcpy (myBuffer, "stop");
          Serial.print(myBuffer);
          break;
        case 0x92:                                                   // SDA 1 , SCL / (1)
          Serial.print("1");
          break;
      }
      Serial.print("\r\n");
    }
  }
}
*/