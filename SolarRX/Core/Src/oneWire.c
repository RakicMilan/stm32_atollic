/**
 ******************************************************************************
 * @file    oneWire.c
 * @author  Milan Rakic
 * @version V1.0.0
 * @date    21-August-2018
 * @brief   1-Wire library.
 *
 ******************************************************************************
 * <h2><center>&copy; COPYRIGHT 2018 MR</center></h2>
 ******************************************************************************
 */

#include "oneWire.h"
#include "main.h"


void _DelayUS(uint32_t us)
{
  volatile uint32_t counter = 8*us;
  while(counter--);
}

void delayus(uint32_t us)
{
  volatile uint32_t counter = 8*us;
  while(counter--);
}

void LH_signal(uint32_t L_time, uint32_t H_time, GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_RESET);
	delayus(L_time);//From pullup_HIGH to GND_LOW:---___
	HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_SET);
	delayus(H_time);//From GND_LOW to pullup_HIGH:___---
}

void write_bit(uint8_t bit, GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	if(bit == 0) LH_signal(60, 5, ow_port, ow_pin);
	else LH_signal(5, 60, ow_port, ow_pin);
}

void write_byte(uint8_t data, GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	for (uint8_t i = 0; i < 8; i++)
		write_bit(data >> i & 1, ow_port, ow_pin);
}

static void A9_as_INPUT(GPIO_TypeDef *ow_port, uint16_t ow_pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = ow_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(ow_port, &GPIO_InitStruct);
}

static void A9_as_OUTPUT(GPIO_TypeDef *ow_port, uint16_t ow_pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = ow_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(ow_port, &GPIO_InitStruct);
}

uint8_t read_bit(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	uint8_t bit = 0;
	LH_signal(1, 10, ow_port, ow_pin);
	A9_as_INPUT(ow_port, ow_pin);
	bit = (HAL_GPIO_ReadPin(ow_port, ow_pin) ? 1 : 0);
	delayus(40);
	A9_as_OUTPUT(ow_port, ow_pin);
	return bit;
}

uint8_t read_byte(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	uint8_t data = 0;
	for (uint8_t i = 0; i < 8; i++)
		data += read_bit(ow_port, ow_pin) << i;
	return data;
}

void A9_wait_for_1(uint32_t time, GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	A9_as_INPUT(ow_port, ow_pin);
	delayus(time);
	while(HAL_GPIO_ReadPin(ow_port, ow_pin) == 0);
	A9_as_OUTPUT(ow_port, ow_pin);
}

void reset(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	LH_signal(500, 500, ow_port, ow_pin);
}

void get_presence(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	uint8_t flag = 1;
	LH_signal(500, 0, ow_port, ow_pin);
	A9_as_INPUT(ow_port, ow_pin);
	delayus(60);//look for GND_LOW = DS18B20 exists, and set flag = 1
	flag = (HAL_GPIO_ReadPin(ow_port, ow_pin) ? 0 : 1); //not ?1:0
	A9_as_OUTPUT(ow_port, ow_pin);
	delayus(400);
	if(flag)
	{
		DEBUG("DS18S20 present");
	}
	else
	{
		DEBUG("DS18S20 not present");
	}
	HAL_Delay(1000);
}

void get_ID(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	uint8_t id_data[] = {0,0,0,0,0,0,0,0};//8 Byte
	reset(ow_port, ow_pin);
	write_byte(0x33, ow_port, ow_pin);//Read ROM [33h] command
	for (uint8_t i = 0; i < 8; i++)
	id_data[i] = read_byte(ow_port, ow_pin);//id_data[0] = 40 = 0x28
	for (uint8_t i = 0; i < 8; i++)
	{
		DEBUG("id: %d", id_data[i]);
		HAL_Delay(200);
	}
	DEBUG("\r\n");
}

float get_temperature(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	uint8_t pad_data[] = {0,0,0,0,0,0,0,0,0}; //9 Byte
	reset(ow_port, ow_pin);
	write_byte(0xCC, ow_port, ow_pin); //Skip ROM [CCh]
	write_byte(0x44, ow_port, ow_pin); //Convert Temperature [44h]
	A9_wait_for_1(20, ow_port, ow_pin);
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
 * @brief  Generate a 1-Wire reset, return 1 if no presence detect was found, return 0 otherwise.
 *         (NOTE: Does not handle alarm presence from DS2404/DS1994)
 * @param  ow_port: GPIOx port, where x can be (A..G) to select the GPIO peripheral.
 * @param  ow_pin: specifies the port bits to be written.
 *   This parameter can be any combination of GPIO_Pin_x where x can be (0..15).
 * @retval 1 if no presence detect was found, 0 otherwise
 */
int OW_reset(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	GPIO_PinState pinState;

	_DelayUS(DELAY_G);
	HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_RESET); // Drives DQ low
	_DelayUS(DELAY_H);
	HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_SET); // Releases the bus
	_DelayUS(DELAY_I);
	pinState = HAL_GPIO_ReadPin(ow_port, ow_pin); // Sample for presence pulse from slave
	_DelayUS(DELAY_J); // Complete the reset sequence recovery
	return pinState; // Return sample presence pulse result
}

/**
 * @brief  Send a 1-Wire write bit. Provide 10us recovery time.
 * @param  ow_port: GPIOx port, where x can be (A..G) to select the GPIO peripheral.
 * @param  ow_pin: specifies the port bits to be written.
 *   This parameter can be any combination of GPIO_Pin_x where x can be (0..15).
 * @param  bit: bit for writing.
 * @retval None
 */
void OW_writeBit(GPIO_TypeDef* ow_port, uint16_t ow_pin, int bit) {
	if (bit) {
		// Write '1' bit
		HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_RESET); // Drives DQ low
		_DelayUS(DELAY_A);
		HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_SET); // Releases the bus
		_DelayUS(DELAY_B); // Complete the time slot and 10us recovery
	} else {
		// Write '0' bit
		HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_RESET); // Drives DQ low
		_DelayUS(DELAY_C);
		HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_SET); // Releases the bus
		_DelayUS(DELAY_D);
	}
}

/**
 * @brief  Read a bit from the 1-Wire bus and return it. Provide 10us recovery time.
 * @param  ow_port: GPIOx port, where x can be (A..G) to select the GPIO peripheral.
 * @param  ow_pin: specifies the port bits to be written.
 *   This parameter can be any combination of GPIO_Pin_x where x can be (0..15).
 * @retval the bit that's read
 */
int OW_readBit(GPIO_TypeDef* ow_port, uint16_t ow_pin) {
	GPIO_PinState pinState;

	HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_RESET); // Drives DQ low
	_DelayUS(DELAY_A);
	HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_SET); // Releases the bus
	_DelayUS(DELAY_E);
	pinState = HAL_GPIO_ReadPin(ow_port, ow_pin); // Sample the bit value from the slave
	_DelayUS(DELAY_F); // Complete the time slot and 10us recovery

	return pinState;
}

/**
 * @brief  Write 1-Wire data byte
 * @param  ow_port: GPIOx port, where x can be (A..G) to select the GPIO peripheral.
 * @param  ow_pin: specifies the port bits to be written.
 *   This parameter can be any combination of GPIO_Pin_x where x can be (0..15).
 * @param  data: data for writing.
 * @retval None
 */
void OW_writeByte(GPIO_TypeDef* ow_port, uint16_t ow_pin, int data) {
	int loop;

	// Loop to write each bit in the byte, LS-bit first
	for (loop = 0; loop < 8; loop++) {
		OW_writeBit(ow_port, ow_pin, data & 0x01);

		// shift the data byte for the next bit
		data >>= 1;
	}
}

/**
 * @brief  Read 1-Wire data byte and return it
 * @param  ow_port: GPIOx port, where x can be (A..G) to select the GPIO peripheral.
 * @param  ow_pin: specifies the port bits to be written.
 *   This parameter can be any combination of GPIO_Pin_x where x can be (0..15).
 * @retval the data that's read
 */
int OW_readByte(GPIO_TypeDef* ow_port, uint16_t ow_pin) {
	int loop, result = 0;

	for (loop = 0; loop < 8; loop++) {
		// shift the result to get it ready for the next bit
		result >>= 1;

		// if result is one, then set MS bit
		if (OW_readBit(ow_port, ow_pin))
			result |= 0x80;
	}
	return result;
}

