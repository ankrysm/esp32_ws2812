/*
 * https_get.c
 *
 *  Created on: Oct 4, 2022
 *      Author: andreas
 */

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
//#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#include "esp_http_client.h"
#include "https_get.h"

extern const char server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const char server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

static https_get_callback s_callback;
static esp_http_client_config_t request_config;
static volatile bool connection_active = false;

static TaskHandle_t xHandle = NULL;

esp_err_t _http_event_handler(esp_http_client_event_t *evt){
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(__func__, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(__func__, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(__func__, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(__func__, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(__func__, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(__func__, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(__func__, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(__func__, "Last esp error code: 0x%x", err);
                ESP_LOGI(__func__, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(__func__, "HTTP_EVENT_REDIRECT");
            //esp_http_client_set_header(evt->client, "From", "user@example.com");
            //esp_http_client_set_header(evt->client, "Accept", "text/html");
            break;
    }
    return ESP_OK;
}

static void do_https_get() {

	ESP_LOGI(__func__, "start '%s'" , request_config.url);
    esp_http_client_handle_t client = esp_http_client_init(&request_config);
    if ( !client ) {
    	ESP_LOGE(__func__, "failed to init request");
    	return;
    }
    esp_err_t err;
    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        ESP_LOGE(__func__, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return;
    }

    int content_length =  esp_http_client_fetch_headers(client);
    ESP_LOGI(__func__, "connected, content len %d", content_length);
    ESP_LOGI(__func__, "HTTP Stream reader Status = %d, content_length = %lld",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
    bool chunked = esp_http_client_is_chunked_response(client);
    ESP_LOGI(__func__,"content chunked? '%s'", chunked?"yes":"no");

    uint8_t *buf;
    size_t expected_len = 0;

    s_callback(HCT_INIT, &buf, &expected_len); // init

    int read_len=0;
    size_t total_read_len = 0;
    do {

    	int len = expected_len;
    	// wait until expected amount read
        total_read_len = 0;
    	while (len > 0 ) {
    		read_len = esp_http_client_read(client, (char *) buf, expected_len);

    		if (read_len < 0) {
    			ESP_LOGE(__func__, "Error read data 0x%04x", read_len);
    			break;
    		} if ( read_len == 0) {
    			ESP_LOGE(__func__, "EOF");
    			break;
    		}
    		len -= read_len;
    		total_read_len += read_len;
    		if ( len>0) {
    			ESP_LOGI(__func__, "%d bytes read, remains %d", read_len, len);
    		}
    	}

    	if ( read_len <=0)
    		break;

        int rc = s_callback(HCT_READING, &buf, &expected_len);
   		if ( rc < 0 )
           	break; // failure

    } while(1);

    if ( total_read_len > 0 ) {
    	// there are some data in buf
    	s_callback(HCT_READING, &buf, &total_read_len);
    }

    expected_len = 0;
    s_callback(HCT_FINISH, &buf, &expected_len);

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    ESP_LOGI(__func__, "finished '%s'" , request_config.url);
    free((void*)request_config.url);

}

static void http_main_task(void *pvParameters)
{
    ESP_LOGI(__func__, "*** start ***");
    do_https_get();
    ESP_LOGI(__func__, "*** do_https_get finished ****");

    connection_active = false;
    ESP_LOGI(__func__, "*** finished, connection active=%s ***", connection_active?"true":"false");
    vTaskDelete(NULL);
}

esp_err_t https_get(char *url, https_get_callback callback) {
	if ( connection_active ) {
		ESP_LOGW(__func__, "there's an open connection url='%s'", url);
		return ESP_FAIL;
	}
	connection_active = true;

	memset(&request_config, 0, sizeof(request_config));
    request_config.url = strdup(url);
    request_config.event_handler = _http_event_handler;
    request_config.cert_pem = server_root_cert_pem_start;
    s_callback = callback;

	xTaskCreate(&http_main_task, "http_main_task", 8192, NULL, 5, &xHandle);
	ESP_LOGI(__func__, "'http_main_task' started");
	return ESP_OK;

}

bool is_https_connection_active() {
	return connection_active;
}
