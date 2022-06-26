/*
 * create_events.c
 *
 *  Created on: 25.06.2022
 *      Author: ankrysm
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"
#include "config.h"
#include "color.h"
#include "timer_events.h"

extern T_CONFIG gConfig;

void create_solid(
		int32_t pos, // start position
		int32_t len, // numer of leds, -1 = strip len
		T_COLOR_RGB *fg_color, // foreground color
		T_COLOR_RGB *bg_color, // background color
		int32_t inset, // number of edge leds
		int32_t outset, // number of edge leds
		int32_t t_start, // in ms
		int32_t duration, // in ms
		int32_t fadein_time, // in ms
		int32_t fadeout_time // in ms
) {
	ESP_LOGI(__func__, "start");

	T_EVENT *evt = calloc(1,sizeof(T_EVENT));
	evt->type = EVT_SOLID;
	evt->pos = pos;
	evt->len = len > 0 ? len : gConfig.numleds;
	evt->t_start = t_start;
	evt->duration = duration;
	evt->t_fade_in = fadein_time;
	evt->t_fade_out = fadeout_time;

	if ( bg_color) {
		evt->bg_color=calloc(1,sizeof(T_COLOR_RGB));
		memcpy(evt->bg_color, bg_color, sizeof(T_COLOR_RGB));
	}

	T_SOLID_DATA *data = calloc(1, sizeof(T_SOLID_DATA));
	evt->data = data;
	// if no color specified assume white
	data->fg_color.r = fg_color? fg_color->r : 255;
	data->fg_color.g = fg_color? fg_color->g : 255;
	data->fg_color.b = fg_color? fg_color->b : 255;
	data->inset = inset;
	data->outset = outset;

	event_list_add(evt);

}
