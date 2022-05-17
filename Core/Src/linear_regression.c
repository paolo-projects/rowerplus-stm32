/*
 * linear_regression.c
 *
 *  Created on: May 8, 2022
 *      Author: infan
 */

#include "linear_regression.h"

linear_regression_result_t linear_regression_result;

linear_regression_result_t* linear_regression_compute(linear_regression_t* regression)
{
	float y_mean = 0.0f;
	float x_mean = 0.0f;
	float ss_res = 0.0f;
	float ss_tot = 0.0f;

	for(uint32_t i = 0; i < regression->count; i++)
	{
		x_mean += regression->x[i];
		y_mean += regression->y[i];
	}

	x_mean /= regression->count;
	y_mean /= regression->count;

	float numerator = 0.0f;
	float denominator = 0.0f;

	for(uint32_t i = 0; i < regression->count; i++)
	{
		numerator += (regression->x[i] - x_mean)*(regression->y[i] - y_mean);
		denominator += sqr(regression->x[i] - x_mean);

		ss_tot += sqr(regression->y[i]- y_mean);
	}

	linear_regression_result.m = numerator / denominator;
	linear_regression_result.q = y_mean - linear_regression_result.m * x_mean;

	for(uint32_t i = 0; i < regression->count; i++)
	{
		ss_res = sqr(regression->y[i] - linear_regression_result.m*regression->x[i] - linear_regression_result.q);
	}

	linear_regression_result.r2 = 1 - ss_res/ss_tot;

	return &linear_regression_result;
}
