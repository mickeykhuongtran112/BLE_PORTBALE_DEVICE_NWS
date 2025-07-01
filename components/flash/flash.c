
#include "flash.h"
#include "main.h"
esp_err_t write_string_to_nvs(const char *key, const char *value)
{
    nvs_handle_t nvs_handle;
    esp_err_t err_add = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err_add != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err_add));
        return err_add;
    }

    // Ghi chuỗi ký tự vào NVS
    err_add = nvs_set_str(nvs_handle, key, value);
    if (err_add != ESP_OK)
    {
        printf("Error (%s) writing string to NVS!\n", esp_err_to_name(err_add));
    }
    else
    {
        printf("String written to NVS successfully!\n");
    }

    // Lưu lại thay đổi
    err_add = nvs_commit(nvs_handle);
    if (err_add != ESP_OK)
    {
        printf("Error (%s) committing changes to NVS!\n", esp_err_to_name(err_add));
    }

    nvs_close(nvs_handle);
    return err_add;
}
esp_err_t read_string_from_nvs(const char *key, char *value, size_t max_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err_add = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err_add != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err_add));
        return err_add;
    }

    size_t required_size = 0;
    err_add = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err_add != ESP_OK)
    {
        if (err_add == ESP_ERR_NVS_NOT_FOUND)
        {
            printf("The key '%s' was not found in NVS!\n", key);
        }
        else
        {
            printf("Error (%s) getting string size from NVS!\n", esp_err_to_name(err_add));
        }
        nvs_close(nvs_handle);
        return err_add;
    }

    if (required_size > max_len)
    {
        printf("Buffer size is too small for the string (required: %zu, provided: %zu)!\n", required_size, max_len);
        nvs_close(nvs_handle);
        return ESP_ERR_NO_MEM;
    }

    // Đọc chuỗi
    err_add = nvs_get_str(nvs_handle, key, value, &required_size);
    if (err_add == ESP_OK)
    {
        printf("String read from NVS successfully: %s\n", value);
    }
    else
    {
        printf("Error (%s) reading string from NVS!\n", esp_err_to_name(err_add));
    }

    nvs_close(nvs_handle);
    return err_add;
}
esp_err_t write_double_to_nvs(const char *key, double value)
{
    nvs_handle_t nvs_handle;
    esp_err_t err_add = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err_add != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err_add));
        return err_add;
    }
    uint8_t data[sizeof(double)];
    memcpy(data, &value, sizeof(double));

    err_add = nvs_set_blob(nvs_handle, key, data, sizeof(double));
    if (err_add != ESP_OK)
    {
        printf("Error (%s) writing value to NVS!\n", esp_err_to_name(err_add));
    }
    else
    {
        printf("Value written double to NVS successfully!\n");
    }

    nvs_close(nvs_handle);
    return err_add;
}

esp_err_t read_double_from_nvs(const char *key, double *value)
{
    nvs_handle_t nvs_handle;
    esp_err_t err_add = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err_add != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err_add));
        return err_add;
    }
    size_t size = sizeof(double);

    err_add = nvs_get_blob(nvs_handle, key, value, &size);
    switch (err_add)
    {
    case ESP_OK:
        printf("Value read double from NVS successfully: %f\n", *value);
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        printf("The value is not found in NVS!\n");
        break;
    default:
        printf("Error (%s) reading value from NVS!\n", esp_err_to_name(err_add));
        break;
    }
    nvs_close(nvs_handle);
    return err_add;
}
