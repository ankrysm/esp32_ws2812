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
#include "wifi_config.h"
#include "timer_events.h"

// useful definitions
#ifndef MIN
#define MIN(a,b) (a<b?a:b)
#endif

#ifndef MAX
#define MAX(a,b) (a>b?a:b)
#endif

#include "common_util.h"
#include "led_strip.h"

#include "esp32_ws2812_types.h"
#include "esp32_ws2812_protos.h"

#endif /* MAIN_ESP32_WS2812_H_ */
