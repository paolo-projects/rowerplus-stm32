/*
 * least_mean_squares.c
 *
 *  Created on: 14 mag 2022
 *      Author: infan
 */

#include "least_mean_squares.h"

#ifdef DEBUG_ARM_DSP
extern UART_HandleTypeDef huart2;
#define LMS_CHECK_STATUS(x) if((x)!=ARM_MATH_SUCCESS) { \
	char message[] = "An error occurred in lms library\r\n"; \
	HAL_Uart_Transmit(&huart2, message, strlen(message), 100); \
	return NULL; \
}
#else
#define LMS_CHECK_STATUS(x) if((x)!=ARM_MATH_SUCCESS) {return NULL;}
#endif

static inline void build_A_mat(float* x_data, uint32_t size, float* matrix);

float A_m[LMS_MAX_OBSERVATIONS*3];
float AT_m[LMS_MAX_OBSERVATIONS*3];
float ATMA_m[LMS_MAX_OBSERVATIONS*3];
float ATMAI_m[LMS_MAX_OBSERVATIONS*3];
float ATMAI_AT_m[LMS_MAX_OBSERVATIONS*3];
float X_m[3];

lms_result_t lms_result;

arm_matrix_instance_f32 A;
arm_matrix_instance_f32 AT;
arm_matrix_instance_f32 ATMA;
arm_matrix_instance_f32 ATMAI;
arm_matrix_instance_f32 ATMAI_AT;
arm_matrix_instance_f32 B;
arm_matrix_instance_f32 X;

lms_result_t* lms_quadratic(float* y_data, float* x_data, uint32_t size)
{
	// A is the matrix with the x values
	// B is the matrix (vector) with the y values
	// X is the matrix with the coefficients

	// X = (At * A)^(-1) * At * B
	arm_mat_init_f32(&A, size, 3, A_m);
	arm_mat_init_f32(&AT, 3, size, AT_m);
	arm_mat_init_f32(&ATMA, 3, 3, ATMA_m);
	arm_mat_init_f32(&ATMAI, 3, 3, ATMAI_m);
	arm_mat_init_f32(&ATMAI_AT, 3, size, ATMAI_AT_m);
	arm_mat_init_f32(&B, size, 1, y_data);
	arm_mat_init_f32(&X, 3, 1, X_m);

	build_A_mat(x_data, size, A_m);

	LMS_CHECK_STATUS(arm_mat_trans_f32(&A, &AT));
	LMS_CHECK_STATUS(arm_mat_mult_f32(&AT, &A, &ATMA));
	LMS_CHECK_STATUS(arm_mat_inverse_f32(&ATMA, &ATMAI));
	LMS_CHECK_STATUS(arm_mat_mult_f32(&ATMAI, &AT, &ATMAI_AT));
	LMS_CHECK_STATUS(arm_mat_mult_f32(&ATMAI_AT, &B, &X));

	float y_avg, ss_res, ss_tot;
	arm_mean_f32(y_data, size, &y_avg);

	for(uint32_t i = 0; i < size; i++)
	{
		ss_res += sqr(y_data[i] - (X_m[0] + X_m[1]*A_m[i*3+1] + X_m[2]*A_m[i*3+2]));
		ss_tot += sqr(y_data[i] - y_avg);
	}

	lms_result.a = X_m[0];
	lms_result.b = X_m[1];
	lms_result.c = X_m[2];
	lms_result.r2 = 1 - ss_res/ss_tot;

	return &lms_result;
}

static inline void build_A_mat(float* x_data, uint32_t size, float* matrix)
{
	// Since we want a quadratic fit, we build the A matrix from the x values with:
	// the first column having value 1 (constant coefficient)
	// the second column having value x (linear coefficient)
	// the third one having value x^2 (quadratic coefficient)

	for(uint32_t i = 0; i < size; i++)
	{
		matrix[i*3] = 1;
		matrix[i*3+1] = x_data[i];
		matrix[i*3+2] = sqr(x_data[i]);
	}
}
