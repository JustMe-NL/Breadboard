/*
  SPI TRANSACTIONS:

  Teensy > PD (Teensy - master):                                  (PD - slave):
    Check PDINT high                                              ...
    SS low                                                        ...
    Send byte                                                     SPI int > byte send/recieved (transactionbusy, writemode)
    Wait for short low pulse from PDINT as ACK from PD            Process byte in INT handler
                                                                  Pulse PDINT low ~ 10us (boolean for delayed ack needed)
    If more data to send goto Send byte
    SS high                                                       SS high detected transaction complete (newSPIdata_avail)
                                                                  if delayed ACK, process ACK

  PD > Teensy (Teensy):                                           (PD):
                                                                  Preload byte to send (readmode)
                                                                  Set PDINT low
    PDINT low detected                                            ...
    SS low                                                        ...
    Read byte                                                     SPI int > byte send/recieved
                                                                  No more bytes to send, goto set PDINT high
                                                                  else prepare next byte to send
                                                                  Pulse PDINT high ~ 10us
    check for PD_INT high                                         
    if pulse (~ 10us) more bytes to recieve, goto Read byte       set PDINT high, transaction complete (datasend)
    else SS high, transaction complete

  COMMUNICATION PROTOCOL:
    All communication (transactions) will start with a commandbyte send from the master to the slave, optionally followed by 
      databytes send by either slave or master.
    The slave acknoledges the send or recieved bytes by pulsing PD_INT so the master knows the slave is ready for more data.
    If data needs to be send from the slave, the master will acknoledge the read by sending READACKNOWLEDGED as commandbyte, 
      after this the data will be send by the slave until PDINT is set high for more then 10 us which indicates end of data.

  ACKNOLEDGE DETECTION ON MASTER:
    We want to detect both short pulses (both positive & negative) as well as a stable LOW signal, set slaveAckReceived on
    reciept of a pulse and set slaveReadRequest on a stable low signal en reset on a stable high signal.

    Triggers an interrupt on the change of the PDINT pin, if the timer is running, sets slaveAckReceived to true. 
    Start or restart the timer.

    On timer interrupt stop the timer. Set slaveAckReceived to true and set slaveReadRequest to the oinverse of PDINT. 
*/

enum spiCommand_e {
  cmdENQ_ReadAck                  = 0x05, // Enquire - Send by master to indicate next bytes will be read from slave
  cmdSTX_DataNext                 = 0x02, // Start Of Text - Send by master to indicate next bytes are data
  cmdESC_ExitCommand              = 0x1B, // Escape - Exit current mode

  modeUSB_Serial                  = 0x31, // Pipe USB Serial data to Serial1
  modeSPI_USB                     = 0x32, // Pipe USB Serial data to SPI
  modeSPI_Serial                  = 0x33, // Pipe SPI data to Serial1
  modeSPI_Keyboard                = 0x34, // Pipe SPI data to USB Keyboard emulation

  cmdSerialPiping                 = 0x41, // A  - followed by 1 byte indicating piping mode
  cmdSetBaudRate                  = 0x42, // B  - followed by 1 byte indicating baudrate used (baudRates[][])

  cmdCheckPowerReady              = 0x43, // C
  cmdCheckPSSReady                = 0x44, // D       
  cmdGetPowerOptions              = 0x45, // E       
  cmdGetPSSOptions                = 0x46, // F       
  cmdGetVoltage                   = 0x47, // G
  cmdGetCurrent                   = 0x48, // H
  cmdSetPPS                       = 0x49, // I
  cmdSetPowerOption               = 0x50, // J
  cmdPowerOn                      = 0x51, // K - no additional data
  cmdPowerOff                     = 0x52, // L - no additional data

};       

enum pdState_e {
  stateIdle,
  stateUSB_Serial,
  stateSPI_USB,
  stateSPI_Serial,
  stateSPI_Keyboard                          // SPI <> Keyboard
};
