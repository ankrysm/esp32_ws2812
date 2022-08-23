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
	//SCENE_INITIALIZED, // before start but initialized
	//SCENE_STARTED, // just started, ramp up
	SCENE_UP,      // main status
	//SCENE_ENDED,   // duration ended, shutdown startet
	SCENE_FINISHED // shutdown ended
} scene_status_type;
// */


/**********************************************
 * location based events
 *********************************************** /
typedef enum {
	LOC_EVENT_CONSTANT,
	LOC_EVENT_TRANS_LIN, // transition linear
	LOC_EVENT_RAINBOW
} loc_event_type;

//typedef enum {
//	fade_in_exp  = 0x0001,
// fade_out_exp = 0x0002,
//	fade_in_lin  = 0x0004,
//	fade_out_lin = 0x0008
//} loc_event_flags;

typedef struct LOC_EVENT{
	loc_event_type type;
	//loc_event_flags flags;
	uint32_t len; // length of event section
	//uint32_t fade_in;
	//uint32_t fade_out;
	//T_COLOR_RGB rgb1;
	//T_COLOR_RGB rgb2;
	//T_COLOR_RGB rgb3;
	T_COLOR_HSV hsv;
	struc LOC_EVENT *nxt;
} T_LOC_EVENT;
// */

/**********************************************
 * movement events
 *********************************************** /
typedef enum {
	MOV_EVENT_FIX, // no move
	MOV_EVENT_ROTATE,
	MOV_EVENT_SHIFT,
	MOV_EVENT_BOUNCE
} mov_event_type;

typedef struct MOV_EVENT {
	mov_event_type type;
	int32_t start; // start position
	uint32_t len;  // length of range (number of pixels)
	uint64_t dt;  // position delta per cycle
	int32_t dir; // direction+1 or -1
	T_COLOR_RGB bg_rgb;

	// working data
	scene_status_type w_status;
	int32_t w_pos; // actual position
	uint64_t w_t; // for timing, contains some ms
} T_MOV_EVENT;
// */

/*
typedef enum {
	MOVE_IDLE,  // initialization needed
	MOVE_INITIALIZED, // initialized
	MOVE_STARTED,  // increase speed
	MOVE_UP,      // main speed
	MOVE_ENDED,   // duration ended, decrease speed
	MOVE_FINISHED // end speed reached
} move_status_type;
// */

typedef enum {
	WT_NOTHING,
	WT_CLEAR, // switch of all leds
	WT_COLOR,
	WT_COLOR_TRANSITION,
	WT_RAINBOW,
	WT_SPARKLE
} what_type;

typedef enum {
	ET_NONE, // nothing to do
	//ET_STEADY, // continue, nothing changed (speed or color or position)
	//ET_WAIT, // wait a moment
	ET_SPEED, // set speed
	ET_SPEEDUP, // set acceleration
	ET_BOUNCE, // change direction speed=-speed
	ET_REVERSE, // change delta_pos to -delta_pos
	ET_JUMP, // jump to position (relative to scene)
	ET_CLEAR,  // clear pixels
	ET_SET_BRIGHTNESS,
	ET_SET_BRIGHTNESS_DELTA
	//ET_REPEAT, // reset all events in the list
	//ET_GLOW, // speed up, light up (or down)
	//ET_REPEAT, // repeat from an given event id
	//ET_STOP, // finished.
	//ET_STOP_CLEAR // finished and clear when finished
} event_type;

// *** what will happen
typedef struct EVT_WHAT_COLORTRANSITION {
	T_COLOR_HSV hsv_from;
	T_COLOR_HSV hsv_to;
	uint32_t repeats;
} T_EVT_WHAT_COLORTRANSITION;

typedef struct EVT_WHAT {
	int32_t id;
	what_type type;
	int32_t pos; // relative start position
	int32_t len; // relative start length
	//double brightness; // 0..1.0

	union {
		T_COLOR_HSV hsv;  // when only one color is needed
		T_EVT_WHAT_COLORTRANSITION tr; // color transition
	} para;
	struct EVT_WHAT *nxt;
} T_EVT_WHAT;

/*
#define EP_SET_ACCELERATION  0x0001
#define EP_SET_SPEED         0x0002
#define EP_SET_POSITION      0x0004
#define EP_SET_SHRINK_RATE   0x0008
#define EP_SET_LEN           0x0010
#define EP_SET_ID            0x0020
#define EP_SET_BRIGHTNESS     0x0040
#define EP_SET_BRIGHTNESS_CHANGE 0x0080

typedef struct EVT_PARAMETER {
	uint32_t set_flags; //EP_SET-Values
	double acceleration; // sets acceleration
	double speed; // speed in leds per ms or HSV-V_percent per sec
	double position; // jump to position
	double shrink_rate; // in leds per ms
	double len; // sets length
	double brightness; // brightess 0.0 .. 1.0
	double brightness_change; // brightness change in [0.0 .. 1.0] per ms
	int32_t id; // for repeat
} T_EVT_PARAMETER;
*/
//  *** when will something happens ***
typedef struct EVT_TIME {
	uint32_t id;
	scene_status_type status;
	event_type type; // what to do
	uint64_t starttime; // when it will start, measured from scene start
	//uint64_t duration; // when will it finished, go to the next
	int64_t w_time; // working time, if greater 0 decrement, if ==0 do work
	//T_EVT_WHAT *what_list;
	//T_EVT_PARAMETER para;

	// parameter:
	//double acceleration; // acceleration
	//double speed; // speed in leds per ms or HSV-V_percent per sec

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

	//T_EVT_PARAMETER para;
	//T_EVT_WHAT *what_list;



	struct EVT_WHERE *nxt;
} T_EVT_WHERE;


//#define EVFL_ISDIRTY    0x0001
#define EVFL_CLEARPIXEL 0x0002
//#define EVFL_DONE    0x0002
//#define EVFL_SP_DONE 0x0004

#define EVFL_WAIT        0x0001 // wait, do not paint something
#define EVFL_CLEARPIXEL  0x0002
#define EVFL_FINISHED    0x0004
/*
typedef struct EVENT_VARYING_DATA {
	union {
		double value;
		T_COLOR_RGB rgb;
		T_COLOR_HSV hsv;
	};
	union {
		double acceleration; // as start parameter
		double delta; // as working parameter
	};
} T_EVENT_VARYING_DATA;

typedef struct EVENT_DATA{
	T_EVENT_VARYING_DATA pos;
	T_EVENT_VARYING_DATA len;
	T_EVENT_VARYING_DATA speed;
	T_EVENT_VARYING_DATA brightness;

	T_EVT_WHAT *what_list;

} T_EVENT_DATA;
*/

typedef struct EVENT{
	uint32_t id; // for reference
	//T_EVENT_DATA start;
	//T_EVENT_DATA working;

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
	//T_EVT_TIME *w_evt_time; // actual timing event
	uint32_t evt_time_list_repeats; // 0=for evener
	uint32_t w_t_repeats;
	T_EVT_TIME *evt_time_list;

	// location based events, example
	// example start at position 30, stop at position 200 with blank
	//T_EVT_WHERE *w_evt_where;
	T_EVT_WHERE *evt_where_list;

	struct EVENT *nxt;
} T_EVENT;

#endif /* MAIN_TIMER_EVENTS_H_ */
