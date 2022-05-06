#include "mg812_task.h"

#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "../user_def.h"

#define ADC1_READ_PIN           ADC1_CHANNEL_5
#define ADC_CALI_SCHEME         ESP_ADC_CAL_VAL_EFUSE_VREF

#define MG812_TASK_INETERVAL_MS (1000)

static esp_adc_cal_characteristics_t adc1_chars;

void mg812_task_init(void)
{
    ESP_ERROR_CHECK(esp_adc_cal_check_efuse(ADC_CALI_SCHEME));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);

    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_READ_PIN, ADC_ATTEN_DB_11));
}

void mg812_task(void* param)
{
    TickType_t xLastWaketime = xTaskGetTickCount();
    EnvData* env_data = (EnvData*)param;

    while(true){
        static double a = (4.0 - log10(400.0))/(2650.0 - 3250.0);
        static double b = log10(400);

        float voltage = (float)esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_5), &adc1_chars); // mV
        env_data->co2_ppm = (float)pow(10.0, (a*(voltage - 3250) + b));

        vTaskDelayUntil(&xLastWaketime, MG812_TASK_INETERVAL_MS/portTICK_RATE_MS);
    }
}
