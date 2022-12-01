/*
 * rest_server_ota.c
 *
 *  Created on: Nov 27, 2022
 *      Author: andreas
 */


#include "esp32_ws2812.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"

typedef enum {
	OST_IDLE,
	OST_CHECK,
	OST_UPDATE,
	OST_UPDATE_FAILED,
	OST_UPDATE_FINISHED
} t_ota_status;

static volatile t_ota_status ota_task_status = OST_IDLE;

extern char *cfg_ota_url;

//static esp_http_client_config_t request_config;

// results
static int status_code;
static int content_length;
static int64_t t_task_start;
static int64_t t_task_end;

static TaskHandle_t xOtaHandle;
//static t_ota_request_data *ota_update_data = NULL;
static char ota_response[128];


static void do_ota_check_https_get(char *url) {

	esp_http_client_config_t request_config;
	memset(&request_config, 0, sizeof(esp_http_client_config_t));
	request_config.url = url;
	request_config.event_handler = common_http_event_handler;
	request_config.crt_bundle_attach = esp_crt_bundle_attach;

	ESP_LOGI(__func__, "start '%s'" , request_config.url);
	esp_http_client_handle_t client = esp_http_client_init(&request_config);
	if ( !client ) {
		snprintf(ota_response, sizeof(ota_response), "failed to init request for %s", request_config.url);
		log_err(__func__, "%s", ota_response);
		return;
	}

	esp_err_t err;
	if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
		snprintf(ota_response, sizeof(ota_response), "Failed to open HTTP connection: %s", esp_err_to_name(err));
		log_err(__func__, "%s", ota_response);
		return;
	}

	memset(ota_response, 0, sizeof(ota_response));

	content_length = esp_http_client_fetch_headers(client);
	status_code = esp_http_client_get_status_code(client);

	ESP_LOGI(__func__, "connected, content len %d", content_length);
	ESP_LOGI(__func__, "HTTP Stream reader Status = %d, content_length = %lld",
			status_code,
			esp_http_client_get_content_length(client));
	if ( content_length >= sizeof(ota_response)) {
		ESP_LOGW(__func__, "content length(%d) larger than allowed (%d)", content_length, sizeof(ota_response)-1);
		content_length = sizeof(ota_response)-1;
	}

	bool chunked = esp_http_client_is_chunked_response(client);
	ESP_LOGI(__func__,"content chunked? '%s'", chunked?"yes":"no");

	if ( status_code == 200 ) {
		int n_read = 0; // upto content_len
		int64_t t_start = esp_timer_get_time();

		do {
			// wait until expected amount read
			n_read = 0;
			while (n_read < content_length ) {
				int read_len = esp_http_client_read(client, ota_response, content_length);
				//ESP_LOGI(__func__, "read_len = %d", read_len);
				if (read_len < 0) {
					log_err(__func__, "Error read data 0x%04x", read_len);
					break;
				} if ( read_len == 0) {
					ESP_LOGE(__func__, "EOF");
					break;
				}
				n_read += read_len;
				if ( n_read < content_length) {
					ESP_LOGI(__func__, "%d bytes read (%d/%d)",  read_len, n_read, content_length);
				}
			} // while
			ESP_LOGI(__func__,"read %d bytes after %lld ms", n_read, (esp_timer_get_time() - t_start) /1000);

			break; // EOF or Error
			// continue with next loop
		} while(1);

	} else {
		log_err(__func__, "Failed to open URL %s, status code is %d", request_config.url, status_code);
		snprintf(ota_response, sizeof(ota_response),"Request failed, status %d", status_code);
	}

	esp_http_client_close(client);
	esp_http_client_cleanup(client);

	ESP_LOGI(__func__, "finished '%s'", url);
}

static void ota_check_main_task(void *pvParameters)
{
	char *url = (char *)pvParameters;

    log_info(__func__, "*** start ***");

    t_task_start = esp_timer_get_time();

    do_ota_check_https_get(url);

    t_task_end = esp_timer_get_time();

    log_info(__func__, "*** finished, duration %lld ms ****",
    		(t_task_end - t_task_start)/1000);

	ota_task_status = OST_IDLE;

    vTaskDelete(NULL);
    // no statements here, task deleted
}

/**
 * reads the file version.txt from ota server
 */
esp_err_t get_handler_ota_check(httpd_req_t *req) {

	esp_err_t rc = ESP_FAIL;
	char *url = NULL;

	do {
		if ( !cfg_ota_url || !strlen(cfg_ota_url)) {
			snprintf(ota_response, sizeof(ota_response), "no URL for OTA configured");
			break;
		}

		if ( ota_task_status != OST_IDLE ) {
			snprintf(ota_response, sizeof(ota_response), "OTA task is already running");
			break;
		}

		asprintf(&url, "%s/version.txt", cfg_ota_url);

		ota_task_status = OST_CHECK;
		memset(ota_response, 0, sizeof(ota_response));
		log_info(__func__, "start check %s", url);

		const uint32_t sz_stack = 8192;
		xTaskCreate(&ota_check_main_task, "ota_check", sz_stack, (void*)url, 0, &xOtaHandle);

		// wait for task end 20 sec
		TickType_t xDelay = 100 / portTICK_PERIOD_MS;
		for ( int i=0; i< 200; i++) {
			vTaskDelay(xDelay);
			if ( ota_task_status == OST_IDLE)
				break; // ended
		}

		if ( ota_task_status != OST_IDLE) {
			vTaskDelete(xOtaHandle);
			snprintf(ota_response, sizeof(ota_response), "time out, server doesn't answer");
			break;
		}

		if (status_code != 200 ) {
			snprintf(ota_response, sizeof(ota_response), "Request failed, status code %d", status_code);
			break;
		}

		if ( !strlen(ota_response)) {
			snprintf(ota_response, sizeof(ota_response), "empty response");
			break;
		}

		httpd_resp_sendstr_chunk(req, ota_response);
		httpd_resp_sendstr_chunk(req, "\n");
		rc = ESP_OK;

	} while(0);

	if ( rc != ESP_OK ) {
		log_err(__func__, "%s", ota_response);
		snprintfapp(ota_response, sizeof(ota_response),"\n");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, ota_response);
	}

	if (url)
		free(url);

	return rc;
}

// ****************************** OTA UPDATE ******************************************

void ota_update_main_task(void *pvParameters)
{
	const esp_app_desc_t *app_desc = esp_app_get_description();
	char *url;
	asprintf(&url, "%s/%s.bin", cfg_ota_url,app_desc->project_name);

	ESP_LOGI(__func__, "Attempting to download update from %s", url);

    t_task_start = esp_timer_get_time();

    memset(ota_response, 0, sizeof(ota_response));

    esp_http_client_config_t request_config;
	memset(&request_config, 0, sizeof(esp_http_client_config_t));
	request_config.url = url;
	request_config.event_handler = common_http_event_handler;
	request_config.crt_bundle_attach = esp_crt_bundle_attach;
	request_config.keep_alive_enable = true;
	request_config.skip_cert_common_name_check = true;

    esp_https_ota_config_t ota_config = {
        .http_config = &(request_config)
    };
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
    	status_code = 1;
    	snprintf(ota_response, sizeof(ota_response), "%s", "OTA successfull, reboot needed");
        log_info(__func__, "OTA Succeed, Reboot needed");
        ota_task_status = OST_UPDATE_FINISHED;
       // esp_restart();
    } else {
    	status_code = -1;
    	snprintf(ota_response, sizeof(ota_response), "%s", "Firmware upgrade failed");
        log_err(__func__, "Firmware upgrade failed");
        ota_task_status = OST_UPDATE_FAILED;
    }

    t_task_end = esp_timer_get_time();

    ESP_LOGI(__func__, "*** finished, duration %lld ms ****",
    		(t_task_end - t_task_start)/1000);

    // wait 20 sec to prevent doing update again
	TickType_t xDelay = 20000 / portTICK_PERIOD_MS;
	vTaskDelay(xDelay);

	if (url)
		free(url);

    ota_task_status = OST_IDLE;
    vTaskDelete(NULL);
    // no statements here, task deleted
}

esp_err_t get_handler_ota_update(httpd_req_t *req) {

	char msg[64];
	memset(msg, 0, sizeof(msg));

	if ( ota_task_status == OST_IDLE ) {
		// doesn't run, start it
		ota_task_status = OST_UPDATE;
		log_info(__func__, "start update");

		const uint32_t sz_stack = 8192;
		xTaskCreate(&ota_update_main_task, "ota_update", sz_stack, NULL, 0, &xOtaHandle);

		snprintf(msg, sizeof(msg), "SUCCESS, firmware update started");
	} else {
		snprintf(msg, sizeof(msg), "update processs busy");
	}

	httpd_resp_sendstr_chunk(req, msg);
	log_info(__func__, "%s", msg);
	httpd_resp_sendstr_chunk(req, "\n");

	return ESP_OK;
}

esp_err_t get_handler_ota_status(httpd_req_t *req) {

	esp_err_t rc = ESP_OK;
	char msg[64];
	memset(msg, 0, sizeof(msg));

	if ( !cfg_ota_url || !strlen(cfg_ota_url)) {
		snprintf(msg, sizeof(msg), "no URL for OTA configured");
	} else {
		switch( ota_task_status) {
		case OST_IDLE:
			snprintf(msg, sizeof(msg), "idle");
			break;

		case OST_CHECK:
			snprintf(msg, sizeof(msg), "check update site is running");
			break;
		case OST_UPDATE:
			snprintf(msg, sizeof(msg), "update is running since %.2f sec",
					(esp_timer_get_time() - t_task_start)/1000000.0 );
			break;
		case OST_UPDATE_FAILED:
			snprintf(msg, sizeof(msg), "update FAILED since %.2f sec",
					(esp_timer_get_time() - t_task_end)/1000000.0 );
			break;
		case OST_UPDATE_FINISHED:
			snprintf(msg, sizeof(msg), "SUCCESS, finished since %.2f sec",
					(esp_timer_get_time() - t_task_end)/1000000.0 );
			break;
		}
	}

	httpd_resp_sendstr_chunk(req, msg);
	log_info(__func__, "%s", msg);
	httpd_resp_sendstr_chunk(req, "\n");
	if ( strlen(ota_response)) {
		httpd_resp_sendstr_chunk(req, ota_response);
		log_info(__func__, "%s", ota_response);
		httpd_resp_sendstr_chunk(req, "\n");
	}

	return rc;
}
