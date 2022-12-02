/**
 * HTTP Restful API Server - POST services
 * based on esp-idf examples
 */

#include "esp32_ws2812.h"

/**
 * decodes JSON content and stores data in memory.
 * scenes will be stopped and data will be overwritten
 */
esp_err_t post_handler_load(httpd_req_t *req, char *content) {
	char msg[255];
	memset(msg, 0, sizeof(msg));

	// stop display, clear data
	run_status_type new_status = RUN_STATUS_STOPPED;
	esp_err_t res = clear_data(msg, sizeof(msg), new_status);
	if ( res != ESP_OK ) {
		log_warn(__func__, "%s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
        return ESP_FAIL;
	}

	if ( decode_json4event_root(content, msg, sizeof(msg)) != ESP_OK) {
		log_warn(__func__, "%s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
        return ESP_FAIL;
	}

	ESP_LOGI(__func__, "ended with '%s'", msg);
    httpd_resp_set_type(req, "plain/text");
	snprintf(msg,sizeof(msg), "data load done");
	httpd_resp_sendstr_chunk(req, msg);

	return res;

}

/**
 * stores content in flash file
 */
esp_err_t post_handler_file_store(httpd_req_t *req, char *content, char *fname, size_t sz_fname) {
	char msg[255];
	memset(msg, 0, sizeof(msg));


	if ( store_events_to_file(fname, content, msg, sizeof(msg))!= ESP_OK) {
		log_err(__func__, "%s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
        return ESP_FAIL;
	}

	snprintf(msg,sizeof(msg), "content saved to %s",fname);
	log_info(__func__, "success: %s", msg);

	// successful
    httpd_resp_set_type(req, "plain/text");
    char resp_txt[256];
	snprintf(resp_txt,sizeof(resp_txt), "content saved to %s",fname);
	snprintfapp(msg,sizeof(msg), "Response: '%s'",resp_txt);
	httpd_resp_sendstr_chunk(req, resp_txt);

	return ESP_OK;
}


/**
 * load data into memory
 * first stop scene
 */
 esp_err_t post_handler_config_set(httpd_req_t *req, char *buf) {
	char msg[255];
	memset(msg, 0, sizeof(msg));

	// stop display for newe config
	run_status_type new_status = RUN_STATUS_STOPPED;
	set_scene_status(new_status);

	char errmsg[64];
	esp_err_t res = decode_json4config_root(buf, errmsg, sizeof(errmsg));

	if (res == ESP_OK) {
		snprintf(msg, sizeof(msg), "decoding data done: %s", errmsg);
		log_info(__func__, "%s", msg);
	} else {
		snprintf(msg, sizeof(msg), "decoding data failed: %s", errmsg);
		log_err(__func__, "%s", msg);
		snprintfapp(msg, sizeof(msg),"\n");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
		return ESP_FAIL;
	}

	get_handler_config(req, msg);

	return ESP_OK;

}

