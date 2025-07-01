#include "main.h"

TaskHandle_t sentRfidTaskHandle = NULL;
TaskHandle_t recieverRfidTaskHandle = NULL;
QueueHandle_t button_events;


// struct tm local_time;
// time_t now;
// char nameFile[50];
// double mode;
double power_module;
double bandwidth;
uint8_t status_reader_on;
bool ble_device_connected = false;

uint8_t session = 0x00;
uint8_t target = 0x00;

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    /*----------------UART--------------------*/
    uart_rfid_config();

    /*----------------GPIO--------------------*/
     configure_gpio_inputs();
     configure_gpio_outputs();

    /*----------------BLE--------------------*/
    init_ble();
    // xTaskCreate(ble_notify_task, "ble_notify_task", 2048, NULL, 5, NULL);


    set_gpio_output(GPIO_OUTPUT_BLE, 1);
    set_gpio_output(GPIO_OUTPUT_WORK, 1);

    BaseType_t ret2 = xTaskCreatePinnedToCore(
        reciever_rfid,
        "reciever_rfid",
        2048 * 4,
        NULL,
        10,
        &recieverRfidTaskHandle,
        0);
    if (ret2 == pdPASS)
    {
        ESP_LOGI("TASK", "Tasks created successfully.");
    }
    else
    {
        ESP_LOGE("TASK", "Failed to create tasks.");
        recieverRfidTaskHandle = NULL;
    }

    set_beep(false);
    //send_request_rfid(1); // debug Temporary

    // while (1)
    // {
    //     /* code */
    //     vTaskDelay(3000 / portTICK_PERIOD_MS);
    //     send_ble_notify("{\"cmd\": \"DI\", \"val\": \"ESP32_RFID_READER\"}");
    // }
    

    
}
