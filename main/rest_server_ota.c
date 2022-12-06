/**
 * HTTP Restful API Server - OTA services
 * based on esp-idf examples
 *
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
	OST_IDLE, // needs check
	OST_CHECK,
	OST_CHECK_FINISHED,
	OST_CHECK_FINISHED_UPTODATE,
	OST_CHECK_FINISHED_UPDATE_NEEDED,
	OST_CHECK_FINISHED_UPDATE_OPTIONAL,
	OST_CHECK_FINISHED_ERROR,
	OST_UPDATE,
	OST_UPDATE_FAILED,
	OST_UPDATE_FINISHED
} t_ota_status;

static volatile t_ota_status ota_task_status = OST_IDLE;

extern char *cfg_ota_url;
extern char sha256_hash_run_partition[];

// results
static int status_code;
static int content_length;
static int64_t t_task_start = 0;
static int64_t t_task_end = 0;

static TaskHandle_t xOtaHandle;
static char ota_response[192];

char *ota_status2text(t_ota_status status) {
	switch(status) {
	case OST_IDLE:                           return "CHECK_NEEDED";
	case OST_CHECK:                          return "CHECK_IS_RUNNING";
	case OST_CHECK_FINISHED:                 return "CHECK_FINISHED";
	case OST_CHECK_FINISHED_UPTODATE:        return "UP_TO_DATE";
	case OST_CHECK_FINISHED_UPDATE_NEEDED:   return "UPDATE_NEEDED";
	case OST_CHECK_FINISHED_UPDATE_OPTIONAL: return "UPDATE_OPTIONAL";
	case OST_CHECK_FINISHED_ERROR:           return "UPDATE_CHECK_FAILED";
	case OST_UPDATE:                         return "UPDATE_IS_RUNNING";
	case OST_UPDATE_FAILED:                  return "UPDATE_FAILED";
	case OST_UPDATE_FINISHED:                return "UPDATE_FINISHED";
	default:
		return "?????";
	}
}

bool ota_is_running(t_ota_status status) {
	switch(status) {
	case OST_IDLE:
	case OST_CHECK:
	case OST_UPDATE:
		return true;
	default:
		return false;
	}

}

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

    do_ota_check_https_get(url);

    t_task_end = esp_timer_get_time();

    log_info(__func__, "*** finished, duration %lld ms ****",
    		(t_task_end - t_task_start)/1000);

	ota_task_status = OST_CHECK_FINISHED;

    vTaskDelete(NULL);
    // no statements here, task deleted
}

/**
 * reads the file version.txt from ota server
 */
esp_err_t get_handler_ota_check(httpd_req_t *req) {

	esp_err_t rc = ESP_FAIL;
	char *url = NULL;
	char msg[128];
	memset(msg,0, sizeof(msg));

	do {
		if ( !cfg_ota_url || !strlen(cfg_ota_url)) {
			snprintf(ota_response, sizeof(ota_response), "no URL for OTA configured");
			break;
		}

		bool failed = false;

		switch ( ota_task_status ) {
		case OST_CHECK:
		case OST_CHECK_FINISHED: // temporary status
		case OST_UPDATE:
			failed = true;
			break;
		default:
			break;
		}

		if ( failed) {
			snprintf(ota_response, sizeof(ota_response), "OTA task is already running");
			break;
		}

		asprintf(&url, "%s/version.txt", cfg_ota_url);

		ota_task_status = OST_CHECK;
		memset(ota_response, 0, sizeof(ota_response));
	    t_task_start = esp_timer_get_time();
	    t_task_end = 0;
		log_info(__func__, "start check %s", url);

		const uint32_t sz_stack = 8192;
		xTaskCreate(&ota_check_main_task, "ota_check", sz_stack, (void*)url, 0, &xOtaHandle);

		// wait for task end 20 sec
		TickType_t xDelay = 100 / portTICK_PERIOD_MS;
		for ( int i=0; i< 200; i++) {
			vTaskDelay(xDelay);
			if ( ota_task_status == OST_CHECK_FINISHED)
				break; // ended
		}

		if ( ota_task_status != OST_CHECK_FINISHED) {
			vTaskDelete(xOtaHandle);
			snprintf(ota_response, sizeof(ota_response), "time out, server doesn't answer");
			ota_task_status = OST_CHECK_FINISHED_ERROR;
			break;
		}

		if (status_code != 200 ) {
			snprintf(ota_response, sizeof(ota_response), "Request failed, status code %d", status_code);
			ota_task_status = OST_CHECK_FINISHED_ERROR;
			break;
		}

		if ( !strlen(ota_response)) {
			ota_task_status = OST_CHECK_FINISHED_ERROR;
			snprintf(ota_response, sizeof(ota_response), "empty response");
			break;
		}

		// check for update OK
		ota_task_status = OST_CHECK_FINISHED_ERROR;

		ESP_LOGI(__func__, "response is '%s'", ota_response);

		char *s, *p, *t, *l, *h , *v;
		p = s = strdup(ota_response);
		h = v = NULL;
		for (t = strtok_r(p, "\n", &l); t; t = strtok_r(NULL, "\n", &l)) {
			ESP_LOGI(__func__,"check '%s'", t?t:"");
			if ( strstr(t, "V=") == t ) {
				// like this
				// V=1.01
				char *vs, *vp, *vl;
				vs = vp = strdup(t);
				strtok_r(vp, "=", &vl); // ignore "V="
				v = strdup(strtok_r(NULL, " ", &vl)); // read version string
				free(vs);
			} else if ( strstr(t, "H=") == t ) {
				// like this:
				// H=fa7414c503331abe22d2e4ccbf93ebdc2caba1d48687076e267db4dab8fc6749  esp32_ws2812-Application.bin
				char *hs, *hp, *hl;
				hs = hp = strdup(t);
				strtok_r(hp, "=", &hl); // ignore "H="
				h = strdup(strtok_r(NULL, " ", &hl)); // read sha256 hash
				free(hs);
			}
		}
		if ( h && v ) {
			if ( ! strcasecmp(h, sha256_hash_run_partition)) {
				snprintf(msg, sizeof(msg), "no update available");
				ota_task_status = OST_CHECK_FINISHED_UPTODATE;

			} else {
				// check version number
				const esp_app_desc_t *app_desc = esp_app_get_description();
				int actual_version = extract_number(app_desc->version);
				int new_version = extract_number(v);
				ESP_LOGI(__func__, "version: act (%s)%d, new (%s)%d", app_desc->version, actual_version, v, new_version);
				if ( new_version > actual_version) {
					snprintf(msg, sizeof(msg),"new version %s available, actual version %s",
							v, app_desc->version);
					ota_task_status = OST_CHECK_FINISHED_UPDATE_NEEDED;

				} else if (new_version == actual_version){
					snprintf(msg, sizeof(msg),"there's no new version, but binary differs, actual version %s",
							v, app_desc->version);
					ota_task_status = OST_CHECK_FINISHED_UPDATE_OPTIONAL;
				} else {
					snprintf(msg, sizeof(msg),"strange, new version %s is lower than actual version %s",
							v, app_desc->version);
					ota_task_status = OST_CHECK_FINISHED_UPDATE_OPTIONAL;
				}
				//httpd_resp_sendstr_chunk(req, msg);
			}
		} else {
			snprintf(msg, sizeof(msg), "missing version data");
		}
		if(h)
			free(h);
		if(v)
			free(v);

		free(s);
		rc = ESP_OK;

	} while(0);

	if ( rc != ESP_OK ) {
		log_err(__func__, "%s", ota_response);
		snprintfapp(ota_response, sizeof(ota_response),"\n");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, ota_response);
	} else {
		get_handler_ota_status(req, msg);
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

    vTaskDelete(NULL);
    // no statements here, task deleted
}

esp_err_t get_handler_ota_update(httpd_req_t *req) {

	char msg[64];
	memset(msg, 0, sizeof(msg));

	bool doit = false;
	switch (ota_task_status) {
	case OST_IDLE: // check needed
		snprintf(msg, sizeof(msg), "update check needed");
		break;
	case OST_CHECK:
	case OST_CHECK_FINISHED:
		snprintf(msg, sizeof(msg), "busy, update check is running");
		break;
	case OST_CHECK_FINISHED_UPTODATE:
		snprintf(msg, sizeof(msg), "application is uptodate");
		break;
	case OST_UPDATE:
		snprintf(msg, sizeof(msg), "update processs busy");
		break;
	case OST_UPDATE_FAILED:
		snprintf(msg, sizeof(msg), "update processs failed");
		break;
	case OST_UPDATE_FINISHED:
		snprintf(msg, sizeof(msg), "update processs finished, reboot needed");
		break;
	default:
		doit = true;
	}

	if ( doit ) {
		// doesn't run, start it
		ota_task_status = OST_UPDATE;
	    t_task_start = esp_timer_get_time();
	    t_task_end = 0;
		log_info(__func__, "start update");

		const uint32_t sz_stack = 8192;
		xTaskCreate(&ota_update_main_task, "ota_update", sz_stack, NULL, 0, &xOtaHandle);

		snprintf(msg, sizeof(msg), "firmware update started");
	}

	return get_handler_ota_status(req, msg);
}

esp_err_t get_handler_ota_status(httpd_req_t *req, char *msg) {

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "status", ota_status2text(ota_task_status));
    cJSON_AddNumberToObject(root, "start_time", t_task_start/1000);
    if (ota_is_running(ota_task_status)) {
    	cJSON_AddNumberToObject(root, "end_time", esp_timer_get_time()/1000);
    } else {
    	cJSON_AddNumberToObject(root, "end_time", t_task_end/1000);
    }

	cJSON_AddStringToObject(root, "ota",  strlen(ota_response) ? ota_response : " ");

	if ( msg && strlen(msg)) {
		cJSON_AddStringToObject(root, "msg", msg && strlen(msg) ? msg : " ");
	}

    char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"RESP=%s", resp?resp:"nix");

	httpd_resp_sendstr_chunk(req, resp);
	httpd_resp_sendstr_chunk(req, "\n"); // response more readable

    free((void *)resp);
    cJSON_Delete(root);
    return ESP_OK;
}


