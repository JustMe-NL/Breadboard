#include <breadboard.h>

// void oneWireScan()
// bool read_scratchpad()
// void oneWireTest()

//------------------------------------------------------------------------------ One wire scan
void oneWireScan() {
  uint8_t addr[8];                          // ROM address of current sensor
  uint8_t buffer0[9];                       // buffers for scratchpad register

  oled.setCursor(0, 32);
  oled.print("scanning ...");

  setValue = 0;
  ds = new OneWire(GPIO1);
  ds->reset_search();
  while (ds->search(addr) || setValue > 7) {  // maximum of 8 devices
    read_scratchpad(addr, buffer0);
    for (uint8_t i = 0; i<8; i++) {
      buffer[(setValue * 0x10)+ i + 1] = addr[i];
    }
    setValue++;
  }
  if (setValue > 0) {
    for (uint8_t i = 0; i < setValue; i++) {
      ds->reset();
      for (uint8_t j = 0; j < 8; j++) {
        addr[j] = buffer[(i * 0x10) + j + 1];
      }
      ds->select(addr);
      ds->write(0x44);
      delay(1000);
      read_scratchpad(addr, buffer0);
      buffer[(i * 0x10) + 9] = buffer0[0];
      buffer[(i * 0x10) + 10] = buffer0[1];
    }
  }
  buffer[0] = setValue;
  setValue = 0;
  delete(ds);
}

//------------------------------------------------------------------------------ test DS18x20 scratchpad
bool read_scratchpad(uint8_t *addr, uint8_t *buff9) {
  ds->reset();
  ds->select(addr);
  ds->write(0xBE); // read scratchpad
  int idx;
  for (idx=0; idx<9; idx++)
    buff9[idx] = ds->read();
  return 0 == OneWire::crc8(buff9, 9);
}

//------------------------------------------------------------------------------ test DS18x20
void oneWireTest() {
  uint8_t addr[8];                          // ROM address of current sensor
  uint8_t buffer0[9];                       // buffers for scratchpad register
  uint8_t buffer1[9];
  uint8_t buffer2[9];
  uint8_t buffer3[9];
  // flag to indicate if validation
  //  should be repeated at a different
  //  sensor temperature
  //bool t_ok;

  for (uint8_t i = 0; i < 11; i++) {
    checklist[i] = false;
  }
  faillures = 0;

  ds = new OneWire(GPIO1);

  ds->reset_search();
  while (ds->search(addr)) {
    int fake_flags = 0;
    if (0 != OneWire::crc8(addr, 8)) {
      // some fake sensors can have their ROM overwritten with
      // arbitrary nonsense, so we don't expect anything good
      // if the ROM doesn't check out
      fake_flags += 1;
      checklist[1] = true;
    }

    if ((addr[6] != 0) || (addr[5] != 0) || (addr[0] != 0x28)) {
      fake_flags += 1;
      checklist[2] = true;
    }    
    
    if (!read_scratchpad(addr, buffer0)) read_scratchpad(addr, buffer0);
    
    if (buffer0[5] != 0xff) {
      fake_flags += 1;
      checklist[3] = true;
    }

    if ( ((buffer0[6] == 0x00) || (buffer0[6] > 0x10)) || // totall wrong value
         ( ((buffer0[0] != 0x50) || (buffer0[1] != 0x05)) && ((buffer0[0] != 0xff) || (buffer0[1] != 0x07)) && // check for valid conversion...
           (((buffer0[0] + buffer0[6]) & 0x0f) != 0x00) ) ) { //...before assessing DS18S20 compatibility.
      fake_flags += 1;
      checklist[4] = true;
    }
    
    if (buffer0[7] != 0x10) {
      fake_flags += 1;
      checklist[5] = true;
    }

    // set the resolution to 10 bit and modify alarm registers    
    ds->reset();
    ds->select(addr);
    ds->write(0x4E); // write scratchpad. MUST be followed by 3 bytes as per datasheet.
    ds->write(buffer0[2] ^ 0xff);
    ds->write(buffer0[3] ^ 0xff);
    ds->write(0x3F);
    ds->reset();

    if (!read_scratchpad(addr, buffer1)) read_scratchpad(addr, buffer1);
    
    if ((buffer1[2] != (buffer0[2] ^ 0xff)) || (buffer1[3] != (buffer0[3] ^ 0xff))) {
      fake_flags += 1;
      checklist[6] = true;
    }

    if (buffer1[4] != 0x3f) {
      fake_flags += 1;
      checklist[7] = true;
    }

    if ((buffer1[5] != buffer0[5]) || (buffer1[6] != buffer0[6]) || (buffer1[7] != buffer0[7])) {
      fake_flags += 1;
      checklist[8] = true;
    }

    // set the resolution to 12 bit
    ds->reset();
    ds->select(addr);
    ds->write(0x4E); // write scratchpad. MUST be followed by 3 bytes as per datasheet.
    ds->write(buffer0[2]);
    ds->write(buffer0[3]);
    ds->write(0x7f);
    ds->reset();

    if (!read_scratchpad(addr, buffer2)) read_scratchpad(addr, buffer2);
    
    if (buffer2[4] != 0x7f) {
      fake_flags += 1;
      checklist[9] = true;
    }

    if ((buffer2[5] != buffer1[5]) || (buffer2[6] != buffer1[6]) || (buffer2[7] != buffer1[7])) {
      fake_flags += 1;
      checklist[10] = true;
    }

    if (( ((buffer2[0] == 0x50) && (buffer2[1] == 0x05)) || ((buffer2[0] == 0xff) && (buffer2[1] == 0x07)) ||
         ((buffer2[6] == 0x0c) && (((buffer2[0] + buffer2[6]) & 0x0f) == 0x00)) ) &&
         ((buffer2[6] >= 0x00) && (buffer2[6] <= 0x10)) ){
      // byte 6 checked out as correct in the initial test but the test ambiguous.
      //   we need to check if byte 6 is consistent after temperature conversion
            
      // We'll do a few temperature conversions in a row.
      // Usually, the temperature rises slightly if we do back-to-back
      //   conversions.
      int count = 5;
      do {
        count -- ;
        if (count < 0)
          break;
        // perform temperature conversion
        ds->reset();
        ds->select(addr);
        ds->write(0x44);
        delay(750);
        
        if (!read_scratchpad(addr, buffer3)) read_scratchpad(addr, buffer3);
        
      } while ( ((buffer3[0] == 0x50) && (buffer3[1] == 0x05)) || ((buffer3[0] == 0xff) && (buffer3[1] == 0x07)) ||
                ((buffer3[6] == 0x0c) && (((buffer3[0] + buffer3[6]) & 0x0f) == 0x00)) );
      if (count < 0) {
        //t_ok = false;
      } else {
        //t_ok = true;
        if ((buffer3[6] != 0x0c) && (((buffer3[0] + buffer3[6]) & 0x0f) == 0x00)) {
        } else {
          fake_flags += 1;
          checklist[11] = true;
        }
      }
    }
    delete(ds);
    faillures = fake_flags;
    setValue = 0;
    if (faillures > 0) { checklist[0] = true; }  
  } // done with all sensors
}