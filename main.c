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
#include "stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "queue.h"

//counters limit
#define SECONDS_LIMIT 60
#define MINUTES_LIMIT 60
#define HOURS_LIMIT 24
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

//Event gorup
EventGroupHandle_t g_time_events;
//Semaphores
SemaphoreHandle_t semaforo_segundos;
SemaphoreHandle_t semaforo_minutos;
SemaphoreHandle_t semaforo_horas;
//Queue for messages
QueueHandle_t xQueue;
//Alarm
alarm_type_t g_alarm = {5, 0, 0};

/* UART instance and clock */
#define DEMO_UART UART0
#define DEMO_UART_CLKSRC UART0_CLK_SRC
#define DEMO_UART_CLK_FREQ CLOCK_GetFreq(UART0_CLK_SRC)
#define ECHO_BUFFER_LENGTH 8

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/* UART user callback */
void UART_UserCallback(UART_Type *base, uart_handle_t *handle, status_t status, void *userData);
char* concat(const char *s1, const char *s2);
/*******************************************************************************
 * Variables
 ******************************************************************************/
uart_handle_t g_uartHandle;
uint8_t g_txBuffer[ECHO_BUFFER_LENGTH] = {0};
uint8_t g_rxBuffer[ECHO_BUFFER_LENGTH] = {0};
volatile bool rxBufferEmpty = true;
volatile bool txBufferFull = false;
volatile bool txOnGoing = false;
volatile bool rxOnGoing = false;

extern void UART0_DriverIRQHandler(void);

/* UART user callback */
void UART_UserCallback(UART_Type *base, uart_handle_t *handle, status_t status, void *userData)
{
    userData = userData;

    if (kStatus_UART_TxIdle == status)
    {
        txBufferFull = false;
        txOnGoing = false;
    }

    if (kStatus_UART_RxIdle == status)
    {
        rxBufferEmpty = false;
        rxOnGoing = false;
    }
}

char* concat(const char *s1, const char *s2){
     char *result = malloc(strlen(s1)+strlen(s2)+1);
     strcpy(result, s1);
     strcat(result, s2);
     return result;
}

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
        //Checar alarma
        if (g_alarm.sec==seconds)
           xEventGroupSetBits(g_time_events,EVENT_SECONDS);
        else
            xEventGroupClearBits (g_time_events,EVENT_SECONDS);
        time_msg_t algo =
            { SECONDS, seconds };
        xQueueSend(xQueue, &algo, portMAX_DELAY);
    }
}

void minutes_task (void *arg)
{
    uint8_t minutes = 0;
    if (g_alarm.min==minutes)
       xEventGroupSetBits(g_time_events,EVENT_MINUTES);
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
        else
            xEventGroupClearBits (g_time_events,EVENT_MINUTES);
        time_msg_t algo =
            { MINUTES, minutes };
        xQueueSend(xQueue, &algo, portMAX_DELAY);
    }
}

void hours_task (void *arg)
{
    uint8_t hours = 0;
    if (g_alarm.hou==hours)
               xEventGroupSetBits(g_time_events,EVENT_HOURS);
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
        else
            xEventGroupClearBits (g_time_events,EVENT_HOURS);
        time_msg_t algo =
            { HOURS, hours };
        xQueueSend(xQueue, &algo, portMAX_DELAY);
    }
}

char* parseToASCII (uint8_t val)
{
    char* ascii = "";
    char decades = 0;
    char units = 0;
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

    char* decades_c = &decades;
    uart_transfer_t xfer2;
    xfer2.data = (uint8_t*) (decades_c);
    xfer2.dataSize = sizeof(decades_c) - 1;
    txOnGoing = true;
    UART_TransferSendNonBlocking(DEMO_UART, &g_uartHandle, &xfer2);

    char* units_c = &units;
    uart_transfer_t xfer;
    xfer.data = (uint8_t*) (units_c);
    xfer.dataSize = sizeof(units_c) - 1;
    txOnGoing = true;
    UART_TransferSendNonBlocking(DEMO_UART, &g_uartHandle, &xfer);

    //PRINTF ((char*) decades); //TODO not working, use UART
    //PRINTF ((char*) units); //TODO not working, use UART

    return ascii;
}

void print_task (void *arg)
{
    uart_transfer_t xfer;
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
                segundos = 0;
                minutos = algoRead.value;
                break;
            }
            case HOURS:
                {
                    segundos = 0;
                    minutos = 0;
                    horas = algoRead.value;
                    break;
                }
            default:
            {
                break;
            }
        }

        char buffer[20];
        itoa(segundos, buffer, 10);//decimal to string

        parseToASCII(segundos);

    }
}

void alarm_task()
{
        for (;;)
        {
        //Espera a que todos los semáforos de la alarma se activen
        xEventGroupWaitBits (g_time_events,
                (EVENT_SECONDS | EVENT_MINUTES | EVENT_HOURS), pdFALSE, pdTRUE,
                portMAX_DELAY);
        xEventGroupClearBits (g_time_events,
                (EVENT_SECONDS | EVENT_MINUTES | EVENT_HOURS));
       //escribir ALARM con la UART
       uart_transfer_t xfer;
       xfer.data = (uint8_t*) ("Alarma");
       xfer.dataSize = sizeof("Alarma") - 1;
       txOnGoing = true;
       UART_TransferSendNonBlocking(DEMO_UART, &g_uartHandle, &xfer);
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

    //Queue
    /* Create a queue big enough to hold 10 chars. */
    xQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    time_msg_t algo =
    { SECONDS, 0 };
    xQueueSend(xQueue, &algo, portMAX_DELAY);
    time_msg_t algoRead;
    xQueuePeek(xQueue, &algoRead, portMAX_DELAY);

    //UART
    uart_config_t config;
    UART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx = true;
    config.enableRx = true;

    UART_Init(DEMO_UART, &config, DEMO_UART_CLK_FREQ);
    UART_TransferCreateHandle(DEMO_UART, &g_uartHandle, UART_UserCallback, NULL);

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
