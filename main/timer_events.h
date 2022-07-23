/*
 * timer_events.h
 *
 *  Created on: 25.06.2022
 *      Author: andreas
 */

#ifndef MAIN_TIMER_EVENTS_H_
#define MAIN_TIMER_EVENTS_H_

#include "timer_event_types.h"
#include "color.h"
//#include "location_based_events.h"
//#include "move_events.h"
/**********************************************
 * status of a single scene
 ***********************************************/
typedef enum {
	SCENE_IDLE,    // befor start time
	SCENE_STARTED, // just started, ramp up
	SCENE_UP,      // main status
	SCENE_ENDED,   // duration ended, shutdown startet
	SCENE_FINISHED // shutdown ended
} scene_status_type;


/**********************************************
 * location based events
 ***********************************************/
typedef enum {
	LOC_EVENT_SOLID,
	LOC_EVENT_SMOOTH
} loc_event_type;

typedef enum {
	fade_in_exp  = 0x0001,
	fade_out_exp = 0x0002,
	fade_in_lin  = 0x0004,
	fade_out_lin = 0x0008
} loc_event_flags;

typedef struct LOC_EVENT{
	loc_event_type type;
	loc_event_flags flags;
	uint32_t len;
	uint32_t fade_in;
	uint32_t fade_out;
	T_COLOR_RGB rgb1;
	T_COLOR_RGB rgb2;
	T_COLOR_RGB rgb3;
} T_LOC_EVENT;


/**********************************************
 * movement events
 ***********************************************/
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





// run status
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



/*
typedef enum {
	AL_LEFT,
	AL_RIGHT,
	AL_CENTER
} alignement_type;
*/

// a process function has changed the display data
//#define EVENT_FLAG_BIT_STRIP_SHOW_NEEDED BIT0
// a process function has cleared the strip data
//#define EVENT_FLAG_BIT_STRIP_CLEARED BIT1

typedef struct EVENT{
	uint32_t lfd; // for logging
	int32_t pos; // start position on strip, negative values - before the beginning
	// text base descriptions:
	T_LOC_EVENT loc_event; // location based event
	char *tim_event; // time base event (sets brightness, color)
	T_MOV_EVENT mov_event; // event moving (sets position, length)

	// working values
	uint8_t isdirty;

	//int32_t w_pos; // working position on strip, negative values - before the beginning
	//int32_t len; // length, -1 = until numleds
	uint64_t w_t; // working time

	struct EVENT *nxt;
} T_EVENT;

/*
// some prototyped
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
esp_err_t event_list_free();
esp_err_t event_list_add(T_EVENT *evt);
esp_err_t obtain_eventlist_lock();
esp_err_t release_eventlist_lock();
void init_eventlist_utils();

esp_err_t process_loc_event(T_EVENT *evt);
esp_err_t process_move_events(T_EVENT *evt, uint64_t timer_period);

*/
#endif /* MAIN_TIMER_EVENTS_H_ */
