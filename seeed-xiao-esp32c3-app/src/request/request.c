#include "request.h"

#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#include "request_config.h"
#include "../env_data.h"
#include "../usr/user_tag.h"

#define MAX_HTTP_RESPONSE_BUFFER (2048)
#define MAX_HTTP_BODY_BUFFER     (2048)

static const char* TAG = "HTTP_CLIENT";
static char local_response_buffer[MAX_HTTP_RESPONSE_BUFFER] = {0};
static char post_data[MAX_HTTP_BODY_BUFFER] = {0};

static esp_http_client_handle_t client;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;

        default:
            ESP_LOGE(TAG, "Unexpected event");
            printf("%d\n", evt->event_id);
            break;
    }
    return ESP_OK;
}

void request_init(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    esp_http_client_config_t config = {
        .url = "http://localhost",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
        .disable_auto_redirect = true,
    };

    client = esp_http_client_init(&config);

    esp_http_client_set_url(client, influxdb_end_point);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Authorization", authorization_token);
    esp_http_client_set_header(client, "Content-Type", "text/plain; charset=utf-8");
    esp_http_client_set_header(client, "Accept", "application/json");
}

void request_task(void *pvParameters)
{
    TickType_t xLastWaketime = xTaskGetTickCount();
    const EnvData* env_data = (EnvData*)pvParameters;

    int continuous_error = 0;
    TickType_t last_error_tick = 0;

    while(1) {
        memset(post_data, 0, MAX_HTTP_BODY_BUFFER);

        bool is_valid = true;

        if (xSemaphoreTake(env_data->semaphore, portMAX_DELAY) == pdTRUE) {
            is_valid = validate_env_data(env_data);
            sprintf(post_data, "%s,sensor_id=%s temperature=%.2f,pressure=%.2f,humidity=%.2f,co2_ppm=%.2f,rssi=%d",
                place_tag,
                sensor_tag,
                env_data->temperature,
                env_data->pressure,
                env_data->humidity,
                env_data->co2_ppm,
                env_data->rssi);

            xSemaphoreGive(env_data->semaphore);
        }

        if (is_valid) {
            esp_http_client_set_post_field(client, post_data, strlen(post_data));

            esp_err_t err = esp_http_client_perform(client);

            if (err != ESP_OK) {
                ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));

                const TickType_t now = xTaskGetTickCount();
                if (now - last_error_tick < 2 * 1000 * 10/portTICK_PERIOD_MS) {
                    ++continuous_error;
                }else {
                    continuous_error = 1;
                }
                last_error_tick = now;
                ESP_LOGI(TAG, "Continuous error: %d, happened tick: %ld", continuous_error, last_error_tick);

                if (continuous_error >= 6) {
                    // Reset
                    esp_restart();
                }
            }
        }

        vTaskDelayUntil(&xLastWaketime, 1000 * 10/portTICK_PERIOD_MS);
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}
