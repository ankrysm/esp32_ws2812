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

void c_hsv2rgb( T_COLOR_HSV *hsv, T_COLOR_RGB *rgb );
void c_checkrgb(T_COLOR_RGB *rgb, T_COLOR_RGB *rgbmin, T_COLOR_RGB *rgbmax);

#endif /* MAIN_COLOR_H_ */
