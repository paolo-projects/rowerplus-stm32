/*
 * fixed_vector.c
 *
 *  Created on: May 24, 2022
 *      Author: infan
 */

#include "fixed_vector.h"

void fixed_vector_f_shift_r(fixed_vector_float_t* fixed_vector);
void fixed_vector_st_shift_r(fixed_vector_float_t* fixed_vector);
void fixed_vector_f_shift_l(fixed_vector_float_t* fixed_vector);
void fixed_vector_st_shift_l(fixed_vector_float_t* fixed_vector);

// =================================================

BOOL fixed_vector_f_push_back(fixed_vector_float_t* fixed_vector, float value)
{
	if(fixed_vector->size < FIXED_VECTOR_SIZE)
	{
		fixed_vector->buffer[fixed_vector->size] = value;
		fixed_vector->size++;

		return TRUE;
	}
	return FALSE;
}
BOOL fixed_vector_st_push_back(fixed_vector_systemtime_t* fixed_vector, const systemtime_t* value)
{
	if(fixed_vector->size < FIXED_VECTOR_SIZE)
	{
		memcpy(&fixed_vector->buffer[fixed_vector->size], value, sizeof(systemtime_t));
		fixed_vector->size++;

		return TRUE;
	}
	return FALSE;
}

BOOL fixed_vector_f_push_front(fixed_vector_float_t* fixed_vector, float value)
{
	if(fixed_vector->size < FIXED_VECTOR_SIZE)
	{
		fixed_vector_f_shift_r(fixed_vector);
		fixed_vector->buffer[0] = value;
		fixed_vector->size++;

		return TRUE;
	}
	return FALSE;
}
BOOL fixed_vector_st_push_front(fixed_vector_systemtime_t* fixed_vector, const systemtime_t* value)
{
	if(fixed_vector->size < FIXED_VECTOR_SIZE)
	{
		fixed_vector_st_shift_r(fixed_vector);
		memcpy(&fixed_vector->buffer[0], value, sizeof(systemtime_t));
		fixed_vector->size++;

		return TRUE;
	}
	return FALSE;
}

BOOL fixed_vector_f_pop_back(fixed_vector_float_t* fixed_vector)
{
	if(fixed_vector->size > 0)
	{
		fixed_vector->size--;
		return TRUE;
	}
	return FALSE;
}
BOOL fixed_vector_st_pop_back(fixed_vector_systemtime_t* fixed_vector)
{
	if(fixed_vector->size > 0)
	{
		fixed_vector->size--;
		return TRUE;
	}
	return FALSE;
}

BOOL fixed_vector_f_pop_front(fixed_vector_float_t* fixed_vector)
{
	if(fixed_vector->size > 0)
	{
		fixed_vector_f_shift_l(fixed_vector);
		fixed_vector->size--;
		return TRUE;
	}
	return FALSE;
}
BOOL fixed_vector_st_pop_front(fixed_vector_systemtime_t* fixed_vector)
{
	if(fixed_vector->size > 0)
	{
		fixed_vector_st_shift_l(fixed_vector);
		fixed_vector->size--;
		return TRUE;
	}
	return FALSE;
}

float* fixed_vector_f_get(fixed_vector_float_t* fixed_vector, size_t index)
{
	if(index < fixed_vector->size)
	{
		return &fixed_vector->buffer[index];
	}
	return NULL;
}
systemtime_t* fixed_vector_st_get(fixed_vector_systemtime_t* fixed_vector, size_t index)
{
	if(index < fixed_vector->size)
	{
		return &fixed_vector->buffer[index];
	}
	return NULL;
}

size_t fixed_vector_f_get_size(fixed_vector_float_t* fixed_vector)
{
	return fixed_vector->size;
}
size_t fixed_vector_st_get_size(fixed_vector_systemtime_t* fixed_vector)
{
	return fixed_vector->size;
}

void fixed_vector_f_clear(fixed_vector_float_t* fixed_vector)
{
	fixed_vector->size = 0;
}
void fixed_vector_st_clear(fixed_vector_systemtime_t* fixed_vector)
{
	fixed_vector->size = 0;
}

void fixed_vector_f_shift_r(fixed_vector_float_t* fixed_vector)
{
	for(size_t i = fixed_vector->size; i > 0; i--)
	{
		fixed_vector->buffer[i] = fixed_vector->buffer[i-1];
	}
}
void fixed_vector_st_shift_r(fixed_vector_float_t* fixed_vector)
{
	for(size_t i = fixed_vector->size; i > 0; i--)
	{
		memccpy(&fixed_vector->buffer[i], &fixed_vector->buffer[i-1], sizeof(systemtime_t));
	}
}

void fixed_vector_f_shift_l(fixed_vector_float_t* fixed_vector)
{
	for(size_t i = 1; i < fixed_vector->size; i++)
	{
		fixed_vector->buffer[i-1] = fixed_vector->buffer[i];
	}
}
void fixed_vector_st_shift_l(fixed_vector_float_t* fixed_vector)
{
	for(size_t i = 1; i < fixed_vector->size; i++)
	{
		memcpy(&fixed_vector->buffer[i-1], &fixed_vector->buffer[i], sizeof(systemtime_t));
	}
}
