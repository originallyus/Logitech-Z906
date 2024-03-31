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

  while(LOGI.request(VERSION) == 0)
  {
    Serial.println("Waiting for Z906 to power up...");
    delay(1000);
  }

  Serial.println("Z906 Version : " + (String) LOGI.request(VERSION));
  Serial.println("Current Input : " + (String) LOGI.request(CURRENT_INPUT));
  Serial.println("Main Level : " + (String) LOGI.request(MAIN_LEVEL));
  Serial.println("Rear Level : " + (String) LOGI.request(REAR_LEVEL));
  Serial.println("Center Level : " + (String) LOGI.request(CENTER_LEVEL));
  Serial.println("Sub Level : " + (String) LOGI.request(SUB_LEVEL));
  Serial.println("Temperature sensor: " + (String) LOGI.sensor_temperature());

  // Power ON amplifier
  LOGI.on();

  // Select RCA 2.0 Input
  LOGI.input(SELECT_INPUT_2);

  // Disable Mute
  LOGI.cmd(MUTE_OFF);

  // Set Main Level to 15 (0 to 255 range)
  LOGI.cmd(MAIN_LEVEL, 15);
}

void loop()
{
  Serial.println("Temperature main sensor: " + (String) LOGI.sensor_temperature());

  delay(1000);
}
