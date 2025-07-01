#ifndef RFID_MODULE__H
#define RFID_MODULE__H

#include "driver/uart.h"

#define TAG_UART "UART"

#define UART_NUM UART_NUM_1
#define TX_GPIO_NUM (GPIO_NUM_17)
#define RX_GPIO_NUM (GPIO_NUM_18)
#define PATTERN_CHR_NUM (3)
#define BUF_SIZE (1024* 2) // 2048 bytes buffer size
#define RD_BUF_SIZE (BUF_SIZE)

static QueueHandle_t uart1_queue = NULL;
static uart_event_t event;


#define FRAME_BUFFER_SIZE 500

void uart_rfid_config(void);

void reciever_rfid(void *arg);

//void send_request_rfid(uint8_t stay0, uint8_t stay1, uint8_t stay2, uint8_t stay3, uint8_t session, uint8_t target);
void send_request_rfid(int index_Antenna);

void process_reciever_data(char *data);

void inventory_task(void *arg);
uint8_t calculate_checksum(uint8_t *frame, uint8_t length);
void decode_frame(uint8_t *frame);
void decode_data(const char *data, size_t length, char *nameFile);

void send_ble_tag_notification(const char *epc_buffer, uint8_t rssi);

void get_power(void);
void get_rf_link(void);
void get_frequency(void);

void set_power(uint8_t power);
void set_beep(bool beep_on);
void set_frequency(const char *region);
void set_rf_link(uint8_t profile);
void set_session(uint8_t session,uint8_t target );
void set_target(uint8_t session,uint8_t target );

//char* check_error_code(uint8_t code);


// Functions to respond to commands from the App
void handle_cmd_DI(const char *device_id);
void handle_cmd_S(void);
void handle_cmd_X(void);
void handle_cmd_T(const char *epc, int rssi);
void handle_cmd_GF(char *freq);
void handle_cmd_SF(bool success);
void handle_cmd_GP(uint8_t power);
void handle_cmd_SP(bool success);
void handle_cmd_BZ(bool success);
// void handle_cmd_GST(uint8_t session, char target);
void handle_cmd_SS(bool success);
void handle_cmd_ST(bool success);
void handle_cmd_GRF(uint8_t val);
void handle_cmd_SRF(bool success);

// Function to handle JSON sent from the App
void handle_received_json(const char *json_str);
#endif