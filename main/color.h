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


#endif /* MAIN_COLOR_H_ */
