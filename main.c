/*
 * Copyright (c) 2017, NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    Tarea 8 - Alarma.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
/* TODO: insert other include files here. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "time.h"

static SemaphoreHandle_t seconds_semaphore;
static SemaphoreHandle_t minutes_semaphore;

void seconds_task()
{
    TickType_t xLastWakeType;
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);
    xLastWakeType = xTaskGetTickCount();
    for(;;)
    {
        vTaskDelayUntil(&xLastWakeType, xPeriod);//pasa 1 seg
        uint8_t seconds = increaseSeconds();
        if(SECONDS_LIMIT == seconds)
        {
            xSemaphoreGive(seconds_semaphore);
            seconds = 0;
        }
        else
            seconds;
    }
}

void minutes_task()
{
    xSemaphoreTake(seconds_semaphore, portMAX_DELAY);
    uint8_t minutes = increaseMinutes();
    if(MINUTES_LIMIT == minutes)
    {
        xSemaphoreGive(minutes_semaphore);
        minutes = 0;
    }
    else
        minutes;
}

void hours_task()
{
    xSemaphoreTake(minutes_semaphore, portMAX_DELAY);
    uint8_t hours = increaseHours();
    if(HOURS_LIMIT == hours)
    {
        resetTime();
        //send s m h
    }
    else
        hours;
}

/*
 * @brief   Application entry point.
 */
int main(void) {

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

    seconds_semaphore;
    minutes_semaphore;

    //xTaskCreate(led_task, "LED task", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);

    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {

    }
    return 0 ;
}
