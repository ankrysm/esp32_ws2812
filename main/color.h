/*
 * color.h
 *
 *  Created on: 25.06.2022
 *      Author: ankrysm
 */

#ifndef MAIN_COLOR_H_
#define MAIN_COLOR_H_

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include <stdio.h>
#include "config.h"

typedef struct {
	int32_t r;
	int32_t g;
	int32_t b;
} T_COLOR_RGB;

typedef struct {
	int32_t h;
	int32_t s;
	int32_t v;
} T_COLOR_HSV;

typedef struct {
	char *name;
	T_COLOR_RGB rgb;
	T_COLOR_HSV hsv;
} T_NAMED_RGB_COLOR;


typedef enum {
	CTIDX_WHITE,
	CTIDX_BLACK,
	CTIDX_RED,
	CTIDX_ORANGE,
	CTIDX_YELLOW,
	CTIDX_YELLOWGREEN,
	CTIDX_GREEN,
	CTIDX_BLUEGREEN,
	CTIDX_CYAN,
	CTIDX_GREENBLUE,
	CTIDX_BLUE,
	CTIDX_VIOLETT,
	CTIDX_MAGENTA,
	CTIDX_BLUERED,
	CTIDX_BROWN,
	CTIDX_END
} t_colortable_index;


#endif /* MAIN_COLOR_H_ */
