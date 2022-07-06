/*
 * create_events.h
 *
 *  Created on: 26.06.2022
 *      Author: ankrysm
 */

#ifndef MAIN_CREATE_EVENTS_H_
#define MAIN_CREATE_EVENTS_H_

/*
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
		int32_t repeats, // -1 forever
		T_MOVEMENT *movement
);

void create_blank(
		int32_t pos, // start position
		int32_t len, // numer of leds, -1 = strip len
		int32_t t_start, // in ms
		int32_t duration, // in ms
		int32_t repeats, // -1 forever
		T_MOVEMENT *movement
);

void create_noops(
		int32_t t_start // in ms
);

T_MOVEMENT *create_movement(
		int32_t pos, // start movement positiin
		int32_t len, // length of moving area, -1 = strip len
		float speed, // leds per second
		int32_t pause, // pause between move cycles in ms
		int32_t repeats, // -1 forever
		movement_type_type type
);
*/

#endif /* MAIN_CREATE_EVENTS_H_ */
