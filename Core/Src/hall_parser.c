/*
 * hall_parser.c
 *
 *  Created on: 8 mag 2022
 *      Author: infan
 */

#include "hall_parser.h"

static inline void shift_array_left_u32(uint32_t* arr, uint32_t size);
static inline void shift_array_left_u16(uint16_t* arr, uint32_t size);
static inline void shift_array_left_f(float* arr, uint32_t size);
static inline void shift_array_left_st(systemtime_t* arr, uint32_t size);
static inline void shift_array_right_u32(uint32_t* arr, uint32_t size);
static inline void shift_array_right_u16(uint16_t* arr, uint32_t size);
static inline void shift_array_right_f(float* arr, uint32_t size);
static inline void shift_array_right_st(systemtime_t* arr, uint32_t size);
void compute_stroke(hall_parser_t* parser);
static inline void angular_velocity_measurement_received(hall_parser_t* parser);
static inline float get_angular_velocity_filtered(hall_parser_t* parser);
static inline STROKE_STATE get_stroke_state(hall_parser_t* parser);
static inline float compute_distance(float angle, float w2, float ka_c, float km_c, float ks_c);
static inline float compute_energy(hall_parser_t* parser, float w2, float w1, systemtime_t* t2, systemtime_t* t1);
static inline void apply_fir(arm_fir_instance_f32* instance, float32_t* data, float32_t* out_data, uint32_t data_size);
static inline BOOL is_w_a_minimum(hall_parser_t* parser);
static inline BOOL is_w_a_maximum(hall_parser_t* parser);

const int MOVING_AVERAGE_TOTAL_POINTS = MOVING_AVERAGE_POINTS_PER_SIDE*2 + 1;
const float ANG_VEL_NUMERATOR = MAGNETS_ANGLE * 1e6;
float fir_coefficients[] = ANGULAR_VELOCITY_FIR_COEFFS;
arm_fir_instance_f32 fir_instance;
float fir_state_buffer[ANGULAR_VELOCITY_FIR_COEFFS_SIZE+FIR_BLOCK_SIZE-1];

float regression_x[LMS_MAX_OBSERVATIONS];
float regression_y[LMS_MAX_OBSERVATIONS];

ergometer_params_t result_params;
systemtime_t system_time;

extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;

ergometer_stroke_params_t stroke_params = {0};

void hall_parser_init()
{
	arm_fir_init_f32(&fir_instance, ANGULAR_VELOCITY_FIR_COEFFS_SIZE, fir_coefficients, fir_state_buffer, FIR_BLOCK_SIZE);
}

static inline void shift_array_left_u32(uint32_t* arr, uint32_t size)
{
	for(uint32_t i = 1; i < size; i++)
	{
		arr[i-1] = arr[i];
	}
}

static inline void shift_array_left_u16(uint16_t* arr, uint32_t size)
{
	for(uint32_t i = 1; i < size; i++)
	{
		arr[i-1] = arr[i];
	}
}

static inline void shift_array_left_f(float* arr, uint32_t size)
{
	for(uint32_t i = 1; i < size; i++)
	{
		arr[i-1] = arr[i];
	}
}

static inline void shift_array_left_st(systemtime_t* arr, uint32_t size)
{
	for(uint32_t i = 1; i < size; i++)
	{
		arr[i-1] = arr[i];
	}
}

static inline void shift_array_right_u32(uint32_t* arr, uint32_t size)
{
	for(uint32_t i = size - 1; i > 0; i--)
	{
		arr[i] = arr[i-1];
	}
}

static inline void shift_array_right_u16(uint16_t* arr, uint32_t size)
{
	for(uint32_t i = size - 1; i > 0; i--)
	{
		arr[i] = arr[i-1];
	}
}

static inline void shift_array_right_f(float* arr, uint32_t size)
{
	for(uint32_t i = size - 1; i > 0; i--)
	{
		arr[i] = arr[i-1];
	}
}

static inline void shift_array_right_st(systemtime_t* arr, uint32_t size)
{
	for(uint32_t i = size - 1; i > 0; i--)
	{
		arr[i] = arr[i-1];
	}
}

void hall_parser_push_trigger(hall_parser_t* parser)
{
#ifdef DEBUG_RAW_HALL
	char buffer[] = "0\r\n1\r\n0\r\n";
	HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 100);
#else
	systemtime_get_time(&system_time);

	uint32_t delta_t = systemtime_time_diff_us(&system_time, &parser->angular_velocities_times[ANGULAR_VELOCITIES_BUFFER_SIZE - 1]);

#ifdef DEBUG_DELTA_T
	char buffer[64];
	sprintf(buffer, "%u,%u,%u,%u,%u\r\n", delta_t, system_time.ms, system_time.us,
			parser->angular_velocities_times[ANGULAR_VELOCITIES_BUFFER_SIZE - 1].ms,
			parser->angular_velocities_times[ANGULAR_VELOCITIES_BUFFER_SIZE - 1].us);
	HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 100);
#endif

	float w = (float)ANG_VEL_NUMERATOR/delta_t;

	if(w > ANGULAR_VELOCITY_LIMIT)
	{
		return;
	}

	parser->angular_velocity_callback(w);

	shift_array_left_f(parser->angular_velocities, ANGULAR_VELOCITIES_BUFFER_SIZE);
	shift_array_left_st(parser->angular_velocities_times, ANGULAR_VELOCITIES_BUFFER_SIZE);

	parser->base_point--;
	parser->turning_point--;
	parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE - 1] = w;
	memcpy(&parser->angular_velocities_times[ANGULAR_VELOCITIES_BUFFER_SIZE - 1], &system_time, sizeof(systemtime_t));

	if(parser->angular_velocities_times[ANGULAR_VELOCITIES_BUFFER_SIZE - 2].ms > 0)
	{
		angular_velocity_measurement_received(parser);
	}
#endif
}

static inline void angular_velocity_measurement_received(hall_parser_t* parser)
{
	// Apply filter (just averaging the last measurement with the previous one to remove zig-zag from curve)
	shift_array_left_f(parser->angular_velocities_filtered, ANGULAR_VELOCITIES_BUFFER_SIZE);
	parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE-1] =
			get_angular_velocity_filtered(parser);

	STROKE_STATE new_state = get_stroke_state(parser);

#ifdef DEBUG_ANGULAR_VELOCITIES
	char buffer[10];
	sprintf(buffer, "%.3f\r\n", parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-1]);
	HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 100);
#elif defined(DEBUG_ANGULAR_VELOCITIES_FILTERED)
	char buffer[10];
	sprintf(buffer, "%.3f\r\n", parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE-1]);
	HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 100);
#elif defined(DEBUG_ANGULAR_VELOCITIES_BOTH_NORMAL_AND_FILTERED)
	char buffer[16];
	sprintf(buffer, "%.3f,%.3f\r\n", parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-1], parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE-1]);
	HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 100);
#elif defined(DEBUG_ANGULAR_VELOCITIES_BOTH_NORMAL_AND_FILTERED_AND_DERIVATIVES)
	char buffer[256];
	float der_1 = parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-1] - parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-2];
	float der_f_1 = parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE-1] - parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE-2];
	float der_1_0 = parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-2] - parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-3];
	float der_f_1_0 = parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE-2] - parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE-3];
	float der_2 = der_1 - der_1_0;
	float der_f_2 = der_f_1 - der_f_1_0;
	sprintf(buffer, "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\r\n",
			parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-1],
			parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE-1],
			der_1,
			der_f_1,
			der_2,
			der_f_2);
	HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 100);
#elif defined(DEBUG_STROKE_STATE)
	char* buffer;
	if(new_state == PULLING) 
	{
		buffer = "PULLING\r\n";
	} else if(new_state == DECELERATING)
	{
		buffer = "DECELERATING\r\n";
	} else
	{
		buffer = "REST\r\n";
	}
	HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 100);
#else
	if(new_state == DECELERATING)
	{
		parser->turning_point = ANGULAR_VELOCITIES_BUFFER_SIZE-ANGULAR_VELOCITIES_LAG;
	} else if (new_state == PULLING)
	{
		compute_stroke(parser);
		parser->base_point = ANGULAR_VELOCITIES_BUFFER_SIZE-ANGULAR_VELOCITIES_LAG;
	}
/*
	if((parser->stroke_state == DECELERATING || parser->stroke_state == REST) && new_state == PULLING)
	{
		compute_stroke(parser);
		parser->base_point = ANGULAR_VELOCITIES_BUFFER_SIZE-ANGULAR_VELOCITIES_LAG+1;
	} else if(parser->stroke_state == PULLING && new_state == DECELERATING)
	{
		parser->turning_point = ANGULAR_VELOCITIES_BUFFER_SIZE-ANGULAR_VELOCITIES_LAG+1;
	}

	parser->stroke_state = new_state;
	*/
#endif
}

static inline STROKE_STATE get_stroke_state(hall_parser_t* parser)
{
	if(parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-ANGULAR_VELOCITIES_LAG] > ANGULAR_VELOCITY_ACTIVATION_TRESHOLD)
	{
		// Moving Average
		/*
		if(parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-1] < get_average_f(parser->angular_velocities + ANGULAR_VELOCITIES_BUFFER_SIZE - 4, 3))
		{
			return DECELERATING;
		} else if(parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-1] > get_average_f(parser->angular_velocities + ANGULAR_VELOCITIES_BUFFER_SIZE - 4, 3))
		{
			return PULLING;
		}
		 */

		// FIR Filter

		/*
		 * We operate on filtered data to remove the noise that would make the detection
		 * of slope change more difficult
		 */
		//apply_fir(&fir_instance, parser->angular_velocities, parser->angular_velocities_filtered, ANGULAR_VELOCITIES_BUFFER_SIZE);
		if(is_w_a_maximum(parser))
		{
			return DECELERATING;
		} else if(is_w_a_minimum(parser))
		{
			return PULLING;
		}
	}

	return REST;
}

static inline void apply_fir(arm_fir_instance_f32* instance, float32_t* data, float32_t* out_data, uint32_t data_size)
{
	uint32_t blocks_count = data_size / FIR_BLOCK_SIZE;
	for(uint32_t i = 0; i < blocks_count; i++)
	{
		arm_fir_f32(instance, data + i * FIR_BLOCK_SIZE, out_data + i * FIR_BLOCK_SIZE, FIR_BLOCK_SIZE);
	}
}

void compute_stroke(hall_parser_t* parser)
{
	if(parser->base_point < ANGULAR_VELOCITIES_BUFFER_SIZE && parser->turning_point < ANGULAR_VELOCITIES_BUFFER_SIZE &&
			parser->turning_point > parser->base_point && ANGULAR_VELOCITIES_BUFFER_SIZE - parser->turning_point > STROKE_DECEL_MIN_POINTS &&
			parser->turning_point - parser->base_point > STROKE_PULL_MIN_POINTS)
	{
		// If we don't have damping params (kA and kM) we can't go on
		if(parser->damping_constants.has_params == TRUE)
		{
			// Compute the energy spent
			float energy = 0.0f;
			float distance = 0.0f;

			float ka_c = parser->damping_constants.ka / DISTANCE_CORRELATION_COEFFICIENT;
			float km_c = parser->damping_constants.km / DISTANCE_CORRELATION_COEFFICIENT;
			float ks_c = parser->damping_constants.ks / DISTANCE_CORRELATION_COEFFICIENT;

			for(uint32_t i = parser->base_point; i < parser->turning_point; i++)
			{
				energy += -compute_energy(parser, parser->angular_velocities_filtered[i], parser->angular_velocities_filtered[i-1],
						&parser->angular_velocities_times[i], &parser->angular_velocities_times[i-1]);
				distance += -compute_distance(M_PI_2, parser->angular_velocities_filtered[i], ka_c, km_c, ks_c);
			}

			// Compute other variables
			float mean_power = (float)(1e6*energy) / systemtime_time_diff_us(&parser->angular_velocities_times[parser->turning_point-1], &parser->angular_velocities_times[parser->base_point]);
			uint32_t stroke_time = systemtime_time_diff_us(&parser->angular_velocities_times[ANGULAR_VELOCITIES_BUFFER_SIZE-2], &parser->angular_velocities_times[parser->turning_point]);

			// Send the stroke results
			stroke_params.energy_j = energy;
			stroke_params.mean_power = mean_power;
			stroke_params.distance = distance;
			parser->callback(&stroke_params);
		}

		// Update the damping factors through a linear fit of the deceleration velocities
		/**
		 * The deceleration phase is where the angular velocity decreases for the effect
		 * of the air resistance, magnetic resistance and other kinds of friction, thus we
		 * can find the damping parameters here through the formula:
		 *
		 * dw / dt = kA * w^2 + kM
		 *
		 * Here we assume the magnetic damping is independent of the angular velocity,
		 * (Tm = kM ===> the magnetic torque is constant through all the motion).
		 * This may not be the case as it could as well be proportional to the angular velocity
		 * (Tm = kM * w) and in this case we can fit to a polynomial.
		 *
		 * dw / dt = kA * w^2 + kM * w + kS
		 *
		 * The constant term kS in this case will account for the friction between the moving parts.
		 *
		 * The first equation can be fitted to a line through the least squares method,
		 * so we can update the kA and kM values after every stroke, taking into account
		 * every modification to the environment that could have happened
		 *
		 * Or we could fit to a quadratic equation
		 */
		uint32_t regression_count = ANGULAR_VELOCITIES_BUFFER_SIZE - parser->turning_point - 9;

		if(regression_count > REGRESSION_MIN_POINTS)
		{
			for(uint32_t i = parser->turning_point + 4; i < ANGULAR_VELOCITIES_BUFFER_SIZE-5; i++)
			{
				regression_y[i - parser->turning_point - 4] = 1e6f*(parser->angular_velocities_filtered[i] - parser->angular_velocities_filtered[i-1])
												/ systemtime_time_diff_us(&parser->angular_velocities_times[i], &parser->angular_velocities_times[i-1]);
				regression_x[i - parser->turning_point - 4] = parser->angular_velocities_filtered[i];
			}

			lms_result_t* result = lms_quadratic(regression_y, regression_x, regression_count);

			// Here we check if the regression is good enough through the R2 parameter,
			// otherwise we discard the results
			if(result != NULL && result->r2 > LINREG_R2_MIN_TRESHOLD)
			{
				parser->damping_constants.ka = result->c;
				parser->damping_constants.km = result->b;
				parser->damping_constants.ks = result->a;
				parser->damping_constants.has_params = TRUE;

				parser->damping_params_callback(&parser->damping_constants);

#ifdef DEBUG_DAMPING_CONSTANTS
				char buffer[48];
				sprintf(buffer, "kA=%4.3e,kM=%4.3e,kS=%4.3e,r2=%.3f\r\n", result->c, result->b, result->a, result->r2);
				HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 100);
#endif
			}
		}
	}
}

static inline float compute_distance(float angle, float w2, float ka_c, float km_c, float ks_c)
{
	return angle * cbrt(ka_c + km_c/w2 + ks_c/sqr(w2));
}

static inline float compute_energy(hall_parser_t* parser, float w2, float w1, systemtime_t* t2, systemtime_t* t1)
{
	/** Using the formula
	 * dE = dTheta * (I * dw/dt + kA * w^2 + kM * w + kS)
	 * This formula includes a magnetic damping (kM) constant
	 * because the ergometer has two permanent magnets to increase the resistance
	 * and kS accounts for other factors that do not depend on angular velocity
	 */
	if(t2 - t1 > 0)
	{
		return
				M_PI_2*(
						parser->params.I *
						1e6f * (w2 - w1) / systemtime_time_diff_us(t2, t1)
						+ parser->damping_constants.ka*sqr(w2)
		+ parser->damping_constants.km*w2
		+ parser->damping_constants.ks
				);
	} else
	{
		return 0;
	}
}

static inline float get_angular_velocity_filtered(hall_parser_t* parser)
{
	// Averaging with last 3 measurements
	// The n-1 and n-2 coefficients are the same to remove the zig-zag noise
	
	return 0.3f * parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-1] 
		+ 0.3f * parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-2]
		+ 0.2f * parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-3]
		+ 0.2f * parser->angular_velocities[ANGULAR_VELOCITIES_BUFFER_SIZE-4];
}

static inline BOOL is_w_a_minimum(hall_parser_t* parser)
{
	BOOL result = TRUE;
	float w = parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE - ANGULAR_VELOCITIES_LAG - 1];
	for(uint32_t i = ANGULAR_VELOCITIES_BUFFER_SIZE - ANGULAR_VELOCITIES_LAG*2 - 1; i < ANGULAR_VELOCITIES_BUFFER_SIZE; i++)
	{
		if(i == ANGULAR_VELOCITIES_BUFFER_SIZE - ANGULAR_VELOCITIES_LAG - 1)
		{
			continue;
		}
		if(w >= parser->angular_velocities_filtered[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}

static inline BOOL is_w_a_maximum(hall_parser_t* parser)
{
	BOOL result = TRUE;
	float w = parser->angular_velocities_filtered[ANGULAR_VELOCITIES_BUFFER_SIZE - ANGULAR_VELOCITIES_LAG - 1];
	for(uint32_t i = ANGULAR_VELOCITIES_BUFFER_SIZE - ANGULAR_VELOCITIES_LAG*2 - 1; i < ANGULAR_VELOCITIES_BUFFER_SIZE; i++)
	{
		if(i == ANGULAR_VELOCITIES_BUFFER_SIZE - ANGULAR_VELOCITIES_LAG - 1)
		{
			continue;
		}
		if(w <= parser->angular_velocities_filtered[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}
