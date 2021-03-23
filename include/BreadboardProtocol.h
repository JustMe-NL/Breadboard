enum spiCommand_e {
  CHECK_POWERREADY                = 0x41, // A
  CHECK_PPS_READY                 = 0x43, // C       
  CHECK_POWEROPTIONS              = 0x44, // D       
  CHECK_PPS_OPTIONS               = 0x45, // E       
  GET_VOLTAGE                     = 0x46, // F
  GET_CURRENT                     = 0x47, // G
  SET_PPS                         = 0x48, // H
  SET_POWER_OPTION                = 0x49, // I
  POWERON                         = 0x50, // J
  POWEROFF                        = 0x51, // K

  SET_BAUDRATE                    = 0x42, // B  - Baudrate for Serial1
  SERIALPIPING                    = 0x59, // S  - S0 Off, S1 USB2Serial, S2 USB2SPI, S3 SPI2Serial
  NOT_ACKNOWLEDGED                = 0x57, // Q  
  ACKNOWLEDGED                    = 0X58  // R
};

#define READACKNOWLEDGED    0x07          // Send by master to indicate next bytes will be read from slave
#define DATAFOLLOWS         0x08          // Send by master to indicate next bytes are data
#define EXITCOMMAND         0x09          // Exit current mode

#define USBSERIAL           0x01
#define USBSPI              0x02
#define SPISERIAL           0x03

enum pdState_e {
  IDLE,
  SPI2USB,
  USB2SERIAL,
  SPI2SERIAL,
};