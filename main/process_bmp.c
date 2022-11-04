/*
 * process_bmp.c
 *
 *  Created on: Oct 6, 2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

//extern BITMAPINFOHEADER bmpInfoHeader;

static uint32_t read_buffer_len = 0; // data length in read_buffer
static uint8_t *read_buffer = NULL;
static uint32_t rd_mempos = 0;


static volatile bool is_bmp_reading = false;

static int bufno=0; // which buffer has data 1 or 2, depends on HAS_DATA-bit

static uint8_t buf[3*500]; // 3 bits for max. 500 leds TODO with calloc
static int32_t bytes_per_line = -1;

void process_object_bmp(int32_t pos, int32_t len, double brightness) {
	// special handling for bmp processing

	if ( ! get_is_bmp_reading()){
		ESP_LOGE(__func__, "bmp reading not active");
		return;
	}

	t_result res = bmp_work(buf, sizeof(buf), brightness);

	if ( res == RES_OK) {
		if ( bytes_per_line < 0 )
			bytes_per_line = get_bytes_per_line();
		led_strip_memcpy(pos, buf, MIN(bytes_per_line, (3*len)));

	} else if ( res == RES_FINISHED ) {
		ESP_LOGI(__func__,"bmp_read_data: all lines read, connection closed");

	} else {
		ESP_LOGW(__func__, "unexpected result %d",res);
		// GRB
		uint8_t r,g,b;
		r = 32; g=0; b=0;
		for (int i=0; i<3*len;) {
			// GRB
			buf[i++] = g;  buf[i++] = r; buf[i++] = b;
		}
		led_strip_memcpy(pos, buf, 3*len);
	}
}


static void bmp_data_reset() {
	read_buffer_len = 0;
	read_buffer = NULL;
	rd_mempos = 0;
	bufno = 0;
}


/**
 * function copies one pixel line from bmp in BGR/BGRx-format into
 * the buffer buf in a GRB-Format
 *
 * maximum bytes are sz_buf
 * brightness between 0 an 1.0
 *
 * returns bytes per line
 */
static size_t bmp_show_data(uint8_t *buf, size_t sz_buf, double brightness ) {
	// 24 bit data: B,G,R
	// 32 bit data: B,G,R,x

	uint8_t bgr[4];

	uint32_t bytes_per_pixel = get_bytes_per_pixel();
	uint32_t bytes_per_line = get_bytes_per_line(); //bgr_idx_max * bmpInfoHeader.biWidth;

	uint32_t wr_mempos = 0;
	uint32_t wr_max_mempos = MAX(0,sz_buf-3); // -3 because check once before write

	int bgr_idx = 0;

	for ( int i=0; i < bytes_per_line; i++ ) {

		bgr[bgr_idx++] = read_buffer[rd_mempos++] * brightness;
		if ( bgr_idx < bytes_per_pixel)
			continue;

		bgr_idx = 0;
		uint8_t r=0,g=0,b=0;
		switch(bytes_per_pixel) {
		case 1:
			r = g = b = bgr[0]; // TODO not really this way, use as grayscale
			break;
		case 2: // 565 schema BBBBBGGG GGGRRRRR
			b = bgr[0] & 0xF1;
			g = ((bgr[0] & 0x07) << 5) | (bgr[1] >> 5);
			r = bgr[1] & 0xF1;
			break;
		case 3: // BGR
			b = bgr[0];
			g = bgr[1];
			r = bgr[2];
			break;
		case 4: // BGRx
			b = bgr[0];
			g = bgr[1];
			r = bgr[2];
			break;
		}

		if ( wr_mempos < wr_max_mempos ) {
			buf[wr_mempos++] = g;
			buf[wr_mempos++] = r;
			buf[wr_mempos++] = b;
		}
	}
	return bytes_per_line;
}

/**
 * function checks if more data needed, if true, check the
 * bits provided from the http client task
 *
 * if a buffer is processed, the http client task will be informed by setting a bit
 */
t_result bmp_work(uint8_t *buf, size_t sz_buf, double brightness) {
	memset(buf, 0, sz_buf);
	if (!is_bmp_reading) {
		ESP_LOGW(__func__, "is_bmp_reading not active");
		return RES_FINISHED;
	}

	t_result res =RES_OK;

	if ( read_buffer_len == 0 ) {
		// more data needed
		EventBits_t uxBits=get_ux_bits(10); // wait max. 10 ms
		if ( uxBits & BMP_BIT_BUFFER1_HAS_DATA ) {
			bufno = 1;
		} else if ( uxBits & BMP_BIT_BUFFER2_HAS_DATA ) {
			bufno = 2;
		} else if (uxBits & BMP_BIT_NO_MORE_DATA ) {
			// no more data
			// finished
			ESP_LOGI(__func__, "finished");
			bmp_data_reset();
			is_bmp_reading = false;
			//set_ux_quit_bits(BMP_BIT_FINISH_PROCESSED);
			return RES_FINISHED;
		} else {
			// no new data
			return RES_NO_DATA;
		}

		// new data buffer available
		rd_mempos = 0;
		read_buffer_len = get_read_length();
		read_buffer = get_read_buffer(bufno);
		//ESP_LOGI(__func__, "buffer %d has %d bytes", bufno, read_buffer_len);
	}

	if ( read_buffer_len > 0 ) {
		// has more data
		bmp_show_data(buf, sz_buf, brightness);

		// buffer processed ?
		if ( rd_mempos < read_buffer_len) {
			return RES_OK; // not yet
		}

		// buffer processed
		set_ux_quit_bits(BMP_BIT_BUFFER_PROCESSED);
		bmp_data_reset();
		return ESP_OK;
	}


	return res;
}

bool get_is_bmp_reading() {
	return is_bmp_reading;
}


static esp_err_t bmp_open_connection(char *url) {
	if ( !url || !strlen(url)) {
		ESP_LOGE(__func__, "missing url");
		return ESP_FAIL;
	}

	if (is_bmp_reading ) {
		ESP_LOGE(__func__, "is_bmp_reading should not be true here");
		return ESP_FAIL;
	}

	if ( is_http_client_task_active()) {
		ESP_LOGE(__func__, "is_https_connection_active() should not be true here");
		return ESP_FAIL;
	}

	ESP_LOGI(__func__,"start");

	is_bmp_reading = true;
	esp_err_t res;

	bmp_data_reset();
	clear_ux_bits();

	res = https_get(url, https_callback_bmp_processing);

	return res;
}

t_result bmp_open_url(char *id) {
	T_DISPLAY_OBJECT *obj = find_object4oid(id);
	if  ( !obj ) {
		ESP_LOGE(__func__, "no object '%s' found", id ? id : "");
		return RES_NOT_FOUND;
	}

	T_DISPLAY_OBJECT_DATA *data = obj->data;
	if ( !data ) {
		ESP_LOGE(__func__, "object '%s' has no data", id?id:"");
		return RES_NO_VALUE;
	}

	if ( data->type != OBJT_BMP) {
		ESP_LOGE(__func__, "object '%s' isn't a bmp object", id?id:"");
		return RES_INVALID_DATA_TYPE;
	}

	bytes_per_line = -1;

	if ( bmp_open_connection(data->para.url) != ESP_OK) {
		ESP_LOGE(__func__,"bmp_open_connection FAILED, url='%s'", data->para.url);
		return RES_FAILED;
	}

	ESP_LOGI(__func__,"bmp_open_connection success, url='%s'", data->para.url);
	return RES_OK;
}

void bmp_stop_processing() {
	ESP_LOGI(__func__,"start");
	set_ux_quit_bits(BMP_BIT_STOP_WORKING); // request for stop working
}


