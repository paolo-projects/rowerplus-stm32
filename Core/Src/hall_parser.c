/*
 * hall_parser.c
 *
 *  Created on: 8 mag 2022
 *      Author: infan
 */

#include "hall_parser.h"

static inline void compute_stroke(hall_parser_t* parser);
static inline void compute_stroke_params(hall_parser_t* parser);
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

void hall_parser_push_trigger(hall_parser_t* parser)
{
	systemtime_get_time(&system_time);
	uint32_t delta_t = systemtime_time_diff_us(&system_time, fixed_vector_st_get(&parser->angular_velocities_times, parser->angular_velocities_times.size - 1));

	float w = (float)ANG_VEL_NUMERATOR/delta_t;

	if(w > ANGULAR_VELOCITY_MAX_LIMIT)
	{
		return;
	}

	fixed_vector_f_push_back(&parser->angular_velocities, w);
	fixed_vector_st_push_back(&parser->angular_velocities_times, &system_time);

	if(parser->angular_velocities_times.size >= 4 && w > ANGULAR_VELOCITY_MIN_LIMIT)
	{
		angular_velocity_measurement_received(parser);
	}
}

static inline void angular_velocity_measurement_received(hall_parser_t* parser)
{
	// Apply filter (just averaging the last measurement with the previous ones to remove zig-zag from curve)
	float filtered_angular_velocity = get_angular_velocity_filtered(parser);
	fixed_vector_f_push_back(&parser->angular_velocities_filtered, filtered_angular_velocity);
	fixed_vector_st_push_back(&parser->angular_velocities_filtered_times, fixed_vector_st_get(&parser->angular_velocities_times, parser->angular_velocities_times.size - 1));

	if(parser->angular_velocities_filtered.size > ANGULAR_VELOCITIES_LAG*2)
	{
		STROKE_STATE new_state = get_stroke_state(parser);

		if(is_w_a_maximum(parser))//parser->stroke_state == PULLING && new_state == DECELERATING)
		{
			compute_stroke(parser);

			fixed_vector_f_clear(&parser->angular_velocities);
			fixed_vector_f_clear(&parser->angular_velocities_filtered);
			fixed_vector_st_clear(&parser->angular_velocities_times);
			fixed_vector_st_clear(&parser->angular_velocities_filtered_times);
		} else if (is_w_a_minimum(parser))//(parser->stroke_state == DECELERATING || parser->stroke_state == REST) && new_state == PULLING)
		{
			compute_stroke_params(parser);

			fixed_vector_f_clear(&parser->angular_velocities);
			fixed_vector_f_clear(&parser->angular_velocities_filtered);
			fixed_vector_st_clear(&parser->angular_velocities_times);
			fixed_vector_st_clear(&parser->angular_velocities_filtered_times);
		}

		parser->stroke_state = new_state;
	}
}

static inline STROKE_STATE get_stroke_state(hall_parser_t* parser)
{
	if(*fixed_vector_f_get(&parser->angular_velocities, parser->angular_velocities.size-1) > ANGULAR_VELOCITY_ACTIVATION_TRESHOLD)
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

static inline void compute_stroke(hall_parser_t* parser)
{
	// We discard 3 points at the end because the peak detection has a delay of 3 points
	int to_discard_end = 3;

	int points_count = parser->angular_velocities_filtered.size - to_discard_end;
	if(points_count > STROKE_PULL_MIN_POINTS)
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

			for(uint32_t i = 0; i < parser->angular_velocities_filtered.size; i++)
			{
				if(i > 0 && i < parser->angular_velocities_filtered.size - to_discard_end)
				{
					energy += compute_energy(parser, *fixed_vector_f_get(&parser->angular_velocities_filtered, i), *fixed_vector_f_get(&parser->angular_velocities_filtered, i-1),
							fixed_vector_st_get(&parser->angular_velocities_filtered_times, i), fixed_vector_st_get(&parser->angular_velocities_filtered_times, i-1));
				}
				distance += compute_distance(M_PI_2, *fixed_vector_f_get(&parser->angular_velocities_filtered, i), ka_c, km_c, ks_c);
			}

			// Compute other variables
			uint32_t stroke_time = systemtime_time_diff_us(fixed_vector_st_get(&parser->angular_velocities_filtered_times, parser->angular_velocities_filtered_times.size - 1 - to_discard_end), fixed_vector_st_get(&parser->angular_velocities_filtered_times, 0));
			float mean_power = (float)(1e6*energy) / stroke_time;

			// Send the stroke results
			stroke_params.energy_kcal = (4 * energy + 0.35f * stroke_time / 1e3f) / 4187.0f;
			stroke_params.mean_power = mean_power;
			// We include here the distance calculated from the previous release phase
			stroke_params.distance = distance + parser->release_distance;
			parser->callback(&stroke_params);

			parser->release_distance = 0.0f;
		}
	}
}

static inline void compute_stroke_params(hall_parser_t* parser)
{
	float ka_c = parser->damping_constants.ka / DISTANCE_CORRELATION_COEFFICIENT;
	float km_c = parser->damping_constants.km / DISTANCE_CORRELATION_COEFFICIENT;
	float ks_c = parser->damping_constants.ks / DISTANCE_CORRELATION_COEFFICIENT;

	parser->release_distance = 0.0f;

	// The distance for the release phase (deceleration) is calculated here taking
	// all the points into account. This value will be added to the next pull phase
	for(uint32_t i = 0; i < parser->angular_velocities_filtered.size; i++)
	{
		parser->release_distance += compute_distance(M_PI_2, *fixed_vector_f_get(&parser->angular_velocities_filtered, i), ka_c, km_c, ks_c);
	}

	// Update the damping factors through a linear fit of the deceleration velocities
	/**
	 * The deceleration phase is where the angular velocity decreases for the effect
	 * of the air resistance, magnetic resistance and other kinds of friction, thus we
	 * can find the damping parameters here through the formula:
	 *
	 * dw / dt = -kA/I * w^2 - kM/I
	 *
	 * Here we assume the magnetic damping is independent of the angular velocity,
	 * (Tm = kM ===> the magnetic torque is constant through all the motion).
	 * This may not be the case as it could as well be proportional to the angular velocity
	 * (Tm = kM * w) and in this case we can fit to a polynomial.
	 *
	 * dw / dt = -kA/I * w^2 - kM/I * w - kS/I
	 *
	 * The constant term kS in this case will account for the friction between the moving parts.
	 *
	 * The first equation can be fitted to a line through the least squares method,
	 * so we can update the kA and kM values after every stroke, taking into account
	 * every modification to the environment that could have happened
	 *
	 * In this case we fit to a quadratic equation
	 */

	// We have to discard:
	// 2 measurements at the beginning (because we're too close to the peak, the flywheel is still subject to external torque)
	// 4 measurements at the end (because we have a couple of points of the next pull phase (due to the filter lag) and the error gets large due to low w)

	int to_discard_begin = 2;
	int to_discard_end = 4;

	// The points used for the regression are the measurements minus the points discarded (for the previous reasons)
	// minus an additional point because having to calculate the angular acceleration (w1 - w0) we have to discard
	// another point
	int regression_count = parser->angular_velocities_filtered.size - to_discard_begin - to_discard_end - 1;

	if(regression_count > REGRESSION_MIN_POINTS)
	{

		int n = 0;

		for(uint32_t i = to_discard_begin + 1; i < parser->angular_velocities_filtered.size - to_discard_end; i++)
		{
			float angular_accel = 1e6f*(*fixed_vector_f_get(&parser->angular_velocities_filtered, i) - *fixed_vector_f_get(&parser->angular_velocities_filtered, i-1))
																					/ systemtime_time_diff_us(fixed_vector_st_get(&parser->angular_velocities_filtered_times, i), fixed_vector_st_get(&parser->angular_velocities_filtered_times, i-1));
			if(angular_accel<0) {
				regression_y[n] = angular_accel;
				regression_x[n] = *fixed_vector_f_get(&parser->angular_velocities_filtered, i);
			}
			n++;
		}

		lms_result_t* result = lms_quadratic(regression_y, regression_x, n);

		// Here we check if the regression is good enough through the R2 parameter,
		// otherwise we discard the results
		if(result != NULL && result->r2 > LINREG_R2_MIN_TRESHOLD)
		{
			parser->damping_constants.ka = -result->c * parser->params.I;
			parser->damping_constants.km = -result->b * parser->params.I;
			parser->damping_constants.ks = -result->a * parser->params.I;
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

	float fir_val = 0.0f;

	fir_val += 0.3f * *fixed_vector_f_get(&parser->angular_velocities, parser->angular_velocities.size - 1);

	fir_val += 0.3f * ((parser->angular_velocities.size > 1) ? *fixed_vector_f_get(&parser->angular_velocities, parser->angular_velocities.size - 2) : 0.0f);
	fir_val += 0.2f * ((parser->angular_velocities.size > 2) ? *fixed_vector_f_get(&parser->angular_velocities, parser->angular_velocities.size - 3) : 0.0f);
	fir_val += 0.2f * ((parser->angular_velocities.size > 3) ? *fixed_vector_f_get(&parser->angular_velocities, parser->angular_velocities.size - 4) : 0.0f);

	return fir_val;
}

static inline BOOL is_w_a_minimum(hall_parser_t* parser)
{
	float w = *fixed_vector_f_get(&parser->angular_velocities_filtered, parser->angular_velocities_filtered.size - ANGULAR_VELOCITIES_LAG - 1);
	for(uint32_t i = parser->angular_velocities_filtered.size - ANGULAR_VELOCITIES_LAG*2 - 1; i < parser->angular_velocities_filtered.size; i++)
	{
		if(i == parser->angular_velocities_filtered.size - ANGULAR_VELOCITIES_LAG - 1)
		{
			continue;
		}
		if(w >= *fixed_vector_f_get(&parser->angular_velocities_filtered, i))
		{
			return FALSE;
		}
	}
	return TRUE;
}

static inline BOOL is_w_a_maximum(hall_parser_t* parser)
{
	float w = *fixed_vector_f_get(&parser->angular_velocities_filtered, parser->angular_velocities_filtered.size - ANGULAR_VELOCITIES_LAG - 1);
	for(uint32_t i = parser->angular_velocities_filtered.size - ANGULAR_VELOCITIES_LAG*2 - 1; i < parser->angular_velocities_filtered.size; i++)
	{
		if(i == parser->angular_velocities_filtered.size - ANGULAR_VELOCITIES_LAG - 1)
		{
			continue;
		}
		if(w <= *fixed_vector_f_get(&parser->angular_velocities_filtered, i))
		{
			return FALSE;
		}
	}
	return TRUE;
}
