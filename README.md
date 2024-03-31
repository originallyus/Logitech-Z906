# Logitech Z906 Control Library with SoftwareSerial

This is a fork & enhanced version of **Logitech Z906 Control Library** originally by [zarpli](https://github.com/zarpli/Logitech-Z906) with a Pull Request from [systemofapwne](https://github.com/systemofapwne/Logitech-Z906).

On top of these, we have added support for SoftwareSerial & better documentation to support ESP8266 or ESP32 modules.


# ESP Wiring Diagram

These pins are **NOT** 5V tolerant.

If you are using Arduino UNO, MEGA, etc... you need to use a **Logic Level Converter** to convert 5V logic signals to 3.3V.

|ESP Pin||DE-15 Pin|Description|
|---|---|---|---|
|GND|↔|6|Logic GND|
|3.3V|↔|11|Supply power to your ESP from Amp|
|ESP Rx|↔|12|ESP Rx -> Tx of Amp|
|ESP Tx|↔|13|ESP Tx -> Rx of Amp|
|GND|↔|15|GND -> Console Enable of Amp|

<p align="center"><img src=/images/ESP8266_Z906.jpg></p>

Full pinout of the original Z906 DE-15 connector is available at the bottom of this README.



# DE-15 Female Connector

You'll need a **female** DE-15, also known as DB-15 or D-Sub or VGA, solderless break-out connector from AliExpress [here](https://a.aliexpress.com/_opPRJE4).
You may use the shorter version (missing pin 11) if you are not powering your ESP from this connector.

<p align="center"><img src=/images/DE-15.jpg></p>
Note: The plastic case for this connector is too big to fit into the space for console port.



# Basic Usage

Instantiate a Z906 object and attach to a Serial instance, you may create multiple Z906 objects.
```C++
Z906 LOGI(Serial1);

// OR

SoftwareSerial mySerial;
Z906 LOGI(mySerial, 5, 4);
```
**cmd** method uses single or double argument, check next tables.
```C++
LOGI.cmd(arg);
LOGI.cmd(arg_a, arg_b);
```
Examples : 
```C++
LOGI.cmd(MUTE_ON);               // Enable Mute
LOGI.cmd(MAIN_LEVEL, 9);         // Set Main Level to 9 (0 to 43 range)
```
# Request data

```C++
LOGI.request(VERSION);           // return firmware version
LOGI.request(CURRENT_INPUT);     // return current input (1-6)
LOGI.request(MAIN_LEVEL);        // return current Main Level
LOGI.request(REAR_LEVEL);        // return current Rear Level
LOGI.request(CENTER_LEVEL);      // return current Center Level
LOGI.request(SUB_LEVEL);         // return current Subwoofer Level

LOGI.request(GET_STATUS_STBY);        // return stand-by status
LOGI.request(GET_STATUS_AUTO_STBY);   // return auto stand-by status
```

# Single argument commands
|argument|description|
|---|---|
|SELECT_INPUT_1|Enable TRS 5.1 input|
|SELECT_INPUT_2|Enable RCA 2.0 input|
|SELECT_INPUT_3|Enable Optical 1 input|
|SELECT_INPUT_4|Enable Optical 2 input|
|SELECT_INPUT_5|Enable Coaxial input|
|SELECT_INPUT_AUX|Enable TRS 2.0 (console) input|
||
|LEVEL_MAIN_UP|Increase Main Level by one unit|
|LEVEL_MAIN_DOWN|Decrease Main Level by one unit|
|LEVEL_SUB_UP|Increase Subwoofer Level by one unit|
|LEVEL_SUB_DOWN|Decrease Subwoofer Level by one unit|
|LEVEL_CENTER_UP|Increase Center Level by one unit|
|LEVEL_CENTER_DOWN|Decrease Subwoofer Level by one unit|
|LEVEL_REAR_UP|Increase Rear Level by one unit|
|LEVEL_REAR_DOWN|Decrease Rear Level by one unit|
||
|PWM_ON|PWM Generator ON (Turn on Amp)|
|PWM_OFF|PWM Generator OFF (Turn off Amp)|
||
|SELECT_EFFECT_3D|Enable 3D Effect in current input|
|SELECT_EFFECT_41|Enable 4.1 Effect in current input|
|SELECT_EFFECT_21|Enable 2.1 Effect in current input|
|SELECT_EFFECT_NO|Disable all Effects in current input|
||
|BLOCK_INPUTS|Disable signal input|
|NO_BLOCK_INPUTS|Enable signal input|
||
|RESET_PWR_UP_TIME|Reset Power-Up Timer|
|EEPROM_SAVE|Save current settings to EEPROM|
||
|MUTE_ON|Enable Mute|
|MUTE_OFF|Disable Mute|

# Double arguments commands

|argument a|argument b|description|
|---|---|---|
|MAIN_LEVEL|0-43|Set Main Level to argument b value|
|REAR_LEVEL|0-43|Set Rear Level to argument b value|
|CENTER_LEVEL|0-43|Set Center Level to argument b value|
|SUB_LEVEL|0-43|Set Sub Level to argument b value|

# EEPROM

Use the **EEPROM_SAVE** function with caution. Each EEPROM has a limited number of write cycles (~100,000) per address. If you write excessively to the EEPROM, you will reduce the lifespan.

# Get Temperature

return the value of main temperature sensor.
```C++
uint8_t temperature = LOGI.sensor_temperature();    //Celcius
```


# Turn the amplifier On or Off

Turns the amplifier on or off. When turning off, the amplifier will also store the current state of the unit in EEPROM. Note, that the amplifier still draws a certain amount of power in this stage.
```C++
LOGI.on();
LOGI.off();
```
To fully turn off the amplifier, you need to break the connection between **Pin 15** and GND on the DE-15 connector.


# Select input

Select the input and applies the input effect. If no input effect is selected, the device's factory defaults will be used.
```C++
/*
input: 1 to 5 & 6 for AUX
effect:
SELECT_EFFECT_3D  //Enable 3D Effect in current input
SELECT_EFFECT_41  //Enable 4.1 Effect in current input
SELECT_EFFECT_21  //Enable 2.1 Effect in current input
SELECT_EFFECT_NO  //Disable all Effects in current input
*/
LOGI.input(input, effect);
```


# Basic Example

```C++
#include <Z906.h>

//We're using ESP8266 Wemos D1 Mini
#define MY_SW_RX_PIN      5  //D1  //ESP Rx -> Tx of Amp • pin 12 on DE-15 connector
#define MY_SW_TX_PIN      4  //D2  //ESP Tx -> Rx of Amp • pin 13 on DE-15 connector

// Instantiate a Z906 object and attach to SoftwareSerial object
SoftwareSerial mySerial;
Z906 LOGI(mySerial, MY_SW_RX_PIN, MY_SW_TX_PIN);

void setup()
{
  Serial.begin(115200);
  while(!Serial);

  while(LOGI.request(VERSION) == 0)
  {
    Serial.println("Waiting for Z906 to power up...");
    delay(1000);
  }

  Serial.println("==================================");
  Serial.println("Z906 Version : " + (String) LOGI.request(VERSION));
  Serial.println("Current Input : " + (String) LOGI.request(CURRENT_INPUT));
  Serial.println("Main Level : " + (String) LOGI.request(MAIN_LEVEL));
  Serial.println("Rear Level : " + (String) LOGI.request(REAR_LEVEL));
  Serial.println("Center Level : " + (String) LOGI.request(CENTER_LEVEL));
  Serial.println("Sub Level : " + (String) LOGI.request(SUB_LEVEL));
  Serial.println("Temperature sensor: " + (String) LOGI.sensor_temperature());
  Serial.println("==================================");

  // Power ON the amplifier
  LOGI.on();

  // Select RCA 2.0 Input
  LOGI.input(SELECT_INPUT_2);

  // Select Optical Input
  LOGI.input(SELECT_INPUT_4);

  // Disable Mute
  LOGI.cmd(MUTE_OFF);

  // Set Main Level (0 to 43 range)
  LOGI.cmd(MAIN_LEVEL, 9);
}

void loop()
{
  Serial.println("Temperature main sensor: " + (String) LOGI.sensor_temperature());
  delay(1000);
}
```



# DE-15 Z906 Original Console Pinout

This was taken from [nomis](https://github.com/nomis/logitech-z906/blob/main/interface.rst) and modified for better clarity.

The communication between the Digital Audio Processor and the console is done through TTL serial communication at 3.3V (**NOT** 5V tolerant).

The following table illustrates the pinout of the DE-15 connector.

| Pin | Console                     | Amplifier                                       |
|-----|-----------------------------|-------------------------------------------------|
|   1 |                             | Headphones Right                                |
|   2 |                             | Headphones Left                                 |
|   3 | Audio Ground                | Audio Ground                                    |
|   4 | Aux Right                   |                                                 |
|   5 | Aux Left                    |                                                 |
|   6 | Logic Ground                | Logic Ground                                    |
|   7 | Unknown (not required)      | Unknown (not required)                          |
|   8 | Amplifier presence. Pull-down (0V) resistor     | Output 3.3V for 500ms at power up/comms start. Output 3.3V for 100ms after comms stop   |
|   9 | Output 3.3V when powered (not required)   |  Unused                           |
|  10 | Unknown (not required)      | Unused                                          |
|  11 | 3.3V Input from Amp         | Supply 3.3V @ 250mA to power the Console        |
|  12 | Console Rx → Amp Tx         | Amp Tx → Console Rx                             |
|  13 | Console Tx → Amp Rx         | Amp Rx → Console Tx                             |
|  14 | Unused                      | Unused                                          |
|  15 | Output 0V if comms active. Connect permanently to GND to enable console | Contains an internal pull-up (3.3V) resistor. When pulled-down to 0V by console, indicating console is presence  |
