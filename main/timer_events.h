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
//#define EVENT_FLAG_BIT_SCENE_STARTED BIT0
//#define EVENT_FLAG_BIT_SCENE_ENDED BIT1

typedef enum {
	SCENE_IDLE,    // befor start time
	SCENE_STARTED, // just started, ramp up
	SCENE_UP,      // main status
	SCENE_ENDED,   // duration ended, shutdown startet
	SCENE_FINISHED // shutdown ended
} scene_status_type;

#define EVENT_FLAG_BIT_STRIP_SHOW_NEEDED BIT0
#define EVENT_FLAG_BIT_STRIP_CLEARED BIT1

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

typedef enum {
	MOVEMENT_ONCE,
	MOVEMENT_ROTATE,
	MOVEMENT_BOUNCE
} movement_type_type;

typedef struct {
	int32_t pos; // start position on strip, negative values before beginning of the strip
	int32_t len; // length, -1 = until numleds
	float speed; // leds per second
	int32_t pause; // in ms
	int32_t repeats; // -1 forever
	movement_type_type type;
} T_MOVEMENT;

typedef struct EVENT{
	strip_event_type type;
	scene_status_type status;
	uint32_t lfd; // for logging
	int32_t pos; // start position on strip, negative values - before the beginning
	int32_t len; // length, -1 = until numleds
	uint64_t t_start; // Start time in ms
	uint64_t duration; // duration in ms
	int32_t repeats; // -1 forever, 0 once
//	uint32_t flags_origin;
	T_COLOR_RGB *bg_color; // assumed black when NULL
	uint32_t t_fade_in; // in ms
	uint32_t t_fade_out; // in ms
	T_MOVEMENT *movement;
	void *data; // special data
	// working data
	uint64_t w_t; // time from last status change
	int32_t w_pos;
	uint32_t w_len;
	uint32_t w_flags;
	int32_t w_repeats;
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
