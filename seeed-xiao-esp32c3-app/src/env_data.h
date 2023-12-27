#ifndef ENV_DATA_H_
#define ENV_DATA_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdbool.h>

typedef struct
{
    float temperature;
    float humidity;
    float pressure;
    float co2_ppm;

    SemaphoreHandle_t semaphore;
} EnvData;

// Required: already took semphore
bool validate_env_data(const EnvData* env_data);

#endif
