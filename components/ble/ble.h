#ifndef BLE_H
#define BLE_H

#include <stdio.h>
#include <stdbool.h>
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_BLE "BLE_SERVER"
//BLE_SERVER
#define DEVICE_NAME "NEXTWAVES_UHF_BLE"

#define CHAR_VAL_LEN 128
#define PROFILE_APP_ID 0
#define GATTS_NUM_HANDLE_TEST 4

// UUIDs
extern const uint16_t GATTS_SERVICE_UUID_TEST;
extern const uint16_t GATTS_CHAR_UUID_TEST;
extern const uint16_t primary_service_uuid;
extern const uint16_t character_declaration_uuid;
extern const uint16_t character_client_config_uuid;

// Properties
extern const uint8_t char_prop_read_notify_write;

// GATT DB
extern esp_gatts_attr_db_t gatt_db[GATTS_NUM_HANDLE_TEST];
extern uint16_t gatt_handle_table[GATTS_NUM_HANDLE_TEST];

// State
extern bool ble_device_connected;
extern esp_gatt_if_t gatts_if_global;
extern esp_ble_adv_params_t adv_params;

// BLE APIs
void init_ble(void);
void send_ble_notify(const char *input_string);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// Command Struct
typedef struct {
    char cmd[16];
    char val_str[16];
    int val_int;
    bool has_val_str;
    bool has_val_int;
} CommandData;

void parse_command(const char *json_str, CommandData *cmd_data);
void process_command(const CommandData *cmd_data);

#ifdef __cplusplus
}
#endif

#endif // BLE_H
