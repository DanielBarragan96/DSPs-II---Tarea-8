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

//counters limit
#define SECONDS_LIMIT 2
#define MINUTES_LIMIT 2
#define HOURS_LIMIT 2
//event group index
#define EVENT_SECONDS (1<<0)
#define EVENT_MINUTES (1<<1)
#define EVENT_HOURS (1<<2)
//value to obteain decades
#define DECADE 10

//Enum to identify message value
typedef enum
{
    SECONDS, MINUTES, HOURS
} time_types_t;
//struct for messages
typedef struct
{
    time_types_t time_type;
    uint8_t value;
} time_msg_t;
//alarm struct
typedef struct
{
    uint8_t sec;
    uint8_t min;
    uint8_t hou;
}alarm_type_t;

#define QUEUE_LENGTH 3
#define QUEUE_ITEM_SIZE sizeof( time_msg_t )

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
 implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 used by the Idle task. */
void vApplicationGetIdleTaskMemory (StaticTask_t **ppxIdleTaskTCBBuffer,
        StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
    /* If the buffers to be provided to the Idle task are declared inside this
     function then they must be declared static - otherwise they will be allocated on
     the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
     state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     Note that, as the array is necessarily of type StackType_t,
     configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
 application must provide an implementation of vApplicationGetTimerTaskMemory()
 to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory (StaticTask_t **ppxTimerTaskTCBBuffer,
        StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
    /* If the buffers to be provided to the Timer task are declared inside this
     function then they must be declared static - otherwise they will be allocated on
     the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
     task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     Note that, as the array is necessarily of type StackType_t,
     configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

//Event gorup
EventGroupHandle_t g_time_events;
//Semaphores
SemaphoreHandle_t semaforo_segundos;
SemaphoreHandle_t semaforo_minutos;
SemaphoreHandle_t semaforo_horas;
//Queue for messages
QueueHandle_t xQueue;
//Alarm
alarm_type_t g_alarm = {0, 1, 0};

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
        PRINTF("\rSeconds\n");
        if (SECONDS_LIMIT == seconds)
        {
            seconds = 0;
            xSemaphoreGive(semaforo_segundos);
        }
        //Checar alarma
        if (g_alarm.sec==seconds)
           xEventGroupSetBits(g_time_events,EVENT_SECONDS);
        time_msg_t algo =
            { SECONDS, seconds };
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
        if (g_alarm.min==minutes)
           xEventGroupSetBits(g_time_events,EVENT_MINUTES);
        time_msg_t algo =
            { MINUTES, minutes };
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
        if (g_alarm.hou==hours)
           xEventGroupSetBits(g_time_events,EVENT_HOURS);
        time_msg_t algo =
            { HOURS, hours };
        xQueueSend(xQueue, &algo, portMAX_DELAY);
    }
}

char* parseToASCII (uint8_t val)
{
    char* ascii = "";
    uint8_t decades = 0;
    uint8_t units = 0;
    if (DECADE < val)
    {
        decades = val / DECADE;
        units = val - (decades * DECADE);
    }
    else
    {
        units = val;
    }
    units += 48;
    decades += 48;

    //PRINTF ((char*) decades); //TODO not working, use UART
    //PRINTF ((char*) units); //TODO not working, use UART

    return ascii;
}

void print_task (void *arg)
{
    time_msg_t algoRead;
    uint8_t segundos = 0;
    uint8_t minutos = 0;
    uint8_t horas = 0;
    for (;;)
    {
        //false al final para borrar el mensaje leído
        xQueueGenericReceive (xQueue, &algoRead, portMAX_DELAY, pdFALSE);
        //xQueuePeek(xQueue, &algoRead, portMAX_DELAY);
        switch (algoRead.time_type)
        {
            case SECONDS:
            {
                segundos = algoRead.value;
                break;
            }
            case MINUTES:
            {
                minutos = algoRead.value;
                if (0 == minutos)
                {
                    xQueueGenericReceive (xQueue, &algoRead, 10, pdFALSE);
                    if (HOURS == algoRead.time_type)
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

        //imprimir por la UART
//        PRINTF("\r");
//        parseToASCII (segundos);
//        PRINTF(":");
//        parseToASCII (minutos);
//        PRINTF (":");
//        parseToASCII (horas);
//        PRINTF("\n");
    }
}

void alarm_task()
{
        for (;;)
        {
        //Espera a que todos los semáforos de la alarma se activen
        xEventGroupWaitBits (g_time_events,
                (EVENT_SECONDS & EVENT_MINUTES & EVENT_HOURS), pdFALSE, pdTRUE,
                portMAX_DELAY);
        //TODO escribir ALARM con la UART
        xEventGroupClearBits (g_time_events,
                (EVENT_SECONDS & EVENT_MINUTES & EVENT_HOURS));
        PRINTF ("\rAlarm\n");
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
    PRINTF("\rInicio\n\r");

    //Queue
    /* Create a queue big enough to hold 10 chars. */
    xQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    time_msg_t algo =
    { SECONDS, 0 };
    xQueueSend(xQueue, &algo, portMAX_DELAY);
    time_msg_t algoRead;
    xQueuePeek(xQueue, &algoRead, portMAX_DELAY);

    //Semaphorea
    semaforo_segundos = xSemaphoreCreateBinary();
    semaforo_minutos = xSemaphoreCreateBinary();
    semaforo_horas = xSemaphoreCreateBinary();

    g_time_events = xEventGroupCreate();

    //Tasks
    xTaskCreate (seconds_task, "Segundos", 300, NULL,
    configMAX_PRIORITIES - 2, NULL);
    xTaskCreate (minutes_task, "Minutes", 300, NULL,
    configMAX_PRIORITIES - 1, NULL);
    xTaskCreate (hours_task, "Hours", 300, NULL,
    configMAX_PRIORITIES - 1, NULL);
    xTaskCreate (print_task, "Mensaje", 300, NULL,
    configMAX_PRIORITIES - 1, NULL);
    xTaskCreate (alarm_task, "Alarma", 300, NULL,
    configMAX_PRIORITIES - 1, NULL);

    vTaskStartScheduler ();

    while (1)
    {

    }
    return 0;
}
