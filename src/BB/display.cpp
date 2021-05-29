#include <breadboard.h>

// void dispHeader()
// void processCursor()
// void nowcursor()
// void sharedNavigation()
// void setDigitInValue()
// void changeMyValue()
// void getUserValue()
// void setUserValue()
// void showCounter()
// void showFreqLowCount()
// void showFreqHighCount()
// void showI2CResults()
// void showOneWireScanResults()
// void showOneWireTestResults()
// void pinMonitor()
// void showInfo()
// void displayServo()
// void cooldownStepper()
// void pulseGenerator()
// void displayISPAVRProgrammer()
// void displayESPProgrammer()
// void displayVoltemeter()
// void display7Segment()
// void displayLogic()
// void displayScreenSet();
// void displayI2CMonitor()
// void displayExport()

//------------------------------------------------------------------------------ dispHeader
void dispHeader() {
  char dispValue[6];
  oled.fillRect(0, 0, 128, 15, SSD1306_BLACK);
  oled.drawRect(0, 0, 128, 15, SSD1306_WHITE);
  oled.setCursor(4, 11);
  oled.print("V:");
  snprintf(dispValue, 6, "%05.2f", curVolt);
  oled.print (dispValue);
  if (cutOff) {
    oled.fillRect(71, 0, 57, 15, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
  }
    oled.setCursor(75, 11);
    oled.print("A:");
    snprintf(dispValue, 6, "%1.3f", (float) curAmp / 1000);
    oled.print (dispValue);
    oled.setTextColor(SSD1306_WHITE);   
  if (control.powerRelay) {
    oled.fillRect(57, 0, 14, 15, SSD1306_WHITE);
    oled.setCursor(60, 11);
    oled.setTextColor(SSD1306_BLACK);
    oled.print("P");
    oled.setTextColor(SSD1306_WHITE);
  } else {
    oled.drawRect(57, 0, 14, 15, SSD1306_WHITE);
    oled.setCursor(60, 11);
    oled.print("P"); 
  }
  if (control.serialRelay) {
    oled.drawRect(102, 49, 26, 15, SSD1306_WHITE);
    oled.setCursor(104, 60);
    oled.print("R/T");
  } else { oled.fillRect(102, 49, 26, 15, SSD1306_BLACK); }
}

//------------------------------------------------------------------------------ proces Cursor
void processCursor() {
  uint8_t position;
  if (cursoron.blinkon) {
    switch (options) {
      case OPTIONSETUPDOWN:
        position = 26;
        oled.fillRect(position, 49, 22, 15, SSD1306_BLACK);
        if (cursoron.blinkstate) {
          oled.fillRoundRect(position, 49, 22, 15, 3, SSD1306_WHITE);
          oled.setTextColor(SSD1306_BLACK);
          oled.setCursor(position + 2, 59);
          oled.print("<>");   // 26-48 (27)        
          oled.setTextColor(SSD1306_WHITE);
        } else {
          oled.drawRoundRect(position, 49, 22, 15, 3, SSD1306_WHITE);
          oled.setCursor(position + 2, 59);
          oled.print("<>");   // 26-48 (27)
        }
        break;
      case OPTIONVALUES:
      case OPTIONSETVALUES:
        if (options == OPTIONSETVALUES) {
          position = 26;
          oled.fillRect(position, 49, 22, 15, SSD1306_BLACK);
          if (cursoron.blinkstate) {
            oled.fillRoundRect(position, 49, 22, 15, 3, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
            oled.setCursor(position + 2, 59);
            oled.print("10");   // 26-48 (27)        
            oled.setTextColor(SSD1306_WHITE);
          } else {
            oled.drawRoundRect(position, 49, 22, 15, 3, SSD1306_WHITE);
            oled.setCursor(position + 2, 59);
            oled.print("10");   // 26-48 (27)
          }
        }
        if (sysState == MENULOGICACTIVE) {
          position = 0 + (setValue * 64);
          if (cursoron.blinkstate) {
            oled.fillCircle(position + 36, 30, 4, SSD1306_WHITE);
          } else {
            oled.fillCircle(position + 36, 30, 4, SSD1306_BLACK);
          }
        } else {
          oled.setCursor(cursoron.blinkcol, cursoron.blinkrow);
          if (cursoron.blinkstate) {
            if (cursoron.blinktype) {
              oled.fillRect(cursoron.blinkcol, cursoron.blinkrow - 10, 8, 12, SSD1306_BLACK);
            } else {
              oled.drawRect(cursoron.blinkcol, cursoron.blinkrow - 10, 8, 12, SSD1306_WHITE);
              oled.print(cursoron.blinkchar);
            }
          } else {
            oled.print(cursoron.blinkchar);
          }
        }
        break;
      default:
        break;
    }
    oled.display();
    cursoron.blinkstate = !cursoron.blinkstate;
  }
}

//------------------------------------------------------------------------------ set cursor
void nowcursor(char cursorchar, boolean blinktype) {
  cursoron.blinkon = true;
  cursoron.blinkchar = cursorchar;
  cursoron.blinkstate = false;
  cursoron.blinktype = blinktype;
  cursoron.blinkcol = oled.getCursorX();
  cursoron.blinkrow = oled.getCursorY();
}

//------------------------------------------------------------------------------ shared Navifation
void sharedNavigation(bool process) {
  uint8_t position;
  uint8_t savedX, savedY;
  savedX = oled.getCursorX();
  savedY = oled.getCursorY();

  position = 0;
  if (menuoptions != OPTIONSCANCEL) {
    oled.fillRect(position, 49, 23, 15, SSD1306_BLACK);
    if (options == OPTIONOK) {
      oled.fillRoundRect(position, 49, 23, 15, 3, SSD1306_WHITE);
      oled.setCursor(position + 3, 60);
      oled.setTextColor(SSD1306_BLACK);
      oled.print("Ok");   // 0-23 (3)   
      oled.setTextColor(SSD1306_WHITE);
    } else {
      oled.drawRoundRect(position, 49, 23, 15, 3, SSD1306_WHITE);
      oled.setCursor(position + 3, 60);
      oled.print("Ok");   // 0-23 (3)
    }
    position += 26;
  }
  if (menuoptions == OPTIONSOKSET || menuoptions == OPTIONSOKSETCANCEL) {
    oled.fillRect(position, 49, 22, 15, SSD1306_BLACK);
    if (options == OPTIONUPDOWN || options == OPTIONSETVALUES) {
      oled.fillRoundRect(position, 49, 22, 15, 3, SSD1306_WHITE);
      oled.setTextColor(SSD1306_BLACK);
      if (options == OPTIONSETVALUES) {
        oled.setCursor(position + 2, 60);
        oled.print("10");   // 26-48 (27)
      } else {
         oled.setCursor(position + 2, 59);
        oled.print("<>");   // 26-48 (27)           
      }
      oled.setTextColor(SSD1306_WHITE); 
    } else {
      oled.drawRoundRect(position, 49, 22, 15, 3, SSD1306_WHITE);
      if (options == OPTIONSETVALUES) {
        oled.setCursor(position + 2, 60);
        oled.print("10");   // 26-48 (27)
      } else {
        oled.setCursor(position + 2, 59);
        oled.print("<>");   // 26-48 (27)
      }
    }
    position += 25;
  }
  if (menuoptions == OPTIONSOKSETCANCEL || menuoptions == OPTIONSCANCEL) {
    oled.fillRect(position, 49, 35, 15, SSD1306_BLACK);
    if (options == OPTIONCANCEL) {
      oled.fillRoundRect(position, 49, 35, 15, 3, SSD1306_WHITE);
      oled.setCursor(position + 3, 60);
      oled.setTextColor(SSD1306_BLACK);
      oled.print("Stop"); // 51-86 (54)  
      oled.setTextColor(SSD1306_WHITE); 
    } else {
      oled.drawRoundRect(position, 49, 35, 15, 3, SSD1306_WHITE);
      oled.setCursor(position + 3, 60);
      oled.print("Stop"); // 51-86 (54)   
    }  
    position += 35;
  }
  oled.fillRect(position, 49, 101 - position, 15, SSD1306_BLACK);
  oled.setCursor(savedX, savedY);
}

//------------------------------------------------------------------------------ set digit in value
void setDigitInValue(boolean subtract) {
  byte myDigit = 8 - maxDigits + editValue;
  uint8_t mystep = 1;
  byte myval = (byte) myValue[myDigit];
  switch (inputType) {
    case INPUTNUMBER:
      if (myDigit == 6) {
        if (valuestep > 10) { mystep = valuestep / 10; }  
      }
      if (subtract) {
        if (myval >= (0x30 + mystep)) { myValue[myDigit] = char(myval - mystep); }  
      } else {
        if (myval <= (0x39 - mystep)) { myValue[myDigit] = char(myval + mystep); } 
      }
      break;
    case INPUTBINARY:
      if (myval > 0x30) { 
        myValue[myDigit] = 0x30;  
      } else {
        myValue[myDigit] = 0x31;
      }
      setValue = 0;
      for (uint8_t i=0; i<8; i++) {
        setValue = setValue << 1;
        if (myValue[i] & 0x01) { setValue++; }
      }
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------ step value up/down
void changeMyValue(boolean changeUp) {
  uint8_t temp = 0;
  switch (inputType) {
    case INPUTNUMBER:
      if (changeUp) {
        setValue = atol(myValue);
        if (setValue > maxValue) { setValue = maxValue; }
        if (setValue <= (maxValue - valuestep)) {
          setValue += valuestep;
          snprintf(myValue, 10, "%08lu", setValue);
        }
      } else {
        setValue = atol(myValue);
        if (setValue > maxValue) { setValue = maxValue; }
        if (setValue >= (0 + valuestep)) { 
          setValue -= valuestep; 
          snprintf(myValue, 10, "%08lu", setValue);
        }
      }
      break;
    case INPUTBAUD:
      if (changeUp) {
        if (baudRate < (sizeof(baudRates)/sizeof(baudRates[0])) - 1) { baudRate++; }
      } else {
        if (baudRate > 0) { baudRate--; }
      }
      setValue = baudRates[baudRate];
      snprintf(myValue, 10, "%08lu", setValue);
      break;
    case INPUTBINARY:
      if (changeUp) {
        setValue++;
      } else {
        setValue--; 
      }
      setValue = setValue & 0xFF;
      temp = setValue;
      for (uint8_t i=8; i>0; i--) {
        myValue[i - 1] = (temp & 0x01) + 0x30;
        temp = temp >> 1;
      }
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------ get values from user
// 0:OPTIONSTART, 1:OPTIONCANCEL, 2:OPTIONUPDOWN, 4:OPTIONOK, 3:OPTIONVALUES, 4:OPTIONSETVALUES, 5:OPTIONSETUPDOWN, 6:OK, 7:CANCEL
void getUserValue() {
  uint8_t mycol;
  enum optionStates temp;
  
  temp = options;
  if (encoder.up) {                                           // dial up
    switch(options) {
      case OPTIONOK:
        if (valuestep < 10) {
          editValue = maxDigits - 1;
        } else {
          editValue = maxDigits - 2;
        }
        if ((inputType == INPUTNUMBER) || (inputType == INPUTBINARY)) {
          temp = OPTIONVALUES;
        } else {
          if (menuoptions == OPTIONSOKSETCANCEL || menuoptions == OPTIONSCANCEL) {
            temp = OPTIONUPDOWN;
          } else {
            temp = OPTIONCANCEL;
          }
        }
        break;
      case OPTIONUPDOWN:
        temp = OPTIONOK;
        break;
      case OPTIONCANCEL:
        temp = OPTIONUPDOWN;
        break;
      case OPTIONVALUES:
        editValue--;
        if (editValue < 0) { temp = OPTIONUPDOWN; }
        break;
      case OPTIONSETVALUES:
        setDigitInValue(true);
        break;
      case OPTIONSETUPDOWN:
        changeMyValue(false);
        break;
      default:
        break;
    }
  }
  if (encoder.down) {                                         // dial down
    switch(options) {
      case OPTIONCANCEL:
        editValue = 0;
        temp = OPTIONVALUES;
        break;
      case OPTIONUPDOWN:
        if ((inputType == INPUTNUMBER) || (inputType == INPUTBINARY)) {
            editValue = 0;
            temp = OPTIONVALUES;
        } else {
          if (menuoptions == OPTIONSOKSETCANCEL || menuoptions == OPTIONSCANCEL) {
            temp = OPTIONCANCEL;
          } else {
            editValue = 0;
            temp = OPTIONVALUES;
          }
        }
        break;
      case OPTIONOK:
        temp = OPTIONUPDOWN;
        break;
      case OPTIONVALUES:
        editValue++;
        if (valuestep < 10) {
          if (editValue >= maxDigits) { temp = OPTIONOK; }
        } else {
          if (editValue >= (maxDigits - 1)) { temp = OPTIONOK; }
        }
        break;
      case OPTIONSETVALUES:
        setDigitInValue(false);
        break;
      case OPTIONSETUPDOWN:
        changeMyValue(true);
        break;
      default:
        break;
    }
  }

  if (encoder.sw) {                                           // dial clicked
    switch(options) {
      case OPTIONCANCEL:
        temp = CANCEL;
        cursoron.blinkon = false;
        forcedisplay++;
        break;
      case OPTIONUPDOWN:
        temp = OPTIONSETUPDOWN;
        break;
      case OPTIONOK:
        if (inputType == INPUTNUMBER) {
          setValue = atol(myValue);
          if (setValue > maxValue) { 
            setValue = maxValue;
            snprintf(myValue, 10, "%08lu", setValue);
          } else {
            temp = OK;
            cursoron.blinkon = false;
            forcedisplay++;
          }
        }
        if (inputType == INPUTBAUD) {
            temp = OK;
            cursoron.blinkon = false;
            forcedisplay++;
        }
        if (inputType == INPUTBINARY) {
            setValue = 0;
            for (uint8_t i=0; i<8; i++) {
              setValue = setValue << 1;
              if (myValue[i] & 0x01) { setValue++; }
            }
            temp = OK;
            cursoron.blinkon = false;
            forcedisplay++;
        }      
        break;
      case OPTIONVALUES:
        temp = OPTIONSETVALUES;
        break;
      case OPTIONSETVALUES:
        temp = OPTIONVALUES;
        break;
      case OPTIONSETUPDOWN:
        temp = OPTIONUPDOWN;
        break;
      default:
        break;     
    }
  }
  options = temp;
  if (inputType == INPUTBINARY) { writeToExpander((uint8_t) setValue); }
  
  sharedNavigation();
  oled.fillRect(0, 16, 128, 17, SSD1306_BLACK);
  oled.setCursor(5, 32);

  for (uint8_t i=8 - maxDigits; i<8; i++) {
    mycol = oled.getCursorX();
    if (i == 8 - maxDigits + editValue) {
      if (options == OPTIONSETVALUES) { nowcursor(myValue[i], true); }
      if (options == OPTIONVALUES) { nowcursor(myValue[i]); }
    }
    oled.print(myValue[i]);
    oled.setCursor(mycol + 7, 32);
  }
  
  switch (valueType) {
    case VALUEA:
      oled.print(" mA");
      break;
    case VALUEV:
      oled.print(" mV");
      break;
    case VALUEBAUD:
      if (slowSlave) {
        snprintf(myValue, 16, " (%.1f) baud", (float) baudErrLow[baudRate] / 10);
      } else {
        snprintf(myValue, 16, " (%.1f) baud", (float) baudErrHigh[baudRate] / 10);
      }
      oled.print(myValue);
      snprintf(myValue, 10, "%08lu", setValue);
      break;
    case VALUENONE:
      break;
  }

  if (sysState == MENUSETPINSSETVALUEACTIVE) {
    bytePins = setValue;
    writeToExpander(bytePins);
  }
}

//------------------------------------------------------------------------------ init get value
void setUserValue(unsigned long valueToSet, uint8_t valstep) {
  char mymaxValue[10];
  setValue = valueToSet;
  editValue = 0;
  options = OPTIONOK;
  encoder.sw = false;
  snprintf(mymaxValue, 10, "%lu", maxValue);
  snprintf(myValue, 10, "%08lu", setValue);
  maxDigits = strlen(mymaxValue);
  valuestep = valstep;
  if (inputType == INPUTBAUD) { valueType = VALUEBAUD; }
  if (inputType == INPUTBINARY) { valueType = VALUENONE; }
  getUserValue();
}

//------------------------------------------------------------------------------ Counter
void showCounter() {

  countme = false;
  if (setValue > 9999999999999999) { setValue = 0; }

  if (encoder.sw) {                                           // clicked
    setValue = 0;
  }

  oled.fillRect(0, 16, 128, 19, SSD1306_BLACK);
  oled.setCursor(0, 32);
  snprintf(myValue, 22, "%016lu", setValue);
  oled.print(myValue);
}

//------------------------------------------------------------------------------ Low frequency
void showFreqLowCount() {
  if (encoder.sw) {                                           // clicked
    options = OK;
    cursoron.blinkon = false;
    forcedisplay++;
  }

  oled.fillRect(0, 16, 128, 19, SSD1306_BLACK);
  oled.setCursor(5, 32);
  if (timeoutTimer > 500) {
    if (count1 > 0) {
      snprintf(myValue, 16, "%.3f", (float) freq1.countToFrequency(sum1 / count1));
      oled.print(myValue);
      oled.print(" Hz");
      count1 = 0;
      sum1 = 0;
      timeoutTimer = 0;
    } else {
      oled.print("(no pulses)");
    }
  }
}

//------------------------------------------------------------------------------ High frequency
void showFreqHighCount() {
  if (encoder.sw) {                                           // clicked
    options = OK;
    cursoron.blinkon = false;
    forcedisplay++;
  }

  oled.fillRect(0, 16, 128, 19, SSD1306_BLACK);
  oled.setCursor(5, 32);
  unsigned long fcount = FreqCount.read();
  float f_fcount = (float) fcount;
  snprintf(myValue, 16, "%.2f", f_fcount / 100);
  oled.print(myValue);
  oled.print(" kHz");
}

//------------------------------------------------------------------------------ show results I2C scan
void showI2CResults() {
  if (encoder.up) {                                           // dial up
    if (i2cpointer > 0) { i2cpointer--; }
  }
  if (encoder.down) {                                         // dial down
    if (i2cpointer < buffer[0]) { i2cpointer++; }
  }

  if (encoder.sw) {                                           // dial clicked
    options = OK;
    cursoron.blinkon = false;
    forcedisplay++;
  }

  oled.fillRect(0, 16, 128, 17, SSD1306_BLACK);
  oled.setCursor(5, 32);
  if (i2cpointer == 0) {
    oled.print(buffer[0]);
    oled.print(" found");
  } else {
    oled.print("#");
    oled.print(i2cpointer);
    oled.print(" @ ");
    oled.print(buffer[i2cpointer]);
    oled.print("  0x");
    oled.print(buffer[i2cpointer], HEX);
  }
}

//------------------------------------------------------------------------------ show results DS18x20 test
void showOneWireScanResults() {
  int temperature;
  sharedNavigation();
  if (encoder.up) {                                           // dial up
    if (setValue > 0 ) { setValue--; }
  }
  if (encoder.down) {                                           // dial down
    if (setValue < (unsigned long) buffer[0] - 1) { setValue++; }
  }
  if (encoder.sw) {                                           // clicked
    options = OK;
    cursoron.blinkon = false;
    forcedisplay++;
  }

  oled.fillRect(0, 16, 128, 17, SSD1306_BLACK);
  oled.setCursor(5, 32);
  if (buffer[0] == 0) {
    oled.print("No sensors found");
  } else {
    oled.print("[..");
#ifdef debug
    dumpBuffer();
#endif
    snprintf(myValue, 10, "%02X%02X%02X]", buffer[(setValue * 0x10) + 6], buffer[(setValue * 0x10) + 7], buffer[(setValue * 0x10) + 8]);
    oled.print(myValue);
    oled.print("  ");
    temperature = (buffer[(setValue * 0x10) + 10] << 8);
    temperature+= buffer[(setValue * 0x10) + 9];
    float floattemp = (float) temperature;
    floattemp = floattemp * 0.0625;
    snprintf(myValue, 10, "%.1f", floattemp);
    oled.print(myValue);
    oled.print("C");
  }
}

//------------------------------------------------------------------------------ show results DS18x20 test
void showOneWireTestResults() {
  uint8_t helpvar;
  sharedNavigation();
  if (encoder.up) {                                           // dial up
    helpvar = (uint8_t) setValue;
    do {
      if (helpvar > 0) { helpvar--; }
    } while (helpvar > 0 && checklist[helpvar] == false);
    if (checklist[helpvar]) { setValue = helpvar; }
  }
  if (encoder.down) {                                           // dial down
    helpvar = (uint8_t) setValue;
    do {
      if (helpvar < 12) { helpvar++; }
    } while (helpvar < 12 && checklist[helpvar] == false);
    if (checklist[helpvar]) { setValue = helpvar; }
  }
  if (encoder.sw) {                                           // clicked
      options = OK;
      cursoron.blinkon = false;
      forcedisplay++;
  }

  oled.fillRect(0, 16, 128, 17, SSD1306_BLACK);
  oled.setCursor(5, 32);
  if (faillures == 0) {
    if (options == OPTIONUPDOWN) { options = OPTIONCANCEL; }
    oled.print("No device or OK");
  } else {
    if (checklist[setValue]) {
      switch (setValue) {
        case 0:
          oled.print("failed on ");
          oled.print(faillures);
          oled.print(" tests");
          break;
        case 1:
          oled.print("CRC error");
          break;
        case 2:
          oled.print("invalid ROM");
          break;
        case 3:
          oled.print("scratchpad 5 err");
          break;
        case 4:
          oled.print("scratchpad 6 err");
          break; 
        case 5:
          oled.print("scratchpad 7 err");
          break;
        case 6:
          oled.print("alarmreg error");
          break; 
        case 7:
          oled.print("no 10 bit resolution");
          break;
        case 8:
          oled.print("reserved err 10b");
          break; 
        case 9:
          oled.print("no 12 bit resolution");
          break;
        case 10:
          oled.print("reserved err 12b");
          break;
        case 11:
          oled.print("temp error");
          break;
      }
    }
  }
}

//------------------------------------------------------------------------------ pin monitor
void pinMonitor() {
  uint8_t position, tmpbyte;
  sharedNavigation();
  if ((inputType != INPUTDECIMAL) && (inputType != INPUTSIGNED) && (inputType != INPUTHEXADECIMAL)) { inputType = INPUTDECIMAL; }
  if (encoder.up) {                                           // dial up
    switch (inputType) {
      case INPUTDECIMAL:
        inputType = INPUTSIGNED;
        break;
      case INPUTSIGNED:
        inputType = INPUTHEXADECIMAL;
        break;
      case INPUTHEXADECIMAL:
        inputType = INPUTDECIMAL;
        break;     
      default:
        break;       
    }
  }
  if (encoder.down) {                                           // dial down
    switch (inputType) {
      case INPUTDECIMAL:
        inputType = INPUTHEXADECIMAL;
        break;
      case INPUTSIGNED:
        inputType = INPUTDECIMAL;
        break;
      case INPUTHEXADECIMAL:
        inputType = INPUTSIGNED;
        break;  
      default:
        break;      
    }
  }
  if (encoder.sw) {                                           // clicked
    options = OK;
    cursoron.blinkon = false;
    forcedisplay++;
  }

  oled.fillRect(0, 16, 128, 17, SSD1306_BLACK);
  tmpbyte = bytePins;
  position = 5;
  for (uint8_t i=0; i<8; i++) {
    oled.setCursor(position, 32);
    if ((tmpbyte & 0x80) > 0) {
      oled.print("1");
    } else {
      oled.print("0");
    }
    position += 8;
    tmpbyte = tmpbyte << 1;
  }
  oled.print("  ");
  switch (inputType) {
    case INPUTDECIMAL:
      snprintf(myValue, 10, "%03ud", bytePins);
      break;
    case INPUTSIGNED:
      snprintf(myValue, 10, "%+03dd", (int8_t) bytePins);
      break;
    case INPUTHEXADECIMAL:
     snprintf(myValue, 10, "%02Xh", bytePins);
     break;
    default:
      break;
  }
  oled.print(myValue);
}

//------------------------------------------------------------------------------ show information
void showInfo(const char* helptext) {
uint16_t screenpos, temppos, startpos, rowcount, txtlength, spacelength, h, i, j, w;
bool eoline, eotext;
char tempword[50];
int16_t  x1, y1;

  txtlength = 1;                                                // Get stringlength
  for(i = 0; i < MAXINFOTEXT; i++) {
    if (helptext[i] == 0) { txtlength = i; break; }
  }

  if (firstrun) {
    encoder.sw = false;
    options = OPTIONOK;
    menuoptions = OPTIONSOKSET;
    sharedNavigation();

#ifdef debug
    j = 0;
    for (i=0; i<txtlength; i++) {
      Serial.print(helptext[i]);
      j++;
      if (j == 8) { Serial.print(" - "); }
      if (j == 16) { Serial.println(); j = 0; }
    }
#endif
    oled.getTextBounds(" ", 0, 0, &x1, &y1, &w, &h);
    spacelength = 3 * w;
    rowcount = 0;                                                 // row counter
    startpos = 0;                                                 // start of charstring at new line
    eotext = false;
    while (!eotext) {                                             // until the end of the text
      if (helptext[startpos] == ' ') { startpos++; }              // skip first space
      buffer[rowcount] = startpos;                                // write start of line
      eoline = false;
      screenpos = 0;                                              // position on the screen
      while (!eoline) {                                           // as long as it fits ...
        temppos = txtlength;                                      // set as maximum if only one word
        for(i = startpos; i <= txtlength; i++) {                  // search for space starting @ startpos
          if (helptext[i] == ' ') {
            temppos = i;                                          // Found a space, store in temppos
            break;
          }
        }
        if (temppos < txtlength) {                                // did we find a space?
#ifdef debug 
        Serial.print("Spatie gevonden, txtlength:"); 
        Serial.print(txtlength); 
        Serial.print(", startpos:");
        Serial.print(startpos); 
        Serial.print(", temppos:"); 
        Serial.print(temppos); 
        Serial.print(", rowpos:");
        Serial.print(rowcount);
        Serial.print(", word:'"); 
#endif
          j = 0;
          if ((temppos - startpos) < 50) {
            for (i = startpos; i < temppos; i++) {                // copy word
              tempword[j] = helptext[i];
              j++;
              tempword[j] = 0;                                    // end with NULL
            }
          }
          oled.getTextBounds(tempword, 0, 0, &x1, &y1, &w, &h);   // how long is the piece of text on screen?
#ifdef debug 
          Serial.print(tempword);
          Serial.print("', text width: "); 
          Serial.print(w); 
          Serial.print(", total: "); 
          Serial.println(screenpos + w); 
#endif
          if (screenpos + w + spacelength <= 115) {               // does it fit?
            screenpos = screenpos + w + spacelength;              // calculate new cursorposition
            startpos = temppos + 1;                               // skip next space
          } else {                                                
            eoline = true;                                        // this word doesn't fit anymore
            rowcount++;                                           // so to the next line
          }
        } else {                                                  // we found no space, so last word
          buffer[rowcount + 1] = 0;                               // this was the last line
          eotext = true;
          eoline = true;
#ifdef debug 
          Serial.println("Geen spatie (meer) gevonden "); 
          dumpBuffer();          
#endif
        }
      }
    }
    firstrun = false;
    setValue = 0;
  }

  if (encoder.up) {
    if (setValue > 0) { setValue--; }
  }
  if (encoder.down) {
    if (buffer[setValue + 1] > 0) { setValue++; }
  }
  if (encoder.sw) {
    options = OK;
    forcedisplay++;
  }

  oled.fillRect(0, 16, 128, 33, SSD1306_BLACK);
  oled.setCursor(0, 28);
  j = buffer[setValue + 1];
  if (j == 0) { j = txtlength; }
  for (i = buffer[setValue]; i < j; i++) {
    oled.print(helptext[i]);
  }
  oled.println();
  if (buffer[setValue + 1] > 0) {
    j = buffer[setValue + 2];
    if (j == 0) { j = txtlength; }
    for (i = buffer[setValue + 1]; i < j; i++) {
      oled.print(helptext[i]);
    }
  }
}

//------------------------------------------------------------------------------ ui servo
void displayServo() {
  enum optionStates temp;
  if (firstrun) {
    firstrun = false;
    encoder.sw = false;
    setValue = 90;
    valuestep = 0;
    menuoptions = OPTIONSOKSET;
    options = OPTIONOK;
    if (sysState == MENUSERVOACTIVE) {
      myservo.attach(GPIO1, 500, 2500);
      FTM0_MODE = FTM_MODE_WPDIS;                                      // enable write for FTM0_C7SC
      FTM0_C7SC &= ~FTM_CSC_MSA;                                       // because detatch doesnt work, reattach doesnt work either
      FTM0_C7SC |= FTM_CSC_MSB;
      FTM0_C7SC &= ~FTM_CSC_ELSA;                                                 
      FTM0_C7SC |= FTM_CSC_ELSB;
      myservo.attach(GPIO1, 500, 2500);                                
      myservo.write(90);
    }
    if (sysState == MENUSTEPPERACTIVE) {
      control.stepper_sleep = true;
      control.stepperRelay = true;
      writeToExpander(0x00, true);
      delay(1);                                                        // 1ms delay for A4988 chargepump
    }
  }

  temp = options;
  if (encoder.up) {
    if (options == OPTIONOK) { 
      temp = OPTIONUPDOWN; 
    } else {
      if (valuestep == 0) {
        temp = OPTIONOK;
      } else {
        if (sysState == MENUSERVOACTIVE) {
          if (setValue > 0) { setValue -= valuestep; }
          if (setValue > 200) { setValue = 0; }
        } else { pulseStepper(true); }
      }
    }
  }
  if (encoder.down) {
    if (options == OPTIONOK) { 
      temp = OPTIONUPDOWN; 
    } else {
      if (valuestep == 0) {
        temp = OPTIONOK;
      } else {
        if (sysState == MENUSERVOACTIVE) {
          if (setValue < 180) { setValue += valuestep; }
          if (setValue > 180) { setValue = 180; }
        } else { pulseStepper(false); }
      }
    }
  }

  if (encoder.sw) {
    if (options == OPTIONOK) {
      if (sysState == MENUSTEPPERACTIVE) {
        control.stepper_sleep = false;
        control.stepperRelay = false;
        writeToExpander(0x00, true);
        delay(1);                                                        // 1ms delay for A4988 chargepump
      }
      temp = OK;
      cursoron.blinkon = false;
      forcedisplay++;
      if (sysState == MENUSERVOACTIVE) {                                  // fix 4 broken library, detatch does not work
        FTM0_MODE = FTM_MODE_WPDIS;                                       // enable write for FTM0_C7SC
        FTM0_C7SC &= ~FTM_CSC_ELSA;                                       // GPIO1 = 5 = PTD7 = FTM0_CH7
        FTM0_C7SC &= ~FTM_CSC_ELSB;                                       // Pin unattached ELSnB:ELSnA = 0
      }
    }
    if (options == OPTIONUPDOWN) {
      switch(valuestep) {
        case 0:
          valuestep = 1;
          break;
        case 1:
          valuestep = 5;
          break;
        case 5:
          valuestep = 10;
          break;
        case 10:
          valuestep = 0;
          break;
      }
    }
  }

  if (sysState == MENUSERVOACTIVE) { myservo.write(setValue); }

  options = temp;
  sharedNavigation();

  if (valuestep == 1) {
    oled.fillRoundRect(0, 20, 12, 17, 3, SSD1306_WHITE);
    oled.setCursor(2, 32);
    oled.setTextColor(SSD1306_BLACK);
    oled.print("1");
    oled.setTextColor(SSD1306_WHITE);
  } else {
    oled.drawRoundRect(0, 20, 12, 17, 3, SSD1306_WHITE);
    oled.setCursor(2, 32);
    oled.print("1");
  }
  if (valuestep == 5) {
    oled.fillRoundRect(16, 20, 12, 17, 3, SSD1306_WHITE);
    oled.setCursor(18, 32);
    oled.setTextColor(SSD1306_BLACK);
    oled.print("5");
    oled.setTextColor(SSD1306_WHITE);
  } else {
    oled.drawRoundRect(16, 20, 12, 17, 3, SSD1306_WHITE);
    oled.setCursor(18, 32);
    oled.print("5");
  }
  if (valuestep == 10) {
    oled.fillRoundRect(32, 20, 22, 17, 3, SSD1306_WHITE);
    oled.setCursor(34, 32);
    oled.setTextColor(SSD1306_BLACK);
    oled.print("10");
    oled.setTextColor(SSD1306_WHITE);
  } else {
    oled.drawRoundRect(32, 20, 22, 17, 3, SSD1306_WHITE);
    oled.setCursor(34, 32);
    oled.print("10");
  }

  oled.setCursor(65, 32);
  if (sysState == MENUSERVOACTIVE) {
    oled.setCursor(65, 32);
    snprintf(myValue, 10, "pos: %+03d", (int16_t) (setValue - 90));
    oled.print(myValue);
  } else {
    oled.print("steps");
  }
}

//------------------------------------------------------------------------------ ui pulse generator
void pulseGenerator() {
  enum optionStates temp;
  uint8_t mycol;
  if (firstrun) {
    firstrun = false;
    enableGPIO();
    usHertz = false;
    encoder.sw = false;
    setValue = 10;
    maxValue = 1000000;
    maxDigits = 7;
    menuoptions = OPTIONSOKSET;
    options = OPTIONOK;
    FrequencyTimer2::setPeriod(setValue);
    FrequencyTimer2::enable();
  }

  temp = options;
  if (encoder.up) {     // CCW
    switch (options) {
      case OPTIONOK:
        temp = OPTIONVALUES;
        editValue = maxDigits - 1;
        cursoron.blinkon = true;
        break;
      case OPTIONUPDOWN:
        temp = OPTIONOK;
        break;
      case OPTIONMEASURE:
        temp = OPTIONUPDOWN;
        break;
      case OPTIONSETUPDOWN:
        if (setValue > 10) { setValue--; }
        break;
      case OPTIONVALUES:
        editValue--;
        if (editValue < 0) { 
          temp = OPTIONMEASURE;
          cursoron.blinkon = false;
        }
        break;
      case OPTIONSETVALUES:
        myValue[editValue]--;
        if (myValue[editValue] < 0x30) { myValue[editValue] = 0x39; }
        if ((editValue == 0) && (myValue[0] > 0x31)) { myValue[0] = 0x30; }
        setValue = atol(myValue);
        break;
      default:
        break;
    }
  }

  if (encoder.down) {     // CW
    switch (options) {
      case OPTIONOK:
        temp = OPTIONUPDOWN;
        break;
      case OPTIONUPDOWN:
        temp = OPTIONMEASURE;
        break;
      case OPTIONMEASURE:
        temp = OPTIONVALUES;
        editValue = 0;
        cursoron.blinkon = true;
        break;
      case OPTIONSETUPDOWN:
        if (setValue < 1000000) { setValue++; }
        break;
      case OPTIONVALUES:
        editValue++;
        if (editValue >= maxDigits - 1) { 
          temp = OPTIONOK; 
          cursoron.blinkon = false;
        }
        break;
      case OPTIONSETVALUES:
        myValue[editValue]++;
        if (myValue[editValue] > 0x39) { myValue[editValue] = 0x30; }
        if ((editValue == 0) && (myValue[0] > 0x31)) { myValue[0] = 0x31; }
        setValue = atol(myValue);
        break;
      default:
        break;
    }
  }

  if (encoder.sw) {
    switch (options) {
      case OPTIONOK:
        temp = OK;
        FrequencyTimer2::disable();
        forcedisplay++;
        break;
      case OPTIONMEASURE:
        usHertz = !usHertz;
        break;
      case OPTIONUPDOWN:
        temp = OPTIONSETUPDOWN;
        cursoron.blinkon = true;
        break;
      case OPTIONSETUPDOWN:
        temp = OPTIONUPDOWN;
        cursoron.blinkon = false;
        break;
      case OPTIONVALUES:
        temp = OPTIONSETVALUES;
        break;
      case OPTIONSETVALUES:
        temp = OPTIONVALUES;
        break;
      default:
        break;
    }
  }
  options = temp;
  if (setValue < 10) { setValue = 10; }

  sharedNavigation();
  if (options == OPTIONMEASURE) {
    oled.fillRoundRect(51, 49, 22, 15, 3, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
    if (usHertz) {
      oled.setCursor(55, 59);
      oled.print("us");
    } else {
      oled.setCursor(54, 60);
      oled.print("Hz");
    }
    oled.setTextColor(SSD1306_WHITE);    
  } else {
    oled.drawRoundRect(51, 49, 22, 15, 3, SSD1306_WHITE);
    if (usHertz) {
      oled.setCursor(55, 59);
      oled.print("us");
    } else {
      oled.setCursor(54, 60);
      oled.print("Hz");
    }
  }
  FrequencyTimer2::setPeriod(setValue);
  oled.setCursor(0, 42);
  if (!usHertz) {
    snprintf(myValue, 22, " act: %07lu us", FrequencyTimer2::getPeriod());
  } else {
    snprintf(myValue, 22, " act: %07lu Hz", 1000000 / FrequencyTimer2::getPeriod());
  }
  oled.println(myValue);
  oled.setCursor(0, 28);
  oled.print(" set: ");
  mycol = oled.getCursorX();
  snprintf(myValue, 22, "%07lu", setValue);
  for (uint8_t i = 0; i < maxDigits; i++) {
    if (i == editValue) {
      if (options == OPTIONSETVALUES) { nowcursor(myValue[i], true); }
      if (options == OPTIONVALUES) { nowcursor(myValue[i]); Serial.println(i); }
    }   
    oled.print(myValue[i]);
    mycol += 8;
    oled.setCursor(mycol, 28);
  }
  oled.print(" us");
}

//------------------------------------------------------------------------------ ui ISPAVR
void displayISPAVRProgrammer() {
  if (firstrun) {
    firstrun = false;
    encoder.sw = false;
    progmode = 0x00;
    error = 0x00;
  }

  if (encoder.lp) {
    sysState = MENUPROGISPAVRONOFF;
    disableSlaveModes();
    encoder.lp = false;
    forcedisplay++;
  }

  sharedNavigation();

  oled.fillRect(0, 18, 120, 16, SSD1306_BLACK);
  oled.setCursor(0, 28);
  oled.println("Programmingmode");
  oled.setCursor(0, 42);
  if (error) {
    oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
    oled.print("error occured");
  } else {
    switch(progmode) {
      case '0':
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
        oled.print("Sign on");
        break;
      case '1':
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
        oled.print("Programmer");
        break;
      case 'A':
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
        oled.print("Get version");
        break;
      case 'B':
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
        oled.print("Set parameters");
        break;
      case 'P':
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
        oled.print("Program");
        break;
      case 0x60:
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
        oled.print("Prog. Flash ");
        for (uint8_t i = 0; i < progblock; i++) { oled.print("."); }
        break;
      case 'a':
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
        oled.print("Prog. Data ");
        for (uint8_t i = 0; i < progblock; i++) { oled.print("."); }
        break;
      case 'd':
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
        oled.print("Prog. page ");
        for (uint8_t i = 0; i < progblock; i++) { oled.print("."); }
        break;
      case 't':
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
        oled.print("Read page ");
        for (uint8_t i = 0; i < progblock; i++) { oled.print("."); }
        break;
      case 'Q':
        oled.fillRect(0, 34, 128, 15, SSD1306_BLACK);
      default:
        break;
    }
  }
}

//------------------------------------------------------------------------------ ui ESP Programmer
void displayESPProgrammer() {
  if (firstrun) {
    firstrun = false;
    encoder.sw = false;
    stopProg = false;
    progmode = 0x00;
    error = 0x00;
  }

  if (encoder.lp) {
    sysState = MENUPROGESPONOFF;
    disableSlaveModes();
    stopProg = true;
    encoder.lp = false;
    forcedisplay++;
  }

  sharedNavigation();
}

//------------------------------------------------------------------------------ ui Voltmeter
void displayVoltmeter() {
  int measurevolt;
  if (encoder.sw) {
    options = OK;
    encoder.sw = false;
    forcedisplay++;
  }
  oled.fillRect(0, 16, 128, 33, SSD1306_BLACK);
  sharedNavigation();
  oled.setCursor(5, 32);
  measurevolt = analogRead(VOLTSENSE);
  snprintf(myValue, 22, "%2.1f", calculateVoltage(measurevolt));
  oled.print(myValue);
  oled.print(" Volt");
}

//------------------------------------------------------------------------------ 7 segment
void display7Segment() {
  uint8_t pos_x = 80;
  uint8_t pos_y = 22;
  if (bytePins & 0b00000001) { oled.fillRoundRect(2 + pos_x, 0 + pos_y, 9, 3, 2, SSD1306_WHITE); }         // SEG a - top
  if (bytePins & 0b00000010) { oled.fillRoundRect(10 + pos_x, 2 + pos_y, 3, 9, 2, SSD1306_WHITE); }        // SEG b - upper right
  if (bytePins & 0b00000100) { oled.fillRoundRect(10 + pos_x, 12 + pos_y, 3, 9, 2, SSD1306_WHITE); }       // SEG c - lower right
  if (bytePins & 0b00001000) { oled.fillRoundRect(2 + pos_x, 20 + pos_y, 9, 3, 2, SSD1306_WHITE); }        // SEG d - bottom
  if (bytePins & 0b00010000) { oled.fillRoundRect(0 + pos_x, 12 + pos_y, 3, 9, 2, SSD1306_WHITE); }        // SEG e - lower left
  if (bytePins & 0b00100000) { oled.fillRoundRect(0 + pos_x, 2 + pos_y, 3, 9, 2, SSD1306_WHITE); }         // SEG f - upper left
  if (bytePins & 0b01000000) { oled.fillRoundRect(2 + pos_x, 10 + pos_y, 9, 3, 2, SSD1306_WHITE); }        // SEG g - middle
}

//------------------------------------------------------------------------------ ui logic blocks
void displayLogic() {
  enum optionStates temp;
  temp = options;
  if (firstrun) {
    disableGPIO();
    firstrun = false;
    encoder.sw = false;
    options = OPTIONOK;
    menuoptions = OPTIONSOKSET;
    writeToExpander(0xFF);                                  // set as inputs
    readFromExpander();                                     // read inputs
  }

  if (encoder.up) { 
    switch (options) {
      case OPTIONOK:
        temp = OPTIONVALUES;
        setValue = 1;
        break;
      case OPTIONVALUES:
        if (setValue > 0) {
          setValue--;
        } else {
          temp = OPTIONOK;
        }
        break;
      case OPTIONSETVALUES:
        if (setValue == 0) {
          if ((logicSet & 0x0F) > 0) { logicSet--; }
        } else {
          if ((logicSet & 0xF0) > 0) { logicSet -= 0x10; }
        }
        break;
      default:
        break;
    }
  }

  if (encoder.down) { 
      switch (options) {
      case OPTIONOK:
        temp = OPTIONVALUES;
        setValue = 0;
        break;
      case OPTIONVALUES:
        if ((setValue < 1) && ((logicSet & 0x0F) < JKFF)) {
          setValue++;
        } else {
          temp = OPTIONOK;
        }
        break;
      case OPTIONSETVALUES:
        if (setValue == 0) {
          if ((logicSet & 0x0F) < JKFF) { logicSet++; }
        } else {
          if (((logicSet & 0xF0) >> 4) < SRFF) { logicSet += 0x10; }
        }
        Serial.println(logicSet, HEX);
        break;
      default:
        break;
    }  
  }

  if (encoder.sw) {                                           // clicked
    switch (temp) {
      case OPTIONOK:
        temp = OK;
        cursoron.blinkon = false;
        forcedisplay++;
        break;
      case OPTIONVALUES:
        temp = OPTIONSETVALUES;
        cursoron.blinkon = true;
        break;
      case OPTIONSETVALUES:
        cursoron.blinkon = false;
        temp = OPTIONVALUES;
        break;
      default:
        break;
    }
  }
  options = temp;

  sharedNavigation();
  processLogic();
  if (options == OPTIONVALUES) {
    oled.fillCircle((setValue * 64) + 36, 30, 4, SSD1306_WHITE);
  }
}

void displayScreenSet() {
  if (firstrun) {
    firstrun = false;
    encoder.sw = false;
    oled.print("klik ok to exit");
  }

  if (encoder.sw) {
    options = OK;
    encoder.sw = false;
    forcedisplay++;
  }

  //oled.fillRect(0, 16, 128, 33, SSD1306_BLACK);
  sharedNavigation();
}

void displayI2CMonitor() {
uint16_t copyPointer;
float tempCalc;
/*
  Statechanges or Timed - frequency
  Trigger @ start
*/
  if (firstrun) {
    enableGPIO();
    detachMainInterrupts();
    oled.fillRect(0, 16, 128, 33, SSD1306_BLACK);
    sharedNavigation();
    oled.setCursor(5, 32);
    oled.print("Capturing...");
    firstrun = false;
    encoder.sw = false;
    options = OPTIONOK;
    menuoptions = OPTIONSOKSET;
    // Setup capture vars, timers & interrupts
    CapturePointer = 0;
    pinMode(GPIO1, INPUT);
    pinMode(GPIO2, INPUT);
    pinMode(I2CTRGR, INPUT);
    PITimer0.period(60.0);
    PITimer1.period(255.0); 
    PITimer1.start(maxtimeCaptureInterrupt);
    bitSet(PIT_TCTRL1, 2);                                          // Chainmode enabled
    //bitClear(PIT_TCTRL1, 1);                                      // Disable interrupts
    PITimer0.start(maxtimeCaptureInterrupt);
    bitClear(PIT_TCTRL0, 1);                                        // Disable interrupts
    attachInterrupt(GPIO2, pinGPIO2Interrupt, RISING);              // SCL
    attachInterrupt(I2CTRGR, pinI2CTRGRInterrupt, FALLING);         // SDA changes with high SCL
  }

  if (encoder.sw) {
    detachInterrupt(I2CTRGR);
    detachInterrupt(GPIO2);
    attachMainInterrupts();
    PITimer0.stop();
    PITimer1.stop();
    options = OK;
    encoder.sw = false;
    forcedisplay++;
  }

  copyPointer = CapturePointer;
  oled.fillRect(0, 16, 128, 33, SSD1306_BLACK);
  oled.setCursor(8, 28);
  if (copyPointer < MAXBUFSIZE) {
    oled.print("Buffer use: ");
    oled.drawRect(8, 33, 112, 10, SSD1306_WHITE);
    tempCalc = (float) copyPointer / MAXBUFSIZE * 112;
    oled.fillRect(8, 33, (int16_t) tempCalc, 10, SSD1306_WHITE);
  } else {
    oled.print("Buffer full!");
    oled.fillRect(8, 33, 112, 10, SSD1306_WHITE);
  }
}

void displayExport() {
  enum optionStates temp;
  temp = options;

  if (firstrun) {
    firstrun = false;
    encoder.sw = false;
    options = OPTIONOK;
    menuoptions = OPTIONSOKSETCANCEL;
    setValue = 0;
    cursoron.blinkrow = 28;
  }

  if (encoder.up) {
    switch (temp) {
      case OPTIONOK:
        temp = OPTIONCANCEL;
        break;
      case OPTIONUPDOWN:
        temp = OPTIONOK;
        break;
      case OPTIONSETVALUES:
        exportoption--;
        exportoption &= 0x07;
        break;
      case OPTIONCANCEL:
        temp = OPTIONUPDOWN;
        break;
      default:
        break;
    }
  }

  if (encoder.down) {
    switch (temp) {
      case OPTIONOK:
        temp = OPTIONUPDOWN;
        break;
      case OPTIONUPDOWN:
        temp = OPTIONCANCEL;
        break;
      case OPTIONSETVALUES:
        exportoption++;
        exportoption &= 0x07;
        break;
      case OPTIONCANCEL:
        temp = OPTIONOK;
        break;
      default:
        break;
    }
  }

  if (encoder.sw) {
    switch (temp) {
      case OPTIONOK:
        SPI_Out.clear();
        if ((exportoption & 0x04) == 0x04) {
          enableUSBKeyboard();
          //allSPIisData = true;
        } else {
          enableUSB2Serial(modeSPI_USB);
          allSPIisData = true;
        }
        switch (exportoption & 0x03) {
          case S_RAW_WO_TIME:
            rawI2CExport(false);
            break;
          case S_RAW_W_TIME:
            rawI2CExport(true);
            break;
          case S_HRF:
            I2CprotocolExport();
            break;
          case S_SALEAE:
            saleaeProtocolExport();
            break;
        }
        break;
      case OPTIONUPDOWN:
        temp = OPTIONSETVALUES;
        cursoron.blinkon = true;
        break;
      case OPTIONSETVALUES:
        cursoron.blinkon = false;
        temp = OPTIONUPDOWN;
        break;
      case OPTIONCANCEL:
        disableSlaveModes();
        encoder.sw = false;
        temp = CANCEL;
        forcedisplay++;
        break;
      default:
        break;
    }
  }
  options = temp;

  sharedNavigation();
  oled.fillRect(0, 16, 128, 33, SSD1306_BLACK);
  oled.setCursor(0, 28);
  switch(exportoption) {
    case S_RAW_WO_TIME:
      oled.println("To serial");
      oled.print("RAW w/o timing");
      break;
    case S_RAW_W_TIME:
      oled.println("To serial");
      oled.print("RAW with timing");
      break;
    case S_HRF:
      oled.println("To serial");
      oled.print("Human readable");
      break;
    case S_SALEAE:
      oled.println("To serial");
      oled.print("Saleae protocol");
      break;
    case K_RAW_WO_TIME:
      oled.println("As keyboard");
      oled.print("RAW w/o timing");
      break;
    case K_RAW_W_TIME:
      oled.println("As keyboard");
      oled.print("RAW with timing");
      break;
    case K_HRF:
      oled.println("As keyboard");
      oled.print("Human readable");
      break;
    case K_SALEAE:
      oled.println("As keyboard");
      oled.print("Saleae protocol");
      break;
  } 
}

