/*
 * process_bmp.c
 *
 *  Created on: Oct 6, 2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

extern BITMAPINFOHEADER bmpInfoHeader;

static uint32_t read_buffer_pos = 0; // current position in read_buffer
static uint32_t read_buffer_len = 0; // data length in read_buffer
static uint8_t *read_buffer = NULL;

static bool is_bmp_reading = false;

static int bufno=0; // which buffer has data 1 or 2, depends on HAS_DATA-bit

static void bmp_data_reset() {
	read_buffer_pos = 0;
	read_buffer_len = 0;
	read_buffer = NULL;
	bufno=0;
}

static void bmp_show_data(int pos, T_COLOR_RGB *rgb ) {
	// 24 bit data: B,G,R
	// 32 bit data: B,G,R,x

	is_bmp_reading = true;

	uint8_t bgr[4];

	int bgr_idx_max = 1;
	if ( bmpInfoHeader.biBitCount == 16) {
		bgr_idx_max = 2;
		ESP_LOGE(__func__, "biBitCount %d NYI",bmpInfoHeader.biBitCount);
		return;
	} else if ( bmpInfoHeader.biBitCount == 24 ) {
		bgr_idx_max = 3;
	} else if (bmpInfoHeader.biBitCount == 32) {
		bgr_idx_max = 4;
	} else {
		ESP_LOGE(__func__, "biBitCount %d NYI",bmpInfoHeader.biBitCount);
		return;
	}

	for ( int bgr_idx = 0; bgr_idx < bgr_idx_max && read_buffer_pos < read_buffer_len; bgr_idx++) {
		bgr[bgr_idx] = read_buffer[read_buffer_pos];
		read_buffer_pos++;
	}


	if ( bmpInfoHeader.biBitCount == 16) {
		// NYI
		rgb->r = 32;
		rgb->g = 0;
		rgb->b = 0;
	} else if ( bmpInfoHeader.biBitCount == 24 ) {
		rgb->r = bgr[2];
		rgb->g = bgr[1];
		rgb->b = bgr[0];
	} else if (bmpInfoHeader.biBitCount == 32) {
		rgb->r = bgr[2];
		rgb->g = bgr[1];
		rgb->b = bgr[0];
	}
 }

esp_err_t bmp_open_connection(char *url) {
	if (is_bmp_reading ) {
		ESP_LOGE(__func__, "is_bmp_reading should not be true here");
		return ESP_FAIL;
	}

	if ( is_https_connection_active()) {
		ESP_LOGE(__func__, "is_https_connection_active() should not be true here");
		return ESP_FAIL;
	}

	ESP_LOGI(__func__,"start");

	is_bmp_reading = true;
	esp_err_t res = ESP_OK;

	res = https_get(url, https_callback_bmp_processing);

	return res;
}

void bmp_stop_processing() {
	ESP_LOGI(__func__,"start");
	set_ux_quit_bits(BMP_BIT_STOP_WORKING);
}

t_result bmp_read_data(int pos, T_COLOR_RGB *rgb) {
	t_result res = RES_OK;

	if ( !is_https_connection_active()) {
		ESP_LOGE(__func__, "there's no open connnection");
		is_bmp_reading = false;
		rgb->r = 32;
		rgb->g = 0;
		rgb->b = 0;
		return RES_FAILED;
	}

	if ( pos >= bmpInfoHeader.biWidth ) {
		rgb->r = 0;
		rgb->g = 0;
		rgb->b = 0;
		return RES_OK;
	}

	// if both values are 0 it is in init, continue with wait for data
	if ( read_buffer_pos < read_buffer_len ) {
		// buffer has more data, show stored buffer data
		bmp_show_data(pos, rgb);

		if ( read_buffer_pos >= read_buffer_len ) {
			// processing buffer finished
			switch(bufno) {
			case 0: break; // may be during init
			case 1: set_ux_quit_bits(BMP_BIT_BUFFER1_PROCESSED); break;
			case 2: set_ux_quit_bits(BMP_BIT_BUFFER2_PROCESSED); break;
			default:
				ESP_LOGE(__func__, "unexpected bufno %d", bufno);
				return ESP_FAIL;
			}
			bmp_data_reset();
		}
		return ESP_OK;
	}

	// wait for new data

	EventBits_t uxBits=get_ux_bits(0);

	if ( uxBits & BMP_BIT_BUFFER1_HAS_DATA ) {
		bufno = 1;
		read_buffer_pos = 0;
		read_buffer_len = get_read_length();
		read_buffer = get_read_buffer(bufno);
		ESP_LOGI(__func__, "buffer %d has data %d", bufno, read_buffer_len);
		//bytes_per_line = get_bytes_per_bmp_line();

		// maybe the buffer has only one pixel line then he is finished here
		bmp_show_data(pos, rgb);
		if ( read_buffer_pos >= read_buffer_len ) {
			bmp_data_reset();
			set_ux_quit_bits(BMP_BIT_BUFFER1_PROCESSED);
		}

	} else if ( uxBits & BMP_BIT_BUFFER2_HAS_DATA ) {
		bufno = 2;
		read_buffer_pos = 0;
		read_buffer_len = get_read_length();
		read_buffer = get_read_buffer(bufno);
		ESP_LOGI(__func__, "buffer %d has data %d", bufno, read_buffer_len);
		//bytes_per_line = get_bytes_per_bmp_line();

		bmp_show_data(pos, rgb);
		if ( read_buffer_pos >= read_buffer_len ) {
			bmp_data_reset();
			set_ux_quit_bits(BMP_BIT_BUFFER2_PROCESSED);
		}

	} else if (uxBits & BMP_BIT_NO_MORE_DATA ) {
		// finished
		ESP_LOGI(__func__, "finished");
		bmp_data_reset();
		is_bmp_reading = false;
		set_ux_quit_bits(BMP_BIT_FINISH_PROCESSED);
		res = RES_FINISHED;
	}
	return res;
}

bool get_is_bmp_reading() {
	return is_bmp_reading;
}
