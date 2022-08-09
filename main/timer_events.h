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
	RUN_STATUS_IDLE,
	RUN_STATUS_STOPPED,
	RUN_STATUS_RUNNING,
	RUN_STATUS_PAUSED,
	RUN_STATUS_RESTART
} run_status_type;

#define RUN_STATUS_TYPE2TEXT(c) ( \
	c == RUN_STATUS_IDLE    ? "IDLE" : \
	c == RUN_STATUS_STOPPED ? "STOPPED" : \
	c == RUN_STATUS_RUNNING ? "RUNNING" : \
	c == RUN_STATUS_PAUSED  ? "PAUSED" : \
	c == RUN_STATUS_RESTART ? "RESTART" : "???" )


/**********************************************
 * timing status of a single scene
 ***********************************************/
typedef enum {
	SCENE_IDLE,    // befor start time
	SCENE_INITIALIZED, // before start but initialized
	SCENE_STARTED, // just started, ramp up
	SCENE_UP,      // main status
	SCENE_ENDED,   // duration ended, shutdown startet
	SCENE_FINISHED // shutdown ended
} scene_status_type;



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

typedef enum {
	MOVE_IDLE,  // initialization needed
	MOVE_INITIALIZED, // initialized
	MOVE_STARTED,  // increase speed
	MOVE_UP,      // main speed
	MOVE_ENDED,   // duration ended, decrease speed
	MOVE_FINISHED // end speed reached
} move_status_type;



#define EVFL_ISDIRTY 0x0001
#define EVFL_DONE    0x0002
#define EVFL_SP_DONE 0x0004

typedef struct EVENT{
	uint32_t id; // for logging
	// Event Parameter
	// location 
	int32_t pos; // start position on strip, negative values - before the beginning
	
	// moving
	uint64_t sp_t_start; // start time in ms 
	double sp_start; // initial speed in leds per ms
	
	uint64_t sp_dt_startup; // startup acceleration time in ms 
	double sp_acc_startup; // acceleration leds per ms^2 (v=a*t)
	
	uint64_t sp_dt_up; // duration of up phase, 0 = forever
	double sp_acc_up; // acceleration in up phase
	
	uint64_t sp_dt_finish; // duration of up phase, 0 = forever
	double sp_acc_finish; // acceleration in finish phase
	
	// working data
	move_status_type sp_status;
	uint64_t w_sp_t; // time
	//double w_sp_acc;
	double w_sp_speed;
	double w_sp_delta_speed;
	
	// scene timing
	uint64_t t_start; // start time
	uint64_t dt_startup; // time difference for startup
	uint64_t dt_up;  // if 0: for ever
	uint64_t dt_finish;

	//T_LOC_EVENT loc_event; // location based event
	//char *tim_event; // time base event (sets brightness, color)
	//T_MOV_EVENT mov_event; // event moving (sets position, length)

	// working values
	scene_status_type status;
	uint32_t flags;

	uint64_t w_t_start;
	double w_pos; // working position on strip, negative values - before the beginning
	//int32_t len; // length, -1 = until numleds
	uint64_t w_t; // working time

	struct EVENT *nxt;
} T_EVENT;

#endif /* MAIN_TIMER_EVENTS_H_ */
