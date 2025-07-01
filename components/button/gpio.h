#ifndef GPIO__H
#define GPIO__H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "sdkconfig.h"
#include "math.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

extern QueueHandle_t button_events;

#define GPIO_INPUT_SCAN 38
#define GPIO_OUTPUT_BLE 45
#define GPIO_OUTPUT_WORK 39 

void gpio_read_task(void *arg);
void configure_gpio_inputs();
void configure_gpio_outputs();
void set_gpio_output(int gpio_num, int level);
#endif