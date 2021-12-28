/**
 ******************************************************************************
 * @file    ds1820.c
 * @author  Milan Rakic
 * @version V1.0.0
 * @date    21-August-2018
 * @brief   Library for DS1820 Digital Thermometer.
 *
 ******************************************************************************
 * <h2><center>&copy; COPYRIGHT 2018 MR</center></h2>
 ******************************************************************************
 */

#include "ds1820.h"
#include "oneWire.h"
#include "ssd1306.h"


int16_t m_temperature[2];

/**
 * @brief  Initializes the one-wire communication for DS1820
 * @retval None
 */
void DS1820_Init(void) {
	GPIO_InitTypeDef GPIO_OneWireInitStruct;

	HAL_GPIO_WritePin(DS1820_PORT_WH, DS1820_PIN_WH, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DS1820_PORT_B, DS1820_PIN_B, GPIO_PIN_SET);

	GPIO_OneWireInitStruct.Pin = DS1820_PIN_WH;
	GPIO_OneWireInitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_OneWireInitStruct.Pull = GPIO_NOPULL;
	GPIO_OneWireInitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DS1820_PORT_WH, &GPIO_OneWireInitStruct);

	GPIO_OneWireInitStruct.Pin = DS1820_PIN_B;
	HAL_GPIO_Init(DS1820_PORT_B, &GPIO_OneWireInitStruct);
}

/**
 * @brief  Read current temperature
 * @param tempSensor temperature sensor for measuring
 * @retval Measured temperature
 */
int16_t OW_ReadTemperature(TempSensor_t tempSensor) {
	GPIO_TypeDef* ow_port;
	uint16_t ow_pin;

//	union {
//		int16_t word;
//
//		struct {
//			uint8_t lsByte;
//			uint8_t msByte;
//		};
//	} temperature;

	switch (tempSensor) {
	case T_BOILER:
		ow_port = DS1820_PORT_B;
		ow_pin = DS1820_PIN_B;
		break;
	case T_WATER_HEATER:
	default:
		ow_port = DS1820_PORT_WH;
		ow_pin = DS1820_PIN_WH;
		break;
	}

//	OW_reset(ow_port, ow_pin);
//	_DelayUS(375);
//	OW_writeByte(ow_port, ow_pin, OW_CMD_SKIPROM);
//	_DelayUS(12);
//	OW_writeByte(ow_port, ow_pin, OW_CONVERT_TEMPERATURE);
//	HAL_Delay(300);
//	OW_reset(ow_port, ow_pin);
//	_DelayUS(375);
//	OW_writeByte(ow_port, ow_pin, OW_CMD_SKIPROM);
//	_DelayUS(12);
//	OW_writeByte(ow_port, ow_pin, OW_READ_SCRATCHPAD);
//	_DelayUS(12);
//	temperature.lsByte = OW_readByte(ow_port, ow_pin);
//	_DelayUS(2);
//	temperature.msByte = OW_readByte(ow_port, ow_pin);
//	OW_reset(ow_port, ow_pin);
//
//	return (int16_t)((float) temperature.word / TEMP_RES);

	uint8_t pad_data[] = {0,0,0,0,0,0,0,0,0}; //9 Byte
	reset(ow_port, ow_pin);
	write_byte(0xCC, ow_port, ow_pin); //Skip ROM [CCh]
	write_byte(0x44, ow_port, ow_pin); //Convert Temperature [44h]
	PIN_wait_for_1(20, ow_port, ow_pin);
	reset(ow_port, ow_pin);
	write_byte(0xCC, ow_port, ow_pin); //Skip ROM [CCh]
	write_byte(0xBE, ow_port, ow_pin); //Read Scratchpad [BEh]
	for (uint8_t i = 0; i < 9; i++)
		pad_data[i] = read_byte(ow_port, ow_pin); //factor out 1/16 and remember 1/16 != 1/16.0
	uint16_t x = (pad_data[1] << 8) + pad_data[0];
	if ((pad_data[1] >> 7) == 1 )
	{
		x -= 1; x = ~x; return x / -16.0;
	}
	else
	{
		return x / 16.0;
	}
}

/**
 * @brief  Determine device power source
 * @param tempSensor temperature sensor for measuring
 * @retval Zero returned if parasitic mode otherwise Vdd source
 */
uint8_t read_power(TempSensor_t tempSensor) {
	GPIO_TypeDef* ow_port;
	uint16_t ow_pin;

	switch (tempSensor) {
	case T_BOILER:
		ow_port = DS1820_PORT_B;
		ow_pin = DS1820_PIN_B;
		break;
	case T_WATER_HEATER:
	default:
		ow_port = DS1820_PORT_WH;
		ow_pin = DS1820_PIN_WH;
		break;
	}

	reset(ow_port, ow_pin);
	write_byte(OW_CMD_SKIPROM, ow_port, ow_pin);
	write_byte(OW_READ_POWERSUPPLY, ow_port, ow_pin);
	return read_byte(ow_port, ow_pin);
}

void MeasureTemperature(TempSensor_t tempSensor) {
	m_temperature[tempSensor] = OW_ReadTemperature(tempSensor);
}

void MeasureTemperatures(void) {
	MeasureTemperature(T_WATER_HEATER);
	//MeasureTemperature(T_BOILER);
}

// Get temperature string for printing on LCD
void GetTemperatureString(int16_t temperature, char *tempString) {
	// Check if temperature is negative
	if (temperature < 0) {
		tempString[0] = '-';
		temperature *= -1;
	} else {
		if (temperature / 100)
			tempString[0] = (uint16_t) temperature / 100 + '0';
		else
			tempString[0] = '+';
	}

	tempString[1] = ((uint16_t) temperature / 10) % 10 + '0'; // Extract tens digit
	tempString[2] = (uint16_t) temperature % 10 + '0'; // Extract ones digit
}

void DisplayTemperatures(void) {
	char tBoiler[] = "000";
	char tWaterHeater[] = "000";
	//char tCollector[] = "000";
	GetTemperatureString(m_temperature[T_BOILER], tBoiler);
	GetTemperatureString(m_temperature[T_WATER_HEATER], tWaterHeater);
	//GetTemperatureString(m_tCollector.i, tCollector);

	SSD1306_Clear();
	SSD1306_GotoXY(0, 0);
	SSD1306_Puts(tWaterHeater, &Font_11x18, 1);
	SSD1306_UpdateScreen();

	//ssd1306_PrintTemperatures(tBoiler, tWaterHeater, tCollector);
}

void Debug_PrintTemperatures(void) {
	DebugChangeColorToWHITE();
	DEBUG("Kotao   : %d\r\n", m_temperature[T_BOILER]);
	DEBUG("Bojler  : %d\r\n", m_temperature[T_WATER_HEATER]);
	//DEBUG("Kolektor: %d\r\n", m_tCollector.i);
	DebugChangeColorToGREEN();
}

