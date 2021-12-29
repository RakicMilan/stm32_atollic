/**
 ******************************************************************************
 * @file    ds1820.h
 * @author  Milan Rakic
 * @version V1.0.0
 * @date    21-August-2018
 * @brief   Library for DS1820 Digital Thermometer.
 *
 ******************************************************************************
 * <h2><center>&copy; COPYRIGHT 2018 MR</center></h2>
 ******************************************************************************
 */

#ifndef __DS1820_H
#define __DS1820_H

#include <stdint.h>
#include "stm32f1xx_hal.h"


#define DS1820_PIN_WH		GPIO_PIN_11
#define DS1820_PORT_WH		GPIOA

#define DS1820_PIN_B		GPIO_PIN_12
#define DS1820_PORT_B		GPIOA

/* temperature resolution => 1/256°C = 0.0039°C */
#define TEMP_RES              0x10 //Calculation for DS18B20 with 0.1 deg C resolution
//#define TEMP_RES              0x02 //Calculation for DS18S20 with 0.5 deg C resolution

typedef enum {
	T_WATER_HEATER = 0,
	T_BOILER
} TempSensor_t;

extern int16_t m_temperature[2];

extern void _DelayUS(uint32_t us);

void DS1820_Init(void);
int16_t OW_ReadTemperature(TempSensor_t tempSensor);
uint8_t OW_ReadPower(TempSensor_t tempSensor);
void MeasureTemperatures(void);
void Debug_PrintTemperatures(void);

#endif	/* __DS1820_H */
