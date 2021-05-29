// ArduinoISP version 04m3
// Copyright (c) 2008-2011 Randall Bohn
// If you require a license, see 
//     http://www.opensource.org/licenses/bsd-license.php
//
// Put an LED (with resistor) on the following pins:
// 9: Heartbeat   - shows the programmer is running
// 8: Error       - Lights up if something goes wrong (use red if that makes sense)
// 7: Programming - In communication with the slave
//
// 23 July 2011 Randall Bohn
// -Address Arduino issue 509 :: Portability of ArduinoISP
// http://code.google.com/p/arduino/issues/detail?id=509
//
// October 2010 by Randall Bohn
// - Write to EEPROM > 256 bytes
// - Better use of LEDs:
// -- Flash LED_PMODE on each flash commit
// -- Flash LED_PMODE while writing EEPROM (both give visual feedback of writing progress)
// - Light LED_ERR whenever we hit a STK_NOSYNC. Turn it off when back in sync.
// - Use pins_arduino.h (should also work on Arduino Mega)
//
// October 2009 by David A. Mellis
// - Added support for the read signature command
// 
// February 2009 by Randall Bohn
// - Added support for writing to EEPROM (what took so long?)
// Windows users should consider WinAVR's avrdude instead of the
// avrdude included with Arduino software.
//
// January 2008 by Randall Bohn
// - Thanks to Amplificar for helping me with the STK500 protocol
// - The AVRISP/STK500 (mk I) protocol is used in the arduino bootloader
// - The SPI functions herein were developed for the AVR910_ARD programmer 
// - More information at http://code.google.com/p/mega-isp

#include <breadboard.h>

//------------------------------------------------------------------------------ local defines
#define HWVER 2
#define SWMAJ 1
#define SWMIN 18
#define PTIME 30
#define SPI_CLOCK 		(10000000/6)
#define EECHUNK (32)

// STK Definitions
#define STK_OK      0x10
#define STK_FAILED  0x11
#define STK_UNKNOWN 0x12
#define STK_INSYNC  0x14
#define STK_NOSYNC  0x15
#define CRC_EOP     0x20 //ok it is a space...
#define SPI_MODE0   0x00

// class SoftSPISetting
// class BitBangedSPI
// void heartbeat()
// void reset_target(bool reset)
// uint8_t getch()
// void fill(int n)
// void pulse(int pin, int times)
// void prog_lamp(int state)
// uint8_t spi_transmission(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
// void empty_reply()
// void breply(uint8_t b)
// void get_version(uint8_t c)
// void set_parameters()
// void start_pmode()
// void end_pmode()
// void universal()
// void flash(uint8_t hilo, unsigned int addr, uint8_t data)
// unsigned int current_page()
// void write_flash(int length)
// uint8_t write_flash_pages(int length)
// uint8_t write_eeprom(unsigned int length)
// uint8_t write_eeprom_chunk(unsigned int start, unsigned int length)
// void program_page() 
// uint8_t flash_read(uint8_t hilo, unsigned int addr)
// char flash_read_page(int length)
// char eeprom_read_page(int length)
// void read_page()
// void read_signature()
// void avrisp()
// void avrisp_loop(void)

//------------------------------------------------------------------------------ classes
class SoftSPISettings {
  public:
    // clock is in Hz
    SoftSPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) : clock(clock) {
      (void) bitOrder;
      (void) dataMode;
    };

  private:
    uint32_t clock;

    friend class BitBangedSPI;
};

class BitBangedSPI {
  public:
    void begin() {
      digitalExWrite(PIN_SCK, LOW);
      digitalExWrite(PIN_MOSI, LOW);
      digitalExWrite(PIN_MISO, HIGH);
    }

    void beginTransaction(SoftSPISettings settings) {}
    void end() {}

    uint8_t transfer (uint8_t b) {
      for (unsigned int i = 0; i < 8; ++i) {
        digitalExWrite(PIN_MOSI, (b & 0x80) ? HIGH : LOW);
        digitalExWrite(PIN_SCK, HIGH);
        b = (b << 1) | digitalExRead(PIN_MISO);
        digitalExWrite(PIN_SCK, LOW); // slow pulse
      }
      return b;
    }

  private:
    unsigned long pulseWidth; // in microseconds
};

static BitBangedSPI SoftSPI;

void heartbeat() {
  static unsigned long last_time = 0;
  unsigned long now = millis();
  if ((now - last_time) < 100)
    return;
  last_time = now;
  hbval++;

  houseKeeping();
  uint8_t alive = (hbval & 0x03);
  if (alive != lasthbval) {
    lasthbval = alive; 
    oled.setCursor(120, 28);
    oled.fillRect(120, 18, 8, 16, SSD1306_BLACK);
    switch (alive) {
      case 0x00:
        oled.print("|");
        break;
      case 0x01:
        oled.print("/");
        break;
      case 0x02:
        oled.print("-");
        break;
      case 0x03:
        oled.print("\\");
        break;
    }
  }
  if (encoder.lp) { 
    displayISPAVRProgrammer();
  }
}

void reset_target(bool reset) {
  digitalExWrite(PIN_RESET, ((reset && rst_active_high) || (!reset && !rst_active_high)) ? HIGH : LOW);
}

uint8_t getch() {
  // modify routine to include housekeeping tasks while waiting for input
  //while (!Serial.available());
  //return Serial.read();
  uint8_t serialData = 0x00;
  while(SPI_In.isEmpty() && sysState == MENUPROGISPAVRACTIVE) {
    houseKeeping();
  }
  SPI_In.pop(serialData);
  return serialData;
}

void fill(int n) {
  for (int x = 0; x < n; x++) {
    buffer[x] = getch();
  }
}

void pulse(int pin, int times) {
  do {
    digitalWrite(pin, HIGH);
    delay(PTIME);
    digitalWrite(pin, LOW);
    delay(PTIME);
  } while (times--);
}

uint8_t spi_transaction(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  SoftSPI.transfer(a);
  SoftSPI.transfer(b);
  SoftSPI.transfer(c);
  return SoftSPI.transfer(d);
}

void empty_reply() {
  if (CRC_EOP == getch()) {
    //Serial.print((char)STK_INSYNC);
    //Serial.print((char)STK_OK);
    SPI_Out.push((char)STK_INSYNC);
    SPI_Out.push((char)STK_OK);
  } else {
    error++;
    //Serial.print((char)STK_NOSYNC);
    SPI_Out.push((char)STK_NOSYNC);
  }
}

void breply(uint8_t b) {
  if (CRC_EOP == getch()) {
    //Serial.print((char)STK_INSYNC);
    //Serial.print((char)b);
    //Serial.print((char)STK_OK);
    SPI_Out.push((char)STK_INSYNC);
    SPI_Out.push((char)b);
    SPI_Out.push((char)STK_OK);
  } else {
    error++;
    //Serial.print((char)STK_NOSYNC);
    SPI_Out.push((char)STK_NOSYNC);
  }
}

void get_version(uint8_t c) {
  switch (c) {
    case 0x80:
      breply(HWVER);
      break;
    case 0x81:
      breply(SWMAJ);
      break;
    case 0x82:
      breply(SWMIN);
      break;
    case 0x93:
      breply('S'); // serial programmer
      break;
    default:
      breply(0);
  }
}

void set_parameters() {
  // call this after reading parameter packet into buffer[]
  param.devicecode = buffer[0];
  param.revision   = buffer[1];
  param.progtype   = buffer[2];
  param.parmode    = buffer[3];
  param.polling    = buffer[4];
  param.selftimed  = buffer[5];
  param.lockbytes  = buffer[6];
  param.fusebytes  = buffer[7];
  param.flashpoll  = buffer[8];
  // ignore buffer[9] (= buffer[8])
  // following are 16 bits (big endian)
  param.eeprompoll = beget16(&buffer[10]);
  param.pagesize   = beget16(&buffer[12]);
  param.eepromsize = beget16(&buffer[14]);

  // 32 bits flashsize (big endian)
  param.flashsize = buffer[16] * 0x01000000
                    + buffer[17] * 0x00010000
                    + buffer[18] * 0x00000100
                    + buffer[19];

  // AVR devices have active low reset, AT89Sx are active high
  rst_active_high = (param.devicecode >= 0xe0);
}

void start_pmode() {

  // Reset target before driving PIN_SCK or PIN_MOSI

  // SPI.begin() will configure SS as output, so SPI master mode is selected.
  // We have defined RESET as pin 10, which for many Arduinos is not the SS pin.
  // So we have to configure RESET as output here,
  // (reset_target() first sets the correct level)
  reset_target(true);
  pinMode(PIN_RESET, OUTPUT);
  SoftSPI.begin();
  SoftSPI.beginTransaction(SoftSPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));

  // See AVR datasheets, chapter "SERIAL_PRG Programming Algorithm":

  // Pulse RESET after PIN_SCK is low:
  digitalExWrite(PIN_SCK, LOW);
  delay(20); // discharge PIN_SCK, value arbitrarily chosen
  reset_target(false);
  // Pulse must be minimum 2 target CPU clock cycles so 100 usec is ok for CPU
  // speeds above 20 KHz
  delayMicroseconds(100);
  reset_target(true);

  // Send the enable programming command:
  delay(50); // datasheet: must be > 20 msec
  spi_transaction(0xAC, 0x53, 0x00, 0x00);
  pmode = 1;
}

void end_pmode() {
  SoftSPI.end();
  // We're about to take the target out of reset so configure SPI pins as input
  digitalExWrite(PIN_MOSI, HIGH);
  digitalExWrite(PIN_SCK, HIGH);
  reset_target(false);
  digitalExWrite(PIN_RESET, HIGH);
  pmode = 0;
}

void universal() {
  uint8_t ch;
  fill(4);
  ch = spi_transaction(buffer[0], buffer[1], buffer[2], buffer[3]);
  breply(ch);
}

void flash(uint8_t hilo, unsigned int addr, uint8_t data) {
  spi_transaction(0x40 + 8 * hilo,
                  addr >> 8 & 0xFF,
                  addr & 0xFF,
                  data);
}
void commit(unsigned int addr) {
    spi_transaction(0x4C, (addr >> 8) & 0xFF, addr & 0xFF, 0);
    delay(PTIME);
}

unsigned int current_page() {
  if (param.pagesize == 32) {
    return here & 0xFFFFFFF0;
  }
  if (param.pagesize == 64) {
    return here & 0xFFFFFFE0;
  }
  if (param.pagesize == 128) {
    return here & 0xFFFFFFC0;
  }
  if (param.pagesize == 256) {
    return here & 0xFFFFFF80;
  }
  return here;
}

void write_flash(int length) {
  fill(length);
  if (CRC_EOP == getch()) {
    //Serial.print((char) STK_INSYNC);
    //Serial.print((char) write_flash_pages(length));
    SPI_Out.push((char) STK_INSYNC);
    SPI_Out.push((char) write_flash_pages(length));
  } else {
    error++;
    //Serial.print((char) STK_NOSYNC);
    SPI_Out.push((char) STK_NOSYNC);
  }
}

uint8_t write_flash_pages(int length) {
  int x = 0;
  unsigned int page = current_page();
  while (x < length) {
    if (page != current_page()) {
      commit(page);
      page = current_page();
    }
    flash(LOW, here, buffer[x++]);
    flash(HIGH, here, buffer[x++]);
    here++;
  }

  commit(page);

  return STK_OK;
}

uint8_t write_eeprom(unsigned int length) {
  // here is a word address, get the byte address
  unsigned int start = here * 2;
  unsigned int remaining = length;
  if (length > param.eepromsize) {
    error++;
    return STK_FAILED;
  }
  while (remaining > EECHUNK) {
    write_eeprom_chunk(start, EECHUNK);
    start += EECHUNK;
    remaining -= EECHUNK;
  }
  write_eeprom_chunk(start, remaining);
  return STK_OK;
}
// write (length) bytes, (start) is a byte address
uint8_t write_eeprom_chunk(unsigned int start, unsigned int length) {
  // this writes byte-by-byte, page writing may be faster (4 bytes at a time)
  fill(length);
  for (unsigned int x = 0; x < length; x++) {
    unsigned int addr = start + x;
    spi_transaction(0xC0, (addr >> 8) & 0xFF, addr & 0xFF, buffer[x]);
    delay(45);
  }
  return STK_OK;
}

void program_page() {
  char result = (char) STK_FAILED;
  unsigned int length = 256 * getch();                                            // Get high byte length
  length += getch();                                                              // Get low byte length
  char memtype = getch();                                                         // Get type Flash : EEprom
  // flash memory @here, (length) bytes
  if (memtype == 'F') {
    write_flash(length);
    return;
  }
  if (memtype == 'E') {
    result = (char)write_eeprom(length);
    if (CRC_EOP == getch()) {
      //Serial.print((char) STK_INSYNC);
      //Serial.print(result);
      SPI_Out.push((char) STK_INSYNC);
      SPI_Out.push(result);
    } else {
      error++;
      //Serial.print((char) STK_NOSYNC);
      SPI_Out.push((char) STK_NOSYNC);
    }
    return;
  }
  //Serial.print((char)STK_FAILED);
  SPI_Out.push((char)STK_FAILED);
  return;
}

uint8_t flash_read(uint8_t hilo, unsigned int addr) {
  return spi_transaction(0x20 + hilo * 8,
                         (addr >> 8) & 0xFF,
                         addr & 0xFF,
                         0);
}

char flash_read_page(int length) {
  for (int x = 0; x < length; x += 2) {
    uint8_t low = flash_read(LOW, here);
    //Serial.print((char) low);
    SPI_Out.push((char) low);
    uint8_t high = flash_read(HIGH, here);
    //Serial.print((char) high);
    SPI_Out.push((char) high);
    here++;
    houseKeeping();
  }
  return STK_OK;
}

char eeprom_read_page(int length) {
  // here again we have a word address
  int start = here * 2;
  for (int x = 0; x < length; x++) {
    int addr = start + x;
    uint8_t ee = spi_transaction(0xA0, (addr >> 8) & 0xFF, addr & 0xFF, 0xFF);
    //Serial.print((char) ee);
    SPI_Out.push((char) ee);
  }
  return STK_OK;
}

void read_page() {
  char result = (char)STK_FAILED;
  int length = 256 * getch();
  length += getch();
  char memtype = getch();
  if (CRC_EOP != getch()) {
    error++;
    //Serial.print((char) STK_NOSYNC);
    SPI_Out.push((char) STK_NOSYNC);
    return;
  }
  //Serial.print((char) STK_INSYNC);
  SPI_Out.push((char) STK_INSYNC);
  if (memtype == 'F') result = flash_read_page(length);
  if (memtype == 'E') result = eeprom_read_page(length);
  //Serial.print(result);
  SPI_Out.push(result);
}

void read_signature() {
  if (CRC_EOP != getch()) {
    error++;
    //Serial.print((char) STK_NOSYNC);
    SPI_Out.push((char) STK_NOSYNC);
    return;
  }
  //Serial.print((char) STK_INSYNC);
  SPI_Out.push((char) STK_INSYNC);
  uint8_t high = spi_transaction(0x30, 0x00, 0x00, 0x00);
  //Serial.print((char) high);
  SPI_Out.push((char) high);
  uint8_t middle = spi_transaction(0x30, 0x00, 0x01, 0x00);
  //Serial.print((char) middle);
  SPI_Out.push((char) middle);
  uint8_t low = spi_transaction(0x30, 0x00, 0x02, 0x00);
  //Serial.print((char) low);
  //Serial.print((char) STK_OK);
  SPI_Out.push((char) low);
  SPI_Out.push((char) STK_OK);
}
//////////////////////////////////////////
//////////////////////////////////////////

void avrisp() {
  uint8_t ch = getch();
  if (!stopProg) {
    progmode = ch;
    switch (ch) {
      case '0': // signon
        error = 0;
        empty_reply();
        break;
      case '1':
        if (getch() == CRC_EOP) {
          //Serial.print((char) STK_INSYNC);
          //Serial.print("AVR ISP");
          //Serial.print((char) STK_OK);
          SPI_Out.push((char) STK_INSYNC);
          SPI_Out.push('A');
          SPI_Out.push('V');
          SPI_Out.push('R');
          SPI_Out.push(' ');
          SPI_Out.push('I');
          SPI_Out.push('S');
          SPI_Out.push('P');
          SPI_Out.push((char) STK_OK);
        }
        else {
          error++;
          //Serial.print((char) STK_NOSYNC);
          SPI_Out.push((char) STK_NOSYNC);
        }
        break;
      case 'A':
        get_version(getch());
        break;
      case 'B':
        fill(20);
        set_parameters();
        empty_reply();
        break;
      case 'E': // extended parameters - ignore for now
        fill(5);
        empty_reply();
        break;
      case 'P':
        if (!pmode) {
          progblock = 1;
          start_pmode();
        }
        empty_reply();
        break;
      case 'U': // set address (word)
        here = getch();
        here += 256 * getch();
        empty_reply();
        progblock++;
        if (progblock > 8) { progblock = 1; }
        break;

      case 0x60: //STK_PROG_FLASH
        getch(); // low addr
        getch(); // high addr
        empty_reply();
        break;
      case 0x61: //STK_PROG_DATA
        getch(); // data
        empty_reply();
        break;

      case 0x64: //STK_PROG_PAGE
        program_page();
        break;

      case 0x74: //STK_READ_PAGE 't'
        read_page();
        break;

      case 'V': //0x56
        universal();
        break;
      case 'Q': //0x51
        error = 0;
        end_pmode();
        empty_reply();
        break;

      case 0x75: //STK_READ_SIGN 'u'
        read_signature();
        break;

      // expecting a command, not CRC_EOP
      // this is how we can get back in sync
      case CRC_EOP:
        error++;
        //Serial.print((char) STK_NOSYNC);
        SPI_Out.push((char) STK_NOSYNC);
        break;

      // anything else we will return STK_UNKNOWN
      default:
        error++;
        if (CRC_EOP == getch())
          //Serial.print((char)STK_UNKNOWN);
          SPI_Out.push((char)STK_UNKNOWN);
        else
          //Serial.print((char)STK_NOSYNC);
          SPI_Out.push((char)STK_NOSYNC);
    }
  }
}

void avrisp_loop() {
  heartbeat();
  if (!SPI_In.isEmpty()) {
    avrisp();
  }
  if (error) { displayISPAVRProgrammer(); }
}