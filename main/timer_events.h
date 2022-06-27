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
	EVT_NOOP, // paints nothing, for control purposes
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
	SCENES_PAUSED,
	SCENES_RESTART
} run_status_type;

#define RUN_STATUS_TYPE2TEXT(c) ( \
	c == SCENES_NOTHING ? "NOTHING" : \
	c == SCENES_STOPPED ? "STOPPED" : \
	c == SCENES_RUNNING ? "RUNNING" : \
	c == SCENES_PAUSED  ? "PAUSED" : \
	c == SCENES_RESTART ? "RESTART" : "???" )

// scene stopped after the event duration is over
#define EVENT_FLAG_BIT_SCENE_STARTED BIT0
#define EVENT_FLAG_BIT_SCENE_ENDED BIT1

#define EVENT_FLAG_BIT_STOP_AFTER_DURATION BIT4
#define EVENT_FLAG_BIT_REPEAT_AFTER_DURATION BIT5
#define EVENT_FLAG_BIT_STRIP_SHOW_NEEDED BIT6

//typedef struct {
//	int32_t duration;
//	int32_t in;
//	int32_t out;
//} T_FADE_INOUT;

typedef struct {
	T_COLOR_RGB fg_color;
	uint32_t inset;  // at the edge of leds
	uint32_t outset;
	//uint32_t flags;
	// working data
	//T_COLOR_RGB color; // actual color
	//T_COLOR_RGB delta_color; // for fade in / fade out
} T_SOLID_DATA;

typedef struct {
	int32_t speed;
	int32_t rotate_flag;
} T_MOVEMENT;

typedef struct EVENT{
	strip_event_type type;
	uint32_t lfd; // for logging
	uint32_t pos; // start position on strip
	uint32_t len; // length
	uint64_t t_start; // Start time in ms
	uint64_t duration; // duration in ms
	uint32_t flags_origin;
	uint32_t flags;
	T_COLOR_RGB *bg_color; // assumed black when NULL
	uint32_t t_fade_in; // in ms
	uint32_t t_fade_out; // in ms
	T_MOVEMENT *movement;
	void *data; // special data
	// working data
	uint64_t t; // time from start
	struct EVENT *nxt;
} T_EVENT;

void init_timer_events(int delta_ms);
void set_timer_cycle(int new_delta_ms);
void scenes_start();
void scenes_stop();
void scenes_pause();
void scenes_restart();
run_status_type get_scene_status();
run_status_type set_scene_status(run_status_type new_status);
uint64_t get_event_timer_period();
uint64_t get_scene_time();
void event_list_add(T_EVENT *evt);



#endif /* MAIN_TIMER_EVENTS_H_ */
