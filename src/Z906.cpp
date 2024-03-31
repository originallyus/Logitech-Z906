#include "Z906.h"

Z906::Z906(HardwareSerial &serial)
{
	hardware_serial = &serial;
	hardware_serial->begin(BAUD_RATE, SERIAL_CONFIG);
}

/*
  Reference:
  	• https://github.com/zarpli/Logitech-Z906
	• https://github.com/nomis/logitech-z906/blob/main/interface.rst
  rxPin: ESP Rx -> Tx of Amp • connect to pin 12 on DB-15 connector
  txPin: ESP Tx -> Rx of Amp • connect to pin 13 on DB-15 connector
*/
Z906::Z906(SoftwareSerial &serial, int8_t rxPin, int8_t txPin)
{
	software_serial = &serial;
	software_serial->begin(BAUD_RATE, SW_SERIAL_CONFIG, rxPin, txPin, false);
}

// Longitudinal Redundancy Check {-1,-1}
uint8_t Z906::LRC(uint8_t* pData, size_t length)
{
	uint8_t LRC = 0;
	for (size_t i = 1; i < length-1; i++)
		LRC -= pData[i];
	return LRC;
}

void Z906::on()
{
	write(PWM_ON);
}

void Z906::off()
{
	write(PWM_OFF);

	uint8_t cmd[] = { RESET_PWR_UP_TIME, 0x37, EEPROM_SAVE };
	write(cmd, sizeof(cmd));
	flush();	// Discard ACK
}

void Z906::input(uint8_t input, uint8_t effect)
{
	// If no effect is select, use default (same as console default)
	if (effect = 0xFF)
		if (input == SELECT_INPUT_2 || input == SELECT_INPUT_AUX){
			effect = SELECT_EFFECT_3D;
		} else {
			effect = SELECT_EFFECT_NO;
		}

	uint8_t cmd[] = { MUTE_ON, input, effect, MUTE_OFF };
	write(cmd, sizeof(cmd));
	flush();	// Discard ACK
}

void Z906::flush()
{
	//Avoid UART TX collisions
	delay(SERIAL_DEADTIME);

	//Clear buffer, just to be sure
	memset(status_buffer, 0, STATUS_BUFFER_SIZE);

	//Read and discard anything left on RX line
	if (hardware_serial)
		while (hardware_serial->available())
			hardware_serial->read();
	if (software_serial)
		while (software_serial->available())
			software_serial->read();
	
	//Clear RX buffer
	if (hardware_serial)
		hardware_serial->flush();
	if (software_serial)
		software_serial->flush();
}

void Z906::write(uint8_t cmd)
{
	flush();

	if (hardware_serial)
	{
		hardware_serial->write(cmd);
		hardware_serial->flush();			//Flush TX buffer to device
	}

	if (software_serial)
	{
		software_serial->write(cmd);
		software_serial->flush();			//Flush TX buffer to device
	}
}

void Z906::write(uint8_t* pCmd, size_t cmd_len)
{
	flush();

	if (hardware_serial)
	{
		for (size_t i = 0; i < cmd_len; i++)
			hardware_serial->write(pCmd[i]);

		hardware_serial->flush();			//Flush TX buffer to device
	}

	if (software_serial)
	{
		for (size_t i = 0; i < cmd_len; i++)
			software_serial->write(pCmd[i]);

		software_serial->flush();			//Flush TX buffer to device
	}
}

int Z906::update()
{
	write(GET_STATUS);

	unsigned long startMillis = millis();

	if (hardware_serial)
	{
		// Determine payload size and total required buffer size
		while (hardware_serial->available() < STATUS_LENGTH_INDEX + 1)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;
		
		for (int i = 0; i <= STATUS_LENGTH_INDEX; i++)
			status_buffer[i] = hardware_serial->read();

		size_t payload_len = status_buffer[STATUS_LENGTH_INDEX];		//Dynamic size of the payload
		status_buffer_len = payload_len + 4;							//Size of full status message in buffer

		while (hardware_serial->available() < payload_len + 1)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;

		//Read payload + checksum
		for (int i = 0; i <= payload_len; i++)
			status_buffer[i + STATUS_LENGTH_INDEX + 1] = hardware_serial->read();
		STATUS_CHECKSUM = status_buffer_len - 1;
	}

	if (software_serial)
	{
		// Determine payload size and total required buffer size
		while (software_serial->available() < STATUS_LENGTH_INDEX + 1)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;
		
		for (int i = 0; i <= STATUS_LENGTH_INDEX; i++)
			status_buffer[i] = software_serial->read();

		size_t payload_len = status_buffer[STATUS_LENGTH_INDEX];		//Dynamic size of the payload
		status_buffer_len = payload_len + 4;							//Size of full status message in buffer

		while (software_serial->available() < payload_len + 1)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;

		//Read payload + checksum
		for (int i = 0; i <= payload_len; i++)
			status_buffer[i + STATUS_LENGTH_INDEX + 1] = software_serial->read();
		STATUS_CHECKSUM = status_buffer_len - 1;
	}

	//Verify header bytes
	if (status_buffer[STATUS_STX] != EXP_STX)
		return 0;
	if (status_buffer[STATUS_MODEL] != EXP_MODEL_STATUS)
		return 0;

	//Verify checksum byte
	if (status_buffer[STATUS_CHECKSUM] == LRC(status_buffer, status_buffer_len))
		return 1;
	
	return 0;
}

int Z906::request(uint8_t cmd)
{
	if (!update())
		return 0;

	if (cmd == VERSION)
		return status_buffer[STATUS_VER_C] + 10 * status_buffer[STATUS_VER_B] + 100 * status_buffer[STATUS_VER_A];

	if (cmd == CURRENT_INPUT)
		return status_buffer[STATUS_CURRENT_INPUT] + 1;

	if (cmd == MAIN_LEVEL || cmd == REAR_LEVEL || cmd == CENTER_LEVEL || cmd == SUB_LEVEL)
		return (uint8_t) status_buffer[cmd];

	return status_buffer[cmd];
}

int Z906::cmd(uint8_t cmd)
{
	write(cmd);

	unsigned long startMillis = millis();

	if (hardware_serial)
	{
		while (hardware_serial->available() == 0)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;

		return hardware_serial->read();
	}

	if (software_serial)
	{
		while (software_serial->available() == 0)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;

		return software_serial->read();
	}
	
	return 0;
}

int Z906::cmd(uint8_t cmd_a, uint8_t cmd_b)
{
	update();
	
	/*
	//Normalize volume to 0...255
	if (cmd_a == MAIN_LEVEL || cmd_a == REAR_LEVEL || cmd_a == CENTER_LEVEL || cmd_a == SUB_LEVEL)
		cmd_b = (uint8_t) cmd_b;
	*/

	status_buffer[cmd_a] = cmd_b;
	status_buffer[STATUS_CHECKSUM] = LRC(status_buffer, status_buffer_len);
	
	write(status_buffer, status_buffer_len);

	flush(); // Discard ACK message
}

void Z906::print_status_buffer()
{
	for (int i = 0; i < status_buffer_len; i++)
	{
		Serial.print(status_buffer[i], HEX);
		Serial.print(" ");
	}

	Serial.print("\n");
}

uint8_t Z906::sensor_temperature()
{
	write(GET_TEMP);
	
	unsigned long startMillis = millis();

	if (hardware_serial)
	{
		while (hardware_serial->available() < TEMPERATURE_MSG_LENGTH)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;

		uint8_t temp[TEMPERATURE_MSG_LENGTH];
		
		for (int i = 0; i < TEMPERATURE_MSG_LENGTH; i++)
			temp[i] = hardware_serial->read();

		if (temp[2] != EXP_MODEL_TEMPERATURE)
			return 0;
	
		return temp[7];
	}
	
	if (software_serial)
	{
		while (software_serial->available() < TEMPERATURE_MSG_LENGTH)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;

		uint8_t temp[TEMPERATURE_MSG_LENGTH];
		
		for (int i = 0; i < TEMPERATURE_MSG_LENGTH; i++)
			temp[i] = software_serial->read();

		if (temp[2] != EXP_MODEL_TEMPERATURE)
			return 0;
	
		return temp[7];
	}

	return 0;
}