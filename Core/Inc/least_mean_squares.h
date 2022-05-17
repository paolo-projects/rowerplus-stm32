/*
 * least_mean_squares.h
 *
 *  Created on: 14 mag 2022
 *      Author: infan
 */

#ifndef INC_LEAST_MEAN_SQUARES_H_
#define INC_LEAST_MEAN_SQUARES_H_

#include "fpu_math.h"
#include <math.h>
#include "ehd_math.h"
#include "common.h"
#include "config.h"

typedef struct {
	float a;
	float b;
	float c;
	float r2;
} lms_result_t;

lms_result_t* lms_quadratic(float* y_data, float* x_data, uint32_t size);

#endif /* INC_LEAST_MEAN_SQUARES_H_ */
