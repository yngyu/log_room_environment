#include "esp_log.h"
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../lib/bme680.h"

#define I2C_MASTER_SCL_IO         (19)
#define I2C_MASTER_SDA_IO         (18)

#define I2C_MASTER_NUM            (0)

#define I2C_MASTER_FREQ_HZ        (100000)
#define I2C_MASTER_TX_BUF_DISABLE (0)
#define I2C_MASTER_RX_BUF_DISABLE (0)
#define I2C_MASTER_TIMEOUT_MS     (1000)

#define BME680_TASK_INTERVAL_MS (1000)

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void bme680_task_init(void* param)
{
    ESP_ERROR_CHECK(i2c_master_init());

    bme680_sensor_t* sensor = bme680_init_sensor(I2C_MASTER_NUM, BME680_I2C_ADDRESS_2, 0);
    bme680_set_oversampling_rates(sensor, osr_16x, osr_16x, osr_16x);
    bme680_set_filter_size(sensor, iir_size_7);
    bme680_set_heater_profile(sensor, 0, 200, 30);
    bme680_use_heater_profile(sensor, 0);
}

void bme680_task(void* param)
{
    TickType_t xLastWaketime = xTaskGetTickCount();
    uint32_t duration = 

    while(true){
        bme680_values_float_t* values = (bme680_values_float_t*)param;

        vTaskDelayUntil(&xLastTickCount, BME680_TASK_INTERVAL_MS/portTICK_RATE_MS);
    }
}

