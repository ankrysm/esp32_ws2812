/*
 * rest_client.c
 *
 *  Created on: Sep 11, 2022
 *      Author: andreas
 */


#include "esp32_ws2812.h"
#include "esp_http_client.h"
#include "base64.h"


// store data for 500 leds
#define MAX_HTTP_RECV_BUFFER 500*3
#define CNT_HTTP_BUFFER 2

typedef enum {
	HC_CLOSED,
	HC_GOT_HEADER,
	HC_READ_DATA
} t_http_connect_status;

// alterning buffers
char data_buffer[CNT_HTTP_BUFFER][MAX_HTTP_RECV_BUFFER];
int data_buffer_status[CNT_HTTP_BUFFER]; //

static char *data_url = NULL;
static esp_http_client_config_t config = {
    .url = data_url,
};
static esp_http_client_handle_t client;
static t_http_connect_status connection_status = HC_CLOSED;
static int content_length = 0;
static int total_read_len = 0;

int data_width = 0;

void set_data_url(char *url) {
	if ( data_url ) {
		free(data_url);
	}
	data_url = strdup(url);

}

// read data format
// header: 4 byte witdth in bytes (3 bytes per pixel
esp_err_t read_from_data_connection() {
	esp_err_t err;
	if ( connection_status == HC_CLOSED) {
		memset(data_buffer, 0, sizeof(data_buffer));
		memset(data_buffer_status, 0, sizeof(data_buffer_status));
		client = esp_http_client_init(&config);
		if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
			ESP_LOGE(__func__, "Failed to open HTTP connection: %s, %s", esp_err_to_name(err), data_url);
			return err;
		}
		content_length =  esp_http_client_fetch_headers(client);
		char buf [16];
		// read header 8 characters  bytes AABBCCDD,

	}

	ESP_LOGI(__func__, "HTTP Stream reader Status = %d, content_length = %lld",
			esp_http_client_get_status_code(client),
			esp_http_client_get_content_length(client));

	int read_len;
	if (total_read_len < content_length) {
		read_len = esp_http_client_read(client, buffer, content_length);
		if (read_len <= 0) {
			ESP_LOGE(__func__, "Error read data");
		}
		buffer[read_len] = 0;
		ESP_LOGD(__func__, "read_len = %d", read_len);
	} else {
	}


}

static void http_perform_as_stream_reader(void)
{
    client = esp_http_client_init(&config);
    esp_err_t err;
    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        ESP_LOGE(__func__, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        free(buffer);
        return;
    }
    int content_length =  esp_http_client_fetch_headers(client);
    int total_read_len = 0, read_len;
    if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER) {
        read_len = esp_http_client_read(client, buffer, content_length);
        if (read_len <= 0) {
            ESP_LOGE(__func__, "Error read data");
        }
        buffer[read_len] = 0;
        ESP_LOGD(__func__, "read_len = %d", read_len);
    }
    ESP_LOGI(__func__, "HTTP Stream reader Status = %d, content_length = %lld",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    free(buffer);
}



static void http_data_task(void *pvParameters) {
    http_perform_as_stream_reader();

}

void res_client_init() {
    xTaskCreate(&http_data_task, "http_data_task", 8192, NULL, 5, NULL);
}
