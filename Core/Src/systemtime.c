/*
 * systemtime.c
 *
 *  Created on: 14 mag 2022
 *      Author: infan
 */

#include "systemtime.h"

uint32_t systemtime_ms = 0;
extern TIM_HandleTypeDef htim1;

void systemtime_increase_ms()
{
	systemtime_ms++;
}

void systemtime_get_time(systemtime_t* st)
{
	st->ms = systemtime_ms;
	st->us = __HAL_TIM_GET_COUNTER(&htim1);
}

float systemtime_time_diff_ms(systemtime_t* st1, systemtime_t* st2)
{
	return (st1->ms - st2->ms) + ((float)st1->us - st2->us)/1000.0f;
}

uint32_t systemtime_time_diff_us(systemtime_t* st1, systemtime_t* st2)
{
	return (st1->ms - st2->ms)*1000 + (st1->us - st2->us);
}
