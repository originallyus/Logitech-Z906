#include "Z906.h"


//---------------------------------------------------------------------
// Constructors
//---------------------------------------------------------------------

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


//---------------------------------------------------------------------
// For debugging
//---------------------------------------------------------------------

void Z906::print_msg_buffer()
{
	for (int i = 0; i < MSG_BUFFER_SIZE; i++)
	{
		if (msg_buffer[i] < 0x10)
			Serial.print("0");

		Serial.print(msg_buffer[i], HEX);
		Serial.print(" ");
	}

	Serial.print("\n");
}

void Z906::debug_update_msg_buffer()
{
	update();

	for (int i = 0; i < MSG_BUFFER_SIZE; i++)
	{
		if (msg_buffer[i] < 0x10)
			Serial.print("0");

		Serial.print(msg_buffer[i], HEX);
		Serial.print(" ");
	}

	Serial.print("\n");
}


//---------------------------------------------------------------------
// Low-level comm functions
//---------------------------------------------------------------------

// Checksum: Longitudinal Redundancy Check {-1,-1}
uint8_t Z906::LRC(uint8_t* pData, size_t length)
{
	uint8_t LRC = 0;
	for (size_t i = 1; i < length-1; i++)
		LRC -= pData[i];
	return LRC;
}

void Z906::flush()
{
	//Avoid UART TX collisions
	delay(SERIAL_DEADTIME);

	//Clear buffer, just to be sure
	memset(msg_buffer, 0, MSG_BUFFER_SIZE);

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


//---------------------------------------------------------------------
// Wrapper functions for sending commands
//---------------------------------------------------------------------

int Z906::cmd(uint8_t cmd)
{
	write(cmd);

	unsigned long startMillis = millis();

	if (hardware_serial)
	{
		//wait til data is available or timeout
		while (hardware_serial->available() == 0)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;

		return hardware_serial->read();
	}

	if (software_serial)
	{
		//wait til data is available or timeout
		while (software_serial->available() == 0)
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;

		return software_serial->read();
	}
	
	return 0;
}

int Z906::cmd(uint8_t cmd_a, uint8_t cmd_b)
{
	if (!update())
		return 0;
	
	//Support volume input range: 0..43 (MAX_VOLUME_VALUE)
	if (cmd_a == MAIN_LEVEL || cmd_a == REAR_LEVEL || cmd_a == CENTER_LEVEL || cmd_a == SUB_LEVEL)
		if (cmd_b < 0 || cmd_b > MAX_VOLUME_VALUE) {
			Serial.println("Invalid volume params: " + (String)cmd_b);
			return 0;
		}

	//Update specific command
	msg_buffer[cmd_a] = cmd_b;

	//Update checksum byte
	msg_buffer[checksum_byte_index] = LRC(msg_buffer, msg_buffer_len);
	
	//Write the entire buffer back to amplifier
	write(msg_buffer, msg_buffer_len);

	flush(); // Discard ACK message
}


//---------------------------------------------------------------------

int Z906::update(uint8_t cmd, uint8_t exp_1st_header, uint8_t exp_2nd_header)
{
	if (Z906_DEBUG) {
		Serial.print("Sending cmd: 0x");
		if (exp_1st_header < 0x10)	Serial.print("0");
		Serial.print(cmd, HEX);
		Serial.println("");
	}

	//default: GET_STATUS
	write(cmd);

	int i = 0;
	int loop_count = 0;
	unsigned long startMillis = millis();

	//Lock on to first expected header byte
	while (i <= HEADER_1ST_INDEX && loop_count < MSG_BUFFER_SIZE)
	{
		//if timeout
		if (millis() - startMillis > SERIAL_TIME_OUT)
			return 0;

		if (hardware_serial)
			if (hardware_serial->available()) {
				msg_buffer[i] = hardware_serial->read();
				loop_count++;
			}
		if (software_serial)
			if (software_serial->available()) {
				msg_buffer[i] = software_serial->read();
				loop_count++;
			}

		if (msg_buffer[i] != exp_1st_header)
			continue;

		//Got the expected header byte, continue reading the remaining bytes
		i++;
		break;
	}

	//Failed to detect 1st header after too many loop
	if (msg_buffer[HEADER_1ST_INDEX] != exp_1st_header) {
		if (Z906_DEBUG) {
			Serial.println("Num bytes read: " + (String)loop_count);
			Serial.print("Failed to lock on to header byte: 0x");
			if (exp_1st_header < 0x10)	Serial.print("0");
			Serial.print(exp_1st_header, HEX);
			Serial.println("");
			print_msg_buffer();
		}
		return 0;
	}

	//Read header bytes until the payload len byte, or timeout
	if (hardware_serial)
		while (i <= PAYLOAD_LENGTH_INDEX) {
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;
			if (hardware_serial->available())
				msg_buffer[i++] = hardware_serial->read();
		}

	if (software_serial)
		while (i <= PAYLOAD_LENGTH_INDEX) {
			if (millis() - startMillis > SERIAL_TIME_OUT)
				return 0;
			if (software_serial->available())
				msg_buffer[i++] = software_serial->read();
		}

	//Dynamic size of the payload
	uint8_t payload_len = msg_buffer[PAYLOAD_LENGTH_INDEX];

	//Sanity check
	if (payload_len >= MSG_BUFFER_SIZE) {
		if (Z906_DEBUG) {
			Serial.println("Num bytes read: " + (String)i);
			Serial.println("Invalid payload len: " + (String)payload_len);
			print_msg_buffer();
		}
		return 0;
	}

	//Size of the full message (incl. control words and checksum)
	//cmd() functions needs to share this variable
	msg_buffer_len = payload_len + 4;

	//Index of the checksum byte
	//cmd() functions needs to share this variable
	checksum_byte_index = msg_buffer_len - 1;

	//Sanity check
	if (checksum_byte_index >= MSG_BUFFER_SIZE) {
		if (Z906_DEBUG) {
			Serial.println("Num bytes read: " + (String)i);
			Serial.println("Invalid checksum_byte_index: " + (String)checksum_byte_index);
			print_msg_buffer();
		}
		return 0;
	}

	//Read til the end of message, or timeout
	if (hardware_serial)
		while (i <= checksum_byte_index && i < MSG_BUFFER_SIZE) {
			if (millis() - startMillis > SERIAL_TIME_OUT) {
				if (Z906_DEBUG) {
					Serial.println("Num bytes read: " + (String)i);
					Serial.println("Serial timeout");
					print_msg_buffer();
				}
				return 0;
			}
			if (hardware_serial->available())
				msg_buffer[i++] = hardware_serial->read();
		}

	if (software_serial)
		while (i <= checksum_byte_index && i < MSG_BUFFER_SIZE) {
			if (millis() - startMillis > SERIAL_TIME_OUT) {
				if (Z906_DEBUG) {
					Serial.println("Num bytes read: " + (String)i);
					Serial.println("Serial timeout");
					print_msg_buffer();
				}
				return 0;
			}
			if (software_serial->available())
				msg_buffer[i++] = software_serial->read();
		}

	//Verify header bytes
	if (msg_buffer[HEADER_1ST_INDEX] != exp_1st_header) {
		if (Z906_DEBUG) {
			Serial.println("Num bytes read: " + (String)i);
			Serial.print("Wrong HEADER_1ST_INDEX header byte (1st byte). Expecting: 0x");
			if (exp_1st_header < 0x10)	Serial.print("0");
			Serial.print(exp_1st_header, HEX);
			Serial.println("");
			print_msg_buffer();
		}
		return 0;
	}
	if (msg_buffer[HEADER_2ND_INDEX] != exp_2nd_header) {
		if (Z906_DEBUG) {
			Serial.println("Num bytes read: " + (String)i);
			Serial.println("Wrong HEADER_2ND_INDEX header byte (2nd byte). Expecting: 0x");
			if (exp_2nd_header < 0x10)	Serial.print("0");
			Serial.print(exp_2nd_header, HEX);
			Serial.println("");
			print_msg_buffer();
		}
		return 0;
	}

	//Verify checksum byte
	if (msg_buffer[checksum_byte_index] != LRC(msg_buffer, msg_buffer_len)) {
		if (Z906_DEBUG) {
			Serial.println("Num bytes read: " + (String)i);
			Serial.println("Wrong package checksum");
			print_msg_buffer();
		}
		return 0;
	}

	//All good

	if (Z906_DEBUG) {
		Serial.print("cmd OK: 0x");
		if (exp_1st_header < 0x10)	Serial.print("0");		
		Serial.print(cmd, HEX);
		Serial.println(" • payload len: " + String(payload_len));
	}

	return 1;
}

int Z906::read_from_buffer(uint8_t cmd)
{
	if (cmd == VERSION)
		return msg_buffer[STATUS_VER_C] + 10 * msg_buffer[STATUS_VER_B] + 100 * msg_buffer[STATUS_VER_A];

	//Input value: 1 to 6
	if (cmd == CURRENT_INPUT)
		return msg_buffer[STATUS_CURRENT_INPUT] + 1;

	//Volume value: 0..43
	if (cmd == MAIN_LEVEL || cmd == REAR_LEVEL || cmd == CENTER_LEVEL || cmd == SUB_LEVEL)
		return (uint8_t) msg_buffer[cmd];

	return msg_buffer[cmd];
}


//---------------------------------------------------------------------
// A dedicated function for reading temperature sensor
//---------------------------------------------------------------------

uint8_t Z906::sensor_temperature()
{
	//AA 0C 05 00 3A 37 36 00 48 

	uint8_t is_valid = update(GET_TEMP, EXP_HEADER_1ST_VALUE, EXP_HEADER_TEMPERATURE_2ND_VALUE);
	if (!is_valid)
		return 0;

	uint8_t val = msg_buffer[TEMPERATURE_BYTE_IDX];

	flush();	// Discard ACK

	return val;
}


//---------------------------------------------------------------------
// High-level helper functions
//---------------------------------------------------------------------

void Z906::on()
{
	write(PWM_ON);

	flush();	// Discard ACK
}

void Z906::off()
{
	write(PWM_OFF);

	uint8_t cmd[] = { RESET_PWR_UP_TIME, 0x37, EEPROM_SAVE };
	write(cmd, sizeof(cmd));

	flush();	// Discard ACK
}

int Z906::request(uint8_t cmd)
{
	if (!update())
		return 0;

	return read_from_buffer(cmd);

	//Don't flush after this
	//flush();	// Discard ACK
}

void Z906::input(uint8_t input, uint8_t effect)
{
	//Sanity check
	if (input < SELECT_INPUT_1 || input > SELECT_INPUT_AUX) {
		Serial.println("Invalid 'input' params: " + (String)input);
		return;
	}

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

