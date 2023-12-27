#include "bme680_task.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#include "../lib/bme68x.h"

#include "../env_data.h"

#define I2C_MASTER_SCL_IO         (GPIO_NUM_7)
#define I2C_MASTER_SDA_IO         (GPIO_NUM_6)

#define I2C_MASTER_NUM            (0)

#define I2C_MASTER_FREQ_HZ        (100000)
#define I2C_MASTER_TX_BUF_DISABLE (0)
#define I2C_MASTER_RX_BUF_DISABLE (0)
#define I2C_MASTER_TIMEOUT_MS     (1000)

#define BME680_TASK_INTERVAL_MS   (1000)

uint8_t dev_addr = BME68X_I2C_ADDR_HIGH;

static struct bme68x_dev bme;
static struct bme68x_conf conf;
static struct bme68x_heatr_conf heatr_conf;

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

bme68x_read_fptr_t bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)intf_ptr;
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, dev_addr, &reg_addr, 1, reg_data, len, I2C_MASTER_TIMEOUT_MS/portTICK_PERIOD_MS);
    return (bme68x_read_fptr_t)ret;
}

bme68x_write_fptr_t bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)intf_ptr;

    uint8_t* tx_data = malloc(len + 1);
    tx_data[0] = reg_addr;
    memcpy(tx_data + 1, reg_data, len);
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, dev_addr, tx_data, len + 1, I2C_MASTER_TIMEOUT_MS/portTICK_PERIOD_MS);
    free(tx_data);

    return (bme68x_write_fptr_t)ret;
}

// period: microsecond
void bme68x_delay_us(uint32_t period, void *intf_ptr)
{
    uint32_t wait = period > 1000*portTICK_PERIOD_MS ? period/1000/portTICK_PERIOD_MS : 1;
    vTaskDelay(wait);
}

void bme680_task_init(void)
{
    ESP_ERROR_CHECK(i2c_master_init());

    bme.read = bme68x_i2c_read;
    bme.write = bme68x_i2c_write;
    bme.intf = BME68X_I2C_INTF;
    bme.delay_us = bme68x_delay_us;
    bme.intf_ptr = &dev_addr;
    bme.amb_temp = 25;
    ESP_ERROR_CHECK(bme68x_init(&bme));

    conf.filter = BME68X_FILTER_SIZE_7;
    conf.odr = BME68X_ODR_NONE;
    conf.os_hum = BME68X_OS_16X;
    conf.os_pres = BME68X_OS_16X;
    conf.os_temp = BME68X_OS_16X;
    ESP_ERROR_CHECK(bme68x_set_conf(&conf, &bme));

    heatr_conf.enable = BME68X_DISABLE;
    ESP_ERROR_CHECK(bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme));
}

void bme680_task(void* pvParameters)
{
    TickType_t xLastWaketime = xTaskGetTickCount();
    EnvData* env_data = (EnvData*)pvParameters;

    while(true){
        ESP_ERROR_CHECK(bme68x_set_op_mode(BME68X_FORCED_MODE, &bme));

        // gets microseconds
        uint32_t duration = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) + (heatr_conf.heatr_dur * 1000);
        vTaskDelay(duration/1000/portTICK_PERIOD_MS);
        uint8_t get_flag = 0;
        struct bme68x_data data;
        ESP_ERROR_CHECK(bme68x_get_data(BME68X_FORCED_MODE, &data, &get_flag, &bme));

        if (xSemaphoreTake(env_data->semaphore, portMAX_DELAY) == pdTRUE) {
            env_data->temperature = data.temperature;
            env_data->pressure = data.pressure;
            env_data->humidity = data.humidity;

            xSemaphoreGive(env_data->semaphore);
        }

        vTaskDelayUntil(&xLastWaketime, BME680_TASK_INTERVAL_MS/portTICK_PERIOD_MS);
    }
}

