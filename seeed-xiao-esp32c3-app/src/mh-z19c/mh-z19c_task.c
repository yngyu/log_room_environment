#include "mh-z19c_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>

#include "../env_data.h"

#define TXD_PIN (GPIO_NUM_21)
#define RXD_PIN (GPIO_NUM_20)
#define  RX_BUF_SIZE (1024)

static void send_read_command(void)
{
    const char* TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);

    const char command[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uart_write_bytes(UART_NUM_1, command, 9);
}

void mh_z19c_task_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void mh_z19c_task(void* pvParameters)
{
    TickType_t xLastWaketime = xTaskGetTickCount();
    EnvData* env_data = (EnvData*)pvParameters;

    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    static uint8_t data[RX_BUF_SIZE + 1];

    while (1)
    {
        send_read_command();
        vTaskDelayUntil(&xLastWaketime, 500/portTICK_PERIOD_MS);

        memset(data, 0, RX_BUF_SIZE + 1);
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[RX_BUF_SIZE] = 0;
            const float co2 = (float)((data[2] << 8) | data[3]);
            if (xSemaphoreTake(env_data->semaphore, portMAX_DELAY) == pdTRUE) {
                    env_data->co2_ppm = co2;

                xSemaphoreGive(env_data->semaphore);
            }
        }

        vTaskDelayUntil(&xLastWaketime, 500/portTICK_PERIOD_MS);
    }
}
