#include <Arduino.h>
#include <HID.h>
#include <Keyboard.h>
#include <avr/power.h>
#include <SPI.h>
#include <RingBuf.h>
#include <PD/PD_UFP.h>
#include <BreadboardProtocol.h>

// Pin definitions
#define PDMOSI  16                                        // PB2 D16
#define PDMISO  14                                        // PB3 D14
#define PDSCLK  15                                        // PB1 D15
#define PDSS    17                                        // PB0 D17 (RXLED)
#define PDINT   18                                        // interrupt low to Master
#define PDDTR   9                                         // PB5 A9 D9
#define PDRTS   8                                         // PB4 A8 D8

#define DBG1    4
#define DBG2    5
#define DBG3    6
#define DBG4    7

//#define PD
#define PULSEDELAY  3                                      // Delay for Master to catch up on PDINT signalling

/*
Needs modified ~/.platformio/packages/framework-arduino-avr/variants/micro/pins_arduino.h
RXLED0 & RXLED1 changed to ensure normal SPI working of RB0:

              //#define RXLED0                  PORTB &= ~(1<<0)
              //#define RXLED1                  PORTB |= (1<<0)
              #define RXLED0                  1
              #define RXLED1                  1
*/


//const long baudRates[] = { 4800, 9600, 14400, 19200, 28800, 38400, 57600, 76800, 115200, 250000 };                  
const long baudRates[][2] = {{4800, 9600}, {9600, 19200}, {14400, 28800}, {19200, 38400}, {28800, 57600}, 
                            {38400, 76800}, {57600, 115200}, {76800, 153600}, {115200, 230400}, {250000, 500000}};

class PD_UFP_c PD_UFP;
RingBuf<uint8_t, 256> SPI_In;                               // Ringbuffer data recieved from SPI
RingBuf<uint8_t, 256> SPI_Out;                              // Ringbuffer data to be transmitted to SPI
RingBuf<uint8_t, 256> Serial_In;                            // Ringbuffer SPI in
RingBuf<uint8_t, 256> Serial_Out;                           // Ringbuffer SPI out
volatile uint8_t spidata;
uint8_t curBaudrate;
bool transactionbusy;                                       // SPI transaction in progress?
bool writemode;                                             // SPI data from master
bool readmode;                                              // SPI data from slave
bool newSPIdata_avail;                                      // New SPI data receieved
bool processingSPIdata;
bool datasend;                                              // All data send
bool delayedACK;                                            // delayed ACK te be send
bool highSpeed;                                             // Speedswitch
bool toggle;                                                // Debugging
enum pdState_e pdState;

void setSpeed(bool highSpeed) {
  if (highSpeed) {                                          // Set high or lowspeed (lowspeed @ 3V3)
    clock_prescale_set(clock_div_1);
    PD_UFP.clock_prescale_set(1);
  } else {
    clock_prescale_set(clock_div_2);
    PD_UFP.clock_prescale_set(2);
  }
}

void setup (void)
{
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);

  pinMode(MISO, OUTPUT);
  pinMode(PDINT, OUTPUT);
  digitalWrite(PDINT, HIGH);
  pinMode(SS, INPUT_PULLUP);
  highSpeed = true;
  Serial.begin(500000);
  Serial1.begin(500000);
  Keyboard.begin(); 
  SPCR = 0xC0;                                           // SPI Slave, Mode0, MSB First, Int enabled
  pdState = stateIdle;
  newSPIdata_avail = false;
  processingSPIdata = false;
#ifdef PD
  PD_UFP.init(PD_POWER_OPTION_MAX_5V);
#endif
}

void processSPIdata(uint8_t spidata_in) {
  uint8_t spidata_out;                                              // Get the data
  if (writemode) {
    if (!SPI_In.isFull()) { 
      SPI_In.push(spidata_in);                                     // If room in the buffer, store it.
      bitClear(PORTF, 7);                                          // Signal reciept
      delayMicroseconds(PULSEDELAY);
      bitSet(PORTF, 7);
    }             
  }
  if (readmode) {
    if (SPI_Out.isEmpty()) {                                       // as long as we have something to say ...
      bitSet(PORTF, 7);                                            // If buffer emnpty, signal 'no more bytes' to master
    } else {
      SPI_Out.pop(spidata_out);                                    // send it out @ next recieved byte from master.
      SPDR = spidata_out;
      bitSet(PORTF, 7);                                            // Signal reciept
      delayMicroseconds(PULSEDELAY);
      bitClear(PORTF, 7);
    }         
  }
  if(!transactionbusy) {                                           // This is the first byte from the master
    if (SPI_In.isEmpty()) {                                        // Did we process the last databurst?
      transactionbusy = true;
      switch (spidata_in) {
        case cmdENQ_ReadAck:                                       // are we in master-read mode?                      
          readmode = true;
          if (SPI_Out.isEmpty()) {                                 // If we have nothing more to say ...
            bitSet(PORTF, 7);                                      // ... signal no more dat to send
          } else {                                                 // But if we do have more to say ...
            SPI_Out.pop(spidata_out);                              // ... send it out @ next recieved byte from master.
            SPDR = spidata_out;
          }
          break;
        case cmdESC_ExitCommand:                                   // writemode & exit commandmode                
          writemode = true;
          pdState = stateIdle;
          break;
        case cmdSTX_DataNext:                                      // Next bytes are "normal" data
          writemode = true;
          break;
        default:                                                   // All other commands (incl cmdSTX_DataNext) 
          SPI_In.push(spidata_in);               
          writemode = true;
          break;
      }
      if (readmode) {                                              // Signal reciept
        bitSet(PORTF, 7);                                          // positive pulse
        delayMicroseconds(PULSEDELAY);
        bitClear(PORTF, 7);
      } else {
        bitClear(PORTF, 7);                                        // negative pulse
        delayMicroseconds(PULSEDELAY);
        bitSet(PORTF, 7);
      }  
    } else {                                                       // We did not process the last databurst ...
      spidata = spidata_in;
      delayedACK = true;
    }
  }
}

// SPI interrupt routine
ISR (SPI_STC_vect) {                                               // His masters voice is heard :)
  processSPIdata(SPDR);
}

void loop (void)
{
  uint8_t mydata;

#ifdef PD
  PD_UFP.run();
#endif
  if (pdState == stateUSB_Serial) {
    digitalWrite(PDDTR, Serial.dtr());                             // Copy DTR status for Teensy (9 -> 10)
    digitalWrite(PDRTS, Serial.rts());                             // Copy RTS status for Teensy (8 -> 12)
  }

  if (digitalRead(SS) == HIGH && transactionbusy) {                // Indicates end of SPI transmission
    transactionbusy = false;
    if (writemode) { 
      newSPIdata_avail = true;                                     // If we are in writemode, we recieved info
      writemode = false;
    }
    if (readmode) {                                                // If we are in readmode, data has been send
      datasend = true; 
      readmode = false;
    }
  }

  if (pdState == stateSPI_USB || pdState == stateSPI_Serial || pdState == stateSPI_Keyboard) {   
    if (!SPI_In.isEmpty()) {                                       // Process Serial data
      processingSPIdata = true;
      SPI_In.lockedPop(mydata);
      if (pdState == stateSPI_USB) {
        Serial.write(mydata);                                      // Data fro SPI to Serial
      } else {
        if (pdState == stateSPI_Serial) {
          Serial1.write(mydata);                                   // Data fro SPI to Serial1
        } else {
          Keyboard.write(mydata);
        }
      }
    } else {
      processingSPIdata = false;
    }
    if (!SPI_Out.isFull()) {
      if (pdState == stateSPI_USB) {
        if (Serial.available()) {                                  // Data from serial to SPI
          SPI_Out.lockedPush(Serial.read());
          digitalWrite(PDINT, LOW);
        }
      } else {
        if (Serial1.available()) {                                 // Data from serial1 to SPI
          SPI_Out.lockedPush(Serial1.read());             
          digitalWrite(PDINT, LOW);
        }        
      }
    }
  }

  if (pdState == stateUSB_Serial) {                                // Proces serial data
    if(Serial.available()) { Serial1.write(Serial.read()); }       // Data from USB to Serial
    if(Serial1.available()) { Serial.write(Serial1.read()); }      // Data from Serial to USB
  } 

  if (newSPIdata_avail && !processingSPIdata) {                    // New SPI transaction so 1st byte tells us what we can expect.
    switch (SPI_In[0]) {
      case cmdSerialPiping:                                        // Piping commands
        if (SPI_In[1] == modeSPI_USB)    { 
          pdState = stateSPI_USB; 
        }
        if (SPI_In[1] == modeUSB_Serial) {
          pdState = stateUSB_Serial;
        }
        if (SPI_In[1] == modeSPI_Serial) { 
          pdState = stateSPI_Serial; 
        }
        if (SPI_In[1] == modeSPI_Keyboard) { 
          pdState = stateSPI_Keyboard;
        }
        break;
      case cmdSetBaudRate:                                         // Set baudrate
        Serial1.end();
        curBaudrate = SPI_In[1];
        if (!highSpeed) { 
          Serial1.begin(baudRates[curBaudrate][1]);
        } else {
          Serial1.begin(baudRates[curBaudrate][0]);
        }
        break;
    }
    newSPIdata_avail = false;                                      // No more SPI data present in buffer
    SPI_In.clear();
  }

  if (delayedACK && !processingSPIdata) {
    delayedACK = false;
    processSPIdata(spidata);
  }
}