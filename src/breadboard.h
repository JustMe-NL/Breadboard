#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>                                        // Interrupt driven encoder
#include <Adafruit_INA219.h>                                // power monitor
#include <FreqCount.h>                                      // high frequency monitor
#include <FreqMeasureMulti.h>                               // low frequency monitor
#include <FrequencyTimer2.h>                                // pulse generator
#include <PWMServo.h>                                       // Servo
#include <avdweb_Switch.h>                                  // Debounce & longpress
#include <SoftWire.h>                                       // software I2C
#include <OneWire.h>                                        // OneWire
#include <DallasTemperature.h>                              // DS18B20
#include <RingBuf.h>
#include <PITimer.h>                                        // Neede changes to IntervalTimer.cpp in ~/.platformio/packages/framework-arduinoteensy/cores/teensy3
#include <verdana6pt7b.h>
#include <BreadboardProtocol.h>

// includes
// defines
// enums
// stucts
// macros
// extern classes
// extern enums & structs
// extern globals
// constants
// prototypes

//------------------------------------------------------------------------------ constants defines
//#define debug

#define PDINT_TIME                      100               // Timeout in us for stable PDINT signal
#define SPITIMEOUT                      100               // Timeout in ms waiting for Acknowledge signal from slave
#define MAXBUFSIZE                      4096              // Maximum capturebuffersize 
#define MAXTIMESPAN                     16                // Maximum recording minutes

#define NVRAM_INTEGER                   0x4003E000
#define MCP_ADDRESS                     0x20
//#define PCF_ADDRESS2                    0x38              // PCF8574A
#define PCF_ADDRESS2                    0x39              // PCF8574A
#define PCF_ADDRESS                     0x38              // PCF8574
#define OLEDADDRESS                     0x3C
#define INA219ADD                       0x40

#define RXD1                            0                 // Serial 1 
#define TXD1                            1                 // Serial 1              
#define PDRTS                           2                 // RTS Signal from PD Micro
#define PDDTR                           3                 // DTR Signal from PD Micro
#define TNC1                            4                 // Spare Digital PWM 
#define GPIO1                           5                 // Softwire SDA, OneWire & COUNTR
#define GPIO2                           6                 // Softwire SCL
#define myMOSI                          7                 // SPI Dout
#define myMISO                          8                 // SPI Din
#define PDSLAVE                         9                 // SPI SS
#define PULLUPEN                        12                // Pull Up enable on GPIO1&2
#define PDINT                           11                // PD Micro interrupt
#define COUNTEN                         10                // Pin 13 connected to GPIO1
#define COUNTR                          13                // External LPTMR input
#define mySCLK                          14                // SPI clock
#define ENCB                            15                // Encoder pin B
#define ENCA                            16                // Encoder pin A
#define ENCSW                           17                // Encoder Switch
#define mySDA                           18                // Hardware SDA
#define mySCL                           19                // Hardware SCL
#define EXPINT                          20                // Expander interrupt
#define I2CTRGR                         21                // Hardwrae trigger for I2C start/stop signals
#define TNC2                            22                // Spare Digital A8 PWM Touch
#define VOLTSENSE                       23                // Stepper voltage

#define beget16(addr) (*addr * 256 + *(addr+1) )          // avrisp
#define PIN_RESET     	                4                 // PCF pin > attiny pin 1
#define PIN_MOSI	                      5                 // PCF pin > attiny pin 5
#define PIN_MISO	                      6                 // PCF pin > attiny pin 6
#define PIN_SCK		                      7                 // PCF pin > attiny pin 7



//------------------------------------------------------------------------------ enums
enum optionStates {
  OPTIONCANCEL,               // Cancel
  OPTIONUPDOWN,               // Up/Down arrows
  OPTIONOK,                   // Ok
  OPTIONVALUES,               // Individual digits selector 
  OPTIONSETVALUES,            // Individual digits values
  OPTIONSETUPDOWN,            // Up/Down values
  OPTIONMEASURE,
  OK,                         // Return OK
  CANCEL                      // Return Cancel
};

enum menuOptions {
  OPTIONSOK,
  OPTIONSOKSET,
  OPTIONSOKSETCANCEL,
  OPTIONSCANCEL
};

enum valueTypes {
  VALUEA,                     // set Amperage in mA
  VALUEV,                     // Set Voltage in mV
  VALUEBAUD,                  // Select Baudrate
  VALUENONE
};

enum inputTypes {
  INPUTNUMBER,
  INPUTBAUD,
  INPUTSERVO,
  INPUTBINARY,
  INPUTDECIMAL,
  INPUTSIGNED,
  INPUTHEXADECIMAL
};

enum I2CBufferStates {
  I2CIDLE                         = 0x00,
  I2CSTART                        = 0x01,
  I2CADDRESSBITS                  = 0x02,
  I2CREADWRITE                    = 0x03,
  I2CADDRESSACKNAK                = 0x04,
  I2CDATABITS                     = 0x05,
  I2CDATAACKNAK                   = 0x06,
  I2CSTOP                         = 0x07
};

enum exportOptions {
  S_RAW_WO_TIME                   = 0x00,
  S_RAW_W_TIME                    = 0x01,
  S_HRF                           = 0x02,
  S_SALEAE                        = 0x03,
  K_RAW_WO_TIME                   = 0x04,
  K_RAW_W_TIME                    = 0x05,
  K_HRF                           = 0x06,
  K_SALEAE                        = 0x07
};

enum systemStates {           
  MENUCOUNTER,                
  MENUCOUNTCOUNTER,           
  MENUCOUNTCOUNTERACTIVE,     
  MENUCOUNTFREQLOW,           
  MENUCOUNTFREQLOWACTIVE,     
  MENUCOUNTFREQHIGH,          
  MENUCOUNTFREQHIGHACTIVE,    
  MENUCOUNTINFO,
  MENUCOUNTINFOACTIVE,
  MENUCOUNTEXIT,              
  
  MENUENCODER,                
  MENUENCODERON,              
  MENUENCODERACTIVE,          
  MENUENCODERINFO,
  MENUENCODERINFOACTIVE,
  MENUENCODEREXIT,      

  MENUI2CSCAN,                
  MENUI2CSCANPULLUPONOFF,
  MENUI2CSCANSTART,           
  MENUI2CSCANACTIVE,          
  MENUI2CSCANCOMPLETE,
  MENUI2CSCANMONITOR,
  MENUI2CSCANMONITORACTIVE,
  MENUI2CSCANEXPORT,
  MENUI2CSCANEXPORTACTIVE,
  MENUI2CSCANINFO,
  MENUI2CSCANINFOACTIVE,
  MENUI2CSCANEXIT,      
  
  MENULOGIC,
  MENULOGICONOFF,
  MENULOGICACTIVE,
  MENULOGICINFO,
  MENULOGICINFOACTIVE,
  MENULOGICEXIT,

  MENUONEWIRE,                
  MENUONEWIREPULLUPONOFF,
  MENUONEWIRESCAN,            
  MENUONEWIRESCANACTIVE,      
  MENUONEWIRETEST,            
  MENUONEWIRETESTACTIVE,      
  MENUONEWIREINFO,
  MENUONEWIREINFOACTIVE,
  MENUONEWIREEXIT,      

  MENUPINMONITOR,
  MENUPINMONITORONOFF,           
  MENUPINMONITORACTIVE,
  MENUPINMONITOR7SEGMENT,
  MENUPINMONITOR7SEGMENTACTIVE,
  MENUPINMONITORCAPTURE,
  MENUPINMONITORCAPTUREACTIVE,
  MENUPINMONITORINFO,
  MENUPINMONITORINFOACTIVE,
  MENUPINMONITOREXIT,    

  MENUPOWER,                  
  MENUPOWERONOFF,             
  MENUPOWERONOFFPROCESS,      
  MENUPOWERSETVOLTAGE,        
  MENUPOWERSETVOLTAGEACTIVE,  
  MENUPOWERSETCURRENT,        
  MENUPOWERSETCURRENTACTIVE,  
  MENUPOWERSETCUTOFF,         
  MENUPOWERSETCUTOFFACTIVE,   
  MENUPOWERINFO,
  MENUPOWERINFOACTIVE,
  MENUPOWEREXIT,    
  
  MENUPROGRAMMER,
  MENUPROGISPAVRONOFF,
  MENUPROGISPAVRACTIVE,
  MENUPROGESPONOFF,
  MENUPROGESPACTIVE,
  MENUPROGRAMMERINFO,
  MENUPROGRAMMERINFOACTIVE,
  MENUPROGRAMMEREXIT,
  
  MENUFREQ,     
  MENUFREQONOFF,              
  MENUFREQSETACTIVE, 
  MENUFREQINFO,
  MENUFREQINFOACTIVE,
  MENUFREQEXIT,         
  
  MENUSCREEN,
  MENUSCREENSETTINGS,
  MENUSCREENSETACTIVE,
  MENUSCREENINFO,
  MENUSCREENINFOACTIVE,
  MENUSCREENEXIT,

  MENUSERIAL,                 
  MENUSERIALSETBAUD,          
  MENUSERIALSETBAUDACTIVE,    
  MENUSERIALONOFF,
  MENUSERIALMONITOR,
  MENUSERIALMONITORACTIVE,            
  MENUSERIALINFO,
  MENUSERIALINFOACTIVE,
  MENUSERIALEXIT,             
  
  MENUSERVO,
  MENUSERVOONOFF,                  
  MENUSERVOACTIVE,
  MENUSERVOINFO,
  MENUSERVOINFOACTIVE,
  MENUSERVOEXIT,            
  
  MENUSETPINS,
  MENUSETPINSONOFF,                
  MENUSETPINSSETVALUEACTIVE,
  MENUSETPINSINFO,
  MENUSETPINSINFOACTIVE,
  MENUSETPINSEXIT,
  
  MENUSTEPPER,   
  MENUSTEPPERONOFF,      
  MENUSTEPPERACTIVE,
  MENUSTEPPERCOOLDOWN,
  MENUSTEPPERINFO,
  MENUSTEPPERINFOACTIVE, 
  MENUSTEPPEREXIT,          
  
  MENUSWDEBOUNCE,             
  MENUSWDEBOUNCEONOFF,        
  MENUSWDEBOUNCEINFO,
  MENUSWDEBOUNCEINFOACTIVE,
  MENUSWDEBOUNCEEXIT,

  MENUVOLTMETER,
  MENUVOLTMETERONOFF,
  MENUVOLTMETERACTIVE,
  MENUVOLTMETERINFO,
  MENUVOLTMETERINFOACTIVE,
  MENUVOLTMETEREXIT        
};

enum logicBlocks_e {
  NONE    = 0x00,
  INVERT  = 0x01,
  AND2    = 0x02,
  AND3    = 0x03,
  NAND2   = 0x04,
  NAND3   = 0x05,
  OR2     = 0x06,
  OR3     = 0x07,
  NOR2    = 0x08,
  NOR3    = 0x09,
  XOR2    = 0x0A,
  XOR3    = 0x0B,
  SRFF    = 0x0C,
  JKFF    = 0x0D
};

//------------------------------------------------------------------------------ structures
typedef union {
  byte all;
  struct {
    byte bit0:1;
    byte bit1:1;
    byte bit2:1;
    byte bit3:1;
    byte bit4:1;
    byte bit5:1;
    byte bit6:1;
    byte bit7:1;
  };
} ByteBits_t;

typedef struct {
  boolean blinkon;
  boolean blinkstate;
  boolean blinktype;
  char    blinkchar;
  uint8_t blinkcol;
  uint8_t blinkrow;
} cursorstruct_t;

typedef struct {
  boolean                up;
  boolean                down;
  boolean                sw;
  boolean                lp;
  boolean                swold;
} Encoder_t;                       // Encoder values

typedef union {
  byte all;
  struct {
    byte powerRelay:1 ;
    byte switchRelay:1 ;
    byte serialRelay:1 ; 
    byte gpioRelay:1;
    byte stepperRelay:1;
    byte stepper_sleep:1;
    byte stepper_step:1;
    byte stepper_direction:1; 
  };
} control_struct;

typedef struct param {
  uint8_t devicecode;
  uint8_t revision;
  uint8_t progtype;
  uint8_t parmode;
  uint8_t polling;
  uint8_t selftimed;
  uint8_t lockbytes;
  uint8_t fusebytes;
  uint8_t flashpoll;
  uint16_t eeprompoll;
  uint16_t pagesize;
  uint16_t eepromsize;
  uint32_t flashsize;
} parameter_struct;

//------------------------------------------------------------------------------ macros

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#ifndef maxpit0
#define maxpit0 (F_BUS * 60) -1
#endif

//------------------------------------------------------------------------------ objects
extern Adafruit_SSD1306 oled;
extern Adafruit_INA219 ina219;                            // current & volt meter
extern Encoder myEnc;
extern Switch multiresponseButton;
extern SoftWire i2c;
extern OneWire *ds;
extern FreqMeasureMulti freq1;
extern PWMServo myservo;
extern RingBuf<uint8_t, 256> SPI_In;
extern RingBuf<uint8_t, 256> SPI_Out;
extern elapsedMillis timeoutTimer;                       // standard timout Timer

//------------------------------------------------------------------------------ enums & structs
extern enum optionStates options;                        // Options states in userinput routine
extern enum valueTypes valueType;                        // Values types for input (mA, mV, Hz enz)
extern enum inputTypes inputType;                        // Input types (number, list, binary)
extern enum systemStates sysState;                       // System state & menu structure
extern enum menuOptions menuoptions;
extern cursorstruct_t cursoron;                          // Cursor related variables
extern Encoder_t encoder;                                // Encoder variables

//------------------------------------------------------------------------------ globals
extern float sum1;
extern float curVolt;                                 // current measured voltage
extern float curAmp;                                  // current measured milliampere
extern long oldPosition;                              // encoder tracking
extern unsigned long setScreenTimeOut;                // Screen timeout for blanking
extern unsigned long last_interrupt_time;             // software debounce
extern unsigned long last_lp_interrupt_time;          // long press
extern unsigned long last_time;                       // cursor timing
extern unsigned long maxValue;                        // maximum value for input
extern unsigned long setValue;                        // current input value
extern uint32_t CaptureSecBuffer[MAXBUFSIZE];        // Buffer for time in us from monitoring functions
extern int count1;                                    // countvar for frequencymeasurment
extern int error;                                     // avrisp
extern int pmode;                                     // avrisp
extern unsigned int here;                             // address for reading and writing, set by 'U' command
extern uint16_t setAmp;                               // req amperage
extern uint16_t setVolt;                              // req voltage
extern uint16_t setAmpCO;                             // set milliampere cut off
extern uint16_t buffer[256];                          // buffer for I2C adresses & oneWire
extern uint16_t buff[256];                            // global block storage
volatile extern uint16_t CapturePointer;              // Pointer to last DMA address
extern char myValue[22];                              // buffer for current setting
extern int8_t editValue;                              // current digit to be edited
extern int8_t hbdelta;                                // avrisp
extern uint8_t forcedisplay;                          // force a display refresh
extern uint8_t valuestep;                             // step of value (eg. 1, 20, 50)
extern uint8_t maxDigits;                             // maximum no of digits (in maxValue)
extern uint8_t bytePins;                              // byte data of pins read
extern uint8_t writeByte;                             // holder for bit operations on expander
extern uint8_t i2cpointer;                            // where are we in showing addresses
extern uint8_t faillures;                             // how many test failed
extern uint8_t hbval;                                 // avrisp
extern uint8_t lasthbval;                             // heartbeat avrisp
extern uint8_t progmode;                              // boolean for avrisp mode
extern uint8_t progblock;                             // counter for progressdisplay
extern uint8_t baudRate;                              // Selected baudrate (default max)
extern uint8_t logicSet;                              // set logic blocks if any
extern uint8_t screenState;                           // screen state for dimming & blanking
extern uint8_t exportoption;
extern uint8_t CaptureDataBuffer[MAXBUFSIZE];         // Buffer for values from monitoring functions
extern uint8_t CaptureMinBuffer[MAXBUFSIZE];
extern bool measure;                                  // flag to start measurement
extern bool cutOff;                                   // cutoff enabled
extern bool firstrun;                                 // flag to indicate display routines this is the 1st time they run
extern bool countme;                                  // Interrupt accrued
extern bool stopProg;                                 // flag to abort avrisp mode  
extern bool usHertz;                                  // flag for display
extern bool curDTR;                                   // last DTR status
extern bool curRTS;                                   // last RTS status
extern bool readTrigger;                              // Trigger to read pins
extern bool lastTrigger;                              // last interrupt status
extern bool readSet;                                  // expander in read mode
extern bool rst_active_high;                          // avrisp
extern bool checklist[12];                            // what wend wrong with this clone
extern bool newSPIdata_avail;                         // We recieved new SPI data
extern bool slave_exit_mode;                          // Data in SPI_Out is irrelevant & flushed if thios flag is set, slave ends current mode 
extern bool slowSlave;                                // Slave is running @ 8MHz
extern bool PullUpActive;                             // PullUp for is active
extern bool lastclock;                                // previous clocksignal for logicblocks
extern bool allSPIisData;                             // Flag for SPI communication
extern bool dmaStopped;                               // DMA channels & PIT running
extern volatile bool slaveTimerRunning;               // Timingflag for SPI acknowledge signals
extern volatile bool slaveAckReceived;                // Flag for Ack recieved from slave
extern volatile bool slaveReadRequest;                // Flag for readrequest from slave
extern control_struct control;                        // status control expander
extern parameter_struct param;                        // parameters for avrisp



//------------------------------------------------------------------------------ constant vars
const long baudRates[] = { 4800, 9600, 14400, 19200, 28800, 38400, 57600, 76800, 115200, 250000 };
const uint8_t baudErrLow[] = {2,  2,   21,    2,    37,    75,    78,    78,    78,     0 }; // Error % * 10
const uint8_t baudErrHigh[] = {2, 2,   6,     2,     8,     2,    21,     2,    37,     0 }; // Error % * 10
const int voltageTable[][2] = {{0, 0}, {163, 50}, {316, 100}, {477, 150}, {630, 200}, {789, 250}, {948, 300}, {1023, 324}};

//                                    01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
#define MAXINFOTEXT 1024

const char infoCounter[]   = PROGMEM {"The Counter functions all act on pin MP3 of BU26. The counter counts until an overflow or until you "
                                     "reset with a click, hold the switch for more than 3 seconds to exit the function. The frequency "
                                      "measurements on are defined over two functions: 0.1 to 10kHz and 10kHz to 50MHz.\0"};
const char infoEncoder[]   = PROGMEM {"Outputs the encoder A and B on pins MP and MP4 of BU26. The switch is debounced and presented on pin "
                                      "MP5. Hold the switch for 3 seconds to exit.\0"};
const char infoI2CScan[]   = PROGMEM {"Scans for I2C slave devices and returns the found addresses. Needs two optional external 2k pull-ups "
                                      "or turn the pull ups on in the menu. Connect SDA to MP3 on BU26 and SCL to MP4 and start the scan. "
                                      "Scroll to see the addresses of the found devices, click to exit.\0"};
const char infoLogBlocks[] = PROGMEM {"Sorry, no information yet.\0"};
const char infoOneWire[]   = PROGMEM {"The OneWire functions work on pin MP3 of BU26 and need an optional 4k7 pull-up or turn the pull up on "
                                      "in the menu. The scan devices function scans for DS18B20 OneWire thermometers and records the "
                                      "temperature, scroll for the results, click to exit. The check function does a counterfeit check on a "
                                      "single DS18B20, scroll through the errors reported (if any) and click to exit.\0"};
const char infoPinmon[]    = PROGMEM {"Sorry, no information yet.\0"};
const char infoPower[]     = PROGMEM {"Sorry, no information yet.\0"};
const char infoAVRISP[]    = PROGMEM {"Emulates a slow AVRISP programmer on the serial port. Press switch for 3 seconds to exit this "
                                      "function. All signals are on BU26: Reset-MP5, MISO-MP6, MOSI-MP7 and SCLK on MP8. Connect respectively "
                                      "to pins 1, 5, 6 & 7 to program a ATtiny45/85.\0"};
const char infoPulseGen[]  = PROGMEM {"Generates pulses with a 50% width ranging from 10us to 1s period on MP3 of BU26. Not all values are "
                                      "possible, the set value is show first, the actual value is shown on the second line. Click us or Hz "
                                      "to toggle the shown actual value between us and Hertz. Click <> to step the value up or down, click "
                                      "a value to set that digit, changes are in effect immediatly.\0"};
const char infoScreen[]    = PROGMEM {"Sorry, no information yet.\0"};
const char infoSerialPT[]  = PROGMEM {"Passes the serial port through to pins MP1 (RxD) and MP2 (TxD) of BU26 until turned off again. Cannot "
                                      "be combined with stepper functions, when stepper is activated, serial pass through wil be turned off. "
                                      "When pass through is activated the bottom right corner will indicate R/T. When you select the baudrate, "
                                      "the numbver in between brackets indicates the percentage the baudrate deviates from the realbaudrate.\0"};
const char infoServo[]     = PROGMEM {"Generates servo pulses in the range of 1.5 to 2.5ms on MP3 of BU26. Click <> to select 1, 5 or 10 "
                                      "steps at once, when 1, 5 or 10 is selected, turning the selector will set the pulsewidth to the angle "
                                      "shown on screen. Actual angles may change with the position of the servo depending on servotype.\0"};
const char infoSetPins[]   = PROGMEM {"Sets the displayed value on pins MP1-MP8 of BU26 unless serial pass through is active, then this "
                                      "function works on pins MP3-MP8 and the two least significant bits are ignored. Click <> to change the "
                                      "value through scrolling and click again to exit change value mode. Scroll to select individual bits, "
                                      "click selected bit to change value through scrolling, click again to select an other bit or exit the "
                                      "function via OK.\0"};
const char infoStepper[]   = PROGMEM {"Sorry, no information yet.\0"};
const char infoDebounce[]  = PROGMEM {"Turn on to engage switch debouncing on SW 1 and 2. The switches give a positive debounced signal on "
                                      "the two innermost connections when activated and the switch is pressed, no additional vcc or ground "
                                      "connections are necessary. When activated, the green LED will turn on.\0"};
const char infoVoltmeter[] = PROGMEM {"Sorry, no information yet.\0"};                                  

//------------------------------------------------------------------------------ graphic bitmaps
// Images made using http://javl.github.io/image2cpp/
// 'AND', 64x32px
const unsigned char bmapAnd [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x1f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 
	0x11, 0x0f, 0xf0, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x40, 0x00, 0x00, 
	0x15, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0xa8, 
	0x15, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xfc, 0x88, 
	0x11, 0x0f, 0xf0, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0xa8, 
	0x15, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 0x15, 0x00, 0x10, 0x00, 0x00, 0x40, 0x00, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x80, 0x00, 0x00, 0x11, 0x0f, 0xf0, 0x00, 0x01, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x1f, 0xff, 0xfc, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'OR', 64x32px
const unsigned char bmapOr [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x1f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 
	0x11, 0x0f, 0xfe, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00, 
	0x15, 0x00, 0x00, 0x80, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x20, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x08, 0x00, 0xa8, 
	0x15, 0x00, 0x00, 0x20, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x0f, 0xfc, 0x88, 
	0x11, 0x0f, 0xff, 0xe0, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x08, 0x00, 0xa8, 
	0x15, 0x00, 0x00, 0x20, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x80, 0x00, 0x20, 0x00, 0x00, 0x15, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00, 
	0x00, 0x00, 0x02, 0x00, 0x00, 0x80, 0x00, 0x00, 0x11, 0x0f, 0xfc, 0x00, 0x01, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x1f, 0xff, 0xfc, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'XOR', 64x32px
const unsigned char bmapXor [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x10, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x40, 0x01, 0x00, 0x00, 0x00, 
	0x11, 0x0f, 0xfe, 0x20, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x40, 0x00, 0x00, 
	0x15, 0x00, 0x00, 0x88, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x44, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x08, 0x00, 0xa8, 
	0x15, 0x00, 0x00, 0x22, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x0f, 0xfc, 0x88, 
	0x11, 0x0f, 0xff, 0xe2, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x08, 0x00, 0xa8, 
	0x15, 0x00, 0x00, 0x22, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x44, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x88, 0x00, 0x20, 0x00, 0x00, 0x15, 0x00, 0x01, 0x10, 0x00, 0x40, 0x00, 0x00, 
	0x00, 0x00, 0x02, 0x20, 0x00, 0x80, 0x00, 0x00, 0x11, 0x0f, 0xfc, 0x40, 0x01, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x80, 0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x11, 0xff, 0xfc, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
const unsigned char bmapFF [] PROGMEM = {
// 'Flipflop', 64x32px
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x1f, 0xff, 0xff, 0xf8, 0x00, 0xa8, 0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 
	0x11, 0x0f, 0xf0, 0x00, 0x00, 0x0f, 0xf0, 0x88, 0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 
	0x15, 0x00, 0x10, 0x00, 0x00, 0x08, 0x80, 0xa8, 0x00, 0x00, 0x10, 0x00, 0x00, 0x09, 0x40, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x09, 0x40, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x09, 0x40, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0xa0, 0x00, 0x00, 0x00, 0x18, 0x20, 0x00, 0x08, 0x00, 0x00, 
	0x15, 0x00, 0x14, 0x50, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x12, 0x40, 0x00, 0x08, 0x00, 0x00, 
	0x11, 0x0f, 0xf1, 0x40, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x12, 0x50, 0x00, 0x08, 0x00, 0x00, 
	0x15, 0x00, 0x14, 0x20, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 0x15, 0x00, 0x10, 0x00, 0x00, 0x09, 0xe0, 0xa8, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 0x11, 0x0f, 0xf0, 0x00, 0x00, 0x0f, 0xf0, 0x88, 
	0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 0x15, 0x00, 0x1f, 0xff, 0xff, 0xf8, 0x80, 0xa8, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
const unsigned char bmapNot [] PROGMEM = {
// 'Not', 64x32px
0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 
0x15, 0x00, 0x00, 0x23, 0x0e, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00, 0x20, 0xd1, 0x00, 0x00, 0x00, 
0x11, 0x0f, 0xff, 0xe0, 0x31, 0xff, 0xf0, 0x88, 0x00, 0x00, 0x00, 0x20, 0xd1, 0x00, 0x00, 0x00, 
0x15, 0x00, 0x00, 0x23, 0x0e, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x23, 0x0e, 0x00, 0x00, 0xa8, 
0x00, 0x00, 0x00, 0x20, 0xd1, 0x00, 0x00, 0x00, 0x11, 0x0f, 0xff, 0xe0, 0x31, 0xff, 0xf0, 0x88, 
0x00, 0x00, 0x00, 0x20, 0xd1, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x23, 0x0e, 0x00, 0x00, 0xa8, 
0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// '1', 3x5px
const unsigned char bmap1 [] PROGMEM = {
	0x40, 0xc0, 0x40, 0x40, 0xe0
};
// '2', 3x5px
const unsigned char bmap2 [] PROGMEM = {
	0xc0, 0x20, 0x40, 0x80, 0xe0
};
// '3', 3x5px
const unsigned char bmap3 [] PROGMEM = {
	0xc0, 0x20, 0x40, 0x20, 0xc0
};
// '4', 3x5px
const unsigned char bmap4 [] PROGMEM = {
	0x20, 0x60, 0xa0, 0xe0, 0x20
};
// '5', 3x5px
const unsigned char bmap5 [] PROGMEM = {
	0xe0, 0x80, 0xc0, 0x20, 0xc0
};
// '6', 3x5px
const unsigned char bmap6 [] PROGMEM = {
	0x60, 0x80, 0xc0, 0xa0, 0x40
};
// '7', 3x5px
const unsigned char bmap7 [] PROGMEM = {
	0xe0, 0x20, 0x40, 0x40, 0x40
};
// '8', 3x5px
const unsigned char bmap8 [] PROGMEM = {
	0x40, 0xa0, 0x40, 0xa0, 0x40
};
// 'J', 3x5px
const unsigned char bmapJ [] PROGMEM = {
	0xe0, 0x20, 0x20, 0xa0, 0x40
};
// 'K', 3x5px
const unsigned char bmapK [] PROGMEM = {
	0xa0, 0xa0, 0xc0, 0xa0, 0xa0
};
// 'S', 3x5px
const unsigned char bmapS [] PROGMEM = {
	0x60, 0x80, 0x40, 0x20, 0xc0
};
// 'R', 3x5px
const unsigned char bmapR [] PROGMEM = {
	0xc0, 0xa0, 0xc0, 0xa0, 0xa0
};


//------------------------------------------------------------------------------ prototypes
#ifdef debug
void serialdebug(int debugStep);
void dumpBuffer();
#endif

// avrisp
void heartbeat();
void reset_target(bool reset);
uint8_t getch();
void fill(int n);
void pulse(int pin, int times);
uint8_t spi_transaction(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
void empty_reply();
void breply(uint8_t b);
void get_version(uint8_t c);
void set_parameters();
void start_pmode();
void end_pmode();
void universal();
void flash(uint8_t hilo, unsigned int addr, uint8_t data);
void commit(unsigned int addr);
unsigned int current_page();
void write_flash(int length);
uint8_t write_flash_pages(int length);
uint8_t write_eeprom(unsigned int length);
uint8_t write_eeprom_chunk(unsigned int start, unsigned int length);
void program_page();
uint8_t flash_read(uint8_t hilo, unsigned int addr);
char flash_read_page(int length);
char eeprom_read_page(int length);
void read_page();
void read_signature();
void avrisp();
void avrisp_loop();
// rest
void expanderInterrupt();
void counterInterrupt();
void nowcursor(char cursorchar, boolean blinktype = false);
void setUserValue(unsigned long valueToSet, uint8_t valstep = 1);
void writeToExpander(uint8_t IOValue, bool update_all = false);
uint8_t digitalExRead(const uint8_t pin);
void digitalExWrite(const uint8_t pin, const uint8_t value);
void getUserValue();
void dispHeader();
void pinMonitor();
void readFromExpander();
void showI2CResults();
bool read_scratchpad(uint8_t *addr, uint8_t *buff9);
void oneWireTest();
void showOneWireTestResults();
void oneWireScan();
void showOneWireScanResults();
void showFreqLowCount();
void showFreqHighCount();
void sharedNavigation(bool process = false);
void processMenu();
void enableGPIO();
void disableGPIO();
void enableUSB2Serial(uint8_t mode);
void enableUSBKeyboard();
void disableSlaveModes();
void disableSlaveModes();
void processCursor();
void showInfo(const char* helptext);
void showCounter();
void displayServo();
void pulseStepper(bool CCW);
void pulseGenerator();
void houseKeeping();
void displayISPAVRProgrammer();
void displayESPProgrammer();
void displayVoltmeter();
float calculateVoltage(int measuredvoltage);
void changePin(uint8_t pin, bool state);
void PullUp(bool OnOff);
void processLogic();
uint8_t calculateLogic(uint8_t myblocks, uint8_t myvalue, uint8_t myx);
void display7Segment();
void displayLogic();
void showlogicPins(uint8_t myx, byte value, bool showPin3);
void displayScreenSet();
void checkScreen();
void sdaInterrupt();
void sclInterrupt();
void dmaTimerInterrupt();
void displayI2CMonitor();
void displayExport();
void rawI2CExport(bool timinginfo);
void saleaeProtocolExport();
void I2CprotocolExport();
void printTo(char bufferToPrint[], uint8_t sizeOfBuffer);
void attachMainInterrupts();
void detachMainInterrupts();
void pdInterrupt();
void pinGPIO2Interrupt();
void pinI2CTRGRInterrupt();
void pinGPIO2InterruptFast();
void pinI2CTRGRInterruptFast();
void maxtimeCaptureInterrupt();
