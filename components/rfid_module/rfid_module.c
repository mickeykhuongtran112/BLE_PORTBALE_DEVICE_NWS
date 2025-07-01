
#include "rfid_module.h"
#include "main.h"
#include "flash.h"
uint8_t uart_rx_buffer[FRAME_BUFFER_SIZE];
typedef struct
{
    uint8_t antenna_id;
    uint8_t frequency;
    uint16_t pc;
    uint8_t epc[FRAME_BUFFER_SIZE];
    uint8_t epc_length;
    uint8_t rssi;
} RFID_Frame;

RFID_Frame parsed_frame;

void uart_rfid_config(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(UART_NUM, &uart_config);
    // Install UART driver, and get the queue.
    uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart1_queue, 0);
    // Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(UART_NUM, TX_GPIO_NUM, RX_GPIO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_enable_pattern_det_baud_intr(UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    // Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(UART_NUM, 20);
}


// void send_request_rfid(uint8_t stay0, uint8_t stay1, uint8_t stay2, uint8_t stay3, uint8_t session, uint8_t target)
// {
//     uint8_t buffer[33];

//     memcpy(buffer, (uint8_t[]){0xA0, 0x20, 0x01, 0x8A, 0x00, stay0, 0x01, stay1, 0x02, stay2, 0x03, stay3, 0x04, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, session, target, 0x00, 0x00, 0x00, 0x00, 0x01, 0x97}, 33);

//     uart_write_bytes(UART_NUM, (const char *)buffer, sizeof(buffer));

//     printf("Sent data:");
//     for (int i = 0; i < sizeof(buffer); i++)
//     {
//         printf("0x%02X ", buffer[i]);
//     }
//     printf("\n");
// }
void send_request_rfid(int index_Antenna)
{
    uint8_t buffer[15];

    switch (index_Antenna)
    {
    case 1:
        memcpy(buffer, (uint8_t[]){0xA0, 0x0D, 0xFF, 0x8A, 0x00, 0x01, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x01, 0x01, 0xC1}, 15);
        break;
    case 2:
        memcpy(buffer, (uint8_t[]){0xA0, 0x0D, 0xFF, 0x8A, 0x00, 0x00, 0x01, 0x01, 0x02, 0x00, 0x03, 0x00, 0x00, 0x01, 0xC2}, 15);
        break;
    case 3:
        memcpy(buffer, (uint8_t[]){0xA0, 0x0D, 0xFF, 0x8A, 0x00, 0x00, 0x01, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00, 0x01, 0xC2}, 15);
        break;
    default:
        printf("Invalid Antenna Index!\n");
        return;
    }

    uart_write_bytes(UART_NUM, (const char *)buffer, sizeof(buffer));

    printf("Sent data to Antenna %d: ", index_Antenna);
    for (int i = 0; i < sizeof(buffer); i++)
    {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
}
void set_power_rfid(int index_Antenna)
{
    uint8_t buffer[] = {0xA0, 0x0D, 0xFF, 0x8A, 0x03, 0x01, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x01, 0x01, 0xC1};

    uart_write_bytes(UART_NUM, (const char *)buffer, sizeof(buffer));

    printf("Sent data: ");
    for (int i = 0; i < sizeof(buffer); i++)
    {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
}

void reciever_rfid(void *arg)
{
    size_t buffered_size;
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *)malloc(RD_BUF_SIZE);

    // State machine variables
    static uint8_t frame_buffer[RD_BUF_SIZE];
    static int frame_index = 0;
    static int expected_length = 0;
    static bool frame_started = false;

    for (;;)
    {
        // Wait for UART event
        if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {

            bzero(dtmp, RD_BUF_SIZE);

            switch (event.type)
            {
            case UART_DATA:
                // if (status_reader_on == 1)
                // {
                int len = uart_read_bytes(UART_NUM, dtmp, event.size, portMAX_DELAY);
                ESP_LOGW("DEBUG", " here");
                for (int i = 0; i < len; i++)
                {
                    uint8_t byte = dtmp[i];

                    if (!frame_started)
                    {

                        if (byte == 0xA0)
                        {
                            frame_started = true;
                            frame_index = 0;
                            frame_buffer[frame_index++] = byte;
                        }
                        // Else: ignore noise before frame
                    }
                    else
                    {
                        frame_buffer[frame_index++] = byte;

                        // Read length byte at index 1 to determine expected length
                        if (frame_index == 2)
                        {
                            expected_length = frame_buffer[1] + 2;

                            if (expected_length > RD_BUF_SIZE)
                            {
                                ESP_LOGW("UART", "Frame too long. Resetting.");
                                frame_started = false;
                                frame_index = 0;
                            }
                        }

                        if (frame_index == expected_length)
                        {
                            // Full frame received
                            decode_frame(frame_buffer);

                            // Debug
                            ESP_LOGI("UART", "Received frame:");
                            for (int j = 0; j < expected_length; j++)
                            {
                                printf("0x%02X ", frame_buffer[j]);
                            }
                            printf("\n");

                            // Reset
                            frame_started = false;
                            frame_index = 0;
                            expected_length = 0;
                        }
                    }
                    // }
                }
                break;

            case UART_FIFO_OVF:
                ESP_LOGW(TAG_UART, "HW FIFO overflow");
                uart_flush_input(UART_NUM);
                frame_started = false;
                frame_index = 0;
                break;

            case UART_BUFFER_FULL:
                ESP_LOGW(TAG_UART, "Ring buffer full");
                uart_flush_input(UART_NUM);
                frame_started = false;
                frame_index = 0;
                break;

            case UART_BREAK:
                ESP_LOGW(TAG_UART, "UART RX Break");
                break;

            case UART_PARITY_ERR:
                ESP_LOGW(TAG_UART, "UART Parity Error");
                break;

            case UART_FRAME_ERR:
                ESP_LOGW(TAG_UART, "UART Frame Error");
                break;

            case UART_PATTERN_DET:
                uart_get_buffered_data_len(UART_NUM, &buffered_size);
                int pos = uart_pattern_pop_pos(UART_NUM);
                ESP_LOGI(TAG_UART, "[PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1)
                {
                    uart_flush_input(UART_NUM);
                }
                else
                {
                    uart_read_bytes(UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                    uint8_t pat[PATTERN_CHR_NUM + 1] = {0};
                    uart_read_bytes(UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG_UART, "Read data: %s", dtmp);
                    ESP_LOGI(TAG_UART, "Pattern   : %s", pat);
                }
                break;

            default:
                ESP_LOGI("UART", "Unknown UART event type: %d", event.type);
                break;
            }
        }
    }

    free(dtmp);
    vTaskDelete(NULL);
}

uint8_t calculate_checksum(uint8_t *frame, uint8_t length)
{
    uint8_t checksum = 0;
    for (int i = 0; i < length; i++)
    {
        checksum += frame[i];
    }
    return (~checksum) + 1; // Example checksum logic (modify if needed)
}

// Decode frame function
void decode_frame(uint8_t *frame)
{
    /*----------------DeBug---------------------*/
    printf("Received frame: ");
    for (int i = 0; i < frame[1] + 2; i++)
    {
        printf("0x%02X ", frame[i]);
    }
    printf("\n");
    // Calculate frame length and checksum
    uint8_t frame_length = frame[1] + 1; // frame[1] does not include Head byte
    uint8_t received_checksum = frame[frame_length];
    uint8_t calculated_checksum = calculate_checksum(frame, frame_length);

    if (received_checksum != calculated_checksum)
    {
        printf("Checksum error! Received: %02X, Calculated: %02X\n", received_checksum, calculated_checksum);
        return;
    }
    uint8_t cmd = frame[3]; // Command byte
    if (cmd == 0x8A)        // epc fast switch
    {
        if (status_reader_on == 1)
        {
            // Parse antenna ID and frequency
            uint8_t freq_ant = frame[4];
            parsed_frame.antenna_id = freq_ant & 0x03; // 2 low bits for Antenna ID
            parsed_frame.antenna_id += 1;
            parsed_frame.frequency = (freq_ant >> 2) & 0x3F; // 6 high bits for Frequency

            // Parse PC (Protocol Control)
            parsed_frame.pc = (frame[5] << 8) | frame[6];

            // Calculate EPC length
            parsed_frame.epc_length = frame_length - 7 - 1; // Subtract Head, Len, Address, Cmd, FreqAnt, PC

            if (parsed_frame.epc_length <= 0)
            {
                printf("Invalid EPC length: %d\n", parsed_frame.epc_length);
                return;
            }

            // Copy EPC to parsed frame
            memcpy(parsed_frame.epc, &frame[7], parsed_frame.epc_length);

            // Extract RSSI
            parsed_frame.rssi = frame[7 + parsed_frame.epc_length];

            // Print the parsed data
            char epc_buffer[64 * 2 + 1]; // Mỗi byte thành 2 ký tự hex + null-terminator
            memset(epc_buffer, 0, sizeof(epc_buffer));
            if (parsed_frame.epc_length > 64)
            {
                printf("Error: EPC length exceeds buffer size\n");
                return; // Exit to prevent buffer overflow
            }

            for (int i = 0; i < parsed_frame.epc_length; i++)
            {
                snprintf(epc_buffer + i * 2, 3, "%02X", parsed_frame.epc[i]);
            }

            if (strcmp(epc_buffer, "000000"))
            {

                send_ble_tag_notification(epc_buffer, parsed_frame.rssi);
            }
        }
    }
    else if (cmd == 0x77) // Response Get RF Power
    {
        uint8_t power = frame[4];
        if (power > 0x21)
        {
            printf("Invalid power value: %d (out of range)\n", power);
            return;
        }
        handle_cmd_GP(power);
    }
    else if (cmd == 0x76) // Response Set RF Power
    {
        uint8_t status = frame[4]; // 0x10 for success, different 0x01 for failure
        if (status == 0x10)
        {
            handle_cmd_SP(true);
        }
        else
        {
            handle_cmd_SP(false);
        }
    }
    else if (cmd == 0x6A) // Response Get Rf Link
    {
        uint8_t profile = frame[4] & 0x03; // 2 low bits for Profile
        handle_cmd_GRF(profile);
    }
    else if (cmd == 0x69) // Response SetRf Link
    {
        uint8_t status = frame[4]; // 0x10 for success, different 0x01 for failure
        if (status == 0x10)
        {
            handle_cmd_SRF(true);
        }
        else
        {
            handle_cmd_SRF(false);
        }
    }
    else if (cmd == 0x8B) // Response Set session_target
    {
    }
    else if (cmd == 0x79) // Response Get Frequency
    {
        uint8_t band = frame[4]; // Band value
        uint8_t start_freq = frame[5];
        uint8_t end_freq = frame[6];

        printf("Frequency Band: ");
        switch (band)
        {
        case 0x01:
            printf("FCC\n");
            if (start_freq == 0x07 && end_freq == 0x3B)
            {
                handle_cmd_GF("US");
            }
            else if (start_freq == 0x27 && end_freq == 0x31)
            {
                handle_cmd_GF("VN");
            }
            break;
        case 0x02:
            printf("ETSI\n");
            handle_cmd_GF("EU");
            break;
        case 0x03:
            printf("CHN\n");
            handle_cmd_GF("CN");
            break;
        default:
            printf("Unknown Band\n");
            break;
        }
    }
    else if (cmd == 0x78) // Response Set Frequency
    {
        uint8_t status = frame[4]; // 0x10 for success, different 0x01 for failure
        if (status == 0x10)
        {
            handle_cmd_SF(true);
        }
        else
        {
            handle_cmd_SF(false);
        }
    }
    else if (cmd == 0x7A) // Response Set beep
    {
        uint8_t status = frame[4]; // 0x10 for success, different 0x01 for failure
        if (status == 0x10)
        {
            handle_cmd_BZ(true);
        }
        else
        {
            handle_cmd_BZ(false);
        }
    }
}
void send_ble_tag_notification(const char *epc_buffer, uint8_t rssi)
{
    char message[500];
    memset(message, 0, sizeof(message));
    snprintf(message, sizeof(message),
             "{\"cmd\": \"T\", \"epc\": \"%s\", \"rssi\": %d}",
             epc_buffer,
             rssi);

    send_ble_notify(message);
}
void inventory_task(void *arg)
{

    while (1)
    {
        if (ble_device_connected)
        {
            vTaskDelay(200 / portTICK_PERIOD_MS);
            send_request_rfid(1);
            // vTaskDelay(200 / portTICK_PERIOD_MS);
            // send_request_rfid(2);
            // i2c_send_to_arduino("0000000000");
            // vTaskDelay(200 / portTICK_PERIOD_MS);
            // send_request_rfid(3);
        }
    }
}

void set_power(uint8_t power)
{
    // Check the power range (0x00 to 0x21 = 0 to 33 dBm)
    if (power > 0x21)
    {
        printf("Power out of range! Must be between 0x00 and 0x21.\n");
        return;
    }

    // Byte frame to set RF power (power level applies to all antennas)
    uint8_t command_frame[9] = {
        0xA0,  // Head
        0x07,  // Len
        0xFF,  // Address (Broadcast)
        0x76,  // Command
        power, // Power1
        power, // Power2
        power, // Power3
        power, // Power4
        0x00   // Checksum (to be calculated)
    };

    // Calculate and set the checksum
    command_frame[8] = calculate_checksum(command_frame, 8);

    // Send command over UART
    uart_write_bytes(UART_NUM, (const char *)command_frame, sizeof(command_frame));
    printf("Sent RF power command with power : %d dBm, checksum: (0x%02X)\n", power, command_frame[8]);
}
void set_beep(bool beep_on)
{
    if (beep_on)
    {
        const uint8_t beep_command[] = {0xA0, 0x04, 0xFF, 0x7A, 0x02, 0xE1};
        uart_write_bytes(UART_NUM, (const char *)beep_command, sizeof(beep_command));
    }
    else
    {
        const uint8_t beep_command[] = {0xA0, 0x04, 0xFF, 0x7A, 0x00, 0xE3};
        uart_write_bytes(UART_NUM, (const char *)beep_command, sizeof(beep_command));
    }

    printf("set beep OK\n");
}

void set_session(uint8_t session,uint8_t target )
{
    uint8_t command_frame[8] = {
        0xA0,           // Head
        0x06,           // Len
        0xFF,           // Address (Broadcast)
        0x8B,           // Command
        session,        // Session
        target,         // Target
        0x00,           // Repeat
        0x00            // Checksum (to be calculated)
    };

    // Calculate and set the checksum
    command_frame[7] = calculate_checksum(command_frame, 7);

    uart_write_bytes(UART_NUM, (const char *)command_frame, sizeof(command_frame));
    printf("Set session and target command sent: session=%d\n", session);
    handle_cmd_SS(true);
}
void set_target(uint8_t session,uint8_t target )
{
    uint8_t command_frame[8] = {
        0xA0,           // Head
        0x06,           // Len
        0xFF,           // Address (Broadcast)
        0x8B,           // Command
        session,        // Session
        target,         // Target
        0x00,           // Repeat
        0x00            // Checksum (to be calculated)
    };

    // Calculate and set the checksum
    command_frame[7] = calculate_checksum(command_frame, 7);

    uart_write_bytes(UART_NUM, (const char *)command_frame, sizeof(command_frame));
    printf("Set session and target command sent: target=%d\n", target);
    handle_cmd_SS(true);
}
void set_frequency(const char *region)
{
    const uint8_t band_commands[4][8] = {
        {0xA0, 0x06, 0xFF, 0x78, 0x03, 0x2B, 0x35, 0x80}, // China (920~925)
        {0xA0, 0x06, 0xFF, 0x78, 0x02, 0x00, 0x06, 0xDB}, // Europe (865-868)
        {0xA0, 0x06, 0xFF, 0x78, 0x01, 0x07, 0x3B, 0xA0}, // North America (902-928)
        {0xA0, 0x06, 0xFF, 0x78, 0x01, 0x27, 0x31, 0x8A}  // Vietnam (918-923)
    };

    int band = -1;

    if (strcmp(region, "CN") == 0)
        band = 1;
    else if (strcmp(region, "EU") == 0)
        band = 2;
    else if (strcmp(region, "US") == 0)
        band = 3;
    else if (strcmp(region, "VN") == 0)
        band = 4;

    if (band == -1)
    {
        printf("Invalid region selection! Must be 'CN', 'EU', 'US', or 'VN'.\n");
        return;
    }

    const uint8_t *command = band_commands[band - 1];
    uart_write_bytes(UART_NUM, (const char *)command, sizeof(band_commands[0]));
    switch (band)
    {
    case 1:
        printf("Set band command sent for China (920~925)\n");
        break;
    case 2:
        printf("Set band command sent for Europe (865-868)\n");
        break;
    case 3:
        printf("Set band command sent for North America (902-928)\n");
        break;
    case 4:
        printf("Set band command sent for Vietnam (918-923)\n");
        break;
    }
}

void set_target_session(void)
{
   
}

void set_rf_link(uint8_t profile)
{
    if (profile > 3)
    {
        printf("Invalid profile! Must be between 0 and 3.\n");
        return;
    }

    uint8_t command_frame[6] = {
        0xA0,           // Head
        0x04,           // Len
        0xFF,           // Address (Broadcast)
        0x69,           // Command
        0xD0 + profile, // Profile (D0 for Profile 0, D1 for Profile 1, etc.)
        0x24 - profile  // Checksum
    };

    uart_write_bytes(UART_NUM, (const char *)command_frame, sizeof(command_frame));
    printf("Set RF link command sent for Profile %d\n", profile);
}

void get_power(void)
{
    const uint8_t get_power_command[] = {0xA0, 0x03, 0xFF, 0x77, 0xE7};
    uart_write_bytes(UART_NUM, (const char *)get_power_command, sizeof(get_power_command));
    printf("Sent command to get power\n");
}

void get_rf_link(void)
{
    const uint8_t get_beep_command[] = {0xA0, 0x03, 0xFF, 0x6A, 0xF4};
    uart_write_bytes(UART_NUM, (const char *)get_beep_command, sizeof(get_beep_command));
    printf("Sent command to get RF link\n");
}

void get_frequency(void)
{
    const uint8_t get_frequency_command[] = {0xA0, 0x03, 0xFF, 0x79, 0xE5};
    uart_write_bytes(UART_NUM, (const char *)get_frequency_command, sizeof(get_frequency_command));
    printf("Sent command to get frequency\n");
}
void handle_cmd_DI(const char *device_id)
{
    char response[128];
    snprintf(response, sizeof(response), "{\"cmd\": \"DI\", \"id\": \"%s\"}", device_id);
    send_ble_notify(response);
}

void handle_cmd_S()
{
    send_ble_notify("{\"cmd\": \"S\", \"status\": \"ok\"}");
}

void handle_cmd_X()
{
    send_ble_notify("{\"cmd\": \"X\", \"status\": \"ok\"}");
}

void handle_cmd_T(const char *epc, int rssi)
{
    if (epc == NULL || strlen(epc) == 0)
    {
        ESP_LOGW(TAG_BLE, "EPC is empty, skipping notification.");
        return;
    }

    char message[256];
    snprintf(message, sizeof(message),
             "{\"cmd\": \"T\", \"epc\": \"%s\", \"rssi\": %d}",
             epc, rssi);
    send_ble_notify(message);
}

void handle_cmd_GF(char *freq)
{
    char response[64];
    snprintf(response, sizeof(response), "{\"cmd\": \"GF\", \"val\": \"%s\"}", freq);
    send_ble_notify(response);
}

void handle_cmd_SF(bool success)
{
    ESP_LOGW(TAG_BLE, "debug in hereeeee");
    send_ble_notify(success ? "{\"cmd\": \"SF\", \"status\": \"ok\"}" : "{\"cmd\": \"SF\", \"status\": \"err\", \"msg\": \"Invalid\"}");
}

void handle_cmd_GP(uint8_t power)
{
    char response[64];
    snprintf(response, sizeof(response), "{\"cmd\": \"GP\", \"val\": %d}", power);
    send_ble_notify(response);
}

void handle_cmd_SP(bool success)
{
    send_ble_notify(success ? "{\"cmd\": \"SP\", \"status\": \"ok\"}" : "{\"cmd\": \"SP\", \"status\": \"err\", \"msg\": \"Invalid\"}");
}

void handle_cmd_BZ(bool success)
{
    send_ble_notify(success ? "{\"cmd\": \"BZ\", \"status\": \"ok\"}" : "{\"cmd\": \"BZ\", \"status\": \"err\", \"msg\": \"Invalid\"}");
}
void handle_cmd_SS(bool success)
{
    send_ble_notify(success ? "{\"cmd\": \"SS\", \"status\": \"ok\"}" : "{\"cmd\": \"SS\", \"status\": \"err\", \"msg\": \"Invalid\"}");
}
void handle_cmd_ST(bool success)
{
    send_ble_notify(success ? "{\"cmd\": \"ST\", \"status\": \"ok\"}" : "{\"cmd\": \"ST\", \"status\": \"err\", \"msg\": \"Invalid\"}");
}
// void handle_cmd_GST(uint8_t session, char target) {
//     char response[128];
//     snprintf(response, sizeof(response),
//              "{\"cmd\": \"GST\", \"session\": %d, \"target\": \"%c\"}", session, target);
//     send_ble_notify(response);
// }

// void handle_cmd_SST(bool success) {
//     send_ble_notify(success ? "{\"cmd\": \"SST\", \"status\": \"ok\"}" :
//                               "{\"cmd\": \"SST\", \"status\": \"err\", \"msg\": \"Invalid\"}");
// }

void handle_cmd_GRF(uint8_t val)
{
    char response[64];
    snprintf(response, sizeof(response), "{\"cmd\": \"GRF\", \"val\": %d}", val);
    send_ble_notify(response);
}

void handle_cmd_SRF(bool success)
{
    send_ble_notify(success ? "{\"cmd\": \"SRF\", \"status\": \"ok\"}" : "{\"cmd\": \"SRF\", \"status\": \"err\", \"msg\": \"Invalid\"}");
}


// extern char* check_error_code(uint8_t code) {
//     switch (code) {
//         case 0x10: return "CommandSuccess";
//         case 0x11: return "command_fail";
//         case 0x20: return "mcu_reset_error";
//         case 0x21: return "cw_on_error";
//         case 0x22: return "antenna_missing_error";
//         case 0x23: return "write_flash_error";
//         case 0x24: return "read_flash_error";
//         case 0x25: return "set_output_power_error";
//         case 0x31: return "tag_inventory_error";
//         case 0x32: return "tag_read_error";
//         case 0x33: return "tag_write_error";
//         case 0x34: return "tag_lock_error";
//         case 0x35: return "tag_kill_error";
//         case 0x36: return "no_tag_error";
//         case 0x37: return "inventory_ok_but_access_fail";
//         case 0x38: return "buffer_is_empty_error";
//         case 0x40: return "access_or_password_error";
//         case 0x41: return "parameter_invalid";
//         case 0x42: return "parameter_invalid_wordCnt_too_long";
//         case 0x43: return "parameter_invalid_membank_out_of_range";
//         case 0x44: return "parameter_invalid_lock_region_out_of_range";
//         case 0x45: return "parameter_invalid_lock_action_out_of_range";
//         case 0x46: return "parameter_reader_address_invalid";
//         case 0x47: return "parameter_invalid_AntennaID_out_of_range";
//         case 0x48: return "parameter_invalid_output_power_out_of_range";
//         case 0x49: return "parameter_invalid_frequency_region_out_of_range";
//         case 0x4A: return "parameter_invalid_baudrate_out_of_range";
//         case 0x4B: return "parameter_beeper_mode_out_of_range";
//         case 0x4C: return "parameter_epc_match_len_too_long";
//         case 0x4D: return "parameter_epc_match_len_error";
//         case 0x4E: return "parameter_invalid_epc_match_mode";
//         case 0x4F: return "parameter_invalid_frequency_range";
//         case 0x50: return "fail_to_get_RN16_from_tag";
//         case 0x51: return "parameter_invalid_drm_mode";
//         case 0x52: return "pll_lock_fail";
//         case 0x53: return "rf_chip_fail_to_response";
//         case 0x54: return "fail_to_achieve_desired_output_power";
//         case 0x55: return "copyright_authentication_fail";
//         case 0x56: return "spectrum_regulation_error";
//         case 0x57: return "output_power_too_low";
//         default:   return "unknown_error";
//     }
// }