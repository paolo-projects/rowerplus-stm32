/*
 * fixed_vector.h
 *
 *  Created on: May 24, 2022
 *      Author: infan
 */

#ifndef INC_FIXED_VECTOR_H_
#define INC_FIXED_VECTOR_H_

#include <stdint.h>
#include "systemtime.h"
#include "common.h"
#include "config.h"

typedef struct {
	float buffer[FIXED_VECTOR_SIZE];
	size_t size;
} fixed_vector_float_t;


typedef struct {
	systemtime_t buffer[FIXED_VECTOR_SIZE];
	size_t size;
} fixed_vector_systemtime_t;

BOOL fixed_vector_f_push_back(fixed_vector_float_t* fixed_vector, float value);
BOOL fixed_vector_st_push_back(fixed_vector_systemtime_t* fixed_vector, const systemtime_t* value);

BOOL fixed_vector_f_push_front(fixed_vector_float_t* fixed_vector, float value);
BOOL fixed_vector_st_push_front(fixed_vector_systemtime_t* fixed_vector, const systemtime_t* value);

BOOL fixed_vector_f_pop_back(fixed_vector_float_t* fixed_vector);
BOOL fixed_vector_st_pop_back(fixed_vector_systemtime_t* fixed_vector);

BOOL fixed_vector_f_pop_front(fixed_vector_float_t* fixed_vector);
BOOL fixed_vector_st_pop_front(fixed_vector_systemtime_t* fixed_vector);

float* fixed_vector_f_get(fixed_vector_float_t* fixed_vector, size_t index);
systemtime_t* fixed_vector_st_get(fixed_vector_systemtime_t* fixed_vector, size_t index);

size_t fixed_vector_f_get_size(fixed_vector_float_t* fixed_vector);
size_t fixed_vector_st_get_size(fixed_vector_systemtime_t* fixed_vector);

void fixed_vector_f_clear(fixed_vector_float_t* fixed_vector);
void fixed_vector_st_clear(fixed_vector_systemtime_t* fixed_vector);

#endif /* INC_FIXED_VECTOR_H_ */
