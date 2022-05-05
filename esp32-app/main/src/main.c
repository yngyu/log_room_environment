#include <stdio.h>

#include "esp_log.h"
#include "driver/i2c.h"

#include "lib/bme680.h"
#include "settings.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());

    bme680_sensor_t* sensor = bme680_init_sensor(I2C_MASTER_NUM, BME680_I2C_ADDRESS_2, 0);
    
    /** -- SENSOR CONFIGURATION PART (optional) --- */

    // Changes the oversampling rates to 4x oversampling for temperature
    // and 2x oversampling for humidity. Pressure measurement is skipped.
    bme680_set_oversampling_rates(sensor, osr_16x, osr_16x, osr_16x);

    // Change the IIR filter size for temperature and pressure to 7.
    bme680_set_filter_size(sensor, iir_size_7);

    // Change the heater profile 0 to 200 degree Celcius for 100 ms.
    bme680_set_heater_profile (sensor, 0, 100, 100);
    bme680_use_heater_profile (sensor, 0);

    bme680_set_ambient_temperature (sensor, 25);

    bme680_values_float_t values;
    uint32_t duration = bme680_get_measurement_duration(sensor);

    while(1){
        if (bme680_force_measurement(sensor)){
            vTaskDelay (duration);

            if (bme680_get_results_float (sensor, &values))
                printf("BME680 Sensor: %.2f °C, %.2f %%, %.2f hPa, %.2f Ohm\n",
                       values.temperature, values.humidity,
                       values.pressure, values.gas_resistance);
        }
        vTaskDelay(100);
    }
}
