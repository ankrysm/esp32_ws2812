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
#include "location_based_events.h"
#include "move_events.h"

/*
// art of scene
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
*/

// run status
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




/*
typedef enum {
	TR_LINEAR,
	TR_EXPONENTIAL
} transition_type;
*/

/*
typedef enum {
	AL_LEFT,
	AL_RIGHT,
	AL_CENTER
} alignement_type;
*/

/*
typedef enum {
	MV_ONE_DIRECTION,
	MV_ROTATE,
	MV_BOUNCE
} movement_type;
**/

/*
// scene parameter (location or time based)
typedef struct SCENE_PARAMETER{
	int32_t v_start; // start in %
	int32_t v_end;  // end in %
	int32_t length; // in ms or %
	transition_type type_t; // 0=linear, 1=exponential
	alignement_type type_a;
	movement_type type_m;
	//
	struct SCENE_PARAMETER *nxt;
} T_SCENE_PARAMETER;
*/

/*
typedef struct SCENE_PARAMETER_LIST {
	int32_t pos; // range start
	int32_t len; // range width
	int32_t repeat; // <0: endless, ==0 once, >0 number of repeats / at the end go to the nxt timing paramter
	T_SCENE_PARAMETER *sp_list;
} T_SCENE_PARAMETER_LIST;
*/
// a process function has changed the display data
#define EVENT_FLAG_BIT_STRIP_SHOW_NEEDED BIT0
// a process function has cleared the strip data
#define EVENT_FLAG_BIT_STRIP_CLEARED BIT1

/*
typedef struct EVENT{
	strip_event_type type;
	scene_status_type status;
	uint32_t lfd; // for logging
	int32_t pos; // start position on strip, negative values - before the beginning
	int32_t len; // length, -1 = until numleds
	T_COLOR_RGB bg_color; // assume black when NULL
	T_COLOR_RGB fg_color; // assume white when NULL

	// location based parameter
	T_SCENE_PARAMETER_LIST *l_brightness; // brightness parameter in location %
	T_SCENE_PARAMETER_LIST *l_color;  // color paramter in location %

	// time based parameter
	T_SCENE_PARAMETER_LIST *t_moving;
	T_SCENE_PARAMETER_LIST *t_brightness;
	T_SCENE_PARAMETER_LIST *t_color;
	T_SCENE_PARAMETER_LIST *t_size;

	void *data; // special data

	uint64_t w_t; // time from last status change

	// location working data
	int32_t w_pos;
	uint32_t w_len;
	uint32_t w_flags;
	int32_t w_repeats;
	struct EVENT *nxt;
} T_EVENT;
*/
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


#endif /* MAIN_TIMER_EVENTS_H_ */
