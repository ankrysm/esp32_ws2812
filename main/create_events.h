/*
 * create_events.h
 *
 *  Created on: 26.06.2022
 *      Author: ankrysm
 */

#ifndef MAIN_CREATE_EVENTS_H_
#define MAIN_CREATE_EVENTS_H_

void create_solid(
		int32_t pos, // start position
		int32_t len, // numer of leds (-1 len)
		T_COLOR_RGB *fg_color, // foreground color, NULL = WHITE
		T_COLOR_RGB *bg_color, // background color, NULL = BLACK
		int32_t inset, // number of edge leds
		int32_t outset, // number of edge leds
		int32_t t_start, // in ms
		int32_t duration, // in ms
		int32_t fadein_time, // in ms
		int32_t fadeout_time, // in ms
		uint32_t flags
);

void create_blank(
		int32_t pos, // start position
		int32_t len, // numer of leds, -1 = strip len
		int32_t t_start, // in ms
		int32_t duration, // in ms
		uint32_t flags
);

void create_noops(
		int32_t t_start, // in ms
		uint32_t flags
);

#endif /* MAIN_CREATE_EVENTS_H_ */
