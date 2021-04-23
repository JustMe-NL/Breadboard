#include <Arduino.h>
#include <avr/power.h>
#include <SPI.h>
#include <RingBuf.h>
#include <PD/PD_UFP.h>
#include <BreadboardProtocol.h>

#define PD

#define PDMOSI  16
#define PDMISO  14
#define PDSCLK  15
#define PDSS    17
#define PDDTR   9
#define PDRTS   8

// Needs modified ~/.platformio/packages/framework-arduino-avr/variants/micro/pins_arduino.h
// RXLED0 & RXLED1 changed to ensure normal SPI working of RB0

#define PDINT   18                                        // interrupt low to Master
//const long baudRates[] = { 4800, 9600, 14400, 19200, 28800, 38400, 57600, 76800, 115200, 250000 };                  
const long baudRates[][2] = {{4800, 9600}, {9600, 19200}, {14400, 28800}, {19200, 38400}, {28800, 57600}, 
                            {38400, 76800}, {57600, 115200}, {76800, 153600}, {115200, 230400}, {250000, 500000}};

class PD_UFP_c PD_UFP;
RingBuf<uint8_t, 256> SPI_In;                               // Ringbuffer SPI in
RingBuf<uint8_t, 256> SPI_Out;                              // Ringbuffer SPI out
RingBuf<uint8_t, 256> Serial_In;                            // Ringbuffer SPI in
RingBuf<uint8_t, 256> Serial_Out;                           // Ringbuffer SPI out
uint8_t spidata, curBaudrate;
bool transactionbusy, writemode, readmode, newSPIdata_avail, datasend, highSpeed;
enum pdState_e pdState;

void setSpeed(bool highSpeed) {
  if (highSpeed) {
    clock_prescale_set(clock_div_1);
    PD_UFP.clock_prescale_set(1);
  } else {
    clock_prescale_set(clock_div_2);
    PD_UFP.clock_prescale_set(2);
  }
}

void setup (void)
{
  highSpeed = true;
  pinMode(MISO, OUTPUT);
  pinMode(PDINT, OUTPUT);
  digitalWrite(PDINT, HIGH);
  pinMode(SS, INPUT_PULLUP);
  Serial.begin (500000);
  Serial1.begin(500000);
  SPCR = 0xC0;                                           // SPI Slave, Mode0, MSB First, Int enabled
  pdState = IDLE;
  newSPIdata_avail = false;
#ifdef PD
  PD_UFP.init(PD_POWER_OPTION_MAX_5V);
#endif
}

// SPI interrupt routine
ISR (SPI_STC_vect) {                                               // His masters voice is heard
  uint8_t spidata_out, spidata_in;
  spidata_in = SPDR;                                               // get the cookie
  if (writemode) {
    if (!SPI_In.isFull()) { SPI_In.push(spidata_in); }             // If room in the buffer, store it.
  }
  if (readmode) {
    if (!SPI_Out.isEmpty()) {                                      // as long as we have something to say ...
      SPI_Out.pop(spidata_out);                                    // send it out @ next recieved byte from master.
      SPDR = spidata_out;
    }
    if (SPI_Out.isEmpty()) { digitalWrite(PDINT, HIGH); }          // If buffer emnpty, signal 'no more bytes' to master
  }
  if(!transactionbusy) {                                           // this is the first byte from the master
    transactionbusy = true;
    if (spidata_in == READACKNOWLEDGED) {                          // are we in master-read mode?
      readmode = true;
      if (!SPI_Out.isEmpty()) {                                    // if we have something to say ...
        SPI_Out.pop(spidata_out);                                  // send it out @ next recieved byte from master.
        SPDR = spidata_out;
      }
      if (SPI_Out.isEmpty()) { digitalWrite(PDINT, HIGH); }
    }
    if (spidata_in == DATAFOLLOWS) { writemode = true; }          // or are we in master-write mode?
    if (spidata_in == EXITCOMMAND) {                              // writemode & exit commandmode
      writemode = true;
      pdState = IDLE;
    }
  }
}

void loop (void)
{
  uint8_t mydata;

#ifdef PD
  PD_UFP.run();
#endif

  digitalWrite(PDDTR, Serial.dtr());                              // Copy DTR status for Teensy (9 -> 10)
  digitalWrite(PDRTS, Serial.rts());                              // Copy RTS status for Teensy (8 -> 12)

  if (digitalRead(SS) == HIGH && transactionbusy) {               // Indicates end of SPI transmission
    transactionbusy = false;
    if (writemode) { 
      newSPIdata_avail = true;                                    // If we are in writemode, we recieved info
      writemode = false;
    }
    if (readmode) {                                               // If we are in readmode, data has been send
      datasend = true; 
      readmode = false;
    }
  }

  if (pdState == SPI2USB || pdState == SPI2SERIAL) {              // Process Serial data
    if (!SPI_In.isEmpty()) {                                      
      SPI_In.lockedPop(mydata);
      if (pdState == SPI2USB) {
        Serial.write(mydata);                                     // Data fro SPI to Serial
      } else {
        Serial1.write(mydata);                                    // Data fro SPI to Serial1
      }
    }
    if (!SPI_Out.isFull()) {
      if (pdState == SPI2USB) {
        if (Serial.available()) {                                 // Data from serial to SPI
          SPI_Out.lockedPush(Serial.read());
          digitalWrite(PDINT, LOW);
        }
      } else {
        if (Serial1.available()) {                                // Data from serial1 to SPI
          SPI_Out.lockedPush(Serial1.read());             
          digitalWrite(PDINT, LOW);
        }        
      }
    }
  }

  if (pdState == USB2SERIAL) {                                    // Proces serial data
    if(Serial.available()) { Serial1.write(Serial.read()); }      // Data from USB to Serial
    if(Serial1.available()) { Serial.write(Serial1.read()); }     // Data from Serial to USB
  } 

  if (newSPIdata_avail) {                                         // New SPI transaction so 1st byte tells us what we can expect.
    switch (SPI_In[0]) {
      case SERIALPIPING:                                          // Piping commands
        if (SPI_In[1] == USBSERIAL) { pdState = USB2SERIAL; }
        if (SPI_In[1] == USBSPI)    { pdState = SPI2USB; }
        if (SPI_In[1] == SPISERIAL) { pdState = SPI2SERIAL; }
        break;
      case SET_BAUDRATE:                                          // Set baudrate
        Serial1.end();
        curBaudrate = SPI_In[1];
        if (!highSpeed) { 
          Serial1.begin(baudRates[curBaudrate][1]);
        } else {
          Serial1.begin(baudRates[curBaudrate][0]);
        }
        break;
    }
    newSPIdata_avail = false;
    SPI_In.clear();
  }
}