/*
 * systemtime.h
 *
 *  Created on: 14 mag 2022
 *      Author: infan
 */

#ifndef INC_SYSTEMTIME_H_
#define INC_SYSTEMTIME_H_

#include "stm32f4xx_hal.h"

/*
	Time structure that holds both
	milliseconds and microseconds separately
	for more accurate measurements
	and at the same time avoiding overflow.
	The ms overflows in around 50 days
*/
typedef struct {
	uint32_t ms;
	uint32_t us;
} systemtime_t;

void systemtime_increase_ms();

/*
	This function reads the milliseconds from the interrupt counter
	and the microseconds from the timer internal counter
*/
void systemtime_get_time(systemtime_t* st);

/*
	Get the time difference between two time points in milliseconds
	(as a float)
	The float will not overflow but will loose precision as the numbers
	get higher
*/
float systemtime_time_diff_ms(systemtime_t* st1, systemtime_t* st2);

/*
	Get the time difference between two time points in microseconds
	(as a uint32_t)
	The uint32 counting microseconds overflows in around 70 minutes
*/
uint32_t systemtime_time_diff_us(systemtime_t* st1, systemtime_t* st2);

#endif /* INC_SYSTEMTIME_H_ */
