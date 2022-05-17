/*
 * ehd_math.h
 *
 *  Created on: May 9, 2022
 *      Author: infan
 */

#ifndef INC_EHD_MATH_H_
#define INC_EHD_MATH_H_

#include <stdint.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
    #define M_PI_2 1.57079632679489661923
#endif

#define sqr(x) ((x)*(x))
#define min(x, y) (((x)<(y)) ? (x) : (y))
#define max(x, y) (((x)>(y)) ? (x) : (y))

uint16_t ehd_get_average(uint16_t* values, uint32_t count);



#endif /* INC_EHD_MATH_H_ */
