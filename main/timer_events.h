/*
 * timer_events.h
 *
 *  Created on: 25.06.2022
 *      Author: andreas
 */

#ifndef MAIN_TIMER_EVENTS_H_
#define MAIN_TIMER_EVENTS_H_

#include "color.h"

/**********************************************
 * status of a play list
 ***********************************************/
typedef enum {
	RUN_STATUS_NOT_SET,
	RUN_STATUS_STOPPED,
	RUN_STATUS_RUNNING,
	RUN_STATUS_PAUSED
} run_status_type;

#define RUN_STATUS_TYPE2TEXT(c) ( \
	c == RUN_STATUS_NOT_SET ? "NOT SET" : \
	c == RUN_STATUS_STOPPED ? "STOPPED" : \
	c == RUN_STATUS_RUNNING ? "RUNNING" : \
	c == RUN_STATUS_PAUSED  ? "PAUSED" : "???" )


/**********************************************
 * timing status of a single scene
 ***********************************************/
typedef enum {
	SCENE_IDLE,    // befor start time
	SCENE_UP,      // main status
	SCENE_FINISHED // shutdown ended
} scene_status_type;

typedef enum {
	WT_NOTHING,
	WT_CLEAR, // switch of all leds
	WT_COLOR,
	WT_COLOR_TRANSITION,
	WT_RAINBOW,
	WT_SPARKLE,
	WT_UNKNOWN
} what_type;

#define TEXT2WT(c) ( \
	!strcasecmp(c,"clear") ? WT_CLEAR : \
	!strcasecmp(c,"color") ? WT_COLOR : \
	!strcasecmp(c,"color_transition") ? WT_COLOR_TRANSITION : \
	!strcasecmp(c,"rainbow") ? WT_RAINBOW : \
	!strcasecmp(c,"sparkle") ? WT_SPARKLE : WT_UNKNOWN \
)

typedef enum {
	ET_NONE, // nothing to do
	ET_WAIT, // do nothing
	ET_SPEED, // set speed
	ET_SPEEDUP, // set acceleration
	ET_BOUNCE, // change direction speed=-speed
	ET_REVERSE, // change delta_pos to -delta_pos
	ET_JUMP, // jump to position (relative to scene)
	ET_CLEAR,  // clear pixels
	ET_SET_BRIGHTNESS,
	ET_SET_BRIGHTNESS_DELTA
} event_type;

// *** what will happen
typedef struct EVT_WHAT_COLORTRANSITION {
	T_COLOR_HSV hsv_from;
	T_COLOR_HSV hsv_to;
} T_EVT_WHAT_COLORTRANSITION;

typedef struct EVT_WHAT {
	int32_t id;
	what_type type;
	int32_t pos; // relative start position
	int32_t len; // relative start length

	union {
		T_COLOR_HSV hsv;  // when only one color is needed
		T_EVT_WHAT_COLORTRANSITION tr; // color transition
	} para;
	struct EVT_WHAT *nxt;
} T_EVT_WHAT;

//  *** when will something happens ***
typedef struct EVT_TIME {
	uint32_t id;
	scene_status_type status;
	event_type type; // what to do
	uint64_t starttime; // when it will start, measured from scene start
	int64_t w_time; // working time, if greater 0 decrement, if ==0 do work

	// what to change when timer arrives
	uint32_t set_flags;
	uint32_t clear_flags;
	double value;

	struct EVT_TIME *nxt; // next event
} T_EVT_TIME;

// where will it happen
typedef struct EVT_WHERE {
	uint32_t id;
	event_type type; // what to do
	double pos;	// at which position
	// TODO which direction?

	struct EVT_WHERE *nxt;
} T_EVT_WHERE;


#define EVFL_WAIT        0x0001 // wait, do not paint something
#define EVFL_CLEARPIXEL  0x0002
#define EVFL_FINISHED    0x0004

typedef struct EVENT{
	uint32_t id; // for reference

	int64_t time; // event time

	uint32_t flags;
	uint32_t w_flags;

	double pos;
	double w_pos;

	double len_factor; // 0..1.0
	double w_len_factor;

	double len_factor_delta;
	double w_len_factor_delta;

	double speed;
	double w_speed;

	double acceleration;
	double w_acceleration;

	double brightness;
	double w_brightness;

	double brightness_delta;
	double w_brightness_delta;

	int32_t delta_pos; // +1 or -1

	// what, example: 10 red pixels, with fade in and fade out
	T_EVT_WHAT *what_list;

	// time dependend events,
	// example: 1.) wait 10 sec, 2.) move with 5 pixel/second
	uint32_t evt_time_list_repeats; // 0=for evener
	uint32_t w_t_repeats;
	T_EVT_TIME *evt_time_list;

	// location based events, example
	// example start at position 30, stop at position 200 with blank
	T_EVT_WHERE *evt_where_list;

	struct EVENT *nxt;
} T_EVENT;

#endif /* MAIN_TIMER_EVENTS_H_ */
