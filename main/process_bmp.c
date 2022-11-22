/*
 * process_bmp.c
 *
 *  Created on: Oct 6, 2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

extern uint32_t extended_log;

static void bmp_data_reset(T_PROCESS_BMP *p_data) {
	p_data->read_buffer_len = 0;
	p_data->read_buffer = NULL;
	p_data->rd_mempos = 0;
	p_data->bufno = 0;
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
static size_t bmp_show_data( T_TRACK_ELEMENT *ele, double brightness ) {
	// 24 bit data: B,G,R
	// 32 bit data: B,G,R,x

	T_BMP_WORKING *w_data = &(ele->w_bmp->w_data);
	T_PROCESS_BMP *p_data = &(ele->w_bmp->p_data);

	uint8_t bgr[4];

	uint32_t bytes_per_pixel = get_bytes_per_pixel(w_data);
	uint32_t bytes_per_line = get_bytes_per_line(w_data);

	if (extended_log) {
		ESP_LOGI(__func__,"bytes per pixel=%d, bytes per line =%d", bytes_per_pixel, bytes_per_line);
	}
	p_data->bytes_per_line = bytes_per_line;

	uint32_t wr_mempos = 0;
	uint32_t wr_max_mempos = MAX(0,sizeof(p_data->buf)-3); // -3 because check mempos before write

	int bgr_idx = 0;

	for ( int i=0; i < bytes_per_line; i++ ) {

		bgr[bgr_idx++] = p_data->read_buffer[(p_data->rd_mempos)++];
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
			if ( g < 5 && r < 5 && b<5 ) {
				g=r=b=0;
			}
			p_data->buf[wr_mempos++] = g * brightness;
			p_data->buf[wr_mempos++] = r * brightness;
			p_data->buf[wr_mempos++] = b * brightness;
		}
	}
	return bytes_per_line;
}

static void check4bmp_errmsg(T_BMP_WORKING *w_data) {
	if ( w_data->has_new_errmsg) {
		log_err(__func__, "open bmp data failed: %s", w_data->errmsg);
		w_data->has_new_errmsg=false;
	}
}

/**
 * function checks if more data needed, if true, check the
 * bits provided from the http client task
 *
 * if a buffer is processed, the http client task will be informed by setting a bit
 */
static t_result bmp_work( T_TRACK_ELEMENT *ele, double brightness) {

	T_BMP_WORKING *w_data = &(ele->w_bmp->w_data );
	T_PROCESS_BMP *p_data = &(ele->w_bmp->p_data);

	memset(p_data->buf, 0,  sizeof(p_data->buf));

	check4bmp_errmsg(w_data);

	t_result res =RES_OK;

	if ( p_data->read_buffer_len == 0 ) {
		// more data needed
		EventBits_t uxBits=get_ux_bits(w_data, 10000); // wait max. 10s

		check4bmp_errmsg(w_data);

		if ( uxBits & BMP_BIT_BUFFER1_HAS_DATA ) {
			p_data->bufno = 1;

		} else if ( uxBits & BMP_BIT_BUFFER2_HAS_DATA ) {
			p_data->bufno = 2;

		} else if (uxBits & BMP_BIT_NO_MORE_DATA ) {
			// no more data
			// finished
			ESP_LOGI(__func__, "finished");
			bmp_data_reset(p_data);
			return RES_FINISHED;
		} else {
			// no new data
			return RES_NO_DATA;
		}

		// new data buffer available
		p_data->rd_mempos = 0;
		p_data->read_buffer_len = get_read_length(w_data);
		p_data->read_buffer = get_read_buffer(w_data, p_data->bufno);
		if ( extended_log) {
			ESP_LOGI(__func__, "buffer %d has %d bytes as BGR", p_data->bufno, p_data->read_buffer_len);
			ESP_LOG_BUFFER_HEXDUMP(__func__, p_data->read_buffer, MIN(192,p_data->read_buffer_len), ESP_LOG_INFO);
		}
	}

	if ( p_data->read_buffer_len > 0 ) {
		check4bmp_errmsg(w_data);

		// has more data
		bmp_show_data(ele, brightness);

		// buffer processed ?
		if ( p_data->rd_mempos < p_data->read_buffer_len) {
			return RES_OK; // not yet
		}

		// buffer processed
		set_ux_quit_bits(w_data, BMP_BIT_BUFFER_PROCESSED);
		bmp_data_reset(p_data);
		return ESP_OK;
	}


	return res;
}

void process_object_bmp(int32_t pos, T_TRACK_ELEMENT *ele, double brightness) {
	// special handling for bmp processing

	if ( ! ele->w_bmp) {
		ESP_LOGE(__func__,"no bmp data");
		return;
	}

	if ( !ele->w_object) {
		ESP_LOGE(__func__,"no object data");
		return;
	}
	LOG_MEM(__func__, 1);

	T_BMP_WORKING *w_data = &(ele->w_bmp->w_data);
	T_PROCESS_BMP *p_data = &(ele->w_bmp->p_data);

	if ( w_data->bmp_read_phase == BRP_IDLE){
		ESP_LOGE(__func__, "bmp reading not active");
		return;
	}

	t_result res = bmp_work(ele,  brightness);


	if ( res == RES_OK) {
		size_t len=MIN(p_data->bytes_per_line, 3*ele->w_object->data->len);
		if ( extended_log) {
			ESP_LOGI(__func__, "id=%d, line=%d, buffer as GRB", ele->id, p_data->line_count);
			ESP_LOG_BUFFER_HEXDUMP(__func__, p_data->buf, len, ESP_LOG_INFO);
		}
		led_strip_memcpy(pos, p_data->buf, len);
		memcpy(p_data->last_buf, p_data->buf, sizeof(p_data->last_buf));

	} else if ( res == RES_FINISHED ) {
		ESP_LOGI(__func__,"bmp_read_data: all lines read, connection closed");
		bmp_free_buffers(w_data);
		LOG_MEM(__func__, 2);

	} else {
		(p_data->missing_lines_count)++;
		ESP_LOGW(__func__, "unexpected result %d, bmp_cnt=%d, bmp_missing_lines=%d", res, p_data->line_count, p_data->missing_lines_count);

		/*
		// GRB
		uint8_t r,g,b;
		r = 32; g=0; b=0;
		for (int i=0; i<3*len;) {
			// GRB
			buf[i++] = g;  buf[i++] = r; buf[i++] = b;
		}
		// */
		led_strip_memcpy(pos, p_data->last_buf, 3*ele->w_object->data->len);
		LOG_MEM(__func__, 3);
	}
	(p_data->line_count)++; //bmp_cnt++;
}



t_result get_is_bmp_reading(T_TRACK_ELEMENT *ele) {

	if ( !ele->w_object) {
		log_err(__func__, "element %d has no object", ele->id);
		return RES_FAILED;
	}
	T_W_BMP *data = ele->w_bmp;
	if ( !data ) {
		log_err(__func__, "object '%d' has no data", ele->id);
		return RES_FAILED;
	}

	T_BMP_WORKING *w_data = &(data->w_data);

	return w_data->bmp_read_phase == BRP_IDLE ? RES_FINISHED : RES_OK;
}

t_result bmp_open_url(T_TRACK_ELEMENT *ele) {

	if ( !ele->w_object) {
		log_err(__func__, "element %d has no w_object", ele->id);
		return RES_FAILED;
	}

	T_DISPLAY_OBJECT_DATA *data = ele->w_object->data;
	if ( !data ) {
		log_err(__func__, "element %d has no w_object-data", ele->id);
		return RES_NO_VALUE;
	}

	if ( data->type != OBJT_BMP) {
		log_err(__func__, "element %d: the object isn't a bmp object", ele->id);
		return RES_INVALID_DATA_TYPE;
	}

	if ( !data->para.bmp.url || !strlen(data->para.bmp.url)) {
		log_err(__func__, "element %d: bmp object has no url", ele->id);
		return ESP_FAIL;
	}

	LOG_MEM(__func__, 1);
	// normally there's no w_bmp data
	if ( ele->w_bmp) {
		if ( ele->w_bmp->w_data.bmp_read_phase != BRP_IDLE) {
			log_err(__func__, "element %d_ bmp object already in use", ele->id);
			return RES_FAILED;
		}
		memset(ele->w_bmp, 0, sizeof(T_W_BMP));
	} else {
		// create new bmp data
		ele->w_bmp = calloc(1, sizeof(T_W_BMP));
		ESP_LOGI(__func__, "calloc %lu bytes",sizeof(T_W_BMP));
	}
	LOG_MEM(__func__, 2);
	// initialise data for bmp handling
	bmp_data_reset(&(ele->w_bmp->p_data));
	memset(ele->w_bmp->p_data.buf, 0, sizeof(ele->w_bmp->p_data.buf));
	memset(ele->w_bmp->p_data.last_buf, 0, sizeof(ele->w_bmp->p_data.last_buf));
	ele->w_bmp->p_data.line_count = ele->w_bmp->p_data.missing_lines_count = 0;

	// init data for web client
	bmp_init_data(&(ele->w_bmp->w_data));
	// not necesssary: clear_ux_bits(ele->w_bmp->w_data);
	LOG_MEM(__func__, 3);

	https_get(data->para.bmp.url, https_callback_bmp_processing, (void*)&(ele->w_bmp->w_data));

	ESP_LOGI(__func__,"bmp_open_connection success, url='%s'", data->para.bmp.url);
	return RES_OK;
}

void bmp_stop_processing(T_TRACK_ELEMENT *ele) {
	ESP_LOGI(__func__,"start");
	//is_bmp_reading = false;
	if ( ele->w_bmp) {
		set_ux_quit_bits(&(ele->w_bmp->w_data), BMP_BIT_STOP_WORKING); // request for stop working
	} else {
		ESP_LOGW(__func__, "already stopped");
	}
	LOG_MEM(__func__, 1);

}


