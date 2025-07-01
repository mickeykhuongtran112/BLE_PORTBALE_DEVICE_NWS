#include "main.h"
#include "gpio.h"
#include "rfid_module.h"
// Hàm cấu hình GPIO
void configure_gpio_inputs()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_INPUT_SCAN), // Chọn GPIO
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}
// Hàm cấu hình GPIO đầu ra
void configure_gpio_outputs()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_OUTPUT_BLE)| (1ULL << GPIO_OUTPUT_WORK), // Chọn GPIO
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}
// set GPIO đầu ra
void set_gpio_output(int gpio_num, int level)
{
    gpio_set_level(gpio_num, level);
}

void gpio_read_task(void *arg)
{
    static int last_start_time = 0;
    static int last_stop_time = 0;
    static int last_operation_state = -1; 
    static int last_config_state = -1;    

    while (1)
    {
        int scan_level = gpio_get_level(GPIO_INPUT_SCAN);     
        int current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
       
            if (scan_level == 0 && (current_time - last_start_time) > 200) // Debounce 200ms
            {
                last_start_time = current_time; 
                ESP_LOGI("GPIO", "START button pressed");

                if (status_reader_on == 0) 
                {
                    status_reader_on = 1;

                    if (sentRfidTaskHandle == NULL && recieverRfidTaskHandle == NULL)
                    {
                        ESP_LOGI("TASK", "Creating inventory_task...");
                        BaseType_t ret1 = xTaskCreate(
                            inventory_task,
                            "inventory_task",
                            2048 * 4,
                            NULL,
                            4,
                            &sentRfidTaskHandle);

                        BaseType_t ret2 = xTaskCreatePinnedToCore(
                            reciever_rfid,
                            "reciever_rfid",
                            2048 * 4,
                            NULL,
                            10,
                            &recieverRfidTaskHandle,
                            0);
                       
                        if (ret1 == pdPASS && ret2 == pdPASS)
                        {
                            ESP_LOGI("TASK", "Tasks created successfully.");
                        }
                        else
                        {
                            ESP_LOGE("TASK", "Failed to create tasks.");
                            sentRfidTaskHandle = NULL;
                            recieverRfidTaskHandle = NULL;
                        }
                    }
                    else
                    {
                        ESP_LOGW("TASK", "Tasks are already running.");
                    }
                }
            }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
