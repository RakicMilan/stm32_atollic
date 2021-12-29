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

static void PIN_as_INPUT(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	HAL_GPIO_WritePin(ow_port, ow_pin, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = ow_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(ow_port, &GPIO_InitStruct);
}

static void PIN_as_OUTPUT(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
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
	PIN_as_INPUT(ow_port, ow_pin);
	bit = (HAL_GPIO_ReadPin(ow_port, ow_pin) ? 1 : 0);
	delayus(40);
	PIN_as_OUTPUT(ow_port, ow_pin);
	return bit;
}

uint8_t read_byte(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	uint8_t data = 0;
	for (uint8_t i = 0; i < 8; i++)
		data += read_bit(ow_port, ow_pin) << i;
	return data;
}

void PIN_wait_for_1(uint32_t time, GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	PIN_as_INPUT(ow_port, ow_pin);
	delayus(time);
	while(HAL_GPIO_ReadPin(ow_port, ow_pin) == 0);
	PIN_as_OUTPUT(ow_port, ow_pin);
}

void reset(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	LH_signal(500, 500, ow_port, ow_pin);
}

void get_presence(GPIO_TypeDef *ow_port, uint16_t ow_pin) {
	uint8_t flag = 1;
	LH_signal(500, 0, ow_port, ow_pin);
	PIN_as_INPUT(ow_port, ow_pin);
	delayus(60);//look for GND_LOW = DS18B20 exists, and set flag = 1
	flag = (HAL_GPIO_ReadPin(ow_port, ow_pin) ? 0 : 1); //not ?1:0
	PIN_as_OUTPUT(ow_port, ow_pin);
	delayus(400);
	if(flag)
	{
		DEBUG("DS18B20 present");
	}
	else
	{
		DEBUG("DS18B20 not present");
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

