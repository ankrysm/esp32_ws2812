/*
 * rest_server_ota.c
 *
 *  Created on: Nov 27, 2022
 *      Author: andreas
 */


#include "esp32_ws2812.h"

extern char *cfg_ota_url;

/**
 * callback for https_get
 *  to do: init, reading or finished
 *  **buf - pointer to put in received data
 *  *buf_len:
 *     out: expected bytes
 *     in: read bytes
 *
 * return:
 *  >0 - ok
 *  <0 - error
 */
int https_callback_ota_check_processing(T_HTTPS_CLIENT_SLOT *slot, uint8_t **buf, uint32_t *buf_len) {
	if (slot->todo == HCT_INIT) {
		ESP_LOGI(__func__,"init");
	} else if (slot->todo == HCT_READING) {
		ESP_LOGI(__func__,"reading");
	} else if ( slot->todo==HCT_FINISH ) {
		ESP_LOGI(__func__,"finished");
	} else if ( slot->todo==HCT_FAILED ) {
		ESP_LOGI(__func__,"failed");
	} else {
		ESP_LOGE( __func__, "unknown todo %d", slot->todo);
	}
	return -1;

}

/**
 * reads the file version.txt from ota server
 */
esp_err_t get_handler_ota_check(httpd_req_t *req) {

	ESP_LOGE(__func__, "NYI");
	return ESP_OK;
}

esp_err_t get_handler_ota_update(httpd_req_t *req) {
	ESP_LOGE(__func__, "NYI");
	return ESP_OK;
}
