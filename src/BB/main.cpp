//------------------------------------------------------------------------------ includes
#include "breadboard.h"

// classes
// enum & structs
// globals
// Interrupts
// void houseKeeping()
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
IntervalTimer slaveTimer;                                                       // Timer for PDINT interpretation
elapsedMillis slaveTimeout;                                                     // timout for waiting fore slave acknowledgements
elapsedMillis screenTimeout;                                                    // timeout for screen blanking
elapsedMillis frequencyTimeout;                                                 // timeout for frequencymeasurements
DMAChannel dmachannel0;
DMAChannel dmachannel1;

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
unsigned long setScreenTimeOut = 60000;           // Timeout set to 5 min.
unsigned long last_interrupt_time = 0;            // software debounce
unsigned long last_lp_interrupt_time = 0;         // long press
unsigned long last_time = 0;                      // cursor timing
unsigned long lastpdInt = 0;                      // last time a PD_INT change was detected
unsigned long maxValue;                           // maximum value for input
unsigned long setValue;                           // current input value
uint32_t CaptureTimeBuffer[MAXBUFSIZE];
uint32_t CaptureMinutesBuffer[MAXTIMESPAN];
uint32_t CapturePointer;
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
uint8_t screenState;                              // screen state for dimming & blanking
uint8_t CaptureDataBuffer[MAXBUFSIZE];
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
bool slave_exit_mode;                             // Data in SPI_Out is irrelevant & flushed if this flag is set, slave ends current mode 
bool slowSlave;                                   // Slave is running @ 8MHz
bool PullUpActive;                                // PullUp is active
bool lastclock;                                   // previous clocksignal for logicblocks
bool screenoff;                                   // 
bool allSPIisData;                                // When enabled all SPI tranasction are preceded with the data command
volatile bool slaveTimerRunning;                  // timeout timer for PDINT is running
volatile bool slaveAckReceived;                   // Acknowledge recieved from slave (pulse from PDINT)
volatile bool slaveReadRequest;                   // PDINT is a stable low

control_struct control;                           // status control expander
parameter_struct param;                           // parameters for avrisp
Encoder_internal_state_t * Encoder::interruptArgs[];

// Counter 100%
// Encoder 100%
// I2C Scanner
//    monitor, capture, export (serial & keyboard?)
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
//    missing
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

void captureBufferFull() {
  dmachannel0.clearInterrupt();
}

void PITimerOverflow() {
  if (CaptureMinutesBuffer[0] < MAXTIMESPAN) {
    CaptureMinutesBuffer[(uint8_t) CaptureMinutesBuffer[0]] = dmachannel0.TCD->DADDR;
    CaptureMinutesBuffer[0]++;
  }
}

void slaveTimerInterrupt() {
  slaveTimer.end();
  if (slaveTimerRunning) { slaveAckReceived = true; }
  slaveTimerRunning = false;
  slaveReadRequest = !digitalReadFast(PDINT);
}

void pdInterrupt() {
  if (slaveTimerRunning) {
digitalWriteFast(21, HIGH);
digitalWrite(21, LOW);
    slaveAckReceived = true;
  }
  slaveTimer.begin(slaveTimerInterrupt, PDINT_TIME);
  slaveTimerRunning = true;
}

//------------------------------------------------------------------------------ process communications
void houseKeeping() {
  unsigned long now_time = millis();                               // check cursor & display 
  if (now_time - last_time > 200) {
    last_time = now_time;
    measure = !measure;
    if (sysState == MENUVOLTMETERACTIVE) { displayVoltmeter(); }
    if (freqLowOn && frequencyTimeout > 500) { showFreqLowCount(); }
    processCursor();
    if (measure) {                                                 // Is it time for a current/voktage measurment?
      curAmp = ina219.getCurrent_mA();
      curVolt = ina219.getBusVoltage_V();
      if (curAmp < 0) { curAmp = 0; }
      dispHeader();
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
      slaveTimeout = 0;
      while(!slaveAckReceived && slaveTimeout < SPITIMEOUT);       // Wait for confirmation from slave for next byte
      slaveAckReceived = false;
      while (!SPI_In.isFull() && !transdone) {                     // Read bytes until buffer full or transfer is done
        spidata = SPI.transfer(0x00);                              // Send 0x00, recieve data from slave
        SPI_In.push(spidata);                                      // Add to ringbuffer
        slaveTimeout = 0;
        while(!slaveAckReceived && slaveTimeout < SPITIMEOUT);     // Wait for confirmation from slave for next byte
        slaveAckReceived = false;
        if (!slaveReadRequest) {                                   // If slave has no more data, it will stop sending Acks, the timer
          transdone = true;                                        //   will runout and we get an slaveAckReceived & slaveReadReq 
        }                                                          //   will be false
      }
      if (slaveTimerRunning) { 
        slaveTimer.end();
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
        slaveTimeout = 0;
        while(!slaveAckReceived && slaveTimeout < SPITIMEOUT);     // Wait for confirmation from slave for next byte
        slaveAckReceived = false;
    }
    while (!SPI_Out.isEmpty())                                     // Keep sending bytes until the buffer is empty
    {
      SPI_Out.pop(spidata);
      spidata = SPI.transfer(spidata);
      slaveTimeout = 0;
      while(!slaveAckReceived && slaveTimeout < SPITIMEOUT);       // Wait for confirmation from slave for next byte
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
  }   // short press encoder
  if (encOn) {
    if (multiresponseButton.pushed()) { writeToExpander(0x10); }  // emulate enc sw on pin 5
    if (multiresponseButton.released()) { writeToExpander(0x0); } 
  }
  if (sysState == MENUPROGISPAVRACTIVE) { displayISPAVRProgrammer(); }
}

//------------------------------------------------------------------------------ setup
void setup() {
  Serial.begin(500000);                                  // Setup serial
  Wire.begin();                                          // Setup I2C @ 400kHz
  Wire.setClock(400000L);
  disableGPIO();
  screenState = 0;
  screenTimeout = 0;
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
  pinMode(PULLUPEN, OUTPUT);
  digitalWrite(PULLUPEN, LOW);
  pinMode(COUNTEN, OUTPUT);
  digitalWrite(COUNTEN, LOW);
  pinMode(PDSLAVE, OUTPUT);
  digitalWrite(PDSLAVE, HIGH);
  PullUp(false);
  analogReadAveraging(100);

  pinMode (21, OUTPUT);
  pinMode (22, OUTPUT);
  digitalWrite (21, LOW);
  digitalWrite (22, LOW);
  
  
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

  dmachannel0.begin(true);                                                      // Channel to capture pindata
  dmachannel0.source(GPIOD_PDIR);                                               // Port D input register
  dmachannel0.destinationBuffer((uint32_t *) CaptureDataBuffer, sizeof(CaptureDataBuffer));
  dmachannel0.triggerAtHardwareEvent(DMAMUX_SOURCE_PORTD);                      // pin changes on PORT D
  dmachannel0.transferSize(1);                                                  // 1 byte each transfer (instead of 4!)
  dmachannel0.transferCount(sizeof(CaptureDataBuffer));                         // Untill buffer full
  dmachannel0.attachInterrupt(captureBufferFull);
  dmachannel0.disableOnCompletion();
  dmachannel0.interruptAtCompletion();
  dmachannel1.begin(true);                                                      // Channel to capture timestamp (1/36 us @ 72MHz)
  dmachannel1.source(PIT_CVAL0);                                                // current countdownvalue of PIT0
  dmachannel1.destinationBuffer((uint8_t *) CaptureTimeBuffer, sizeof(CaptureTimeBuffer)*4); 
  dmachannel1.triggerAtHardwareEvent(DMAMUX_SOURCE_PORTD);                      // pin changes on PORT D
  dmachannel1.transferSize(4);                                                  // 1 byte each transfer?
  dmachannel1.transferCount(sizeof(CaptureTimeBuffer)/4);                       // Untill buffer full
  dmachannel1.disableOnCompletion();

  baudRate = (sizeof(baudRates)/sizeof(baudRates[0])) - 1; // Default baudrate = max
  attachInterrupt(digitalPinToInterrupt(EXPINT), expanderInterrupt, FALLING);
  slaveTimerRunning = false;
  attachInterrupt(digitalPinToInterrupt(PDINT), pdInterrupt, CHANGE);
  
  SPI.setMOSI(myMOSI);
  SPI.setMISO(myMISO);
  SPI.setSCK(mySCLK);
  SPISettings PDmicro(2000000, MSBFIRST, SPI_MODE0);
  slave_exit_mode = false;
  newSPIdata_avail = false;
  allSPIisData = false;
  SPI.begin();
  SPI.transfer(0x00);

  logicSet = 0x37;
}

//------------------------------------------------------------------------------ loop
void loop() {
  bool tmpDTR, tmpRTS;
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
      pinMonitor();
    }
    if (sysState == MENULOGICACTIVE) {
      readFromExpander();
      processLogic();
    }
  }

  if (freqLowOn) {                                                 // if counter mode, count
    if (freq1.available()) {
      sum1 = sum1 + freq1.read();
      count1 = count1 + 1;
    }
  }
  if (freqHighOn && FreqCount.available()) { forcedisplay++; }

  if (sysState == MENUCOUNTCOUNTERACTIVE) {                        // Process & show counter
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

  if (sysState == MENUPROGISPAVRACTIVE) { avrisp_loop(); }

  processMenu();                                                   // Process menu

  if (screenTimeout > setScreenTimeOut) {
    if (screenState == 0) {
      oled.dim(true);
      screenState = 1;
    }
  }
}