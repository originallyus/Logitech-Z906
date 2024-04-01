/*
Torin Nguyen - Originally US
https://github.com/originallyus/Logitech-Z906/

DESCRIPTION
====================
  
Basic example of using the Logitech-Z906 library with Software Serial.
*/

#include <Z906.h>
#include <SoftwareSerial.h>

/*
  Reference:
    • https://github.com/zarpli/Logitech-Z906
    • https://github.com/nomis/logitech-z906/blob/main/interface.rst
  rxPin: ESP Rx -> Tx of Amp • connect to pin 12 on DB-15 connector
  txPin: ESP Tx -> Rx of Amp • connect to pin 13 on DB-15 connector
*/

//We're using ESP8266 Wemos D1 Mini
#define MY_SW_RX_PIN                  5    //D1              //ESP Rx -> Tx of Amp • connect to pin 12 on DE-15 connector
#define MY_SW_TX_PIN                  4    //D2              //ESP Tx -> Rx of Amp • connect to pin 13 on DE-15 connector


// Instantiate a Z906 object and attach to software serial
SoftwareSerial mySerial;
Z906 LOGI(mySerial, MY_SW_RX_PIN, MY_SW_TX_PIN);


//============================================================

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  while (LOGI.request(VERSION) == 0)
  {
    Serial.println("Waiting for Z906 to power up...");
    delay(100);
  }

  Serial.println("");
  Serial.println("");

  Serial.println("==================================");
  Serial.println("Z906 Version : " + (String) LOGI.request(VERSION));
  Serial.println("Current Input : " + (String) LOGI.request(CURRENT_INPUT));
  Serial.println("Main Level : " + (String) LOGI.request(MAIN_LEVEL));
  Serial.println("Rear Level : " + (String) LOGI.request(REAR_LEVEL));
  Serial.println("Center Level : " + (String) LOGI.request(CENTER_LEVEL));
  Serial.println("Sub Level : " + (String) LOGI.request(SUB_LEVEL));
  Serial.println("Temperature sensor: " + (String) LOGI.sensor_temperature());
  Serial.println("==================================");
  
  Serial.println("");
  Serial.println("");

  delay(10);

  //------------
  
  // Power ON amplifier
  Serial.println("Power ON");
  LOGI.on();

  //------------

  Serial.println("Select Input...");
  
  // Select RCA 2.0 Input
  //LOGI.input(SELECT_INPUT_2);
  
  // Select Optical Input
  LOGI.input(SELECT_INPUT_4);

  // Select AUX Input
  //LOGI.input(SELECT_INPUT_AUX);

  //------------
  
  Serial.println("Disable Mute...");
  
  // Disable Mute
  LOGI.cmd(MUTE_OFF);

  //------------

  uint8_t volume = 11;
  
  Serial.println("Set volume: " + String(volume));

  // Set Main Level (0 to 43 range)
  LOGI.cmd(MAIN_LEVEL, volume);
}

void loop()
{
  //If communication failed, simply wait and try again
  while (LOGI.request(VERSION) == 0) {
    delay(50);
  }

  //This is a more efficient way to retrieve all statuses
  //All status has been internally retrieved at once by LOGI.request(VERSION) above, we only need to read from buffer
  Serial.println("Z906 Version : " + (String) LOGI.read_from_buffer(VERSION));
  Serial.println("Current Input : " + (String) LOGI.read_from_buffer(CURRENT_INPUT));
  Serial.println("Standby Status : " + (String) LOGI.read_from_buffer(GET_STATUS_STBY));
  Serial.println("Auto Standby Status : " + (String) LOGI.read_from_buffer(GET_STATUS_AUTO_STBY));
  Serial.println("Main Level : " + (String) LOGI.read_from_buffer(MAIN_LEVEL));
  Serial.println("Rear Level : " + (String) LOGI.read_from_buffer(REAR_LEVEL));
  Serial.println("Center Level : " + (String) LOGI.read_from_buffer(CENTER_LEVEL));
  Serial.println("Sub Level : " + (String) LOGI.read_from_buffer(SUB_LEVEL));

  //Temperature update requires a separate function call
  Serial.println("Temperature sensor: " + (String) LOGI.sensor_temperature());

  //Optional
  delay(3000);
}
