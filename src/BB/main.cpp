//------------------------------------------------------------------------------ includes
#include "breadboard.h"

// classes
// enum & structs
// globals
// void expanderInterrupt()
// void counterInterrupt()
// void slaveTimerInterrupt()
// void pdInterrupt()
// void pinGPIO2Interrupt()
// void pinI2CTRGRInterrupt()
// void maxtimeCaptureInterrupt()
// void houseKeeping()
// void setup()
// void loop()

//------------------------------------------------------------------------------ objects
Adafruit_SSD1306 oled(128, 64, &Wire);                                          // Oled display
Adafruit_INA219 ina219;                                                         // Power & current measurment
Encoder myEnc(ENCA, ENCB, GPIO1, GPIO2);                                        // Modified encoder library using interrupts 
Switch multiresponseButton = Switch(ENCSW, INPUT, LOW, 50, 3000);               // Switch debouncing with longpress
SoftWire i2c(GPIO1, GPIO2);                                                     // Software I2C library
OneWire *ds;                                                                    // Dallas OneWire library 
FreqMeasureMulti freq1;                                                         // Low frequency measurment
PWMServo myservo;                                                               // PWM generato for Servo's
RingBuf<uint8_t, 256> SPI_In;                                                   // Ringbuffer for SPI input
RingBuf<uint8_t, 256> SPI_Out;                                                  // Ringbuffer for SPI output
elapsedMillis timeoutTimer;                                                     // standard timeout Timer

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
unsigned long setScreenTimeOut = 60500;           // Timeout set to 5 min.
unsigned long last_interrupt_time = 0;            // software debounce
unsigned long last_lp_interrupt_time = 0;         // long press
unsigned long last_time = 0;                      // cursor timing
unsigned long lastpdInt = 0;                      // last time a PD_INT change was detected
unsigned long maxValue;                           // maximum value for input
unsigned long setValue;                           // current input value
uint32_t CaptureSecBuffer[MAXBUFSIZE];
int count1;                                       // countvar for frequencymeasurment
int error = 0;                                    // avrisp
int pmode = 0;                                    // avrisp
unsigned int here;                                // address for reading and writing, set by 'U' command
uint16_t setAmp = 0;                              // req amperage
uint16_t setVolt = 0;                             // req voltage
uint16_t setAmpCO = 0;                            // set milliampere cut off
uint16_t buffer[256];                             // buffer for I2C adresses & oneWire
uint16_t buff[256];                               // global block storage
volatile uint16_t CapturePointer;
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
uint8_t screenState;                              // screen state for dimming & blanking
uint8_t exportoption;
uint8_t CaptureDataBuffer[MAXBUFSIZE];
uint8_t CaptureMinBuffer[MAXBUFSIZE];
bool measure = false;                             // flag to start measurement
bool cutOff = false;                              // cutoff enabled
bool firstrun;                                    // flag to indicate display routines this is the 1st time they run
bool countme;                                     // Interrupt accrured
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
bool slave_exit_mode;                             // Data in SPI_Out is irrelevant & flushed if this flag is set, slave ends current mode 
bool slowSlave;                                   // Slave is running @ 8MHz
bool PullUpActive;                                // PullUp is active
bool lastclock;                                   // previous clocksignal for logicblocks
bool screenoff;                                   // 
bool allSPIisData;                                // When enabled all SPI tranasction are preceded with the data command
bool dmaStopped;
volatile bool slaveTimerRunning;                  // timeout timer for PDINT is running
volatile bool slaveAckReceived;                   // Acknowledge recieved from slave (pulse from PDINT)
volatile bool slaveReadRequest;                   // PDINT is a stable low

control_struct control;                           // status control expander
parameter_struct param;                           // parameters for avrisp
Encoder_internal_state_t * Encoder::interruptArgs[];

// Counter 100%
// Encoder 100%
// I2C Scanner 100%
// Logic Blocks 100%
// OneWire 100%
// Pin monitor
//    capture, export, 7-segments (broken)
// Power
//    set voltage, current
//    Cut off
// Programmer
//    (Re)check, ESP
// Pulse generator 100%
// Screen settings
//    timeout?
// Serial
//    monitor
// Servo 100%
// Set pins 100%
// Stepper 100%
// Sw debounce 100%
// Voltmeter 100%

//------------------------------------------------------------------------------ I/O expander needs to tell us something
void expanderInterrupt() {
  readTrigger = true;
}

//------------------------------------------------------------------------------ FTM0 Counter trigger
void counterInterrupt() {
  setValue++;
  countme = true;
}

//------------------------------------------------------------------------------ Slave timer timeout
void slaveTimerInterrupt() {
  PITimer2.stop();
  if (slaveTimerRunning) { slaveAckReceived = true; }
  slaveTimerRunning = false;
  slaveReadRequest = !digitalReadFast(PDINT);
}

//------------------------------------------------------------------------------ Slave signalling
void pdInterrupt() {
  if (slaveTimerRunning) { slaveAckReceived = true; }
  PITimer2.period(PDINT_TIME);
  PITimer2.start(slaveTimerInterrupt);
  slaveTimerRunning = true;
}

//------------------------------------------------------------------------------ I2C SCL trigger
void pinGPIO2Interrupt() {
  if (CapturePointer < MAXBUFSIZE) {
    CaptureDataBuffer[CapturePointer] = GPIOD_PDIR & 0xFE;
    CaptureSecBuffer[CapturePointer] = PIT_CVAL0;
    CaptureMinBuffer[CapturePointer] = (uint8_t) PIT_CVAL1;
    CapturePointer++;
  }
}
  
//------------------------------------------------------------------------------ I2C start/stop trigger
void pinI2CTRGRInterrupt() {
  if (CapturePointer < MAXBUFSIZE) {
    CaptureDataBuffer[CapturePointer] = GPIOD_PDIR | 0x01;
    CaptureSecBuffer[CapturePointer] = PIT_CVAL0;
    CaptureMinBuffer[CapturePointer] = (uint8_t) PIT_CVAL1;
    CapturePointer++;
  }
}

//------------------------------------------------------------------------------ maximum capture time exceeded
void maxtimeCaptureInterrupt() {
  // Maximum of 255 minutes capture time exceeded.
  encoder.sw = true;
  forcedisplay++;
} 

//------------------------------------------------------------------------------ process communications
void houseKeeping() {
  unsigned long now_time = millis();                               // check cursor & display 
  if (now_time - last_time > 200) {
    last_time = now_time;
    measure = !measure;
    processCursor();
    if (measure) {                                                 // Is it time for a current/voktage measurment?
      curAmp = ina219.getCurrent_mA();
      curVolt = ina219.getBusVoltage_V();
      if (curAmp < 0) { curAmp = 0; }
      dispHeader();
      switch (sysState) {                                            // Slow refresh
        case MENUVOLTMETERACTIVE:
        case MENUI2CSCANMONITORACTIVE:
        case MENUPROGISPAVRACTIVE:
          forcedisplay++;
          break;
        default:
          break;
      }
    }
  }

  if (slaveReadRequest) {                                          // Does the SPI slave want to tell me something?
    uint8_t spidata;
    bool transdone;
    if (!SPI_In.isFull()) {                                        // Check buffer status
      transdone = false;
      SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
      slaveTimerRunning = false;
      slaveAckReceived = false;                                    // init vars
      digitalWrite(PDSLAVE, LOW);                                  // start transaction
      SPI.transfer(cmdENQ_ReadAck);                                // Confirm send request from slave
      timeoutTimer = 0;
      while(!slaveAckReceived && timeoutTimer < SPITIMEOUT);       // Wait for confirmation from slave for next byte
      slaveAckReceived = false;
      while (!SPI_In.isFull() && !transdone) {                     // Read bytes until buffer full or transfer is done
        spidata = SPI.transfer(0x00);                              // Send 0x00, recieve data from slave
        SPI_In.push(spidata);                                      // Add to ringbuffer
        timeoutTimer = 0;
        while(!slaveAckReceived && timeoutTimer < SPITIMEOUT);     // Wait for confirmation from slave for next byte
        slaveAckReceived = false;
        if (!slaveReadRequest) {                                   // If slave has no more data, it will stop sending Acks, the timer
          transdone = true;                                        //   will runout and we get an slaveAckReceived & slaveReadReq 
        }                                                          //   will be false
      }
      if (slaveTimerRunning) { 
        PITimer2.stop();
        slaveTimerRunning = false;
      } 
      digitalWrite(PDSLAVE, HIGH);
      SPI.endTransaction();
      newSPIdata_avail = true;
    }
  }

  if (!SPI_Out.isEmpty()) {                                        // Do we have something to tell the SPI slave?
    uint8_t spidata;
    digitalWrite(PDSLAVE, LOW);
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    slaveAckReceived = false;
    if (allSPIisData) {                                            // In this mode all outgoing transactions are automatically
        spidata = SPI.transfer(cmdSTX_DataNext);                   //     preceded by the cmdSTX_DataNext command to the slave
        timeoutTimer = 0;
        while(!slaveAckReceived && timeoutTimer < SPITIMEOUT);     // Wait for confirmation from slave for next byte
        slaveAckReceived = false;
    }
    while (!SPI_Out.isEmpty())                                     // Keep sending bytes until the buffer is empty
    {
      SPI_Out.pop(spidata);
      spidata = SPI.transfer(spidata);
      timeoutTimer = 0;
      while(!slaveAckReceived && timeoutTimer < SPITIMEOUT);       // Wait for confirmation from slave for next byte
      slaveAckReceived = false;
    }
    SPI.endTransaction();
    digitalWrite(PDSLAVE, HIGH);
    delayMicroseconds(50);                                         // Give slave some time to process the rise of the SS signal
  }

  long newPosition = myEnc.read() >> 2;                            // Read encoder
  if (newPosition != oldPosition) {
    checkScreen();
    if (oldPosition > newPosition) { encoder.up = true; }
    if (oldPosition < newPosition) { encoder.down = true; }
    oldPosition = newPosition;
  }

  multiresponseButton.poll();                                     // Poll switches
  if (multiresponseButton.singleClick()) { 
    checkScreen();
    encoder.sw = true; 
  }
}

//------------------------------------------------------------------------------ setup
void setup() {
  Wire.begin();                                                   // Setup I2C @ 400kHz
  Wire.setClock(400000L);
  control.all = 0x00;
  control.powerRelay = true;
  writeToExpander(0x00, true);
  Serial.begin(500000);                                           // Setup serial
  screenState = 0;
  timeoutTimer = 0;
  oled.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS);
  oled.clearDisplay();
  oled.setFont(&verdana6pt7b);
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextWrap(false);
  ina219.begin();                                                 // Initialize V/A monitor

  pinMode(ENCSW, INPUT_PULLUP);                                   // Setup I/O pins
  pinMode(EXPINT, INPUT_PULLUP);
  pinMode(COUNTR, INPUT_PULLUP);
  pinMode(PDINT, INPUT_PULLUP);
  pinMode(PDDTR, INPUT_PULLUP);
  pinMode(PDRTS, INPUT_PULLUP);
  pinMode(PULLUPEN, OUTPUT);
  digitalWrite(PULLUPEN, LOW);
  pinMode(COUNTEN, OUTPUT);
  digitalWrite(COUNTEN, LOW);
  pinMode(PDSLAVE, OUTPUT);
  digitalWrite(PDSLAVE, HIGH);
  PullUp(false);
  analogReadAveraging(100);
  NVIC_SET_PRIORITY(IRQ_PORTD, 17);                               // Port D is a higher priority then timers
  
  //sysState = MENUCOUNTER;                                       // Initialize default states
  sysState = MENUI2CSCAN;
  firstrun = true;
  stopProg = true;
  menuoptions = OPTIONSOK;
  options = OPTIONOK;
  forcedisplay = 1;                                               // Preload display
  exportoption = 0;
  logicSet = 0x11;
  baudRate = (sizeof(baudRates)/sizeof(baudRates[0])) - 1;        // Default baudrate = max

  cursoron.blinkon = false;
  encoder.up = false;
  encoder.down = false;
  encoder.sw = false;
  encoder.swold = true;                                           // if pull-up
  sharedNavigation();

  SPI.setMOSI(myMOSI);                                            // Readjust pins
  SPI.setMISO(myMISO);
  SPI.setSCK(mySCLK);
  SPISettings PDmicro(2000000, MSBFIRST, SPI_MODE0);              // Preset transactionsettings
  slave_exit_mode = false;                                        // Preset flags for communication
  newSPIdata_avail = false;
  allSPIisData = false;   
  slaveTimerRunning = false;
  attachMainInterrupts();                    
  SPI.begin();
}

//------------------------------------------------------------------------------ loop
void loop() {
  bool tmpDTR, tmpRTS;
  
  if (multiresponseButton.longPress()) { encoder.lp = true; }     // long press encoder
  houseKeeping();

  switch (sysState) {                                             // Fast refresh
    case MENUPROGISPAVRACTIVE:
      forcedisplay++;
      break;
    case MENUCOUNTCOUNTERACTIVE:                                  // Update counters  
      if (countme) {
        forcedisplay++;
      }                           
      if (encoder.lp) {
        detachInterrupt(digitalPinToInterrupt(GPIO1));
        options = OK;
        sysState = MENUCOUNTCOUNTER;
        cursoron.blinkon = false;
        countme = false;
        forcedisplay++;
      }
      break;
    case MENUPROGESPACTIVE:
      tmpDTR = digitalRead(PDDTR);                                // 'Mirror' RTS/DTR to MP3/4 
      tmpRTS = digitalRead(PDRTS);                                // DTR = GPIO0 unless RTS is also low
      if (tmpDTR != curDTR || tmpRTS != curRTS) {                 // RTS = RST unless DTR is also low
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
      break;
    case MENUPINMONITORACTIVE:                                    // Read expander if interrupt & display
      if (readTrigger) {
        readTrigger = false;
        readFromExpander();
        forcedisplay++;
      }
      break;
    case MENULOGICACTIVE:                                         // Read expander if interrupt & display
      if (readTrigger) {
        readTrigger = false;
        readFromExpander();
        forcedisplay++;
      }    
      break;
    case MENUENCODERACTIVE:                                       // Process encoder simulation endrequest
      if (encoder.lp) {
        encoder.lp = false;
        myEnc.setCopy(false);
        sysState = MENUENCODERON;
        forcedisplay++;
      }
      if (multiresponseButton.pushed()) { writeToExpander(0x10); } // emulate enc sw on pin 5
      if (multiresponseButton.released()) { writeToExpander(0x00); }
      break;
    case MENUCOUNTFREQLOWACTIVE:                                  // Update counters if needed
      if (freq1.available()) {
        sum1 = sum1 + freq1.read();
        count1 = count1 + 1;
      }
      if (timeoutTimer > 500) { forcedisplay++; }
      break;
    case MENUCOUNTFREQHIGHACTIVE:                                 // Update counters if needed
      if (FreqCount.available()) { 
        forcedisplay++; 
      }
      break;
    default:
      break;
  }

  processMenu();                                                   // Process menu

  if (timeoutTimer > (setScreenTimeOut)) {
    if (screenState == 0) {
      oled.dim(true);
      screenState = 1;
    }
  }
}