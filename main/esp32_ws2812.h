/*
 * esp32_ws2812.h
 * main include file for the project
 *
 *  Created on: 23.07.2022
 *      Author: ankrysm
 */

#ifndef MAIN_ESP32_WS2812_H_
#define MAIN_ESP32_WS2812_H_

#include "esp32_ws2812_basic.h"

#include "config.h"
#include "color.h"
//#include "led_strip.h"
#include "wifi_config.h"
#include "timer_events.h"
#include "paint_pixel.h"

// useful definitions
#ifndef MIN
#define MIN(a,b) (a<b?a:b)
#endif

#ifndef MAX
#define MAX(a,b) (a>b?a:b)
#endif

#include "esp32_ws2812_protos.h"

//#ifdef MAIN
//size_t s_numleds;
//#else
extern size_t s_numleds;
//#endif

#endif /* MAIN_ESP32_WS2812_H_ */
