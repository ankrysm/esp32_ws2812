/*
 * process_bmp.c
 *
 *  Created on: Oct 6, 2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"


esp_err_t bmp_open_connection(char *url) {
	esp_err_t res = ESP_OK;

	res = https_get(url, https_callback_bmp_processing);

	return res;
}

esp_err_t bmp_read_data() {
	esp_err_t res = ESP_OK;
	uint8_t *read_buf = NULL;

	if ( !is_https_connection_active()) {
		ESP_LOGE(__func__, "there's no open connnection");
		return ESP_FAIL;
	}

	EventBits_t uxBits=get_ux_bits(0);

	if ( uxBits & BMP_BIT_BUFFER1_HAS_DATA ) {
		read_buf = get_read_buffer(1);

		// TODO


		set_ux_quit_bits(BMP_BIT_BUFFER1_PROCESSED);

	} else if ( uxBits & BMP_BIT_BUFFER2_HAS_DATA ) {
		read_buf = get_read_buffer(2);


		// TODO

		set_ux_quit_bits(BMP_BIT_BUFFER2_PROCESSED);

	} else if (uxBits & BMP_BIT_NO_MORE_DATA ) {
		//
	}
	return res;
}
