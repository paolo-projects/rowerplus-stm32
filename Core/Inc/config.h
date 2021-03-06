/*
 * config.h
 *
 *  Created on: 10 mag 2022
 *      Author: infan
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

// By enabling one of the following debug definitions the firmware
// will print the variable to the serial as string-formatted number

//#define DEBUG_RAW_HALL
//#define DEBUG_DELTA_T
//#define DEBUG_ANGULAR_VELOCITIES
//#define DEBUG_ANGULAR_VELOCITIES_FILTERED
//#define DEBUG_ANGULAR_VELOCITIES_BOTH_NORMAL_AND_FILTERED
//#define DEBUG_ANGULAR_VELOCITIES_BOTH_NORMAL_AND_FILTERED_AND_DERIVATIVES
//#define DEBUG_STROKE_STATE
//#define DEBUG_STROKE_PARAMS
//#define DEBUG_DAMPING_CONSTANTS

#define FIXED_VECTOR_SIZE 128

#define MAGNETS_ANGLE M_PI/2
//#define MOMENT_OF_INERTIA 5.337e-3f  // A good estimate
//#define MOMENT_OF_INERTIA 1.454e-2f  // Approximation to a solid cylinder of same mass
#define MOMENT_OF_INERTIA 4.361e-2f  // Looser approximation to make results fit
#define DISTANCE_CORRELATION_COEFFICIENT 2.8f

#define ANGULAR_VELOCITIES_BUFFER_SIZE 256
#define ANGULAR_VELOCITIES_LAG 3

#define FIR_BLOCK_SIZE 64

#define ANGULAR_VELOCITY_ACTIVATION_TRESHOLD 10.0f
#define ANGULAR_VELOCITY_MAX_LIMIT 250.0f
#define ANGULAR_VELOCITY_MIN_LIMIT 1.0f

#define ANGULAR_VELOCITY_FIR_COEFFS { \
		5/14.0f, \
		5/14.0f, \
		2/14.0f, \
		2/14.0f, \
}
#define ANGULAR_VELOCITY_FIR_COEFFS_SIZE 4

#define SAVGOL_COEFFS { \
	-2/21.0f, \
	3/21.0f, \
	6/21.0f, \
	7/21.0f, \
	6/21.0f, \
	3/21.0f, \
	-2/21.0f, \
}

#define MOVING_AVERAGE_POINTS_PER_SIDE 2
#define TRIGGER_DEBOUNCE_MS 15

#define STROKE_PULL_MIN_POINTS 7

#define REGRESSION_MIN_POINTS 24
#define LMS_MAX_OBSERVATIONS 128

#define LINREG_R2_MIN_TRESHOLD 0.9f

#endif /* INC_CONFIG_H_ */
