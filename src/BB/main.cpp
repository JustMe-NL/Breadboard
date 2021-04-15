//------------------------------------------------------------------------------ includes
#include <breadboard.h>

// classes
// enum & structs
// globals
// Interrupts
// void setDigitInValue()
// void changeMyValue()
// void getUserValue()
// void setUserValue()
// void setup()
// void loop()

//------------------------------------------------------------------------------ objects
Adafruit_SSD1306 oled = Adafruit_SSD1306(128, 64, &Wire);                       // Oled display
Adafruit_INA219 ina219;                           // current & volt meter       // Power & current measurment
Encoder myEnc(ENCA, ENCB, GPIO1, GPIO2);                                        // Modified encoder library using interrupts 
Switch multiresponseButton = Switch(ENCSW, INPUT, LOW, 50, 3000);               // Switch debouncing with longpress
SoftWire i2c(GPIO1, GPIO2);                                                     // Software I2C library
OneWire *ds;                                                                    // Dallas OneWire library 
FreqMeasureMulti freq1;                                                         // Low frequency measurment
PWMServo myservo;                                                               // PWM generato for Servo's
RingBuf<uint8_t, 256> SPI_In;                                                   // Ringbuffer for SPI input
RingBuf<uint8_t, 256> SPI_Out;                                                  // Ringbuffer for SPI output
RingBuf<capture_struct, 4096> CaptureBuf;                                       // Ringbuffer for measurments

//------------------------------------------------------------------------------ enums & structs

enum optionStates options;                        // Menu options states in userinput routine
enum valueTypes valueType;                        // Values types for input (mA, mV, Hz enz)
enum inputTypes inputType;                        // Input types (number, list, binary)
enum systemStates sysState;                       // System state & menu structure
enum menuOptions menuoptions;                     // UI Navigation options
cursorstruct_t cursoron;                          // Cursor related variables
Encoder_t encoder;                                // Encoder variables

//------------------------------------------------------------------------------ global vars
float sum1;
float curVolt = 0;                                // current measured voltage
float curAmp = 0;                                 // current measured milliampere
long oldPosition = 0;                             // encoder tracking
unsigned long last_interrupt_time = 0;            // software debounce
unsigned long last_lp_interrupt_time = 0;         // long press
unsigned long last_time = 0;                      // cursor timing
unsigned long maxValue;                           // maximum value for input
unsigned long setValue;                           // current input value
int count1;                                       // countvar for frequencymeasurment
int error = 0;                                    // avrisp
int pmode = 0;                                    // avrisp
unsigned int here;                                // address for reading and writing, set by 'U' command
uint16_t setAmp = 0;                              // req amperage
uint16_t setVolt = 0;                             // req voltage
uint16_t setAmpCO = 0;                            // set milliampere cut off
uint16_t buffer[256];                             // buffer for I2C adresses & oneWire
uint16_t buff[256];                               // global block storage
char myValue[22];                                 // buffer for current setting
int8_t editValue;                                 // current digit to be edited
int8_t hbdelta = 8;                               // avrisp
uint8_t forcedisplay;                             // force a display refresh
uint8_t valuestep;                                // step of value (eg. 1, 20, 50)
uint8_t maxDigits;                                // maximum no of digits (in maxValue)
uint8_t bytePins;                                 // byte data of pins read
uint8_t writeByte;                                // holder for bit operations on expander
uint8_t i2cpointer;                               // where are we in showing addresses
uint8_t faillures;                                // how many test failed
uint8_t hbval = 128;                              // avrisp
uint8_t lasthbval;                                // heartbeat avrisp
uint8_t progmode;                                 // boolean for avrisp mode
uint8_t progblock;                                // counter for progressdisplay
uint8_t baudRate;                                 // Selected baudrate (default max)
uint8_t logicSet;                                 // set logic blocks if any
bool measure = false;                             // flag to start measurement
bool cutOff = false;                              // cutoff enabled
bool encOn = false;                               // Encoder passthrough enabled
bool pinmonOn = false;                            // Pin monitor interrupt enabled
bool freqLowOn = false;                           // flag for low frequency measurement 
bool freqHighOn = false;                          // flag for high frequency measurement
bool firstrun;                                    // flag to indicate display routines this is the 1st time they run
bool countme;                                     // Interrupt accrued
bool stopProg;                                    // flag to abort avrisp mode  
bool usHertz;                                     // flag for display
bool curDTR;                                      // last DTR status
bool curRTS;                                      // last RTS status
bool readTrigger = false;                         // Trigger to read pins
bool lastTrigger = false;                         // last interrupt status
bool readSet = false;                             // expander in read mode
bool rst_active_high;                             // avrisp
bool checklist[12];                               // what wend wrong with this clone
bool newSPIdata_avail;                            // We recieved new SPI data
bool slave_exit_mode;                             // Data in SPI_Out is irrelevant & flushed if thios flag is set, slave ends current mode 
bool slowSlave;                                   // Slave is running @ 8MHz
bool owPullUpActive;                              // PullUp for OneWire is active
bool i2cPullUpActive;                             // PullUp for I2C is active
bool lastclock;                                   // previous clocksignal
control_struct control;                           // status control expander
elapsedMillis timeout;                            // timeout for requencumeasurements
parameter_struct param;                           // parameters for avrisp

// Counter 100%
// Encoder 100%
// I2C Scanner
//    monitor, capture, export
// Logic Blocks
//    inverter, fix signal display
// OneWire 100%
// Pin monitor
//    monitor, capture, export, 7-segments
// Power
//    set voltage, current
//    Cut off
// Programmer 100%
// Pulse generator 100%
// Screen settings
// Serial
//    monitor
// Servo 100%
// Set pins 100%
// Stepper 100%
// Sw debounce 100%
// Voltmeter 100%

void expanderInterrupt() {
  readTrigger = true;
}

void counterInterrupt() {
  setValue++;
  countme = true;
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

void houseKeeping() {
  unsigned long now_time = millis();                      // check cursor & display 
  if (now_time - last_time > 200) {
    last_time = now_time;
    measure = !measure;
    if (sysState == MENUVOLTMETERACTIVE) { displayVoltmeter(); }

    if (freqLowOn && timeout > 500) { showFreqLowCount(); }
    
    processCursor();
    
    if (measure) {                                                 // Is it time for a current/voktage measurment?
      curAmp = ina219.getCurrent_mA();
      curVolt = ina219.getBusVoltage_V();
      if (curAmp < 0) { curAmp = 0; }
      dispHeader();
    }
  } 

  if (!digitalRead(PDINT)) {                                      // Does the SPI slave want to tell me something?
    uint8_t spidata;
    bool transdone;
    if (!SPI_In.isFull()) {
      transdone = false;
      SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
      digitalWrite(PDSLAVE, LOW);
      SPI.transfer(READACKNOWLEDGED);
      while (!SPI_In.isFull() && !transdone) {
        delay(2);
        spidata = SPI.transfer(0x00);
        SPI_In.push(spidata);
        if (digitalRead(PDINT) == HIGH) {
          transdone = true;
        }
      }
      digitalWrite(PDSLAVE, HIGH);
      SPI.endTransaction();
      newSPIdata_avail = true;
    }
  }

  if (!SPI_Out.isEmpty()) {                                     // Do we have something to tell the SPI slave?
    uint8_t spidata;
    digitalWrite(PDSLAVE, LOW);
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    if (slave_exit_mode) {
      SPI.transfer(EXITCOMMAND);
      slave_exit_mode = false;
      SPI_Out.clear();
    } else {
      SPI.transfer(DATAFOLLOWS);
    }
    while (!SPI_Out.isEmpty())
    {
      delay(2);
      SPI_Out.pop(spidata);
      SPI.transfer(spidata);
    }
    SPI.endTransaction();
    digitalWrite(PDSLAVE, HIGH);
  }

  long newPosition = myEnc.read() >> 2;                           // Read encoder
  if (newPosition != oldPosition) {
    if (oldPosition > newPosition) { encoder.up = true; }
    if (oldPosition < newPosition) { encoder.down = true; }
    oldPosition = newPosition;
  }

  multiresponseButton.poll();                                     // Poll switches
  if (multiresponseButton.singleClick()) { encoder.sw = true; }   // short press encoder
  if (encOn) {
    if (multiresponseButton.pushed()) { writeToExpander(0x10); }  // emulate enc sw on pin 5
    if (multiresponseButton.released()) { writeToExpander(0x0); } 
  }

  if (!stopProg) { displayISPAVRProgrammer(); }
}

//------------------------------------------------------------------------------ setup
void setup() {
  Serial.begin(115200);                                  // Setup serial
  Wire.begin();                                          // Setup I2C @ 400kHz
  Wire.setClock(400000L);
  disableGPIO();
  oled.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS);         // Initialize OLED
  oled.clearDisplay();
  oled.setFont(&verdana6pt7b);
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextWrap(false);
  ina219.begin();

  pinMode(ENCSW, INPUT_PULLUP);
  pinMode(EXPINT, INPUT_PULLUP);
  pinMode(COUNTR, INPUT_PULLUP);
  pinMode(PDINT, INPUT_PULLUP);
  pinMode(PDDTR, INPUT_PULLUP);
  pinMode(PDRTS, INPUT_PULLUP);
  pinMode(PDSLAVE, OUTPUT);
  digitalWrite(PDSLAVE, HIGH);
  digitalWrite(COUNTPINEN, LOW);
  i2cPullUp(false);
  oneWirePullUp(false);
  analogReadAveraging(100);
  
  control.all = 0x00;
  writeToExpander(0x00, true);
  sysState = MENUCOUNTER;                                // Initialize default states
  firstrun = true;
  stopProg = true;
  menuoptions = OPTIONSOK;
  options = OPTIONOK;
  forcedisplay = 1;                                      // Preload display
  cursoron.blinkon = false;
  encoder.up = false;
  encoder.down = false;
  encoder.sw = false;
  encoder.swold = true;                                  // if pull-up
  sharedNavigation();
  
  baudRate = (sizeof(baudRates)/sizeof(baudRates[0])) - 1; // Default baudrate = max
  attachInterrupt(digitalPinToInterrupt(EXPINT), expanderInterrupt, FALLING);
  
  SPI.setMOSI(myMOSI);
  SPI.setMISO(myMISO);
  SPI.setSCK(mySCLK);
  SPISettings PDmicro(2000000, MSBFIRST, SPI_MODE0);
  slave_exit_mode = false;
  newSPIdata_avail = false;
  SPI.begin();

  owPullUpActive = false;
  i2cPullUpActive = false;
  logicSet = 0x37;
}

//------------------------------------------------------------------------------ loop
void loop() {
  bool tmpDTR, tmpRTS;
  capture_struct capturedata;
  houseKeeping();

  if (sysState == MENUPROGESPACTIVE) {
    tmpDTR = digitalRead(PDDTR);                                  // 'Mirror' RTS/DTR to MP3/4 
    tmpRTS = digitalRead(PDRTS);                                  // DTR = GPIO0 unless RTS is also low
    if (tmpDTR != curDTR || tmpRTS != curRTS) {                   // RTS = RST unless DTR is also low
      curDTR = tmpDTR;
      curRTS = tmpRTS;
      if (curDTR == curRTS) {
        writeByte = 0x0C;
      } else {
        bytePins = 0x00;
        if (curRTS) { writeByte += 0x04; }
        if (curDTR) { writeByte += 0x08; }
      }
      writeToExpander(writeByte);
    }
  }

  if (multiresponseButton.longPress()) { encoder.lp = true; }     // long press encoder
  if (encoder.lp && encOn) {                                      // end encoder mode
    encOn = false;
    encoder.lp = false;
    myEnc.setCopy(false);
    forcedisplay++;
    sysState = MENUENCODERON;
  }

  if (readTrigger) {                                              // read expnder in pin momitor mode
    readTrigger = false;
    if (sysState == MENUPINMONITORACTIVE) { 
      readFromExpander();
      if (!CaptureBuf.isFull()) {
        capturedata.time = millis();
        capturedata.value = bytePins;
        CaptureBuf.push(capturedata);
      }
      pinMonitor();
    }
    if (sysState == MENULOGICACTIVE) {
      readFromExpander();
      processLogic();
    }
  }

  if (freqLowOn) {                                                // if counter mode, count
    if (freq1.available()) {
      sum1 = sum1 + freq1.read();
      count1 = count1 + 1;
    }
  }
  if (freqHighOn && FreqCount.available()) { forcedisplay++; }

  if (sysState == MENUCOUNTCOUNTERACTIVE) { 
    if (countme) { showCounter(); }
    if (encoder.lp) {
      detachInterrupt(digitalPinToInterrupt(GPIO1));
      options = OK;
      sysState = MENUCOUNTCOUNTER;
      cursoron.blinkon = false;
      countme = false;
      forcedisplay++;
    }
  }

  if (!stopProg) { avrisp_loop(); }

  processMenu();                                                  // Process menu
}