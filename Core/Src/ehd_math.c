/*
 * ehd_math.c
 *
 *  Created on: May 9, 2022
 *      Author: infan
 */

#include "ehd_math.h"

uint16_t ehd_get_average(uint16_t* values, uint32_t count)
{
	uint32_t val = 0;
	for(uint32_t i = 0; i < count; i++)
	{
		val += values[i];
	}

	return (uint16_t)(val / count);
}
