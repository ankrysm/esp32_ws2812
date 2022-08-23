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

static int  is_dirty=0;

#define STRIP_INITIALIZED (led_strip_pixels ? true : false)

static void do_set_pixel(int32_t idx, T_COLOR_RGB *rgb) {
	if ( idx < 0 || idx >= get_numleds()) {
		return;
	}
	uint32_t opos, pos;

	opos = pos = 3 * idx;
	uint8_t r,g,b,nr,ng,nb;
	g=led_strip_pixels[opos++];
	r=led_strip_pixels[opos++];
	b=led_strip_pixels[opos];

	ng=rgb ? rgb->g : 0;
	nr=rgb ? rgb->r : 0;
	nb=rgb ? rgb->b : 0;

	if (g != ng || r != nr ||b != nb) {
		is_dirty = 1;
		led_strip_pixels[pos++] = ng;
		led_strip_pixels[pos++] = nr;
		led_strip_pixels[pos]   = nb;
	}

}


/**
 * sets the color for a pixel range
 */
void strip_set_range(int32_t start_idx, int32_t end_idx,  T_COLOR_RGB *rgb) {
	if (!STRIP_INITIALIZED) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}

	for(int i = start_idx; i <= end_idx; i++) {
		if ( i<0 || i >= get_numleds())
			continue;
		do_set_pixel(i, rgb);
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
	do_set_pixel(idx, rgb);
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
	is_dirty = 1;
}

/**
 * flush the pixel buffer to the strip
 */
void strip_show() {
	if (!STRIP_INITIALIZED) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}
	if (! is_dirty)
		return;

	is_dirty = 0;
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
#endif
	led_strip_refresh();
}

/**
 * sets the color of the first led
 * notice: it has its own transmition, so do it after rendering a scene
 */
void firstled(int red, int green, int blue) {
	led_strip_firstled(red,green,blue);
}

