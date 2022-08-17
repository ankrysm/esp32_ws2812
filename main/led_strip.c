/*
 * led__strip.c
 *
 *  Created on: 26.07.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

#define STRIP_DEMO

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
#ifdef STRIP_DEMO
	{
		uint32_t pos=0;
		printf("\n#### LED:<");
		for (int i=0; i<get_numleds(); i++) {
			uint32_t s=led_strip_pixels[pos++];
			s+= led_strip_pixels[pos++];
			s+= led_strip_pixels[pos++];
			if ( s>0) {
				printf("X");
			} else {
				printf(".");
			}
		}
		printf(">\n");
	}
#else
	led_strip_refresh();
#endif
}

/**
 * sets the color of the first led
 * notice: it has its own transmition, so do it after rendering a scene
 */
void firstled(int red, int green, int blue) {
	led_strip_firstled(red,green,blue);
}

