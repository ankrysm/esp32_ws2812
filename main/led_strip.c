/*
 * led__strip.c
 *
 *  Created on: 26.07.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

extern size_t s_size_led_strip_pixels;
extern uint8_t *led_strip_pixels;


#define STRIP_INITIALIZED (led_strip_pixels ? true : false)

/**
 * sets the color for a pixel range
 */
void strip_set_color(int32_t start_idx, int32_t end_idx,  T_COLOR_RGB *rgb) {
	if (!STRIP_INITIALIZED) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}

	for(int i = start_idx; i <= end_idx; i++) {
		if ( i<0 || i >= get_numleds())
			continue;
		uint32_t pos = 3 * i;
		led_strip_pixels[pos++] = rgb->g;
		led_strip_pixels[pos++] = rgb->r;
		led_strip_pixels[pos] = rgb->b;
	}
}

/**
 * sets a single pixel, black if rgb is NULL
 */
void strip_set_pixel(int32_t idx, T_COLOR_RGB *rgb) {
	if (!STRIP_INITIALIZED) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}
	if ( idx < 0 || idx >= get_numleds()) {
		return;
	}
	uint32_t pos = 3 * idx;
	led_strip_pixels[pos++] = rgb ? rgb->g : 0;
	led_strip_pixels[pos++] = rgb ? rgb->r : 0;
	led_strip_pixels[pos]   = rgb ? rgb->b : 0;
}

/**
 * clears the strip
 */
void strip_clear()  {
	if (!STRIP_INITIALIZED) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}
	//ESP_ERROR_CHECK(gVstrip->clear(gVstrip, 100));
	memset(led_strip_pixels, 0, s_size_led_strip_pixels);
}

/**
 * flush the pixel buffer to the strip
 */
void strip_show() {
	if (!STRIP_INITIALIZED) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}
	led_strip_refresh();
	//ESP_ERROR_CHECK(gVstrip->refresh(gVstrip, 100));
}

/**
 * sets the color of the first led
 * notice: it has its own transmition, so do it after rendering a scene
 */
void firstled(int red, int green, int blue) {
	led_strip_firstled(red,green,blue);
}

