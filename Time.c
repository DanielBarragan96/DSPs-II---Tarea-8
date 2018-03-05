#include "Time.h"

volatile static Time g_time;
volatile static Time g_alarm;

uint8_t increaseSeconds()
{
    return ++g_time.seconds;
}

uint8_t increaseMinutes()
{
    g_time.seconds = 0;
    return ++g_time.minutes;
}

uint8_t increaseHours()
{
    g_time.minutes = 0;
    return ++g_time.hours;
}

void resetTime()
{
    g_time.seconds = 0;
    g_time.minutes = 0;
    g_time.hours= 0;
}
