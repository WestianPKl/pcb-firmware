#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "stm32f7xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

extern SemaphoreHandle_t printf_mutex;

extern TaskHandle_t task_button_handle;
extern TaskHandle_t task_uart2_handle;
extern TaskHandle_t task_can_handler;
extern TaskHandle_t eth_rx_task_handle;

#endif // MAIN_H
