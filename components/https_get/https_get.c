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
#include "esp_crt_bundle.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"

#include "https_get.h"

static T_HTTPS_CLIENT_SLOT https_get_slots[N_HTTPS_CLIENTS];
static bool init_needed=true;

//static int64_t t_task_start;

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

static void do_https_get(T_HTTPS_CLIENT_SLOT *slot) {

	uint32_t expected_len = 0;
	uint8_t *buf = NULL;

	ESP_LOGI(__func__, "start '%s'" , slot->request_config.url);
	esp_http_client_handle_t client = esp_http_client_init(&(slot->request_config));
	if ( !client ) {
		strlcpy(slot->errmsg, "failed to init request", sizeof(slot->errmsg));
		ESP_LOGE(__func__, "%s" , slot->errmsg);
		return;
	}

	esp_err_t err;
	if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
		snprintf(slot->errmsg, sizeof(slot->errmsg), "Failed to open HTTP connection: %s", esp_err_to_name(err));
		ESP_LOGE(__func__, "%s" , slot->errmsg);
		expected_len = 0;
		slot->todo = HCT_FAILED;
		slot->callback(slot, &buf, &expected_len);
		return;
	}

	int content_length =  esp_http_client_fetch_headers(client);
	int status_code = esp_http_client_get_status_code(client);
	ESP_LOGI(__func__, "connected, content len %d", content_length);
	ESP_LOGI(__func__, "HTTP Stream reader Status = %d, content_length = %lld",
			status_code,
			esp_http_client_get_content_length(client));
	bool chunked = esp_http_client_is_chunked_response(client);
	ESP_LOGI(__func__,"content chunked? '%s'", chunked?"yes":"no");

	if ( status_code == 200 ) {
		slot->todo = HCT_INIT;
		slot->callback(slot, &buf, &expected_len); // init

		int read_len=0;
		int total_read_len = 0;
		int n_read = 0 ;

		do {
			// wait until expected amount read
			int64_t t_start = esp_timer_get_time();
			int64_t t_end = t_start;
			n_read = 0;
			while (n_read < expected_len ) {
				read_len = esp_http_client_read(client, (char *) buf, expected_len);
				//ESP_LOGI(__func__, "read_len = %d", read_len);
				if (read_len < 0) {
					ESP_LOGE(__func__, "Error read data 0x%04x", read_len);
					break;
				} if ( read_len == 0) {
					ESP_LOGE(__func__, "EOF");
					break;
				}
				n_read += read_len;
				total_read_len += read_len;
				if ( n_read < expected_len) {
					ESP_LOGI(__func__, "%d bytes read (%d/%d)",  read_len, n_read , expected_len);
				}
				if ( slot->logflag ) {
					ESP_LOGI(__func__,"first %d bytes after %lld ms", read_len, (esp_timer_get_time() - slot->t_task_start) /1000);
					slot->logflag = false;
				}
			}

			int rc;
			if ( read_len <=0) {
				if ( n_read > 0) {
					t_end = esp_timer_get_time();
					ESP_LOGI(__func__,"last read %d bytes in %lld ms", expected_len, (t_end-t_start)/1000);

					// there's something remaining in the buffer
					slot->todo = HCT_READING;
					rc = slot->callback(slot, &buf, &expected_len);
					if ( rc < 0) {
						ESP_LOGW(__func__,"callback after EOF or error failed %s", slot->errmsg);
					}
				}
				break; // EOF or Error
			}

			t_end = esp_timer_get_time();
			ESP_LOGI(__func__,"read %d bytes in %lld ms", expected_len, (t_end-t_start)/1000);

			// all read; n_read == expected_len
			slot->todo = HCT_READING;
			rc = slot->callback(slot, &buf, &expected_len);
			if ( rc < 0 ) {
				ESP_LOGW(__func__,"callback failed: %s", slot->errmsg);
			}
			if ( rc < 0 )
				break; // failure or want to stop connection

			// continue with next loop
		} while(1);


		expected_len = 0;
		slot->todo = HCT_FINISH;
		slot->callback(slot, &buf, &expected_len);

	} else {
		expected_len = 0;
		slot->todo = HCT_FAILED;
		slot->callback(slot, &buf, &expected_len);
	}

	esp_http_client_close(client);
	esp_http_client_cleanup(client);

	ESP_LOGI(__func__, "(%s) finished '%s'" , slot->name, slot->request_config.url);
	free((void*)slot->request_config.url);
}

static void http_main_task(void *pvParameters)
{
    T_HTTPS_CLIENT_SLOT *slot = (T_HTTPS_CLIENT_SLOT*) pvParameters;
    ESP_LOGI(__func__, "*** (%s) start ***", slot->name);
    slot->t_task_start = esp_timer_get_time();

    do_https_get(slot);

    slot->t_task_end = esp_timer_get_time();

    ESP_LOGI(__func__, "*** (%s) finished, duration %lld ms ****",
    		slot->name, (slot->t_task_end - slot->t_task_start)/1000);
    slot->status = HSS_EMPTY;
    //http_client_task_active = false;

    vTaskDelete(NULL);
    // no statements here, task deleted
}

/*
 * main function for https get
 * returns
 *  - ESP_OK if client slot could started
 *  - ESP_FAIL if not slot is free
 */
T_HTTPS_CLIENT_SLOT *https_get(char *url, https_get_callback callback, void *user_args) {
	if ( init_needed) {
		memset(https_get_slots, 0, sizeof(https_get_slots));
		for ( int i=0; i < N_HTTPS_CLIENTS; i++) {
			snprintf(https_get_slots[i].name, LEN_HTTPS_CLIENT_SLOT_NAME, "webclnt_%d", i);
		}
		init_needed=false;
	}

	// look for a free slot
	T_HTTPS_CLIENT_SLOT *slot = NULL;
	int idx=-1;
	for ( int i=0; i < N_HTTPS_CLIENTS; i++) {
		if ( https_get_slots[i].status == HSS_EMPTY) {
			idx=i;
			slot = &(https_get_slots[idx]);
			slot->status = HSS_ACTIVE;
			slot->callback = callback;
			slot->user_args = user_args;
			slot->logflag = true;
			memset(slot->errmsg, 0, LEN_HTTPS_CLIENT_ERRMSG);
			break;
		}
	}
	if ( idx < 0) {
		ESP_LOGE(__func__, "there's no free slot for request '%s'", url);
		return NULL;
	}
	//  if ( http_client_task_active ) {
	//    ESP_LOGW(__func__, "there's an open connection url='%s'", url);
	//    return ESP_FAIL;
	//  }
	//  http_client_task_active = true;

	esp_http_client_config_t *request_config = &(slot->request_config);
	memset(request_config, 0, sizeof(esp_http_client_config_t));
	request_config->url = strdup(url);
	request_config->event_handler = _http_event_handler;
	request_config->crt_bundle_attach = esp_crt_bundle_attach;

	xTaskCreate(&http_main_task, slot->name, 16384, (void*) slot, 5, &(slot->xHandle));
	ESP_LOGI(__func__, "'http_main_task' started");
	return slot;

}

//bool is_http_client_task_active() {
//  return http_client_task_active;
//}
