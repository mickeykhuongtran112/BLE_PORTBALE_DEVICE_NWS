#include "ble.h"
#include "main.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include <string.h>
#define MAX_RECEIVED_DATA_LEN 512

// ======================== GATT STRUCT & CONFIG ========================
const uint16_t GATTS_SERVICE_UUID_TEST = 0x00FF;
const uint16_t GATTS_CHAR_UUID_TEST = 0xFF01;

const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

const uint8_t char_prop_read_notify_write = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

esp_gatts_attr_db_t gatt_db[GATTS_NUM_HANDLE_TEST] = {
    [0] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid,
           ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&GATTS_SERVICE_UUID_TEST}},
    [1] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
           ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_notify_write}},
    [2] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_TEST,
           ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, CHAR_VAL_LEN, 0, NULL}},
    [3] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid,
           ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},
};

uint16_t gatt_handle_table[GATTS_NUM_HANDLE_TEST];
esp_gatt_if_t gatts_if_global = 0;

static struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    uint16_t descr_handle;
    esp_bt_uuid_t char_uuid;
} profile;

static esp_attr_value_t char_val = {
    .attr_max_len = CHAR_VAL_LEN,
    .attr_len = 0,
    .attr_value = NULL,
};

static esp_gatt_char_prop_t gatt_property = 0;

// ======================== BLE FUNCTIONS ========================


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT) {
        esp_ble_gap_start_advertising(&adv_params);
    }
}
esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

void init_ble(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_set_device_name(DEVICE_NAME));

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(PROFILE_APP_ID));

    // Thêm dòng này để cấu hình dữ liệu quảng bá
    ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&adv_data));
}
void send_ble_notify(const char *input_string)
{
    if (!ble_device_connected || !(gatt_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)) {
        ESP_LOGW(TAG_BLE, "Notify not enabled or device not connected");
        // stop task 
        if (sentRfidTaskHandle != NULL)
        {
            ESP_LOGI("TASK", "Stopping RFID tasks...");

            vTaskDelete(sentRfidTaskHandle);
            sentRfidTaskHandle = NULL;
            status_reader_on=0;
            set_gpio_output(GPIO_OUTPUT_WORK, 0);
        }
        else
        {
            ESP_LOGW("TASK", "No tasks to stop.");
        }
        return;
    }

    esp_err_t err = esp_ble_gatts_send_indicate(
        profile.gatts_if,
        profile.conn_id,
        profile.char_handle,
        strlen(input_string),
        (uint8_t *)input_string,
        false
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG_BLE, "Send notify failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG_BLE, "Notify sent: %s", input_string);
    }
}

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                         esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG_BLE, "ESP_GATTS_REG_EVT: Register application");
        esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, GATTS_NUM_HANDLE_TEST, 0);
        break;

    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    if (param->add_attr_tab.status == ESP_GATT_OK) {
        memcpy(gatt_handle_table, param->add_attr_tab.handles, sizeof(uint16_t) * GATTS_NUM_HANDLE_TEST);
        esp_ble_gatts_start_service(gatt_handle_table[0]);


        profile.service_handle = gatt_handle_table[0];
        profile.char_handle = gatt_handle_table[2];
        profile.descr_handle = gatt_handle_table[3];
    }

    case ESP_GATTS_CONNECT_EVT:
    ESP_LOGI(TAG_BLE, "Device connected");
    ble_device_connected = true;
    gatts_if_global = gatts_if;
    profile.gatts_if = gatts_if; // Lưu lại gatts_if
    profile.conn_id = param->connect.conn_id; // Lưu lại conn_id
    set_gpio_output(GPIO_OUTPUT_BLE, 1);
    break;

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG_BLE, "Device disconnected");
        ble_device_connected = false;
        esp_ble_gap_start_advertising(&adv_params);
        set_gpio_output(GPIO_OUTPUT_BLE, 0);
        break;

    case ESP_GATTS_WRITE_EVT:
        // Kiểm tra nếu là ghi vào CCCD (descriptor enable notify/indicate)
        if (param->write.handle == gatt_handle_table[3] && param->write.len == 2) {
            uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
            if (descr_value == 0x0001) {
                // Enable notify
                gatt_property |= ESP_GATT_CHAR_PROP_BIT_NOTIFY;
                ESP_LOGI(TAG_BLE, "Notifications enabled by client");
            } else if (descr_value == 0x0000) {
                // Disable notify
                gatt_property &= ~ESP_GATT_CHAR_PROP_BIT_NOTIFY;
                ESP_LOGI(TAG_BLE, "Notifications disabled by client");
            }
        }
        // Xử lý ghi dữ liệu thông thường vào characteristic
        else if (!param->write.is_prep) {
            ESP_LOGI(TAG_BLE, "Write Event: len=%d, value=%.*s",
                     param->write.len, param->write.len, (char *)param->write.value);

            if (param->write.value[param->write.len - 1] == '}') {
                CommandData cmd_data;
                char json_data[MAX_RECEIVED_DATA_LEN];
                memcpy(json_data, param->write.value, param->write.len);
                json_data[param->write.len] = '\0';
                ESP_LOGI(TAG_BLE, "Received complete command: %s", json_data);
                parse_command(json_data, &cmd_data);
                process_command(&cmd_data);
            } else {
                ESP_LOGW(TAG_BLE, "Received partial or invalid data");
            }
        }
        break;

    default:
        break;
    }
}

void process_command(const CommandData *cmd_data)
{
    if (strcmp(cmd_data->cmd, "DI") == 0)
    {
        ESP_LOGI(TAG_BLE, "Command: Get Device Information");
        char response[64];
        snprintf(response, sizeof(response), "{\"cmd\": \"DI\", \"val\": %s}", DEVICE_NAME);
        send_ble_notify(response);
        // Add logic to retrieve and send device information
    }
    else if (strcmp(cmd_data->cmd, "S") == 0)
    {
        ESP_LOGI(TAG_BLE, "Command: Start RFID Scanning");
        send_request_rfid(1);

        if (sentRfidTaskHandle == NULL) // Ensure the task is not already running
        {
            ESP_LOGI("TASK", "Creating RFID tasks...");

            BaseType_t ret1 = xTaskCreate(
                inventory_task,
                "inventory_task",
                2048 * 4,
                NULL,
                4,
                &sentRfidTaskHandle);

            if (ret1 == pdPASS)
            {
                ESP_LOGI("TASK", "Tasks created successfully.");
                status_reader_on=1;
                set_gpio_output(GPIO_OUTPUT_WORK, 1);
            }
            else
            {
                ESP_LOGE("TASK", "Failed to create tasks.");
                sentRfidTaskHandle = NULL;
            }
        }
        else
        {
            ESP_LOGW("TASK", "Tasks already running.");
        }
    }
    else if (strcmp(cmd_data->cmd, "X") == 0)
    {
        ESP_LOGI(TAG_BLE, "Command: Stop RFID Scanning");

        if (sentRfidTaskHandle != NULL)
        {
            ESP_LOGI("TASK", "Stopping RFID tasks...");

            vTaskDelete(sentRfidTaskHandle);
            sentRfidTaskHandle = NULL;
            status_reader_on=0;
            set_gpio_output(GPIO_OUTPUT_WORK, 0);
        }
        else
        {
            ESP_LOGW("TASK", "No tasks to stop.");
        }
    }
    else if (strcmp(cmd_data->cmd, "GF") == 0)
    {
        ESP_LOGI(TAG_BLE, "Command: Get Operating Frequency");
        get_frequency();
    }
    else if (strcmp(cmd_data->cmd, "SF") == 0 && cmd_data->has_val_str)
    {
        ESP_LOGI(TAG_BLE, "Command: Set Operating Frequency - %s", cmd_data->val_str);
        set_frequency(cmd_data->val_str);
    }
    else if (strcmp(cmd_data->cmd, "GP") == 0)
    {
        ESP_LOGI(TAG_BLE, "Command: Get Current Power");
        get_power();
    }
    else if (strcmp(cmd_data->cmd, "SP") == 0 && cmd_data->has_val_int)
    {
        ESP_LOGI(TAG_BLE, "Command: Set Transmission Power - %d", cmd_data->val_int);
        set_power(cmd_data->val_int);
    }
    else if (strcmp(cmd_data->cmd, "SBZ") == 0 && cmd_data->has_val_str)
    {
        if (strcmp(cmd_data->val_str, "on") == 0)
        {
            set_beep(true);
            ESP_LOGI(TAG_BLE, "Buzzer turned on");
        }
        else if (strcmp(cmd_data->val_str, "off") == 0)
        {
            set_beep(false);
            ESP_LOGI(TAG_BLE, "Buzzer turned off");
        }
        else
        {
            ESP_LOGW(TAG_BLE, "Invalid value for Buzzer Control");
        }
    }
    else if (strcmp(cmd_data->cmd, "ST") == 0)
    {
        ESP_LOGI(TAG_BLE, "Command: Set Session Target - %d", cmd_data->val_int);
        if(cmd_data->val_int < 0 || cmd_data->val_int > 1){
            ESP_LOGW(TAG_BLE, "Invalid session value. Must be between 0 and 1.");
            send_ble_notify("{\"cmd\": \"SS\", \"status\": \"err\", \"msg\": \"Value must be between 0 and 1.\"}");
            return;
        }
        set_target(session,cmd_data->val_int);

    }
    else if (strcmp(cmd_data->cmd, "SS") == 0)
    {
        ESP_LOGI(TAG_BLE, "Command: Set Session - %d", cmd_data->val_int);
        if(cmd_data->val_int < 0 || cmd_data->val_int > 3){
            ESP_LOGW(TAG_BLE, "Invalid session value. Must be between 0 and 3.");
            send_ble_notify("{\"cmd\": \"SS\", \"status\": \"err\", \"msg\": \"Value must be between 0 and 3.\"}");
            return;
        }
        set_session(cmd_data->val_int, target);

    }
    else if (strcmp(cmd_data->cmd, "GRF") == 0)
    {
        ESP_LOGI(TAG_BLE, "Command: Get RFLink");
        get_rf_link();
    }
    else if (strcmp(cmd_data->cmd, "SRF") == 0 && cmd_data->has_val_int)
    {
        ESP_LOGI(TAG_BLE, "Command: Set RFLink - %d", cmd_data->val_int);
        set_rf_link(cmd_data->val_int);
    }
    else
    {
        ESP_LOGW(TAG_BLE, "Unknown or unsupported command.");
    }
}

void parse_command(const char *json_str, CommandData *cmd_data)
{
    memset(cmd_data, 0, sizeof(CommandData));

    // Parse "cmd"
    char *cmd_pos = strstr(json_str, "\"cmd\"");
    if (cmd_pos)
    {
        char *quote1 = strchr(cmd_pos + 5, '\"');
        char *quote2 = strchr(quote1 + 1, '\"');
        if (quote1 && quote2 && (quote2 - quote1 - 1) < sizeof(cmd_data->cmd))
        {
            strncpy(cmd_data->cmd, quote1 + 1, quote2 - quote1 - 1);
            cmd_data->cmd[quote2 - quote1 - 1] = '\0';
        }
    }

    // Parse "val"
    char *val_pos = strstr(json_str, "\"val\"");
    if (val_pos)
    {
        char *val_start = strchr(val_pos, ':');
        if (val_start)
        {
            val_start++; // qua dấu ':'
            while (*val_start == ' ' || *val_start == '\"')
                val_start++; // bỏ trắng và dấu "

            // Nếu là số
            if (*val_start >= '0' && *val_start <= '9')
            {
                cmd_data->val_int = atoi(val_start);
                cmd_data->has_val_int = true;
            }
            // Nếu là chuỗi
            else
            {
                char *quote1 = strchr(val_pos + 5, '\"');
                char *quote2 = strchr(quote1 + 1, '\"');
                if (quote1 && quote2 && (quote2 - quote1 - 1) < sizeof(cmd_data->val_str))
                {
                    strncpy(cmd_data->val_str, quote1 + 1, quote2 - quote1 - 1);
                    cmd_data->val_str[quote2 - quote1 - 1] = '\0';
                    cmd_data->has_val_str = true;
                }
            }
        }
    }
}

// if (param->write.len == 2)
// {
//     uint8_t cmd0 = param->write.value[0];
//     uint8_t cmd1 = param->write.value[1];

//     if (cmd0 == 0x01 && cmd1 == 0x00) // Start RFID
//     {
//         ESP_LOGI(TAG_BLE, "BLE command: START RFID");

//         if (status_reader_on == 0)
//         {
//             status_reader_on = 1;

//             if (sentRfidTaskHandle == NULL && recieverRfidTaskHandle == NULL)
//             {
//                 ESP_LOGI("TASK", "Creating RFID tasks...");

//                 BaseType_t ret1 = xTaskCreate(
//                     inventory_task,
//                     "inventory_task",
//                     2048 * 4,
//                     NULL,
//                     4,
//                     &sentRfidTaskHandle);

//                 BaseType_t ret2 = xTaskCreatePinnedToCore(
//                     reciever_rfid,
//                     "reciever_rfid",
//                     2048 * 4,
//                     NULL,
//                     10,
//                     &recieverRfidTaskHandle,
//                     0);

//                 if (ret1 == pdPASS && ret2 == pdPASS) //
//                 {
//                     ESP_LOGI("TASK", "Tasks created successfully.");
//                 }
//                 else
//                 {
//                     ESP_LOGE("TASK", "Failed to create tasks.");
//                     sentRfidTaskHandle = NULL;
//                     recieverRfidTaskHandle = NULL;
//                 }
//             }
//             else
//             {
//                 ESP_LOGW("TASK", "Tasks already running.");
//             }
//         }
//     }
//     else if (cmd0 == 0x00 && cmd1 == 0x00) // Stop RFID
//     {
//         ESP_LOGI(TAG_BLE, "BLE command: STOP RFID");
//         if (status_reader_on == 1)
//         {
//             status_reader_on = 0;

//             if (sentRfidTaskHandle != NULL || recieverRfidTaskHandle != NULL)
//             {
//                 ESP_LOGI("TASK", "Stopping RFID tasks...");

//                 if (sentRfidTaskHandle != NULL)
//                 {
//                     vTaskDelete(sentRfidTaskHandle);
//                     sentRfidTaskHandle = NULL;
//                 }

//                 if (recieverRfidTaskHandle != NULL)
//                 {
//                     vTaskDelete(recieverRfidTaskHandle);
//                     recieverRfidTaskHandle = NULL;
//                 }
//             }
//             else
//             {
//                 ESP_LOGW("TASK", "No tasks to stop.");
//             }
//         }
//     }
// }