#ifndef Z906_H
#define Z906_H

#include "Arduino.h"
#include <SoftwareSerial.h>

// Serial Settings

/*
   * Requirements for Logitech Z906
   * Baud rate   57600
   * Data bit    8 bit
   * Stop bit    1 bit
   * Parity      Odd
*/
#define BAUD_RATE            57600
#define SERIAL_CONFIG        SERIAL_8O1
#define SW_SERIAL_CONFIG     SWSERIAL_8O1
#define SERIAL_TIME_OUT      1000
#define SERIAL_DEADTIME      5
#define MAX_VOLUME_VALUE     43            // Z906 works with 0-43 range internally
#define Z906_DEBUG           false

#define MSG_BUFFER_SIZE           0x20
#define ACK_TOTAL_LENGTH          0x05     //Fixed length of an ack message (not used)
#define TEMPERATURE_MSG_LENGTH    0x0A     //Fixed length of a temperature message

// Single Commands

#define SELECT_INPUT_1      0x02
#define SELECT_INPUT_2      0x05
#define SELECT_INPUT_3      0x03
#define SELECT_INPUT_4      0x04
#define SELECT_INPUT_5      0x06
#define SELECT_INPUT_AUX    0x07

#define LEVEL_MAIN_UP       0x08
#define LEVEL_MAIN_DOWN     0x09
#define LEVEL_SUB_UP        0x0A
#define LEVEL_SUB_DOWN      0x0B
#define LEVEL_CENTER_UP     0x0C
#define LEVEL_CENTER_DOWN   0x0D
#define LEVEL_REAR_UP       0x0E
#define LEVEL_REAR_DOWN     0x0F

#define PWM_OFF             0x10
#define PWM_ON              0x11

#define SELECT_EFFECT_3D    0x14    //1 • 3D Effect
#define SELECT_EFFECT_41    0x15    //2 • 4.1 Effect
#define SELECT_EFFECT_21    0x16    //3 • 2.1 Effect
#define SELECT_EFFECT_NO    0x35    //0 • No effect

#define EEPROM_SAVE         0x36

#define MUTE_ON             0x38
#define MUTE_OFF            0x39

#define BLOCK_INPUTS        0x22
#define RESET_PWR_UP_TIME   0x30
#define NO_BLOCK_INPUTS     0x33

// Double commmands

#define MAIN_LEVEL          0x03
#define REAR_LEVEL          0x04
#define CENTER_LEVEL        0x05
#define SUB_LEVEL           0x06

// Requests

#define VERSION             0xF0
#define CURRENT_INPUT       0xF1
#define CURRENT_EFFECT      0xF2
#define GET_INPUT_GAIN      0x2F
#define GET_TEMP            0x25
#define GET_PWR_UP_TIME     0x31
#define GET_STATUS          0x34

#define GET_STATUS_STBY         0x14
#define GET_STATUS_AUTO_STBY    0x15

// MASK

#define EFFECT_3D           0x00			
#define EFFECT_21           0x01			
#define EFFECT_41           0x02			
#define EFFECT_NO           0x03			

#define SPK_NONE            0x00			
#define SPK_ALL             0xFF
#define SPK_FR              0x01
#define SPK_FL              0x10
#define SPK_RR              0x02
#define SPK_RL              0x08
#define SPK_CENTER          0x04
#define SPK_SUB             0x20

class Z906
{
public:

   Z906(HardwareSerial &serial);

   /*
     Reference:
      • https://github.com/zarpli/Logitech-Z906
      • https://github.com/nomis/logitech-z906/blob/main/interface.rst
     rxPin: ESP Rx -> Tx of Amp • connect to pin 12 on DB-15 connector
     txPin: ESP Tx -> Rx of Amp • connect to pin 13 on DB-15 connector
   */
   Z906(SoftwareSerial &serial, int8_t rxPin, int8_t txPin);

   //For debugging
   void     debug_update_msg_buffer();
   void     print_msg_buffer();

   //Make sure our comm line & buffer is clear
   void     flush();

   //Wrapper functions for sending commands
   int      cmd(uint8_t);
   int      cmd(uint8_t, uint8_t);
   
   //Read latest value from buffer (to be used after an 'update' function call)
   int      read_from_buffer(uint8_t);

   //A dedicated function for reading temperature sensor
   uint8_t  sensor_temperature();

   void     on();
   void     off();
   void     save();
   int      request(uint8_t);
   void     input(uint8_t);

   //A dedicated function for selecting effect for current input
   // 0 or EFFECT_3D or SELECT_EFFECT_3D • 3D Effect
   // 1 or EFFECT_21 or SELECT_EFFECT_21 • 2.1 Effect
   // 2 or EFFECT_41 or SELECT_EFFECT_41 • 4.1 Effect
   // 3 or EFFECT_3D or SELECT_EFFECT_NO • No effect
   void     effect(uint8_t);

private:

   const static uint8_t EXP_HEADER_1ST_VALUE  = 0xAA;             //expect 1st header value for all messages
   const static uint8_t EXP_HEADER_2ND_VALUE  = 0x0A;             //expect 2nd header value for status message
   const static uint8_t EXP_HEADER_TEMPERATURE_2ND_VALUE = 0x0C;  //expect 2nd header value for temperature message

   //Indexes of various bytes in message
   const uint8_t HEADER_1ST_INDEX      = 0x00;        //first header byte
   const uint8_t HEADER_2ND_INDEX      = 0x01;
   const uint8_t PAYLOAD_LENGTH_INDEX  = 0x02;        //index of payload length byte

   const uint8_t STATUS_MAIN_LEVEL     = 0x03;
   const uint8_t STATUS_REAR_LEVEL     = 0x04;
   const uint8_t STATUS_CENTER_LEVEL   = 0x05;
   const uint8_t STATUS_SUB_LEVEL      = 0x06;
   const uint8_t TEMPERATURE_BYTE_IDX  = 0x06;       //for temperature message only
   const uint8_t STATUS_CURRENT_INPUT  = 0x07;
   const uint8_t STATUS_UNKNOWN        = 0x08;
   const uint8_t STATUS_FX_INPUT_4     = 0x09;
   const uint8_t STATUS_FX_INPUT_5     = 0x0A;
   const uint8_t STATUS_FX_INPUT_2     = 0x0B;
   const uint8_t STATUS_FX_INPUT_AUX   = 0x0C;
   const uint8_t STATUS_FX_INPUT_1     = 0x0D;
   const uint8_t STATUS_FX_INPUT_3     = 0x0E;
   const uint8_t STATUS_SPDIF_STATUS   = 0x0F;     //?
   const uint8_t STATUS_SIGNAL_STATUS  = 0x10;     //?
   const uint8_t STATUS_VER_A          = 0x11;
   const uint8_t STATUS_VER_B          = 0x12;
   const uint8_t STATUS_VER_C          = 0x13;
   const uint8_t STATUS_STBY           = 0x14;
   const uint8_t STATUS_AUTO_STBY      = 0x15;
   
   SoftwareSerial * software_serial;
   HardwareSerial * hardware_serial;

   //Checksum function
   uint8_t  LRC(uint8_t*, size_t);

   void     write(uint8_t);
   void     write(uint8_t*, size_t);

   //Use GET_STATUS command to retrieve all status at once
   int      update(uint8_t = GET_STATUS, uint8_t = EXP_HEADER_1ST_VALUE, uint8_t = EXP_HEADER_2ND_VALUE);

   uint8_t  msg_buffer[MSG_BUFFER_SIZE];

   //Shared variables between update() and cmd(cmd_a, cmd_b) functions
   size_t   msg_buffer_len;   
   size_t   checksum_byte_index;
};

#endif // Z906_H