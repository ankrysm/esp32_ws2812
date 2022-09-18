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

static uint32_t last_hash = 0;

static bool is_dirty = false;

#define STRIP_INITIALIZED (led_strip_pixels ? true : false)

static void do_set_pixel(int32_t idx, T_COLOR_RGB *rgb) {
	if ( idx < 0 || idx >= get_numleds()) {
		return;
	}

	if (!is_dirty) {
		memset(led_strip_pixels, 0, s_size_led_strip_pixels);
	}

	uint32_t pos = 3 * idx;
	led_strip_pixels[pos++] = rgb ? rgb->g : 0;
	led_strip_pixels[pos++] = rgb ? rgb->r : 0;
	led_strip_pixels[pos]   = rgb ? rgb->b : 0;

	is_dirty = true;
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
	is_dirty = true;
}

/**
 * flush the pixel buffer to the strip
 */
void strip_show(bool forced) {
	if (!STRIP_INITIALIZED) {
		ESP_LOGE(__func__, "not initalized");
		return;
	}
	uint32_t hash = crc32b(led_strip_pixels, s_size_led_strip_pixels);
	if (!forced && !is_dirty && hash == last_hash)
		return;

#ifdef STRIP_DEMO
	{
		char txt[1024];
		uint32_t pos=0;
		snprintf(txt,sizeof(txt),"%s%u->%u #### LED:<", (forced?"(f)":""), last_hash, hash);
		for (int i=0; i<get_numleds(); i++) {
			uint32_t g= led_strip_pixels[pos++]; // g
			uint32_t r= led_strip_pixels[pos++]; // r
			uint32_t b= led_strip_pixels[pos++]; //b
			uint32_t s = g+r+b;
			if ( s>0) {
				if (g>r && g>b)
					strlcat(txt,"G",sizeof(txt));
				else if(r>g && r>b)
					strlcat(txt,"R",sizeof(txt));
				else if(b>r && b>g)
					strlcat(txt,"B",sizeof(txt));
				else strlcat(txt,"X",sizeof(txt));
			} else {
				strlcat(txt,".",sizeof(txt));
			}
		}
		strlcat(txt,">",sizeof(txt));
		ESP_LOGI(__func__, "%s", txt);
	}
#endif
	// send data to strip
	led_strip_refresh();
	// clear all pixels for next cycle
	// memset(led_strip_pixels, 0, s_size_led_strip_pixels);
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

