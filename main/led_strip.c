/*
 * led__strip.c
 *
 *  Created on: 26.07.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

//#define STRIP_DEMO
extern uint32_t cfg_flags;


//extern size_t s_size_led_strip_pixels;
//extern uint8_t *led_strip_pixels;

static uint32_t last_hash = 0;

static bool is_dirty = false;


static void do_set_pixel(int32_t idx, T_COLOR_RGB *rgb) {

	if (!is_dirty) {
		led_strip_clear();
	}

	if ( rgb )
		led_strip_set_pixel(idx, rgb->r, rgb->g, rgb->b);
	else
		led_strip_set_pixel(idx, 0, 0, 0);
	is_dirty = true;
}


/**
 * sets the color for a pixel range
 */
void strip_set_range(int32_t start_idx, int32_t end_idx,  T_COLOR_RGB *rgb) {
	if (!is_led_strip_initialized()) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}

	for(int i = start_idx; i <= end_idx; i++) {
		do_set_pixel(i, rgb);
	}
}

/**
 * sets a single pixel, black if rgb is NULL
 */
void strip_set_pixel(int32_t idx, T_COLOR_RGB *rgb) {
	if (!is_led_strip_initialized()) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}
	do_set_pixel(idx, rgb);
}

/**
 * clears the strip
 */
void strip_clear()  {
	if (!is_led_strip_initialized()) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}
	led_strip_clear();
	is_dirty = true;
}

/**
 * flush the pixel buffer to the strip
 */
void strip_show(bool forced) {
	if (!is_led_strip_initialized()) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}
	uint32_t hash = get_led_strip_data_hash(); //crc32b(led_strip_pixels, s_size_led_strip_pixels);
	if (!forced && !is_dirty && hash == last_hash)
		return;

	if (cfg_flags & CFG_STRIP_DEMO) {
		char msg[32];
		snprintf(msg,sizeof(msg),"%s%u->%u", (forced?"(f)":""), last_hash, hash);
		led_strip_demo(msg);
	}
	// send data to strip
	led_strip_refresh();

	last_hash = hash;
	is_dirty = false;
}

/**
 * sets the color of the first led
 * notice: it has its own transmition, so do it after rendering a scene
 */
void firstled(int red, int green, int blue) {
	led_strip_firstled(red,green,blue);
}

