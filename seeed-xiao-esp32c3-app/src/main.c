#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "env_data.h"
#include "mh-z19c/mh-z19c_task.h"
#include "bme680/bme680_task.h"
#include "wifi/wifi.h"
#include "request/request.h"

static EnvData env_data;

void print_task(void* pvParameters);

void nvs_init(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void app_init()
{
    memset(&env_data, 0x00, sizeof(EnvData));
    env_data.semaphore = xSemaphoreCreateMutex();

    nvs_init();
    wifi_init_sta();

    mh_z19c_task_init();
    bme680_task_init();
    request_init();
}

void app_main()
{
    app_init();

    xTaskCreate(mh_z19c_task, "mh_z19c_task", 2048, &env_data, 1, NULL);
    xTaskCreate(bme680_task, "bme680_task", 2048, &env_data, 1, NULL);
    xTaskCreate(wifi_task, "wifi_task", 2048, &env_data, 1, NULL);

    xTaskCreate(request_task, "request_task", 16384, &env_data, 2, NULL);

    // xTaskCreate(print_task, "print_task", 2048, &env_data, 1, NULL);
}

void print_task(void* pvParameters)
{
    TickType_t xLastWaketime = xTaskGetTickCount();
    EnvData* env_data = (EnvData*)pvParameters;

    while(true){
        if (xSemaphoreTake(env_data->semaphore, portMAX_DELAY) == pdTRUE) {
            printf("%f, %f, %f, %f, %d\n",
                env_data->temperature,
                env_data->pressure,
                env_data->humidity,
                env_data->co2_ppm,
                env_data->rssi);

            xSemaphoreGive(env_data->semaphore);
        }

        vTaskDelayUntil(&xLastWaketime, 2000/portTICK_PERIOD_MS);
    }
}
