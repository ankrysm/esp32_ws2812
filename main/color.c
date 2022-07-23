/*
 * color.c
 *
 *  Created on: 25.06.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812_basic.h"
#include "color.h"


/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void c_hsv2rgb( T_COLOR_HSV *hsv, T_COLOR_RGB *rgb )
{

	if ( hsv->v == 0) {
		rgb->r = rgb->g = rgb->b = 0;
		return;
	}
    hsv->h %= 360; // h -> [0,360]
    uint32_t rgb_max = hsv->v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - hsv->s) / 100.0f;

    uint32_t i = hsv->h / 60;
    uint32_t diff = hsv->h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        rgb->r = rgb_max;
        rgb->g = rgb_min + rgb_adj;
        rgb->b = rgb_min;
        break;
    case 1:
    	rgb->r = rgb_max - rgb_adj;
    	rgb->g = rgb_max;
    	rgb->b = rgb_min;
        break;
    case 2:
    	rgb->r = rgb_min;
    	rgb->g = rgb_max;
    	rgb->b = rgb_min + rgb_adj;
        break;
    case 3:
    	rgb->r = rgb_min;
    	rgb->g = rgb_max - rgb_adj;
    	rgb->b = rgb_max;
        break;
    case 4:
    	rgb->r = rgb_min + rgb_adj;
    	rgb->g = rgb_min;
    	rgb->b = rgb_max;
        break;
    default:
    	rgb->r = rgb_max;
    	rgb->g = rgb_min;
    	rgb->b = rgb_max - rgb_adj;
        break;
    }
}

void c_checkrgb(T_COLOR_RGB *rgb, T_COLOR_RGB *rgbmin, T_COLOR_RGB *rgbmax) {
	if ( rgb->r < MIN(rgbmin->r, rgbmax->r)) {
		rgb->r = MIN(rgbmin->r, rgbmax->r);
	} else if ( rgb->r > MAX(rgbmax->r, rgbmin->r) ) {
		rgb->r = MAX(rgbmax->r, rgbmin->r);
	}

	if ( rgb->g < MIN(rgbmin->g, rgbmax->g)) {
		rgb->g = MIN(rgbmin->g, rgbmax->g);
	} else if ( rgb->g > MAX(rgbmax->g, rgbmin->g) ) {
		rgb->g = MAX(rgbmax->g, rgbmin->g);
	}

	if ( rgb->b < MIN(rgbmin->b, rgbmax->b)) {
		rgb->b = MIN(rgbmin->b, rgbmax->b);
	} else if ( rgb->b > MAX(rgbmax->b, rgbmin->b) ) {
		rgb->b = MAX(rgbmax->b, rgbmin->b);
	}
}
