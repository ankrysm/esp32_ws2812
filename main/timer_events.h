/*
 * timer_events.h
 *
 *  Created on: 25.06.2022
 *      Author: andreas
 */

#ifndef MAIN_TIMER_EVENTS_H_
#define MAIN_TIMER_EVENTS_H_

#include "color.h"

typedef enum {
	EVT_NOTHING, // nothing to do
	EVT_BLANK, // switch off all leds
	EVT_SOLID, // one color for all
	EVT_BLINKING, // blinking
	EVT_RAIBNOW, // show a rainbow
	EVT_SPARKLE, // light on random places
	EVT_MOVE
} strip_event_type;

typedef enum {
	SCENES_NOTHING,
	SCENES_STOPPED,
	SCENES_RUNNING,
	SCENES_PAUSED
} run_status_type;

#define RUN_STATUS_TYPE2TEXT(c) ( \
	c == SCENES_NOTHING ? "NOTHING" : \
	c == SCENES_STOPPED ? "STOPPED" : \
	c == SCENES_RUNNING ? "RUNNING" : \
	c == SCENES_PAUSED  ? "PAUSED" : "???" )


typedef struct {
	int32_t duration;
	int32_t in;
	int32_t out;
} T_FADE_INOUT;

typedef struct {
	int32_t speed;
	int32_t rotate_flag;
} T_MOVEMENT;

typedef struct EVENT{
	strip_event_type type;
	int64_t t_start; // Start time
	int64_t duration; // duration
	int32_t pos; // start position on strip
	int32_t len; // length
	T_COLOR_RGB bg_color;
	T_FADE_INOUT *fade_in;
	T_FADE_INOUT *fade_out;
	T_MOVEMENT *movement;
	struct EVENT *nxt;
} T_EVENT;

void init_timer_events(int delta_ms);
void scenes_start();
void scenes_stop();
void scenes_pause();
run_status_type get_scene_status();
run_status_type set_scene_status(run_status_type new_status);
uint64_t get_event_timer_period();
uint64_t get_scene_time();
void event_list_add(T_EVENT *evt);



#endif /* MAIN_TIMER_EVENTS_H_ */
