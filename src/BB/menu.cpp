#include <breadboard.h>

// Counter
// Encoder
// I2C Scanner
// Logic blocks
// OneWire
// Pin monitor
// Power
// Programmer
// Pulse generator
// Screensettings
// Serial
// Servo
// Set pins
// Stepper
// Switch debounce
// Voltmeter

//------------------------------------------------------------------------------ main menu structure
void processMenu() {
  if (encoder.up || encoder.down || encoder.sw) {       // Process menu navigation
#ifdef debug
    serialdebug(0);
#endif
    switch (sysState) {

//Counter
      case MENUCOUNTER:                
        if (encoder.up) { sysState = MENUVOLTMETER; }
        if (encoder.down) { sysState = MENUENCODER; }
        if (encoder.sw) { sysState = MENUCOUNTCOUNTER; }
        break;
      case MENUCOUNTCOUNTER:
        if (encoder.up) { sysState = MENUCOUNTEXIT; }
        if (encoder.down) { sysState = MENUCOUNTFREQLOW; }
        if (encoder.sw) { 
          enableGPIO();
          encoder.sw = false;
          encoder.lp = false;
          setValue = 0;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
          sysState = MENUCOUNTCOUNTERACTIVE;
          attachInterrupt(digitalPinToInterrupt(GPIO1), counterInterrupt, RISING);
        }
        break;
      //case MENUCOUNTCOUNTERACTIVE:          //  special
      case MENUCOUNTFREQLOW:
        if (encoder.up) { sysState = MENUCOUNTCOUNTER; }
        if (encoder.down) { sysState = MENUCOUNTFREQHIGH; }
        if (encoder.sw) { 
          enableGPIO();
          digitalWrite(COUNTPINEN, HIGH);
          sysState = MENUCOUNTFREQLOWACTIVE;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
          count1 = 0;
          sum1 = 0;
          timeout = 0;
          freqLowOn = true;
          firstrun = true;
          encoder.sw = false;
          freq1.begin(GPIO1);
        }
        break;
      //case MENUCOUNTFREQLOWACTIVE:             //  special
      case MENUCOUNTFREQHIGH:
        if (encoder.up) { sysState = MENUCOUNTFREQLOW; }
        if (encoder.down) { sysState = MENUCOUNTINFO; }
        if (encoder.sw) { 
          enableGPIO();
          pinMode(COUNTR, INPUT_PULLUP);
          sysState = MENUCOUNTFREQHIGHACTIVE;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
          freqHighOn = true;
          firstrun = true;
          FreqCount.begin(100);
          encoder.sw = false;
        }
        break;
      //case MENUCOUNTFREQHIGHACTIVE:            //  special
      case MENUCOUNTINFO:
        if (encoder.up) { sysState = MENUCOUNTFREQHIGH; }
        if (encoder.down) { sysState = MENUCOUNTEXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUCOUNTINFOACTIVE;
        }
        break;      
      case MENUCOUNTEXIT:
        if (encoder.up) { sysState = MENUCOUNTINFO; }
        if (encoder.down) { sysState = MENUCOUNTCOUNTER; }
        if (encoder.sw) { sysState = MENUCOUNTER; }
        break;
//Encoder
      case MENUENCODER:                
        if (encoder.up) { sysState = MENUCOUNTER; }
        if (encoder.down) { sysState = MENUI2CSCAN; }
        if (encoder.sw) { sysState = MENUENCODERON; }
        break;
      case MENUENCODERON:             
        if (encoder.up) { sysState = MENUENCODEREXIT; }
        if (encoder.down) { sysState = MENUENCODERINFO; }
        if (encoder.sw) { 
          enableGPIO();
          pinMode(GPIO1, OUTPUT);
          pinMode(GPIO2, OUTPUT);
          sysState = MENUENCODERACTIVE;
          menuoptions = OPTIONSOK;
          myEnc.setCopy(true);
          encOn = true;
        }
        break;
      case MENUENCODERINFO:
        if (encoder.up) { sysState = MENUENCODERON; }
        if (encoder.down) { sysState = MENUENCODEREXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUENCODERINFOACTIVE;
        }
        break;
      case MENUENCODEREXIT:
        if (encoder.up) { sysState = MENUENCODERINFO; }
        if (encoder.down) { sysState = MENUENCODERON; }
        if (encoder.sw) { sysState = MENUENCODER; }
        break; 
      //case MENUENCODERACTIVE:          //  special
//I2CScanner
      case MENUI2CSCAN:                
        if (encoder.up) { sysState = MENUENCODER; }
        if (encoder.down) { sysState = MENULOGIC; }
        if (encoder.sw) { sysState = MENUI2CSCANPULLUPONOFF; }
        break;
      case MENUI2CSCANPULLUPONOFF:
        if (encoder.up) { sysState = MENUI2CSCANEXIT; }
        if (encoder.down) { sysState = MENUI2CSCANSTART; }
        if (encoder.sw) { 
          if (i2cPullUpActive) {
            i2cPullUp(false);
          } else {
            i2cPullUp(true);
          }
        }
        break;
      case MENUI2CSCANSTART:           
        if (encoder.up) { sysState = MENUI2CSCANPULLUPONOFF; }
        if (encoder.down) { sysState = MENUI2CSCANINFO; }
        if (encoder.sw) { 
          enableGPIO();
          oled.setCursor(0, 32);
          oled.fillRect(0, 16, 128, 32, SSD1306_BLACK);
          oled.print("Scanning ...");
          oled.display();
          i2c.setDelay_us(4);
          i2c.setTimeout_ms(40);
          i2c.begin();
          uint8_t j = 1;
          buffer[0] = 0;
          for (uint8_t i = 0; i < 127; i++) {
            uint8_t startResult = i2c.llStart((i << 1) + 1); // Signal a read
            i2c.stop();
            if (startResult == 0) { 
              buffer[j] = i;
              buffer[0] = j;
              j++;
            }
          }
          oled.setTextColor(SSD1306_WHITE);
          i2c.end();
          sysState = MENUI2CSCANCOMPLETE;
          menuoptions = OPTIONSOKSET;
          options = OPTIONOK;
          encoder.sw = false;
          i2cpointer = 0;
          forcedisplay++;
        }
        break;
      //case MENUI2CSCANCOMPLETE:        //  special
      case MENUI2CSCANINFO:
        if (encoder.up) { sysState = MENUI2CSCANSTART; }
        if (encoder.down) { sysState = MENUI2CSCANEXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUI2CSCANINFOACTIVE;
        }
        break;
      case MENUI2CSCANEXIT:            
        if (encoder.up) { sysState = MENUI2CSCANINFO; }
        if (encoder.down) { sysState = MENUI2CSCANPULLUPONOFF; }
        if (encoder.sw) { 
          i2cPullUp(false);
          sysState = MENUI2CSCAN;
        }
        break;
//Logic blocks
      case MENULOGIC:
        if (encoder.up) { sysState = MENUI2CSCAN; }
        if (encoder.down) { sysState = MENUONEWIRE; }
        if (encoder.sw) { sysState = MENULOGICONOFF; }
        break;
      case MENULOGICONOFF:
        if (encoder.up) { sysState = MENULOGICEXIT; }
        if (encoder.down) { sysState = MENULOGICINFO; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENULOGICACTIVE; 
        }
        break;
      case MENULOGICINFO:
        if (encoder.up) { sysState = MENULOGICONOFF; }
        if (encoder.down) { sysState = MENULOGICEXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENULOGICINFOACTIVE; 
        }
        break;
      case MENULOGICEXIT:
        if (encoder.up) { sysState = MENULOGICINFO; }
        if (encoder.down) { sysState = MENULOGICONOFF; }
        if (encoder.sw) { sysState = MENULOGIC; }
        break;
//OneWire
      case MENUONEWIRE:               
        if (encoder.up) { sysState = MENULOGIC; }
        if (encoder.down) { sysState = MENUPINMONITOR; }
        if (encoder.sw) { sysState = MENUONEWIRESCAN; }
        break;
      case MENUONEWIREPULLUPONOFF:
        if (encoder.up) { sysState = MENUONEWIREEXIT; }
        if (encoder.down) { sysState = MENUONEWIRESCAN; }
        if (encoder.sw) { 
          if (owPullUpActive) {
            oneWirePullUp(false);
          } else {
            oneWirePullUp(true);
          }
        }
        break;   
      case MENUONEWIRESCAN:
        if (encoder.up) { sysState = MENUONEWIREPULLUPONOFF; }
        if (encoder.down) { sysState = MENUONEWIRETEST; }
        if (encoder.sw) { 
          enableGPIO();
          oneWireScan();
          encoder.sw = false;
          options = OPTIONCANCEL;
          menuoptions = OPTIONSOKSET;
          sysState = MENUONEWIRESCANACTIVE;
        }
        break;
      case MENUONEWIRETEST:
        if (encoder.up) { sysState = MENUONEWIRESCAN; }
        if (encoder.down) { sysState = MENUONEWIREINFO; }
        if (encoder.sw) { 
          enableGPIO();
          oneWireTest();
          encoder.sw = false;
          options = OPTIONCANCEL;
          menuoptions = OPTIONSOKSET;
          sysState = MENUONEWIRETESTACTIVE;
        }
        break;
      case MENUONEWIREINFO:
        if (encoder.up) { sysState = MENUONEWIRETEST; }
        if (encoder.down) { sysState = MENUONEWIREEXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUONEWIREINFOACTIVE;
        }
        break;
      case MENUONEWIREEXIT:
        if (encoder.up) { sysState = MENUONEWIREINFO; }
        if (encoder.down) { sysState = MENUONEWIREPULLUPONOFF; }
        if (encoder.sw) { 
          oneWirePullUp(false);
          sysState = MENUONEWIRE; 
        }
        break;
//Pin monitor
      case MENUPINMONITOR:             
        if (encoder.up) { sysState = MENUONEWIRE; }
        if (encoder.down) { sysState = MENUPOWER; }
        if (encoder.sw) { sysState = MENUPINMONITORONOFF; }
        break;
      case MENUPINMONITORONOFF:
        if (encoder.up) { sysState = MENUPINMONITOREXIT; }
        if (encoder.down) { sysState = MENUPINMONITOR7SEGMENT; }
        if (encoder.sw) { 
          sysState = MENUPINMONITORACTIVE;
          menuoptions = OPTIONSOKSET;
          options = OPTIONOK;
          disableGPIO();
          writeToExpander(0xFF);
          CaptureBuf.clear();
          pinmonOn = true;
          encoder.sw = false;
          readFromExpander();
        }
        break;
      case MENUPINMONITOR7SEGMENT:
        if (encoder.up) { sysState = MENUPINMONITORONOFF; }
        if (encoder.down) { sysState = MENUPINMONITORCAPTURE; }
        if (encoder.sw) { 
          sysState = MENUPINMONITOR7SEGMENTACTIVE;
          menuoptions = OPTIONSOKSET;
          options = OPTIONOK;
          disableGPIO();
          writeToExpander(0xFF);
          CaptureBuf.clear();
          pinmonOn = true;
          encoder.sw = false;
          readFromExpander();
        }
        break;
      case MENUPINMONITORCAPTURE:
        if (encoder.up) { sysState = MENUPINMONITOR7SEGMENT; }
        if (encoder.down) { sysState = MENUPINMONITORINFO; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUPINMONITORCAPTUREACTIVE;
        }
        break;
        break;
      case MENUPINMONITORINFO:
        if (encoder.up) { sysState = MENUPINMONITORCAPTURE; }
        if (encoder.down) { sysState = MENUPINMONITOREXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUPINMONITORINFOACTIVE;
        }
        break;
      case MENUPINMONITOREXIT:
        if (encoder.up) { sysState = MENUPINMONITORINFO; }
        if (encoder.down) { sysState = MENUPINMONITORONOFF; }
        if (encoder.sw) { sysState = MENUPINMONITOR; }
        break;
//Power
      case MENUPOWER:                  
        if (encoder.up) { sysState = MENUPINMONITOR; }
        if (encoder.down) { sysState = MENUPROGRAMMER; }
        if (encoder.sw) { sysState = MENUPOWERONOFF; }
        break;
      case MENUPOWERONOFF:             
        if (encoder.up) { sysState = MENUPOWEREXIT; }
        if (encoder.down) { sysState = MENUPOWERSETVOLTAGE; }
        //if (encoder.sw) { sysState = MENUPOWERONOFFPROCESS; }
        if (encoder.sw) { 
          if (control.powerRelay) {
            control.powerRelay = false;
          } else { 
            control.powerRelay = true; 
          }
          writeToExpander(writeByte, true);
        }
        break;
      //case MENUPOWERONOFFPROCESS:
      case MENUPOWERSETVOLTAGE:        
        if (encoder.up) { sysState = MENUPOWERONOFF; }
        if (encoder.down) { sysState = MENUPOWERSETCURRENT; }
        //if (encoder.sw) { sysState = MENUPOWERSETVOLTAGEACTIVE; }
        break;
      //case MENUPOWERSETVOLTAGEACTIVE:
      case MENUPOWERSETCURRENT:        
        if (encoder.up) { sysState = MENUPOWERSETVOLTAGE; }
        if (encoder.down) { sysState = MENUPOWERSETCUTOFF; }
        if (encoder.sw) { 
          sysState = MENUPOWERSETCURRENTACTIVE;
          inputType = INPUTNUMBER;
          menuoptions = OPTIONSOKSETCANCEL;
          maxValue = 5000;
          setUserValue(setAmp, 50);
        }
        break;
      //case MENUPOWERSETCURRENTACTIVE:
      case MENUPOWERSETCUTOFF:         
        if (encoder.up) { sysState = MENUPOWERSETCURRENT; }
        if (encoder.down) { sysState = MENUPOWERINFO; }
        //if (encoder.sw) { sysState = MENUPOWERSETCUTOFFACTIVE; }
        break;
      //case MENUPOWERSETCUTOFFACTIVE:
      case MENUPOWERINFO:
        if (encoder.up) { sysState = MENUPOWERSETCUTOFF; }
        if (encoder.down) { sysState = MENUPOWEREXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUPOWERINFOACTIVE;
        }
        break;
      case MENUPOWEREXIT:              
        if (encoder.up) { sysState = MENUPOWERINFO; }
        if (encoder.down) { sysState = MENUPOWERONOFF; }
        if (encoder.sw) { sysState = MENUPOWER; }
        break;
//Programmer
      case MENUPROGRAMMER:
        if (encoder.up) { sysState = MENUPOWER; }
        if (encoder.down) { sysState = MENUFREQ; }
        if (encoder.sw) { sysState = MENUPROGISPAVRONOFF; }
        break;
      case MENUPROGISPAVRONOFF:
        if (encoder.up) { sysState = MENUPROGRAMMEREXIT; }
        if (encoder.down) { sysState = MENUPROGESPONOFF; }
        if (encoder.sw) { 
          enableUSB2Serial(USBSPI);
          disableGPIO();
          stopProg = false;
          firstrun = true;
          sysState = MENUPROGISPAVRACTIVE;
        }
        break;
      case MENUPROGESPONOFF:
        if (encoder.up) { sysState = MENUPROGISPAVRONOFF; }
        if (encoder.down) { sysState = MENUPROGRAMMERINFO; }
        if (encoder.sw) { 
          enableUSB2Serial(USBSERIAL);
          disableGPIO();
          stopProg = false;
          firstrun = true;
          sysState = MENUPROGESPACTIVE;
        }
        break;      
      case MENUPROGRAMMERINFO:
        if (encoder.up) { sysState = MENUPROGESPONOFF; }
        if (encoder.down) { sysState = MENUPROGRAMMEREXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUPROGRAMMERINFOACTIVE; 
        }
        break;
      case MENUPROGRAMMEREXIT:
        if (encoder.up) { sysState = MENUPROGRAMMERINFO; }
        if (encoder.down) { sysState = MENUPROGISPAVRONOFF; }
        if (encoder.sw) { sysState = MENUPROGRAMMER; }
        break;
//PulseGenerator
      case MENUFREQ:                 
        if (encoder.up) { sysState = MENUPROGRAMMER; }
        if (encoder.down) { sysState = MENUSERIAL; }
        if (encoder.sw) { sysState = MENUFREQONOFF; } 
        break;
      case MENUFREQONOFF:
        if (encoder.up) { sysState = MENUFREQEXIT; }
        if (encoder.down) { sysState = MENUFREQINFO; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUFREQSETACTIVE;
        }
        break;
      //case MENUFREQSETACTIVE:          //  special
      case MENUFREQINFO:
        if (encoder.up) { sysState = MENUFREQONOFF; }
        if (encoder.down) { sysState = MENUFREQEXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUFREQINFOACTIVE;
        }
        break;
      case MENUFREQEXIT:             
        if (encoder.up) { sysState = MENUFREQINFO; }
        if (encoder.down) { sysState = MENUFREQONOFF; }
        if (encoder.sw) { sysState = MENUFREQ; }
        break;
//Serial
      case MENUSERIAL:                 
        if (encoder.up) { sysState = MENUFREQ; }
        if (encoder.down) { sysState = MENUSERVO; }
        if (encoder.sw) { sysState = MENUSERIALSETBAUD; }
        break;
      case MENUSERIALSETBAUD:         
        if (encoder.up) { sysState = MENUSERIALEXIT; }
        if (encoder.down) { sysState = MENUSERIALONOFF; }
        if (encoder.sw) { 
          sysState = MENUSERIALSETBAUDACTIVE;
          menuoptions = OPTIONSOKSETCANCEL;
          inputType = INPUTBAUD; 
          maxValue = baudRates[(sizeof(baudRates)/sizeof(baudRates[0])) - 1];
          setUserValue(baudRate, 1);
        }
        break;
      //case MENUSERIALSETBAUDACTIVE:    //  special
      case MENUSERIALONOFF:           
        if (encoder.up) { sysState = MENUSERIALSETBAUD; }
        if (encoder.down) { sysState = MENUSERIALINFO; }
        if (encoder.sw) {
          if (control.serialRelay) {
            disableSlaveModes();
          } else {
            enableUSB2Serial(USBSERIAL);
          }
        }
        break;
      case MENUSERIALINFO:
        if (encoder.up) { sysState = MENUSERIALONOFF; }
        if (encoder.down) { sysState = MENUSERIALEXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUSERIALINFOACTIVE;
        }
        break;
      case MENUSERIALEXIT:             
        if (encoder.up) { sysState = MENUSERIALINFO; }
        if (encoder.down) { sysState = MENUSERIALSETBAUD; }
        if (encoder.sw) { sysState = MENUSERIAL; }
        break;
//Servo
      case MENUSERVO:                 
        if (encoder.up) { sysState = MENUSERIAL; }
        if (encoder.down) { sysState = MENUSETPINS; }
        if (encoder.sw) { sysState = MENUSERVOONOFF; }
        break;
      case MENUSERVOONOFF:
        if (encoder.up) { sysState = MENUSERVOEXIT; }
        if (encoder.down) { sysState = MENUSERVOINFO; }
        if (encoder.sw) { 
          firstrun = true;
          enableGPIO();
          sysState = MENUSERVOACTIVE;
          menuoptions = OPTIONSOKSET;
          options = OPTIONOK;
        }
        break;
      //case MENUSERVOACTIVE:
      case MENUSERVOINFO:
        if (encoder.up) { sysState = MENUSERVOONOFF; }
        if (encoder.down) { sysState = MENUSERVOEXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUSERVOINFOACTIVE; 
        }
        break;
      //case MENUSERVOINFOACTIVE:
      case MENUSERVOEXIT:
        if (encoder.up) { sysState = MENUSERVOINFO; }
        if (encoder.down) { sysState = MENUSERVOONOFF; }
        if (encoder.sw) { sysState = MENUSERVO; }
        break;
//SetPins
      case MENUSETPINS:             
        if (encoder.up) { sysState = MENUSERVO; }
        if (encoder.down) { sysState = MENUSTEPPER; }
        if (encoder.sw) { sysState = MENUSETPINSONOFF; }
        break;
      case MENUSETPINSONOFF:
        if (encoder.up) { sysState = MENUSETPINSEXIT; }
        if (encoder.down) { sysState = MENUSETPINSINFO; }
        if (encoder.sw) { 
          sysState = MENUSETPINSSETVALUEACTIVE;
          menuoptions = OPTIONSOKSET;
          options = OPTIONOK;
          inputType = INPUTBINARY;
          disableGPIO();
          maxValue = 11111111;
          setUserValue(0, 1);
        }
        break;
      case MENUSETPINSINFO:
        if (encoder.up) { sysState = MENUSETPINSONOFF; }
        if (encoder.down) { sysState = MENUSETPINSEXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUSETPINSINFOACTIVE; 
        }
        break;
      case MENUSETPINSEXIT:
        if (encoder.up) { sysState = MENUSETPINSINFO; }
        if (encoder.down) { sysState = MENUSETPINSONOFF; }
        if (encoder.sw) { sysState = MENUSETPINS; }
        break;
//Stepper
      case MENUSTEPPER:              
        if (encoder.up) { sysState = MENUSETPINS; }
        if (encoder.down) { sysState = MENUSWDEBOUNCE; }
        if (encoder.sw) { sysState = MENUSTEPPERONOFF; }
        break;
      case MENUSTEPPERONOFF:
        if (encoder.up) { sysState = MENUSTEPPEREXIT; }
        if (encoder.down) { sysState = MENUSTEPPERINFO; }
        if (encoder.sw) { 
          firstrun = true;
          disableGPIO();
          menuoptions = OPTIONSOKSET;
          options = OPTIONOK;
          sysState = MENUSTEPPERACTIVE; 
        }
        break;
      case MENUSTEPPERINFO:
        if (encoder.up) { sysState = MENUSTEPPERONOFF; }
        if (encoder.down) { sysState = MENUSTEPPEREXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUSTEPPERINFOACTIVE; 
        }
        break;
      case MENUSTEPPEREXIT:
        if (encoder.up) { sysState = MENUSTEPPERINFO; }
        if (encoder.down) { sysState = MENUSTEPPERONOFF; }
        if (encoder.sw) { sysState = MENUSTEPPER; }
        break;
//SwitchDebounce
      case MENUSWDEBOUNCE:             
        if (encoder.up) { sysState = MENUSTEPPER; }
        if (encoder.down) { sysState = MENUVOLTMETER; }
        if (encoder.sw) { sysState = MENUSWDEBOUNCEONOFF; }
        break;
      case MENUSWDEBOUNCEONOFF:        
        if (encoder.up) { sysState = MENUSWDEBOUNCEEXIT; }
        if (encoder.down) { sysState = MENUSWDEBOUNCEINFO; }
        if (encoder.sw) { 
          if (control.switchRelay) {
            control.switchRelay = false;
          } else {
            control.switchRelay = true;            
          }
          writeToExpander(bytePins, true);
        }
        break;
      case MENUSWDEBOUNCEINFO:
        if (encoder.up) { sysState = MENUSERIALONOFF; }
        if (encoder.down) { sysState = MENUSWDEBOUNCEEXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUSWDEBOUNCEINFOACTIVE;
        }
        break;        
      case MENUSWDEBOUNCEEXIT:         
        if (encoder.up) { sysState = MENUSWDEBOUNCEINFO; }
        if (encoder.down) { sysState = MENUSWDEBOUNCEONOFF; }
        if (encoder.sw) { sysState = MENUSWDEBOUNCE; }
        break;
//Voltmeter
      case MENUVOLTMETER:
        if (encoder.up) { sysState = MENUSWDEBOUNCE; }
        if (encoder.down) { sysState = MENUCOUNTER; }
        if (encoder.sw) { sysState = MENUVOLTMETERONOFF; }
        break;
      case MENUVOLTMETERONOFF:        
        if (encoder.up) { sysState = MENUVOLTMETEREXIT; }
        if (encoder.down) { sysState = MENUVOLTMETERINFO; }
        if (encoder.sw) { 
          encoder.sw = false;
          sysState = MENUVOLTMETERACTIVE; 
        }      
        break;
      case MENUVOLTMETERINFO:
        if (encoder.up) { sysState = MENUVOLTMETERONOFF; }
        if (encoder.down) { sysState = MENUVOLTMETEREXIT; }
        if (encoder.sw) { 
          firstrun = true;
          sysState = MENUVOLTMETERINFOACTIVE;
        }
        break;        
      case MENUVOLTMETEREXIT:         
        if (encoder.up) { sysState = MENUVOLTMETERINFO; }
        if (encoder.down) { sysState = MENUVOLTMETERONOFF; }
        if (encoder.sw) { sysState = MENUVOLTMETER; }
        break;
      default:
        break;
    }
//End
    forcedisplay++;
  }   // If encoder activity



  if (forcedisplay > 0) {                              
    oled.fillRect(0, 16, 128, 33, SSD1306_BLACK);
    oled.setCursor(0, 28);
    switch (sysState) {

//Counter
      case MENUCOUNTER:                
        sharedNavigation();
        oled.print(F("Counter"));
        break;
      case MENUCOUNTCOUNTER:
        sharedNavigation();
        oled.println(F("Counter:"));
        oled.print(F(" Simple counter"));
        break;
      case MENUCOUNTCOUNTERACTIVE:
        showCounter();
        break;
      case MENUCOUNTFREQLOW:
        sharedNavigation();
        oled.println(F("Counter:"));
        oled.print(F(" Measure 0.1-10kHz"));
        break;
      case MENUCOUNTFREQLOWACTIVE:
        showFreqLowCount();
        if ((options == OK) || (options == CANCEL)) { 
          freqLowOn = false;
          freq1.end();
          digitalWrite(COUNTPINEN, LOW);
          sysState = MENUCOUNTFREQLOW;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
        }
        break;
      case MENUCOUNTFREQHIGH:
        sharedNavigation();
        oled.println(F("Counter:"));
        oled.print(F(" Measure 1k-50MHz"));
        break;
      case MENUCOUNTFREQHIGHACTIVE:
        showFreqHighCount();
        if ((options == OK) || (options == CANCEL)) { 
          freqHighOn = false;
          FreqCount.end();
          changePin(COUNTR, false);
          sysState = MENUCOUNTFREQHIGH;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
        }
        break;
      case MENUCOUNTINFO:
        sharedNavigation();
        oled.println(F("Counter:"));
        oled.print(F(" Information"));
        break;
      case MENUCOUNTINFOACTIVE:
        showInfo(infoCounter);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUCOUNTINFO;
          options = OPTIONOK;
        }
        break;
      case MENUCOUNTEXIT:
        sharedNavigation();
        oled.println(F("Counter:"));
        oled.print(F(" Exit counter"));
        break;

//Encoder
      case MENUENCODER:                
        sharedNavigation();
        oled.print(F("Encoder"));
        break;
      case MENUENCODERON:        
        sharedNavigation();
        oled.println(F("Encoder:"));
        oled.print(F(" Turn on (3sec: off)"));
        break;
      case MENUENCODERACTIVE:          //  special
        if (encoder.up || encoder.down || encoder.sw) { oled.print(oldPosition); }
        break;
      case MENUENCODERINFO:
        sharedNavigation();
        oled.println(F("Encoder:"));
        oled.print(F(" Information"));
        break;
      case MENUENCODERINFOACTIVE:
        showInfo(infoEncoder);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUENCODERINFO;
          options = OPTIONOK;
        }
        break;
      case MENUENCODEREXIT:          
        sharedNavigation();
        oled.println(F("Encoder:"));
        oled.print(F(" Exit encoder"));
        break;

//I2CScanner
      case MENUI2CSCAN:               
        sharedNavigation();
        oled.print(F("I2C scanner"));
        break;
      case MENUI2CSCANPULLUPONOFF:         
        sharedNavigation();
        oled.print(F("I2C scanner"));
        if (i2cPullUpActive) { oled.println(": (PU)"); } else { oled.println(":"); }
        oled.print(F(" Set pull-Ups "));
        if (i2cPullUpActive) { oled.print("off"); } else { oled.print("on"); }
        break;
      case MENUI2CSCANSTART:         
        sharedNavigation();
        oled.print(F("I2C scanner"));
        if (i2cPullUpActive) { oled.println(": (PU)"); } else { oled.println(":"); }
        oled.print(F(" Start (3:sda-4:scl)"));
        break;
      case MENUI2CSCANCOMPLETE:        //  special
        showI2CResults();
        if ((options == OK) || (options == CANCEL)) { 
          sysState = MENUI2CSCANSTART;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
        }
        break;
      case MENUI2CSCANINFO:
        sharedNavigation();
        oled.print(F("I2C scanner"));
        if (i2cPullUpActive) { oled.println(": (PU)"); } else { oled.println(":"); }
        oled.print(F(" Information"));
        break;
      case MENUI2CSCANINFOACTIVE:
        showInfo(infoI2CScan);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUI2CSCANINFO;
          options = OPTIONOK;
        }
        break;
      case MENUI2CSCANEXIT:           
        sharedNavigation();
        oled.print(F("I2C scanner"));
        if (i2cPullUpActive) { oled.println(": (PU)"); } else { oled.println(":"); }
        oled.print(F(" Exit I2C"));
        break;

//Lgic Blocks
      case MENULOGIC:           
        sharedNavigation();
        oled.print(F("Logic blocks"));
        break;
      case MENULOGICONOFF:
        sharedNavigation();
        oled.println(F("Logic blocks:"));
        oled.print(F(" Emulate logic"));
        break;
      case MENULOGICACTIVE:
        displayLogic();
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENULOGICONOFF;
          options = OPTIONOK;
        }
        break;
      case MENULOGICINFO:
        sharedNavigation();
        oled.println(F("Logic blocks:"));
        oled.print(F(" Information"));
        break;
      case MENULOGICINFOACTIVE:
        showInfo(infoLogBlocks);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENULOGICINFO;
          options = OPTIONOK;
        }
        break;
      case MENULOGICEXIT:
        sharedNavigation();
        oled.println(F("Logic blocks:"));
        oled.print(F(" Exit logic blocks"));
        break;

//OneWire
      case MENUONEWIRE:
        sharedNavigation();
        oled.print(F("One wire"));
        break;
      case MENUONEWIREPULLUPONOFF:         
        sharedNavigation();
        oled.println(F("One wire:"));
        oled.print(F(" Pull-Ups "));
        if (owPullUpActive) { oled.print("on"); } else { oled.print("off"); }
        break;
      case MENUONEWIRESCAN:
        sharedNavigation();
        oled.println(F("One wire:"));
        oled.print(F(" Scan for DS18B20"));
        break;
      case MENUONEWIRESCANACTIVE:
        showOneWireScanResults();
        if ((options == OK) || (options == CANCEL)) { 
          sysState = MENUONEWIRESCAN;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
        }        
        break;
      case MENUONEWIRETEST:
        sharedNavigation();
        oled.println(F("One wire:"));
        oled.print(F(" Check DS18B20"));
        break;
      case MENUONEWIRETESTACTIVE:
        showOneWireTestResults();
        if ((options == OK) || (options == CANCEL)) { 
          sysState = MENUONEWIRETEST;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
        }
        break;
      case MENUONEWIREINFO:
        sharedNavigation();
        oled.println(F("One wire:"));
        oled.print(F(" Information"));
        break;
      case MENUONEWIREINFOACTIVE:
        showInfo(infoOneWire);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUONEWIREINFO;
          options = OPTIONOK;
        }
        break;
      case MENUONEWIREEXIT:
        sharedNavigation();
        oled.println(F("One wire:"));
        oled.print(F(" Exit one wire"));
        break;

//PinMonitor
      case MENUPINMONITOR:            
        sharedNavigation();
        oled.print(F("Pin monitor"));
        break;
      case MENUPINMONITORONOFF:
        sharedNavigation();
        oled.println(F("Pin monitor:"));
        oled.print(F(" Live monitor"));
        break;     
      case MENUPINMONITORACTIVE:
        pinMonitor();
        if (options == CANCEL || options == OK) {
          pinmonOn = false;
          menuoptions = OPTIONSOK;
          sysState = MENUPINMONITOR;
          options = OPTIONOK;
        }
        break;
      case MENUPINMONITOR7SEGMENT:
        sharedNavigation();
        oled.println(F("Pin monitor:"));
        oled.print(F(" 7 segments"));
        break;  
      case MENUPINMONITOR7SEGMENTACTIVE:
        display7Segment();
        if (options == CANCEL || options == OK) {
          pinmonOn = false;
          menuoptions = OPTIONSOK;
          sysState = MENUPINMONITOR7SEGMENT;
          options = OPTIONOK;
        }
        break;
      case MENUPINMONITORCAPTURE:
        sharedNavigation();
        oled.println(F("Pin monitor:"));
        oled.print(F(" Capture pins"));
        break;
      case MENUPINMONITORCAPTUREACTIVE:
        break;
      case MENUPINMONITORINFO:
        sharedNavigation();
        oled.println(F("Pin monitor:"));
        oled.print(F(" Information"));
        break;
      case MENUPINMONITORINFOACTIVE:
        showInfo(infoPinmon);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUONEWIREINFO;
          options = OPTIONOK;
        }
        break;      
      case MENUPINMONITOREXIT:
        sharedNavigation();
        oled.println(F("Pin monitor:"));
        oled.print(F(" Exit pin monitor"));
        break;

//Power
      case MENUPOWER:                 
        sharedNavigation();
        oled.print(F("Powersettings"));
        break;
      case MENUPOWERONOFF:             
        sharedNavigation();
        oled.println(F("Powersettings:"));
        oled.print(F(" Turn power "));
        if (control.powerRelay) { oled.print("off"); } else { oled.print("on"); }
        break;
      case MENUPOWERONOFFPROCESS:      
        break;
      case MENUPOWERSETVOLTAGE:        
        sharedNavigation();
        oled.println(F("Powersettings:"));
        oled.print(F(" Set voltage"));
        break;
      case MENUPOWERSETVOLTAGEACTIVE: 
        getUserValue();
        if (options == OK) { 
          //setVolt = setValue;
          sysState = MENUPOWERSETVOLTAGE;
        }
        if (options == CANCEL) { sysState = MENUPOWERSETVOLTAGE; }
        break;
      case MENUPOWERSETCURRENT:        
        sharedNavigation();
        oled.println(F("Powersettings:"));
        oled.print(F(" Set current"));
        break;
      case MENUPOWERSETCURRENTACTIVE:  
        getUserValue();
        if (options == OK) { 
          setAmp = setValue;
          sysState = MENUPOWERSETCURRENT;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
        }
        if (options == CANCEL) { 
          sysState = MENUPOWERSETCURRENT;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
        }
        break;
      case MENUPOWERSETCUTOFF:         
        sharedNavigation();
        oled.println(F("Powersettings:"));
        oled.print(F(" Set currentlimit"));
        break;
      case MENUPOWERSETCUTOFFACTIVE:   
        getUserValue();
        if (options == OK) { 
          setAmpCO = setValue;
          sysState = MENUPOWERSETCUTOFF;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
        }
        if (options == CANCEL) { 
          sysState = MENUPOWERSETCUTOFF;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
        }
        break;
      case MENUPOWERINFO:
        sharedNavigation();
        oled.println(F("Powersettings:"));
        oled.print(F(" Information"));
        break;
      case MENUPOWERINFOACTIVE:
        showInfo(infoPower);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUPOWERINFO;
          options = OPTIONOK;
        }
        break;
      case MENUPOWEREXIT:              
        sharedNavigation();
        oled.println(F("Powersettings:"));
        oled.print(F(" Exit power"));
        break;

//Programmer
      case MENUPROGRAMMER:
        sharedNavigation();
        oled.print(F("Programmer"));
        break;
      case MENUPROGISPAVRONOFF:
        sharedNavigation();
        oled.println(F("Programmer:"));
        oled.print(F(" Start ISPAVR"));
        break;
      case MENUPROGISPAVRACTIVE:
        displayISPAVRProgrammer();
        break;
      case MENUPROGESPONOFF:
        sharedNavigation();
        oled.println(F("Programmer:"));
        oled.print(F(" Start ESPxxxx"));
        break;
      case MENUPROGESPACTIVE:
        displayESPProgrammer();
        break;
      case MENUPROGRAMMERINFO:
        sharedNavigation();
        oled.println(F("Programmer:"));
        oled.print(F(" Information"));
        break;
      case MENUPROGRAMMERINFOACTIVE:
        showInfo(infoAVRISP);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUPROGRAMMERINFO;
          options = OPTIONOK;
        }
        break;
      case MENUPROGRAMMEREXIT:
        sharedNavigation();
        oled.println(F("Programmer:"));
        oled.print(F(" Exit programmer"));
        break;

//Pulsegenerator
      case MENUFREQ:                   
        sharedNavigation();
        oled.print(F("Pulse generator"));
        break;
      case MENUFREQONOFF:
        sharedNavigation();
        oled.println(F("Pulse generator:"));
        oled.print(F(" Generator on"));
        break;        
      case MENUFREQSETACTIVE:          
        pulseGenerator();
        if ((options == OK) || (options == CANCEL)) { 
          sysState = MENUFREQONOFF;
          menuoptions = OPTIONSOK;
          options = OPTIONOK;
          disableGPIO();
        }
        break;
      case MENUFREQINFO:
        sharedNavigation();
        oled.println(F("Pulse generator:"));
        oled.print(F(" Information"));
        break;        
      case MENUFREQINFOACTIVE:
        showInfo(infoPulseGen);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUFREQINFO;
          options = OPTIONOK;
        }
        break;      
      case MENUFREQEXIT:
        sharedNavigation();
        oled.println(F("Pulse generator:"));
        oled.print(F(" Exit generator"));
        break;  

//Serial
      case MENUSERIAL:                 
        sharedNavigation();
        oled.print(F("Serial"));
        break;
      case MENUSERIALSETBAUD:          
        sharedNavigation();
        oled.println(F("Serial:"));
        oled.print(F(" Set baudrate"));
        break;
      case MENUSERIALSETBAUDACTIVE:    //  special
          getUserValue();
          if (options == OK) { 
            baudRate = setValue;
            menuoptions = OPTIONSOK;
            sysState = MENUSERIALSETBAUD;
            options = OPTIONOK;
          }
          if (options == CANCEL) { 
            menuoptions = OPTIONSOK;
            sysState = MENUSERIALSETBAUD; 
            options = OPTIONOK;
          }
        break;
      case MENUSERIALONOFF:            
        sharedNavigation();
        oled.println(F("Serial:"));
        oled.print(F(" Serial "));
        if (control.serialRelay) { oled.print("off (1:R,2:T)"); } else { oled.print("on (1:R,2:T)"); }        
        break;
      case MENUSERIALINFO:
        sharedNavigation();
        oled.println(F("Serial:"));
        oled.print(F(" Information"));
        break;
      case MENUSERIALINFOACTIVE:
        showInfo(infoSerialPT);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUSERIALINFO;
          options = OPTIONOK;
        }
        break;
      case MENUSERIALEXIT:             
        sharedNavigation();
        oled.println(F("Serial:"));
        oled.print(F(" Exit serial"));
        break;

//Servo
      case MENUSERVO:                  
        sharedNavigation();
        oled.print(F("Servo"));
        break;
      case MENUSERVOONOFF:
        sharedNavigation();
        oled.println(F("Servo:"));
        oled.print(F(" Servo on"));
        break;
      case MENUSERVOACTIVE:  
        displayServo();
        if (options == OK) {
          menuoptions = OPTIONSOK;
          sysState = MENUSERVOONOFF;
          options = OPTIONOK;
        }         
        break;
      case MENUSERVOINFO:
        sharedNavigation();
        oled.println(F("Servo:"));
        oled.print(F(" Information"));
        break;
      case MENUSERVOINFOACTIVE:
        showInfo(infoServo);
        if (options == OK) { 
          disableGPIO();
          menuoptions = OPTIONSOK;
          sysState = MENUSERIALINFO;
          options = OPTIONOK;
        }
        break;
      case MENUSERVOEXIT:
        sharedNavigation();
        oled.println(F("Servo:"));
        oled.print(F(" Exit servo"));
        break;

//Set Pins
      case MENUSETPINS:                
        sharedNavigation();
        oled.print(F("Set pins"));
        break;
      case MENUSETPINSONOFF:
        sharedNavigation();
        oled.println(F("Set pins:"));
        oled.print(F(" set value"));
        break;
      case MENUSETPINSSETVALUEACTIVE:  //  special
          getUserValue();
          if ((options == OK) || (options == CANCEL)) { 
            sysState = MENUSETPINS;
            menuoptions = OPTIONSOK;
            options = OPTIONOK;
            cursoron.blinkon = false;
          }
        break;
      case MENUSETPINSINFO:
        sharedNavigation();
        oled.println(F("Set pins:"));
        oled.print(F(" Information"));
        break;
      case MENUSETPINSINFOACTIVE:
        showInfo(infoSetPins);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUSETPINSINFO;
          options = OPTIONOK;
        }
        break;
      case MENUSETPINSEXIT:
        sharedNavigation();
        oled.println(F("Set pins:"));
        oled.print(F(" Exit set pins"));
        break;

//Stepper
      case MENUSTEPPER:               
        sharedNavigation();
        oled.print(F("Stepper"));
        break;
      case MENUSTEPPERONOFF:
        sharedNavigation();
        oled.println(F("Stepper:"));
        oled.print(F(" Stepper on"));
        break;
      case MENUSTEPPERACTIVE:  
        displayServo(); // Not just for servo only
        if (options == OK) {
          menuoptions = OPTIONSOK;
          sysState = MENUSTEPPERONOFF;
          options = OPTIONOK;
        }         
        break;
      case MENUSTEPPERINFO:
        sharedNavigation();
        oled.println(F("Stepper:"));
        oled.print(F(" Information"));
        break;
      case MENUSTEPPERINFOACTIVE:
        showInfo(infoStepper);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUSTEPPERINFO;
          control.stepperRelay = false;
          writeToExpander(0xFF, true);
          options = OPTIONOK;
        }
        break;
      case MENUSTEPPEREXIT:
        sharedNavigation();
        oled.println(F("Stepper:"));
        oled.print(F(" Exit stepper"));
        break;

//SwitchDebounce
      case MENUSWDEBOUNCE:             
        sharedNavigation();
        oled.print(F("Switch debounce"));
        break;
      case MENUSWDEBOUNCEONOFF:        
        sharedNavigation();
        oled.println(F("Switch debounce:"));
        oled.print(F(" Turn debounce "));
        if (control.switchRelay) { oled.print("off"); } else { oled.print("on"); }   
        break;
      case MENUSWDEBOUNCEINFO:
        sharedNavigation();
        oled.println(F("Switch debounce:"));
        oled.print(F(" Information"));
        break;
      case MENUSWDEBOUNCEINFOACTIVE:
        showInfo(infoDebounce);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUSWDEBOUNCEINFO;
          options = OPTIONOK;
        }
        break;
      case MENUSWDEBOUNCEEXIT:         
        sharedNavigation();
        oled.println(F("Switch debounce:"));
        oled.print(F(" Exit debounce"));
        break;

//Voltmeter
      case MENUVOLTMETER:             
        sharedNavigation();
        oled.print(F("Voltmeter"));
        break;
      case MENUVOLTMETERONOFF:        
        sharedNavigation();
        oled.println(F("Voltmeter:"));
        oled.print(F(" Voltmeter on"));  
        break;
      case MENUVOLTMETERACTIVE:  
        displayVoltmeter(); // Not just for servo only
        if (options == OK) {
          menuoptions = OPTIONSOK;
          sysState = MENUVOLTMETERONOFF;
          forcedisplay++;
        }         
        break;
      case MENUVOLTMETERINFO:
        sharedNavigation();
        oled.println(F("Voltmeter:"));
        oled.print(F(" Information"));
        break;
      case MENUVOLTMETERINFOACTIVE:
        showInfo(infoVoltmeter);
        if (options == OK) { 
          menuoptions = OPTIONSOK;
          sysState = MENUVOLTMETERINFO;
          options = OPTIONOK;
        }
        break;
      case MENUVOLTMETEREXIT:         
        sharedNavigation();
        oled.println(F("Voltmeter:"));
        oled.print(F(" Exit Voltmeter"));
        break;
      default:
        break;

//End
    }
    if (forcedisplay > 1) { dispHeader(); }
    if (forcedisplay > 0) { forcedisplay--; }
  }
  encoder.up = false;
  encoder.down = false;
  encoder.sw = false;
  oled.display();
}