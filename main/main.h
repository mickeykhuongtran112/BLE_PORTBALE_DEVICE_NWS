#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
// #include "esp_check.h"
#include "nvs.h"

// #include "mqtt_client.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "sdkconfig.h"
#include "math.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "sdmmc_cmd.h"
#include "soc/soc_caps.h"
#include <time.h>
#include <sys/time.h>
#include "esp_task_wdt.h"
#include "esp_err.h"

#include "rfid_module.h"
#include "flash.h"
#include "ble.h"
#include "gpio.h"


static char PATH_FILE[100];
extern TaskHandle_t sentRfidTaskHandle;
extern TaskHandle_t recieverRfidTaskHandle;
extern uint8_t status_reader_on;

extern uint8_t session;
extern uint8_t target;

extern double mode;
extern char nameFile[50];
extern struct tm local_time;
extern time_t now;
extern double power_module;
extern double bandwidth;
extern bool ble_device_connected;
void send_ble_notify(const char *input_string);
