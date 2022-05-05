#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "lib/bme68x.h"

#include "bme680/bme680_task.h"

void init_task(void* param)
{
    bme680_task_init();

    vTaskDelete(NULL);
}

void app_main(void)
{
    struct bme68x_data result;

    xTaskCreatePinnedToCore(init_task, "init_task", 4096, NULL, 10, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(bme680_task, "bme680_task", 4096, &result, 5, NULL, APP_CPU_NUM);
}
