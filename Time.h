/*
 * Time.h
 *
 *  Created on: Mar 5, 2018
 *      Author: lei-n
 */

#ifndef TIME_H_
#define TIME_H_

#include "stdint.h"

#define SECONDS_LIMIT 60
#define MINUTES_LIMIT 60
#define HOURS_LIMIT 24

typedef enum
{
    SECONDS,
    MINUTES,
    HOURS
}time_types_t;

typedef struct
{
    uint8_t  seconds;
    uint8_t  minutes;
    uint8_t  hours;
}Time;

typedef struct
{
    time_types_t time_type;
    uint8_t value;
} time_msg_t;


uint8_t increaseSeconds();

uint8_t increaseMinutes();

uint8_t increaseHours();

void resetTime();

#endif /* TIME_H_ */
