/*
 * util.c
 *
 *  Created on: 05.08.2022
 *      Author: andreas
 */


#include "esp32_ws2812.h"

int32_t get_random(int32_t min, uint32_t diff) {
	if ( diff == 0)
		return min;

	int64_t drr = (int64_t)(diff)*esp_random() / UINT32_MAX ;
	return min + drr;
}
