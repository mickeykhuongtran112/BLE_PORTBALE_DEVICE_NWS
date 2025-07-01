#ifndef FLASH_H
#define FLASH_H
#include "esp_err.h"
#include "hal/gpio_types.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define NVS_NAMESPACE "storage"
#define KEY_DOUBLE_1 "double_1"
#define KEY_DOUBLE_2 "double_2"
#define KEY_DOUBLE_3 "double_3"
#define KEY_DOUBLE_4 "double_4"

esp_err_t write_string_to_nvs(const char *key, const char *value);
esp_err_t read_string_from_nvs(const char *key, char *value, size_t max_len);
esp_err_t read_double_from_nvs(const char *key, double *value);
esp_err_t write_double_to_nvs(const char *key, double value);
#endif