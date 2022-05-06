#include "ble_task.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

#define DEVICE_NAME "ESP32-WROVER-E"
#define ADV_DATA_LEN (18)

#define BLE_TASK_INTERVAL_MS (1000)

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static uint8_t raw_adv_data[ADV_DATA_LEN];

static void set_data_to_ble(uint8_t* adv_data, const EnvData* env_data)
{
    memcpy(adv_data + 2,  &env_data->temperature, sizeof(float));
    memcpy(adv_data + 6,  &env_data->pressure, sizeof(float));
    memcpy(adv_data + 10, &env_data->humidity, sizeof(float));
    memcpy(adv_data + 14, &env_data->co2_ppm, sizeof(float));
}

void ble_task(void* param)
{
    EnvData* env_data = (EnvData*)param;

    TickType_t xLastWaketime = xTaskGetTickCount();

    while(true){
        raw_adv_data[0] = ADV_DATA_LEN - 1;
        raw_adv_data[1] = 0xFF;
        set_data_to_ble(raw_adv_data, env_data);
        ESP_ERROR_CHECK(esp_ble_gap_config_adv_data_raw(raw_adv_data, ADV_DATA_LEN));

        vTaskDelayUntil(&xLastWaketime, BLE_TASK_INTERVAL_MS/portTICK_RATE_MS);
    }
}

void ble_start(EnvData* env_data)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));

    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_set_device_name(DEVICE_NAME));
    raw_adv_data[0] = ADV_DATA_LEN - 1;
    raw_adv_data[1] = 0xFF;
    set_data_to_ble(raw_adv_data, env_data);
    ESP_ERROR_CHECK(esp_ble_gap_config_adv_data_raw(raw_adv_data, ADV_DATA_LEN));
    esp_ble_gap_start_advertising(&adv_params);

    xTaskCreatePinnedToCore(ble_task, "ble_task", 4096, env_data, 10, NULL, APP_CPU_NUM);
}
