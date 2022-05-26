#ifndef HALLPARSER_H
#define HALLPARSER_H

#include <math.h>
#include <assert.h>
#include <stdint.h>
#include "systemtime.h"
#include "fpu_math.h"
#include "least_mean_squares.h"
#include "ehd_math.h"
#include "fixed_vector.h"
#include "common.h"
#include "config.h"

typedef struct {
	float energy_kcal;
	float mean_power;
	float distance;
} ergometer_stroke_params_t;

typedef struct {
	float I; // moment of inertia of the flywheel
} ergometer_params_t;

typedef struct {
	BOOL has_params;
	float ka; // air damping coefficient - proportional to w^2
	float km; // magnetic damping coefficient - proportional to w
	float ks; // static damping coefficient - constant
} damping_constants_t;

typedef void(*ergometer_stroke_callback)(ergometer_stroke_params_t* stroke_params);
typedef void(*ergometer_damping_constants_callback)(damping_constants_t* stroke_params);
typedef void(*ergometer_angular_velocity_callback)(float angular_velocity);

typedef enum {
	REST,
	DECELERATING,
	PULLING,
} STROKE_STATE;

typedef struct {
	STROKE_STATE stroke_state;
	damping_constants_t damping_constants;
	fixed_vector_float_t angular_velocities;
	fixed_vector_systemtime_t angular_velocities_times;
	fixed_vector_float_t angular_velocities_filtered;
	fixed_vector_systemtime_t angular_velocities_filtered_times;
	ergometer_stroke_callback callback;
	ergometer_damping_constants_callback damping_params_callback;
	ergometer_params_t params;
	float release_distance;
} hall_parser_t;

void hall_parser_init();
void hall_parser_push_trigger(hall_parser_t* parser);

#endif
