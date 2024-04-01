/*
  Zarpli - Tecnología Interactiva
  30042022 Alejandro Zárate
  https://github.com/zarpli/LOGItech-Z906/

  DESCRIPTION
  ====================
  
  Basic example of the LOGItech-Z906 library.
*/

#include <Z906.h>

// Instantiate a Z906 object and Attach to Serial1
Z906 LOGI(Serial1);

void setup()
{
  Serial.begin(9600);
  while(!Serial);

  //If communication failed, wait and try again
  while (LOGI.request(VERSION) == 0)
  {
    Serial.println("Waiting for Z906 to power up...");
    delay(100);
    _version = LOGI.request(VERSION);
  }

  // Power ON amplifier
  LOGI.on();

  // Select RCA 2.0 Input
  LOGI.input(SELECT_INPUT_2);

  // Disable Mute
  LOGI.cmd(MUTE_OFF);

  // Set Main Level (0 to 43 range)
  LOGI.cmd(MAIN_LEVEL, 11);
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
