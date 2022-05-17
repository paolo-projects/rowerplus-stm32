/*
 * data_storage.c
 *
 *  Created on: May 17, 2022
 *      Author: infan
 */

#include "data_storage.h"
#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

typedef struct {
	float ka;
	float km;
	float ks;
	uint16_t checksum;
	char padding[2];
} storage_internal_data_t;

storage_internal_data_t int_data = {0};

void storage_write(storage_data_t* data)
{
	memset(&int_data, 0, sizeof(storage_internal_data_t));
	int_data.ka = data->ka;
	int_data.km = data->km;
	int_data.ks = data->ks;

	for(uint32_t i = 0; i < sizeof(storage_internal_data_t); i++)
	{
		if(i != 12 && i != 13)
		{
			int_data.checksum += ((char*)&int_data)[i];
		}
	}

	int_data.checksum += 10;

	HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, 0, 2, &int_data, sizeof(storage_internal_data_t), 100);
}

BOOL storage_read(storage_data_t* output)
{
	memset(&int_data, 0, sizeof(storage_internal_data_t));
	if(HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, 0, 2, &int_data, sizeof(storage_internal_data_t), 100) == HAL_OK)
	{
		uint16_t checksum = 10;

		for(uint32_t i = 0; i < sizeof(storage_internal_data_t); i++)
		{
			if(i != 12 && i != 13)
			{
				checksum += ((char*)&int_data)[i];
			}
		}

		if(checksum == int_data.checksum)
		{
			output->ka = int_data.ka;
			output->km = int_data.km;
			output->ks = int_data.ks;

			return TRUE;
		}
	}

	return FALSE;
}
