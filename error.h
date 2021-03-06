/*
 * error.h
 *
 *  Created on: 14 nov. 2017
 *      Author: timmy.lindholm
 */

#ifndef ERROR_H_
#define ERROR_H_

#define T_CRIT 0
#define T_WARNING 1

// Macro to output an error and cease execution if
// the type is of error
#define ERROR(error_type, format, ...) {fprintf(stderr, error_type ? "Warning: " : "Error: "); fprintf(stderr, format, __VA_ARGS__);  if(error_type == T_CRIT) {exit(0);}}


#endif /* ERROR_H_ */
