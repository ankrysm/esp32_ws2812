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

//#include "esp_event.h"
#include "esp_log.h"
//#include "esp_system.h"
//#include "esp_timer.h"
//#include "esp_event.h"

#include "bmp.h"
#include "https_get.h"

typedef enum {
	BRP_GOT_FILE_HEADER, // 0
	BRP_GOT_INFO_HEADER, // 1
	BRP_GOT_SKIPPED_DATA, // 2
	BRP_GOT_FIRST_DATA_BUFFER1, // 3
	BRP_GOT_DATA_BUFFER1, // 4
	BRP_GOT_DATA_BUFFER2 // 5
} t_bmp_read_phase;

#define BRP2TXT(c) ( \
		c==BRP_GOT_FILE_HEADER ? "GOT_FILE_HEADER" :  \
		c==BRP_GOT_INFO_HEADER ? "GOT_INFO_HEADER" : \
		c==BRP_GOT_SKIPPED_DATA ? "GOT_SKIPPED_DATA" : \
		c==BRP_GOT_FIRST_DATA_BUFFER1 ? "GOT_FIRST_DATA_BUFFER1" : \
		c==BRP_GOT_DATA_BUFFER1 ? "GOT_DATA_BUFFER1" : \
		c==BRP_GOT_DATA_BUFFER2 ? "GOT_DATA_BUFFER2" : "UNKNOWN" )

/* FreeRTOS event group to signal when something is read or something is processed */
static EventGroupHandle_t s_bmp_event_group_for_reader; // read process waits for ...PROCESSED
static EventGroupHandle_t s_bmp_event_group_for_worker; // worker process waits for ...HAS_DATA, ...NO_MORE_DATA


static BITMAPFILEHEADER bmpFileHeader;
static BITMAPINFOHEADER bmpInfoHeader;
static t_bmp_read_phase bmp_read_phase = BRP_GOT_FILE_HEADER;

static uint32_t buf_len_expected = 0;
static uint32_t total_length = 0;

static uint8_t *read_buffer1 = NULL;
static uint8_t *read_buffer2 = NULL;

// data amount
static uint32_t read_buffer_length = 0;
//static uint32_t read_buffer1_length = 0;
//static uint32_t read_buffer2_length = 0;

static uint8_t *buffer = NULL;

size_t sz_read_buffer = 0;
uint32_t lines_read_buffer = 0;

static int log_cnt = 1;
static int extended_log = 0;

static uint32_t led_pos =0;
static int pixel_position =0;

//#define NUMLEDS 200

static uint8_t bgr[3];
static int bgr_idx = 0;

void bmp_init() {

	ESP_LOGI(__func__, "started");
	s_bmp_event_group_for_reader = xEventGroupCreate();
	s_bmp_event_group_for_worker = xEventGroupCreate();
	xEventGroupClearBits(s_bmp_event_group_for_reader, 0xFFFF);
	xEventGroupClearBits(s_bmp_event_group_for_worker, 0xFFFF);

}

//char ledzeile[NUMLEDS+1];

/*
char pixel4rgb(uint8_t r, uint8_t g, uint8_t b) {
	uint32_t s = g+r+b;
	if ( s>0) {
		if (g>r && g>b)
			return 'G';
		else if(r>g && r>b)
			return 'R';
		else if(b>r && b>g)
			return 'B';
		else
			return 'X';
	}
	return '.';
}
*/
/*
void show_strip(int bufno) {
	ESP_LOGI(__func__, "(bufno: %d) <%s>", bufno, ledzeile);
	memset(ledzeile, 0, sizeof(ledzeile));
	led_pos = 0;
}
*/

/*
void paint_pixel(uint8_t r, uint8_t g, uint8_t b) {
	// BGR
	if (led_pos >= NUMLEDS)
		return;

	char c = pixel4rgb(r, g, b);
	ledzeile[led_pos] = c;
	led_pos++;
}
*/

size_t get_linesize() {
	return  bmpInfoHeader.biWidth * (
			bmpInfoHeader.biBitCount == 32 ? 4 :
			bmpInfoHeader.biBitCount == 24 ? 3 :
			bmpInfoHeader.biBitCount == 16 ? 2 :
			1
	);
}

/**
 * callback for https_get
 * to do: 0 - init
 *        1 - read data
 *        2 - finished
 *  **buf - pointer to put in received data
 *  *buf_len:
 *     out: expected bytes
 *     in: read bytes
 *
 * return:
 *  1 - more data needed
 *  0 - wait
 * -1 - error
 */
int https_callback_bmp_processing(t_https_callback_todo todo, uint8_t **buf, size_t *buf_len) {

	ESP_LOGI(__func__,"todo=%d, phase=%d(%s), buf_len=%u", todo, bmp_read_phase, BRP2TXT(bmp_read_phase), *buf_len);

	EventBits_t uxBits;


	if (todo == HCT_INIT) {
		// init
		memset(&bmpFileHeader, 0, sizeof(bmpFileHeader));
		memset(&bmpInfoHeader, 0, sizeof(bmpInfoHeader));

		bmp_read_phase = BRP_GOT_FILE_HEADER;
		*buf_len = buf_len_expected = sizeof(bmpFileHeader);
		*buf = (uint8_t *) &bmpFileHeader;

		if (read_buffer1) {
			free(read_buffer1);
			read_buffer1 = NULL;
		}
		if (read_buffer2) {
			free(read_buffer2);
			read_buffer2 = NULL;
		}
		if ( buffer) {
			free(buffer);
			buffer=NULL;
		}
		sz_read_buffer = 0;
		pixel_position = 0;
		bgr_idx =0;
		total_length = 0;
		log_cnt = 1; // log first buffer

		led_pos = 0;
		//memset(ledzeile, 0, sizeof(ledzeile));
		return 1;

	} else if (todo == HCT_READING) {

		// read data
		if ( buf_len_expected > 0 &&  *buf_len != buf_len_expected) {
			ESP_LOGW(__func__,"buf_len_received(%ld) != buf_len_expected(%ld)", *buf_len, buf_len_expected);
			//return -1;
		}

		switch(bmp_read_phase) {

		case BRP_GOT_FILE_HEADER: // got file header
			ESP_LOGI(__func__, "got file header: bfType='%c%c', bfSize=0x%08x, bfOffBits=0x%08x",
					bmpFileHeader.bfType[0],bmpFileHeader.bfType[1],
					bmpFileHeader.bfSize, bmpFileHeader.bfOffBits);
			ESP_LOG_BUFFER_HEXDUMP(__func__, &bmpFileHeader, sizeof(bmpFileHeader), ESP_LOG_INFO);

			if ( bmpFileHeader.bfType[0] != 'B' || bmpFileHeader.bfType[1] != 'M') {
				ESP_LOGE(__func__, "is not a bitmap file, first bytes have to be 'BM'");
				return -1;
			}

			total_length += *buf_len;
			bmp_read_phase = BRP_GOT_INFO_HEADER;

			*buf_len = buf_len_expected = sizeof(bmpInfoHeader);
			*buf = (uint8_t *) &bmpInfoHeader;
			return 1;

		case BRP_GOT_INFO_HEADER:
			ESP_LOGI(__func__,"got info header: biSize=%u, biWidth=%ld, biHeight%ld, biPlanes=%d, biBitCount=%d, biCompression=%u, biSizeImage=%u, biClrUsed=%u, biClrImportant=%u",
					bmpInfoHeader.biSize, bmpInfoHeader.biWidth, bmpInfoHeader.biHeight, bmpInfoHeader.biPlanes,
					bmpInfoHeader.biBitCount, bmpInfoHeader.biCompression, bmpInfoHeader.biSizeImage, bmpInfoHeader.biClrUsed, bmpInfoHeader.biClrImportant);

			ESP_LOG_BUFFER_HEXDUMP(__func__, &bmpInfoHeader, sizeof(bmpInfoHeader), ESP_LOG_INFO);
			if ( bmpInfoHeader.biCompression != 0 ) {
				ESP_LOGE(__func__, "unsupported compression method"); // TODO: implement it
				return -1;
			}
			if ( bmpInfoHeader.biBitCount != 24 && bmpInfoHeader.biBitCount != 32) {
				ESP_LOGE(__func__, "24-bit or 32-bit pixel expected");
				return -1;
			}
			if ( bmpInfoHeader.biWidth > 500 ) {
				ESP_LOGE(__func__, "bitmap width greater 500");
				return -1;
			}

			total_length += *buf_len;

			// prepare data read
			// data len as multiple amount of data per line
			size_t linesize =get_linesize();
			sz_read_buffer = linesize;
			while (sz_read_buffer < 1024 ) {
				sz_read_buffer += linesize;
			}
			ESP_LOGI(__func__, "sz_read_buffer = %d",sz_read_buffer);
			if ( !(read_buffer1 = calloc(sz_read_buffer, sizeof(char)))) {
				ESP_LOGE(__func__, "could not allocate %u bytes for read buffer1", sz_read_buffer);
				return -1;
			}

			if ( !(read_buffer2 = calloc(sz_read_buffer, sizeof(char)))) {
				ESP_LOGE(__func__, "could not allocate %u bytes for read buffer2", sz_read_buffer);
				return -1;
			}

			// is there some data to skip? (palette data)
			int skip_data_cnt  = bmpFileHeader.bfOffBits - total_length;
			if ( skip_data_cnt > 0) {
				ESP_LOGI(__func__, "skip %d bytes", skip_data_cnt);
				buffer = calloc(skip_data_cnt, sizeof(char));
				*buf_len= buf_len_expected = skip_data_cnt;
				*buf=buffer;
				bmp_read_phase = BRP_GOT_SKIPPED_DATA;
				return 1;
			}

			// initial read for buffer 1
			*buf_len = buf_len_expected = sz_read_buffer;
			*buf = read_buffer1;
			bmp_read_phase = BRP_GOT_FIRST_DATA_BUFFER1;
			return 1;

		case BRP_GOT_SKIPPED_DATA:
			total_length += *buf_len;
			if (buffer) {
				free(buffer); // maybe ther's some use case for the data
				buffer = NULL;
			}
			// now: initial read for buffer 1
			*buf_len = buf_len_expected = sz_read_buffer;
			*buf = read_buffer1;
			bmp_read_phase = BRP_GOT_FIRST_DATA_BUFFER1;
			return 1;

		case BRP_GOT_FIRST_DATA_BUFFER1:
			// buffer 1 is filled, can be processes, try to fill buffer 2 immediately
			total_length += *buf_len;
			if ( extended_log)
				ESP_LOGI(__func__, "read data to buffer 1 len=%lu, total = %lu", *buf_len, total_length);

			read_buffer_length = *buf_len;

			*buf_len = buf_len_expected = sz_read_buffer;
			*buf = read_buffer2;
			bmp_read_phase = BRP_GOT_DATA_BUFFER2;
			xEventGroupClearBits(s_bmp_event_group_for_worker, 0xFFFF);
			xEventGroupSetBits(s_bmp_event_group_for_worker, BMP_BIT_BUFFER1_HAS_DATA);
			return 1;

		case BRP_GOT_DATA_BUFFER2:
			// buffer 2 filled
			total_length += *buf_len;
			if ( extended_log)
				ESP_LOGI(__func__, "read data to buffer 2 len=%lu, total = %lu", *buf_len, total_length);

			read_buffer_length = *buf_len;

			*buf_len = buf_len_expected = sz_read_buffer;
			*buf = read_buffer1;
			bmp_read_phase = BRP_GOT_DATA_BUFFER1;
//			xEventGroupClearBits(s_bmp_event_group_for_worker, 0xFFFF);
//			xEventGroupSetBits(s_bmp_event_group_for_worker, BMP_BIT_BUFFER2_HAS_DATA);

			// wait for end of processing buffer1
			// true: clear bits before function return, false: wait for one of the specified bits
	        uxBits = xEventGroupWaitBits(s_bmp_event_group_for_reader, BMP_BIT_BUFFER1_PROCESSED, pdTRUE, pdFALSE, portMAX_DELAY);
			// xEventGroupClearBits(s_bmp_event_group_for_reader, 0xFFFF);
			if ( extended_log)
				ESP_LOGI(__func__, "xEventGroupWaitBits_for_reader return 0x%02x", uxBits);
	    	if ( uxBits & BMP_BIT_BUFFER1_PROCESSED) {
				xEventGroupClearBits(s_bmp_event_group_for_worker, 0xFFFF);
				xEventGroupSetBits(s_bmp_event_group_for_worker, BMP_BIT_BUFFER2_HAS_DATA);
	    		return 1;
	    	}
	    	ESP_LOGE(__func__,"unexpected uxBits 0x%04x",uxBits);
			return -1;

		case BRP_GOT_DATA_BUFFER1:
			total_length += *buf_len;
			if ( extended_log)
				ESP_LOGI(__func__, "read data to buffer 1 len=%lu, total = %lu", *buf_len, total_length);

			read_buffer_length = *buf_len;

			*buf_len = buf_len_expected = sz_read_buffer;
			*buf = read_buffer2;
			bmp_read_phase = BRP_GOT_DATA_BUFFER2;
			//xEventGroupClearBits(s_bmp_event_group_for_worker, 0xFFFF);
			//xEventGroupSetBits(s_bmp_event_group_for_worker, BMP_BIT_BUFFER1_HAS_DATA);

			// wait for process of buffer 2
			// true: clear bits before function return, false: wait for one of the specified bits
	        uxBits = xEventGroupWaitBits(s_bmp_event_group_for_reader, BMP_BIT_BUFFER2_PROCESSED, pdTRUE, pdFALSE, portMAX_DELAY);
			//xEventGroupClearBits(s_bmp_event_group_for_reader, 0xFFFF);
			if ( extended_log)
				ESP_LOGI(__func__, "xEventGroupWaitBits_for_reader return 0x%02x", uxBits);
	    	if ( uxBits & BMP_BIT_BUFFER2_PROCESSED) {
				xEventGroupClearBits(s_bmp_event_group_for_worker, 0xFFFF);
				xEventGroupSetBits(s_bmp_event_group_for_worker, BMP_BIT_BUFFER1_HAS_DATA);
	    		return 1;
	    	}
	    	ESP_LOGE(__func__,"unexpected uxBits 0x%04x",uxBits);
	    	return -1;

		}
	} else if ( todo==HCT_FINISH ) {
		if ( extended_log)
			ESP_LOGI(__func__,"finished");
		xEventGroupClearBits(s_bmp_event_group_for_worker, 0xFFFF);
		xEventGroupSetBits(s_bmp_event_group_for_worker, BMP_BIT_NO_MORE_DATA);

		return 0;
	} else {
		ESP_LOGE(__func__,"unknown todo %d", todo);
	}
	return -1;
}

void bmp_show_line(uint8_t *read_buffer, int bufno, int32_t *mempos, int32_t max_mempos) {

	// 24 bit data: B,G,R
	// 32 bit data: B,G,R,x

	// bytes per line
	size_t bytes_per_line = get_linesize();

	if ( log_cnt > 0) {
		ESP_LOG_BUFFER_HEXDUMP(__func__,
				 read_buffer, bytes_per_line, ESP_LOG_INFO);
		log_cnt--;
	}

	bgr_idx = 0;
	pixel_position = 0;

	for ( int i=0; i<bytes_per_line; i++) {
		bgr[bgr_idx++] = read_buffer[*mempos];
		(*mempos)++;
		if ( bmpInfoHeader.biBitCount == 24 ) {
			if ( bgr_idx >= 3 ) {
				// TODO: set pixel data
				//paint_a_pixel(bgr[2],bgr[1],bgr[0]);
				bgr_idx = 0;
				pixel_position++;
				if ( pixel_position >= bmpInfoHeader.biWidth ) {
					// TODO line complete
					//show_strip(bufno);
					pixel_position = 0; // next line TODO wait ??
				}
			}
		} else if ( bmpInfoHeader.biBitCount == 32) {
			// byte folge rgbx
			if ( bgr_idx >= 4 ) {
				// TODO set pixel data
				// paint_a_pixel(bgr[2],bgr[1],bgr[0]);
				bgr_idx = 0;
				pixel_position++;
				if ( pixel_position >= bmpInfoHeader.biWidth ) {
					// TODO line complete
					//show_strip(bufno);
					pixel_position = 0; // next line TODO wait ??
				}
			}
		}
	}

}


/*

void bmp_() {
	EventBits_t uxBits, uxQuitBits=0;
	ESP_LOGI(__func__, "START");

	int32_t mempos;
	do {
		uxQuitBits=0;
		// true: clear bits before function return, false: wait for one of the specified bits
		uxBits = xEventGroupWaitBits(s_bmp_event_group_for_worker,
				BMP_BIT_BUFFER1_HAS_DATA | BMP_BIT_BUFFER2_HAS_DATA | BMP_BIT_NO_MORE_DATA,
				pdTRUE, pdFALSE, 0);


		//ESP_LOGI(__func__, "xEventGroupWaitBits_for_worker return 0x%02x", uxBits);

		if (uxBits & BMP_BIT_NO_MORE_DATA ) {
			if ( extended_log)
				ESP_LOGI(__func__, "no more data");
			break;
		}

		mempos=0;

		if ( uxBits & BMP_BIT_BUFFER1_HAS_DATA ) {
			if ( extended_log)
				ESP_LOGI(__func__, "buffer 1 has data");

			while ( mempos < read_buffer_length) {
				bmp_show_line(read_buffer1, 1, &mempos, read_buffer_length);
			}

			if ( extended_log)
				ESP_LOGI(__func__, "#### buffer 1 processed #####");
			uxQuitBits |= BMP_BIT_BUFFER1_PROCESSED;
		}
		if ( uxBits & BMP_BIT_BUFFER2_HAS_DATA ) {
			if ( extended_log)
				ESP_LOGI(__func__, "buffer 2 has data");

			while ( mempos < read_buffer_length) {
				bmp_show_line(read_buffer2, 2, &mempos, read_buffer_length);
			}

			if ( extended_log)
				ESP_LOGI(__func__, "##### buffer 2 processed #####");
			uxQuitBits |= BMP_BIT_BUFFER2_PROCESSED;
		}

		xEventGroupClearBits(s_bmp_event_group_for_reader, 0xFFFF);
		if ( extended_log)
			ESP_LOGI(__func__, "xEventGroupSetBits_for_reader return 0x%02x", uxQuitBits);
		xEventGroupSetBits(s_bmp_event_group_for_reader, uxQuitBits);
	} while(1);

	ESP_LOGI(__func__, "STOPP");


}
*/

EventBits_t get_ux_bits(TickType_t xTicksToWait) {
	// true: clear bits before function return, false: wait for one of the specified bits
	return xEventGroupWaitBits(s_bmp_event_group_for_worker,
			BMP_BIT_BUFFER1_HAS_DATA | BMP_BIT_BUFFER2_HAS_DATA | BMP_BIT_NO_MORE_DATA,
			pdTRUE, pdFALSE, xTicksToWait);

}

void set_ux_quit_bits(EventBits_t uxQuitBits) {
	xEventGroupClearBits(s_bmp_event_group_for_reader, 0xFFFF);
	if ( extended_log)
		ESP_LOGI(__func__, "xEventGroupSetBits_for_reader return 0x%02x", uxQuitBits);
	xEventGroupSetBits(s_bmp_event_group_for_reader, uxQuitBits);

}

uint32_t get_read_length() {
	return read_buffer_length;
}

uint8_t *get_read_buffer(int bufnr) {
	switch(bufnr) {
	case 1: return read_buffer1;
	case 2: return read_buffer2;
	default:
		ESP_LOGE(__func__, "wrong bufnr %d", bufnr);
	}
	return NULL;
}
