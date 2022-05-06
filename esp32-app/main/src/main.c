#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "user_def.h"
#include "bme680/bme680_task.h"
#include "mg812/mg812_task.h"

void print_task(void* param)
{
    TickType_t xLastWaketime = xTaskGetTickCount();

    EnvData* env_data = (EnvData*)param;
    while(true){
        printf("%f, %f, %f, %f\n",
                env_data->temperature,
                env_data->pressure,
                env_data->humidity,
                env_data->co2_ppm);

        vTaskDelayUntil(&xLastWaketime, 1000/portTICK_RATE_MS);
    }
}

void init_task(void* param)
{
    bme680_task_init();
    mg812_task_init();

    vTaskDelete(NULL);
}

void app_main(void)
{
    EnvData env_data;

    xTaskCreatePinnedToCore(init_task, "init_task", 4096, NULL, 10, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(bme680_task, "bme680_task", 4096, &env_data, 5, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(mg812_task, "mg812_task", 4096, &env_data, 5, NULL, APP_CPU_NUM);

    xTaskCreatePinnedToCore(print_task, "print_task", 4096, &env_data, 5, NULL, APP_CPU_NUM);
}
