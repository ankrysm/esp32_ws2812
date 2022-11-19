/*
 * process_bmp.c
 *
 *  Created on: Oct 6, 2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

//#define LEN_RD_BUF 3*1000 // 3 bytes for n leds

/*
static uint32_t read_buffer_len = 0; // data length in read_buffer
static uint8_t *read_buffer = NULL;
static uint32_t rd_mempos = 0;


//static volatile bool is_bmp_reading = false;

static int bufno=0; // which buffer has data 1 or 2, depends on HAS_DATA-bit

static uint8_t buf[LEN_RD_BUF]; // TODO with calloc
static uint8_t last_buf[LEN_RD_BUF]; // TODO with calloc
static int32_t bytes_per_line = -1;

static uint32_t bmp_cnt=0;
static uint32_t bmp_missing_lines=0;
*/

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
static size_t bmp_show_data( T_TRACK_ELEMENT *ele, /*uint8_t *buf, size_t sz_buf,*/ double brightness ) {
	// 24 bit data: B,G,R
	// 32 bit data: B,G,R,x

	T_BMP_WORKING *w_data = &(ele->w_bmp->w_data);
	T_PROCESS_BMP *p_data = &(ele->w_bmp->p_data);

	uint8_t bgr[4];

	uint32_t bytes_per_pixel = get_bytes_per_pixel(w_data);
	uint32_t bytes_per_line = get_bytes_per_line(w_data); //bgr_idx_max * bmpInfoHeader.biWidth;

	uint32_t wr_mempos = 0;
	uint32_t wr_max_mempos = MAX(0,sizeof(p_data->buf)-3); // -3 because check once before write

	int bgr_idx = 0;

	for ( int i=0; i < bytes_per_line; i++ ) {

		bgr[bgr_idx++] = p_data->read_buffer[p_data->rd_mempos++] * brightness;
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
			p_data->buf[wr_mempos++] = g;
			p_data->buf[wr_mempos++] = r;
			p_data->buf[wr_mempos++] = b;
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
static t_result bmp_work( T_TRACK_ELEMENT *ele, /*uint8_t *buf, size_t sz_buf,*/ double brightness) {

	T_BMP_WORKING *w_data = &(ele->w_bmp->w_data );
	T_PROCESS_BMP *p_data = &(ele->w_bmp->p_data);

	//p_data->buf, sizeof(p_data->buf)
	memset(p_data->buf, 0,  sizeof(p_data->buf));
//	if (!is_bmp_reading) {
//		ESP_LOGW(__func__, "is_bmp_reading not active");
//		return RES_FINISHED;
//	}

	t_result res =RES_OK;

	if ( p_data->read_buffer_len == 0 ) {
		// more data needed
		EventBits_t uxBits=get_ux_bits(w_data, 1000); // wait max. 1s
		if ( uxBits & BMP_BIT_BUFFER1_HAS_DATA ) {
			p_data->bufno = 1;

		} else if ( uxBits & BMP_BIT_BUFFER2_HAS_DATA ) {
			p_data->bufno = 2;

		} else if (uxBits & BMP_BIT_NO_MORE_DATA ) {
			// no more data
			// finished
			ESP_LOGI(__func__, "finished");
			bmp_data_reset(p_data);
			//is_bmp_reading = false;
			//set_ux_quit_bits(BMP_BIT_FINISH_PROCESSED);
			return RES_FINISHED;
		} else {
			// no new data
			return RES_NO_DATA;
		}

		// new data buffer available
		p_data->rd_mempos = 0;
		p_data->read_buffer_len = get_read_length(w_data);
		p_data->read_buffer = get_read_buffer(w_data, p_data->bufno);
		ESP_LOGI(__func__, "buffer %d has %d bytes", p_data->bufno, p_data->read_buffer_len);
	}

	if ( p_data->read_buffer_len > 0 ) {
		// has more data
		bmp_show_data(ele, /*p_data->buf, sizeof(p_data->buf),*/ brightness);

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

	T_BMP_WORKING *w_data = &(ele->w_bmp->w_data);
	T_PROCESS_BMP *p_data = &(ele->w_bmp->p_data);

	if ( w_data->bmp_read_phase == BRP_IDLE){
		ESP_LOGE(__func__, "bmp reading not active");
		return;
	}

	t_result res = bmp_work(ele, /*obj_data, p_data->buf, sizeof(p_data->buf),*/ brightness);

	(p_data->line_count)++; //bmp_cnt++;

	if ( res == RES_OK) {
		if ( p_data->bytes_per_line < 0 )
			p_data->bytes_per_line = get_bytes_per_line(w_data);
		led_strip_memcpy(pos, p_data->buf, MIN(p_data->bytes_per_line, 3*ele->w_object->data->len));
		memcpy(p_data->last_buf, p_data->buf, sizeof(p_data->last_buf));

	} else if ( res == RES_FINISHED ) {
		ESP_LOGI(__func__,"bmp_read_data: all lines read, connection closed");

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
	}
}



t_result get_is_bmp_reading(T_TRACK_ELEMENT *ele) {

//	T_DISPLAY_OBJECT *obj = find_object4oid(id);
//	if  ( !obj ) {
//		log_err(__func__, "no object '%s' found", id ? id : "");
//		return false;
//	}
//
	if ( !ele->w_object) {
		log_err(__func__, "element %d has no object", ele->id);
		return RES_FAILED;
	}
	T_W_BMP *data = ele->w_bmp;
	if ( !data ) {
		log_err(__func__, "object '%d' has no data", ele->id);
		return RES_FAILED;
	}
//
//	if ( data->type != OBJT_BMP) {
//		log_err(__func__, "object '%s' isn't a bmp object", id?id:"");
//		return false;
//	}

	T_BMP_WORKING *w_data = &(data->w_data); // get_bmp_slot();

	return w_data->bmp_read_phase == BRP_IDLE ? RES_FINISHED : RES_OK;
}

/*
static esp_err_t bmp_open_connection(char *url) {
	if ( !url || !strlen(url)) {
		ESP_LOGE(__func__, "missing url");
		return ESP_FAIL;
	}

	if (is_bmp_reading ) {
		ESP_LOGE(__func__, "is_bmp_reading should not be true here");
		return ESP_FAIL;
	}

//	if ( is_http_client_task_active()) {
//		ESP_LOGE(__func__, "is_https_connection_active() should not be true here");
//		return ESP_FAIL;
//	}

	ESP_LOGI(__func__,"start");

	is_bmp_reading = true;
	esp_err_t res;

	bmp_data_reset();
	clear_ux_bits();
	memset(buf, 0, sizeof(buf));
	memset(last_buf, 0, sizeof(last_buf));

	T_HTTPS_CLIENT_SLOT *slot = https_get(url, https_callback_bmp_processing);

	return res;
}
*/

t_result bmp_open_url(T_TRACK_ELEMENT *ele) {
//	T_DISPLAY_OBJECT *obj = find_object4oid(ele->w_object);
//	if  ( !obj ) {
//		log_err(__func__, "no object '%s' found", ele->w_object_oid ? ele->w_object_oid : "");
//		return RES_NOT_FOUND;
//	}
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

	// normally there's no w_bmp data
	if ( ele->w_bmp) {
		if ( ele->w_bmp->w_data.bmp_read_phase != BRP_IDLE) {
			log_err(__func__, "element %d_ bmp object already in use", ele->id);
			return RES_FAILED;
		}
		free(ele->w_bmp);
		ele->w_bmp = NULL;
	}

	// create new bmp data
	ele->w_bmp = calloc(1, sizeof(T_W_BMP));

	// T_BMP_WORKING *bmp_data = &(data->para.bmp.w_data); // get_bmp_slot();
//	if ( bmp_open_connection(data->para.url) != ESP_OK) {
//		ESP_LOGE(__func__,"bmp_open_connection FAILED, url='%s'", data->para.url);
//		return RES_FAILED;
//	}

	// initialise data for bmp handling
	bmp_data_reset(&(ele->w_bmp->p_data));
	memset(ele->w_bmp->p_data.buf, 0, sizeof(ele->w_bmp->p_data.buf));
	memset(ele->w_bmp->p_data.last_buf, 0, sizeof(ele->w_bmp->p_data.last_buf));
	ele->w_bmp->p_data.line_count = ele->w_bmp->p_data.missing_lines_count = 0;

	// init data for web client
	bmp_init_data(&(ele->w_bmp->w_data));
	// not necesssary: clear_ux_bits(ele->w_bmp->w_data);

	//T_HTTPS_CLIENT_SLOT *slot =
	https_get(data->para.bmp.url, https_callback_bmp_processing, (void*)&(ele->w_bmp->w_data));

	ESP_LOGI(__func__,"bmp_open_connection success, url='%s'", data->para.bmp.url);
	return RES_OK;
}

void bmp_stop_processing(T_TRACK_ELEMENT *ele) {
	ESP_LOGI(__func__,"start");
	//is_bmp_reading = false;
	set_ux_quit_bits(&(ele->w_bmp->w_data), BMP_BIT_STOP_WORKING); // request for stop working
}


