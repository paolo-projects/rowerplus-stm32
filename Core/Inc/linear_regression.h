/*
 * linear_regression.h
 *
 *  Created on: May 8, 2022
 *      Author: infan
 */

#ifndef INC_LINEAR_REGRESSION_H_
#define INC_LINEAR_REGRESSION_H_

#include <stdint.h>
#include "ehd_math.h"

typedef struct {
	float m;
	float q;
	float r2;
} linear_regression_result_t;

typedef struct {
	float* x;
	float* y;
	uint32_t count;
} linear_regression_t;

linear_regression_result_t* linear_regression_compute(linear_regression_t* regression);

#endif /* INC_LINEAR_REGRESSION_H_ */
