/*
 * bmp.c
 *
 *  Created on: Sep 27, 2022
 *      Author: andreas
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include "sdkconfig.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "bmp.h"
#include "https_get.h"

#define SZ_BUFFER 8192

static T_BMP_WORKING bmp_working_data[N_HTTPS_CLIENTS];

/* FreeRTOS event group to signal when something is read or something is processed */
/*
static EventGroupHandle_t s_bmp_event_group_for_reader; // read process (it's me) waits for ...PROCESSED or STOP_WORKING
static EventGroupHandle_t s_bmp_event_group_for_worker; // worker process waits for ...HAS_DATA, ...NO_MORE_DATA

BITMAPFILEHEADER bmpFileHeader;
BITMAPINFOHEADER bmpInfoHeader;
static t_bmp_read_phase bmp_read_phase = BRP_GOT_FILE_HEADER;

static uint32_t buf_len_expected = 0;
static uint32_t total_length = 0;

static uint8_t *read_buffer1 = NULL;
static uint8_t *read_buffer2 = NULL;


// data amount
static uint32_t read_buffer_length = 0;

static uint8_t *buffer = NULL;

size_t sz_read_buffer = 0;
uint32_t lines_read_buffer = 0;
*/

static const TickType_t xTicksToWait = 30000 / portTICK_PERIOD_MS;
static int extended_log = 0;


void bmp_init() {
	ESP_LOGI(__func__, "started");
	for(int i=0; i< N_HTTPS_CLIENTS; i++) {
		T_BMP_WORKING *data = &(bmp_working_data[i]);
		memset(data, 0, sizeof(T_BMP_WORKING));
		data->s_bmp_event_group_for_reader = xEventGroupCreate();
		data->s_bmp_event_group_for_worker = xEventGroupCreate();
		xEventGroupClearBits(data->s_bmp_event_group_for_reader, 0xFFFF);
		xEventGroupClearBits(data->s_bmp_event_group_for_worker, 0xFFFF);
	}
}

uint32_t get_bytes_per_pixel(T_BMP_WORKING *data) {
	uint32_t bytes_per_pixel = 1;
	if ( data->bmpInfoHeader.biBitCount == 16) {
		bytes_per_pixel = 2;
	} else if ( data->bmpInfoHeader.biBitCount == 24 ) {
		bytes_per_pixel = 3;
	} else if (data->bmpInfoHeader.biBitCount == 32) {
		bytes_per_pixel = 4;
	} else {
		ESP_LOGE(__func__, "biBitCount %d NYI", data->bmpInfoHeader.biBitCount);
	}
	return bytes_per_pixel;
}

uint32_t get_bytes_per_line(T_BMP_WORKING *data) {
	return data->bmpInfoHeader.biWidth * get_bytes_per_pixel(data);
}


static void free_buffers(T_BMP_WORKING *data) {
	if (data->read_buffer1) {
		free(data->read_buffer1);
		data->read_buffer1 = NULL;
	}
	if (data->read_buffer2) {
		free(data->read_buffer2);
		data->read_buffer2 = NULL;
	}
	if ( data->buffer) {
		free(data->buffer);
		data->buffer=NULL;
	}
	data->sz_read_buffer = 0;
	data->total_length = 0;

}
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
int https_callback_bmp_processing(T_HTTPS_CLIENT_SLOT *slot, uint8_t **buf, uint32_t *buf_len) {

	//ESP_LOGI(__func__,"todo=%d, phase=%d(%s), buf_len=%u", to_do, bmp_read_phase, BRP2TXT(bmp_read_phase), *buf_len);

	T_BMP_WORKING *data = (T_BMP_WORKING *) slot->user_args;
	EventBits_t uxBits;

	if (slot->todo == HCT_INIT) {
		// init
		memset(&(data->bmpFileHeader), 0, sizeof(data->bmpFileHeader));
		memset(&(data->bmpInfoHeader), 0, sizeof(data->bmpInfoHeader));
		free_buffers(data);

		data->bmp_read_phase = BRP_GOT_FILE_HEADER;
		*buf_len = data->buf_len_expected = sizeof(data->bmpFileHeader);
		*buf = (uint8_t *) &(data->bmpFileHeader);
		ESP_LOGI(__func__,"todo=%d(init), phase=%d(%s), buf_len=%u", slot->todo, data->bmp_read_phase, BRP2TXT(data->bmp_read_phase), *buf_len);

		return 1;

	} else if (slot->todo == HCT_READING) {
		// gets and clears the bit
		uxBits = xEventGroupClearBits(data->s_bmp_event_group_for_reader, BMP_BIT_STOP_WORKING);
		//if ( uxBits)
		//	ESP_LOGI(__func__, "uxBits = 0x%X", uxBits);

		// read data
		if ( data->buf_len_expected > 0 &&  *buf_len > data->buf_len_expected) {
			ESP_LOGW(__func__,"buf_len_received(%ld) > buf_len_expected(%ld)", *buf_len, data->buf_len_expected);
			return -1;
		}

		switch(data->bmp_read_phase) {

		case BRP_GOT_FILE_HEADER: // got file header
			ESP_LOGI(__func__, "got file header: bfType='%c%c', bfSize=0x%08x, bfOffBits=0x%08x",
					data->bmpFileHeader.bfType1, data->bmpFileHeader.bfType2,
					data->bmpFileHeader.bfSize, data->bmpFileHeader.bfOffBits);
			ESP_LOG_BUFFER_HEXDUMP(__func__, &(data->bmpFileHeader), sizeof(data->bmpFileHeader), ESP_LOG_INFO);

			if ( data->bmpFileHeader.bfType1 != 'B' || data->bmpFileHeader.bfType2 != 'M') {
				ESP_LOGE(__func__, "is not a bitmap file, first bytes have to be 'BM'");
				return -1;
			}

			data->total_length += *buf_len;
			data->bmp_read_phase = BRP_GOT_INFO_HEADER;

			*buf_len = data->buf_len_expected = sizeof(data->bmpInfoHeader);
			*buf = (uint8_t *) &(data->bmpInfoHeader);
			//ESP_LOGI(__func__,"todo=%d, phase=%d(%s), buf_len=%u", to_do, bmp_read_phase, BRP2TXT(bmp_read_phase), *buf_len);
			return 1;

		case BRP_GOT_INFO_HEADER:
			ESP_LOGI(__func__,"got info header: biSize=%u, biWidth=%ld, biHeight%ld, biPlanes=%d, biBitCount=%d, biCompression=%u, biSizeImage=%u, biClrUsed=%u, biClrImportant=%u",
					data->bmpInfoHeader.biSize, data->bmpInfoHeader.biWidth, data->bmpInfoHeader.biHeight, data->bmpInfoHeader.biPlanes,
					data->bmpInfoHeader.biBitCount, data->bmpInfoHeader.biCompression, data->bmpInfoHeader.biSizeImage, data->bmpInfoHeader.biClrUsed, data->bmpInfoHeader.biClrImportant);

			ESP_LOG_BUFFER_HEXDUMP(__func__, &(data->bmpInfoHeader), sizeof(data->bmpInfoHeader), ESP_LOG_INFO);
			if ( data->bmpInfoHeader.biCompression != 0 ) {
				snprintf(slot->errmsg, sizeof(slot->errmsg), "unsupported compression method"); // TODO: implement it
				ESP_LOGE(__func__, "%s", slot->errmsg);
				return -1;
			}
			if ( data->bmpInfoHeader.biBitCount != 24 && data->bmpInfoHeader.biBitCount != 32) {
				snprintf(slot->errmsg, sizeof(slot->errmsg), "24-bit or 32-bit pixel expected");
				ESP_LOGE(__func__, "%s", slot->errmsg);
				return -1;
			}
			if ( data->bmpInfoHeader.biWidth > 500 ) {
				snprintf(slot->errmsg, sizeof(slot->errmsg), "bitmap width greater 500");
				ESP_LOGE(__func__, "%s", slot->errmsg);
				return -1;
			}

			data->total_length += *buf_len;

			// prepare data read
			// data len as multiple amount of data per line
			size_t linesize =get_bytes_per_line(data);
			data->sz_read_buffer = linesize;
			while (data->sz_read_buffer < SZ_BUFFER ) {
				data->sz_read_buffer += linesize;
			}
			ESP_LOGI(__func__, "sz_read_buffer = %d",data->sz_read_buffer);
			if ( !(data->read_buffer1 = calloc(data->sz_read_buffer, sizeof(char)))) {
				ESP_LOGE(__func__, "could not allocate %u bytes for read buffer1", data->sz_read_buffer);
				return -1;
			}

			if ( !(data->read_buffer2 = calloc(data->sz_read_buffer, sizeof(char)))) {
				ESP_LOGE(__func__, "could not allocate %u bytes for read buffer2", data->sz_read_buffer);
				return -1;
			}

			// is there some data to skip? (palette data)
			int skip_data_cnt  = data->bmpFileHeader.bfOffBits - data->total_length;
			if ( skip_data_cnt > 0) {
				ESP_LOGI(__func__, "skip %d bytes", skip_data_cnt);
				data->buffer = calloc(skip_data_cnt, sizeof(char));
				*buf_len= data->buf_len_expected = skip_data_cnt;
				*buf=data->buffer;
				data->bmp_read_phase = BRP_GOT_SKIPPED_DATA;
				//ESP_LOGI(__func__,"todo=%d, phase=%d(%s), buf_len=%u", to_do, bmp_read_phase, BRP2TXT(bmp_read_phase), *buf_len);
				return 1;
			}

			// initial read for buffer 1
			*buf_len = data->buf_len_expected = data->sz_read_buffer;
			*buf = data->read_buffer1;
			data->bmp_read_phase = BRP_GOT_FIRST_DATA_BUFFER1;
			//ESP_LOGI(__func__,"todo=%d, phase=%d(%s), buf_len=%u", to_do, bmp_read_phase, BRP2TXT(bmp_read_phase), *buf_len);
			return 1;

		case BRP_GOT_SKIPPED_DATA:
			data->total_length += *buf_len;
			if (data->buffer) {
				free(data->buffer); // maybe ther's some use case for the data
				data->buffer = NULL;
			}
			// now: initial read for buffer 1
			*buf_len = data->buf_len_expected = data->sz_read_buffer;
			*buf = data->read_buffer1;
			data->bmp_read_phase = BRP_GOT_FIRST_DATA_BUFFER1;
			//ESP_LOGI(__func__,"todo=%d, phase=%d(%s), buf_len=%u", to_do, bmp_read_phase, BRP2TXT(bmp_read_phase), *buf_len);
			return 1;

		case BRP_GOT_FIRST_DATA_BUFFER1:
			// buffer 1 is filled, can be processes, try to fill buffer 2 immediately
			ESP_LOG_BUFFER_HEXDUMP(__func__, data->read_buffer1, 32, ESP_LOG_INFO);
			data->total_length += *buf_len;
			if ( extended_log)
				ESP_LOGI(__func__, "read first data to buffer 1 len=%lu, total = %lu", *buf_len, data->total_length);

			data->read_buffer_length = *buf_len;

			*buf_len = data->buf_len_expected = data->sz_read_buffer;
			*buf = data->read_buffer2;
			data->bmp_read_phase = BRP_GOT_DATA_BUFFER2;
			xEventGroupClearBits(data->s_bmp_event_group_for_worker, 0xFFFF);
			xEventGroupSetBits(data->s_bmp_event_group_for_worker, BMP_BIT_BUFFER1_HAS_DATA);
			//ESP_LOGI(__func__,"todo=%d, phase=%d(%s), buf_len=%u", to_do, bmp_read_phase, BRP2TXT(bmp_read_phase), *buf_len);
			return 1;

		case BRP_GOT_DATA_BUFFER2:
			// buffer 2 filled
			data->total_length += *buf_len;
			if ( extended_log)
				ESP_LOGI(__func__, "read data to buffer 2 len=%lu, total = %lu", *buf_len, data->total_length);

			data->read_buffer_length = *buf_len;

			*buf_len = data->buf_len_expected = data->sz_read_buffer;
			*buf = data->read_buffer1;
			data->bmp_read_phase = BRP_GOT_DATA_BUFFER1;

			// wait for end of processing buffer1

			// first boolan true: clear bits before function return, second boolean false: wait for one of the specified bits
			uxBits = xEventGroupWaitBits(data->s_bmp_event_group_for_reader, BMP_BIT_BUFFER_PROCESSED | BMP_BIT_STOP_WORKING, pdTRUE, pdFALSE, xTicksToWait);
			if ( extended_log)
				ESP_LOGI(__func__, "xEventGroupWaitBits_for_reader return 0x%02x", uxBits);
			if ( uxBits & BMP_BIT_BUFFER_PROCESSED) {
				// buffer 1 processed, continue with buffer 2
				xEventGroupClearBits(data->s_bmp_event_group_for_worker, 0xFFFF);
				xEventGroupSetBits(data->s_bmp_event_group_for_worker, BMP_BIT_BUFFER2_HAS_DATA);
				//ESP_LOGI(__func__,"todo=%d, phase=%d(%s), buf_len=%u", to_do, bmp_read_phase, BRP2TXT(bmp_read_phase), *buf_len);
				return 1;
			}
			if ( uxBits & BMP_BIT_STOP_WORKING ) {
				// stop processing BMP Stream
				ESP_LOGI(__func__,"stop working(1)");
				return -1;
			}
			ESP_LOGE(__func__,"unexpected uxBits 0x%04x",uxBits);
			return -1;

		case BRP_GOT_DATA_BUFFER1:
			data->total_length += *buf_len;
			if ( extended_log)
				ESP_LOGI(__func__, "read data to buffer 1 len=%lu, total = %lu", *buf_len, data->total_length);

			data->read_buffer_length = *buf_len;

			*buf_len = data->buf_len_expected = data->sz_read_buffer;
			*buf = data->read_buffer2;
			data->bmp_read_phase = BRP_GOT_DATA_BUFFER2;

			// wait for process of buffer 2
			// true: clear bits before function return, false: wait for one of the specified bits
			uxBits = xEventGroupWaitBits(data->s_bmp_event_group_for_reader, BMP_BIT_BUFFER_PROCESSED | BMP_BIT_STOP_WORKING, pdTRUE, pdFALSE, xTicksToWait);
			//xEventGroupClearBits(s_bmp_event_group_for_reader, 0xFFFF);
			if ( extended_log)
				ESP_LOGI(__func__, "xEventGroupWaitBits_for_reader return 0x%02x", uxBits);
			if ( uxBits & BMP_BIT_BUFFER_PROCESSED) {
				xEventGroupClearBits(data->s_bmp_event_group_for_worker, 0xFFFF);
				xEventGroupSetBits(data->s_bmp_event_group_for_worker, BMP_BIT_BUFFER1_HAS_DATA);
				//ESP_LOGI(__func__,"todo=%d, phase=%d(%s), buf_len=%u", to_do, bmp_read_phase, BRP2TXT(bmp_read_phase), *buf_len);
				return 1;
			}
			if ( uxBits & BMP_BIT_STOP_WORKING ) {
				// stop processing BMP Stream
				ESP_LOGI(__func__,"stop working(2)");
				return -1;
			}
			ESP_LOGE(__func__,"unexpected uxBits 0x%04x",uxBits);
			return -1;

		}

	} else if ( slot->todo==HCT_FINISH ) {
		ESP_LOGI(__func__,"finished");
        xEventGroupClearBits(data->s_bmp_event_group_for_worker, 0xFFFF);
		xEventGroupSetBits(data->s_bmp_event_group_for_worker, BMP_BIT_NO_MORE_DATA);

        free_buffers(data);
    	//ESP_LOGI(__func__,"todo=%d(finish), buf_len=%u", to_do, *buf_len);

	} else if ( slot->todo==HCT_FAILED ) {
		ESP_LOGI(__func__,"failed");
        xEventGroupClearBits(data->s_bmp_event_group_for_worker, 0xFFFF);
		xEventGroupSetBits(data->s_bmp_event_group_for_worker, BMP_BIT_NO_MORE_DATA);

        free_buffers(data);
    	//ESP_LOGI(__func__,"todo=%d(failed), buf_len=%u", to_do, *buf_len);

	} else {
		ESP_LOGE(__func__,"unknown todo %d", slot->todo);
	}
	return -1;
}

EventBits_t get_ux_bits(T_BMP_WORKING *data, uint32_t wait_ms) {
	const TickType_t xTicksToWait = wait_ms / portTICK_PERIOD_MS;

	// true: clear bits before function return, false: wait for one of the specified bits
	return xEventGroupWaitBits(data->s_bmp_event_group_for_worker,
			BMP_BIT_BUFFER1_HAS_DATA | BMP_BIT_BUFFER2_HAS_DATA | BMP_BIT_NO_MORE_DATA,
			pdTRUE, pdFALSE, xTicksToWait);

}

void set_ux_quit_bits(T_BMP_WORKING *data, EventBits_t uxQuitBits) {
	xEventGroupClearBits(data->s_bmp_event_group_for_reader, 0xFFFF);
	if ( extended_log)
		ESP_LOGI(__func__, "xEventGroupSetBits_for_reader return 0x%02x", uxQuitBits);
	if (uxQuitBits)
		xEventGroupSetBits(data->s_bmp_event_group_for_reader, uxQuitBits);

}

void clear_ux_bits(T_BMP_WORKING *data) {
	xEventGroupClearBits(data->s_bmp_event_group_for_reader, 0xFFFF);
    xEventGroupClearBits(data->s_bmp_event_group_for_worker, 0xFFFF);
}

uint32_t get_read_length(T_BMP_WORKING *data) {
	return data->read_buffer_length;
}

uint8_t *get_read_buffer(T_BMP_WORKING *data, int bufnr) {
	switch(bufnr) {
	case 1: return data->read_buffer1;
	case 2: return data->read_buffer2;
	default:
		ESP_LOGE(__func__, "wrong bufnr %d", bufnr);
	}
	return NULL;
}
