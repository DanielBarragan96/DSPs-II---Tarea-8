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
 * @file    Tarea8.c
 * @brief   Application entry point.
 */
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_uart.h"
#include "stdint.h"
#include "string.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "queue.h"

#define SECONDS_LIMIT 60
#define MINUTES_LIMIT 60
#define HOURS_LIMIT 24

#define EVENT__SECONDS (1<<0)
#define EVENT_60_MINUTES (1<<1)
#define EVENT_60_HOURS (1<<2)

#define DECADE 10

typedef enum
{
    SECONDS,
    MINUTES,
    HOURS
}time_types_t;

typedef struct
{
    time_types_t time_type;
    uint8_t value;
} time_msg_t;

#define QUEUE_LENGTH 3
#define QUEUE_ITEM_SIZE sizeof( time_msg_t )

SemaphoreHandle_t semaforo_segundos;
SemaphoreHandle_t semaforo_minutos;
SemaphoreHandle_t semaforo_horas;

QueueHandle_t xQueue;

void seconds_task (void *arg)
{
    TickType_t LastWakeTime;
    const TickType_t periodo = pdMS_TO_TICKS(1000);
    LastWakeTime = xTaskGetTickCount ();
    uint8_t seconds = 0;
    for (;;)
    {
        vTaskDelayUntil (&LastWakeTime, periodo);
        seconds++;
        if (SECONDS_LIMIT == seconds)
        {
            seconds = 0;
            xSemaphoreGive(semaforo_segundos);
        }
        time_msg_t algo = {SECONDS,seconds};
        xQueueSend(xQueue, &algo, portMAX_DELAY);
    }
}

void minutes_task (void *arg)
{
    uint8_t minutes = 0;
    for (;;)
    {
        xSemaphoreTake(semaforo_segundos, portMAX_DELAY);
        minutes++;
        if (MINUTES_LIMIT == minutes)
        {
            minutes = 0;
            xSemaphoreGive(semaforo_minutos);
        }
        time_msg_t algo = {MINUTES,minutes};
        xQueueSend(xQueue, &algo, portMAX_DELAY);
    }
}

void hours_task (void *arg)
{
    uint8_t hours = 0;
    for (;;)
    {
        xSemaphoreTake(semaforo_minutos, portMAX_DELAY);
        hours++;
        if (HOURS_LIMIT == hours)
        {
            hours = 0;
        }
        time_msg_t algo = {HOURS,hours};
        xQueueSend(xQueue, &algo, portMAX_DELAY);
    }
}

char* parseToASCII(uint8_t val)
{
    char* ascii="";
    uint8_t decades=0;
    uint8_t units=0;
    if(DECADE < val)
    {
        decades = val/DECADE;
        units = val -(decades*DECADE);
    }
    else
    {
        units = val;
    }
    units += 48;
    decades+= 48;
    PRINTF((char*) units);
    return ascii;
}

void print_task (void *arg)
{
    time_msg_t algoRead;
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
    uint8_t segundos = 0;
    uint8_t minutos = 0;
    uint8_t horas = 0;
    for(;;)
    {
        xQueueGenericReceive(xQueue, &algoRead, portMAX_DELAY, pdFALSE);
        //xQueuePeek(xQueue, &algoRead, portMAX_DELAY);
        switch(algoRead.time_type)
        {
            case SECONDS:
            {
                segundos = algoRead.value;
                break;
            }
            case MINUTES:
            {
                minutos = algoRead.value;
                if(0==minutos)
                {
                    xQueueGenericReceive(xQueue, &algoRead, 10, pdFALSE);
                    if(HOURS == algoRead.time_type)
                    {
                        horas = algoRead.value;
                    }
                }
                break;
            }
            case HOURS:
                break;
            {
                horas = algoRead.value;
                break;
            }
            default:
            {
                break;
            }
        }
        parseToASCII(segundos);
    }
}

int main (void)
{

    /* Init board hardware. */
    BOARD_InitBootPins ();
    BOARD_InitBootClocks ();
    BOARD_InitBootPeripherals ();
    /* Init FSL debug console. */
    BOARD_InitDebugConsole ();

    //UART

    //Queue
    /* Create a queue big enough to hold 10 chars. */
    xQueue = xQueueCreate( QUEUE_LENGTH, QUEUE_ITEM_SIZE );
    time_msg_t algo = {SECONDS,0};
    xQueueSend(xQueue, &algo, portMAX_DELAY);
    time_msg_t algoRead;
    xQueuePeek(xQueue, &algoRead, portMAX_DELAY);


    //Semaphorea
    semaforo_segundos = xSemaphoreCreateBinary();
    semaforo_minutos = xSemaphoreCreateBinary();
    semaforo_horas = xSemaphoreCreateBinary();

    //Tasks
    xTaskCreate (seconds_task, "Segundos", configMINIMAL_STACK_SIZE, NULL,
    configMAX_PRIORITIES - 1, NULL);
    xTaskCreate (minutes_task, "Minutes", configMINIMAL_STACK_SIZE, NULL,
    configMAX_PRIORITIES - 1, NULL);
    xTaskCreate (hours_task, "Hours", configMINIMAL_STACK_SIZE, NULL,
    configMAX_PRIORITIES - 1, NULL);
    xTaskCreate (print_task, "Mensaje", 110, NULL,
    configMAX_PRIORITIES - 2, NULL);

    vTaskStartScheduler ();

    while (1)
    {

    }
    return 0;
}
