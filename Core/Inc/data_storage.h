/*
 * data_storage.h
 *
 *  Created on: May 17, 2022
 *      Author: infan
 */

#ifndef INC_DATA_STORAGE_H_
#define INC_DATA_STORAGE_H_

#include <stdint.h>
#include "common.h"

#define EEPROM_ADDR 0xA0
#define PAGE_SIZE 64
#define PAGE_NUM 512

typedef struct {
	float ka;
	float km;
	float ks;
} storage_data_t;

void storage_write(storage_data_t* data);
BOOL storage_read(storage_data_t* output);

#endif /* INC_DATA_STORAGE_H_ */
