/*
 * timer_events.h
 *
 *  Created on: 25.06.2022
 *      Author: andreas
 */

#ifndef MAIN_TIMER_EVENTS_H_
#define MAIN_TIMER_EVENTS_H_

#include "color.h"
#include "esp32_ws2812_types.h"
#include "process_bmp.h"

#define LEN_EVT_MARKER 8+1
#define LEN_EVT_OID 16
#define LEN_EVT_ID 16



/**********************************************
 * status of a play list
 ***********************************************/
typedef enum {
	RUN_STATUS_NOT_SET,
	RUN_STATUS_STOPPED,
	RUN_STATUS_RUNNING,
	RUN_STATUS_PAUSED,
	RUN_STATUS_STOP_AND_BLANK,
	RUN_STATUS_ASK
} run_status_type;

#define RUN_STATUS_TYPE2TEXT(c) ( \
	c == RUN_STATUS_NOT_SET ? "NOT SET" : \
	c == RUN_STATUS_STOPPED ? "STOPPED" : \
	c == RUN_STATUS_RUNNING ? "RUNNING" : \
	c == RUN_STATUS_PAUSED  ? "PAUSED" : \
	c == RUN_STATUS_STOP_AND_BLANK ? "STOPP AND BLANK" : \
	c == RUN_STATUS_ASK ? "ASK" : \
			"???" )


/**********************************************
 * status of an event
 ***********************************************/
typedef enum {
	EVT_STS_READY,    // ready for start
	EVT_STS_STARTING, // wait for start up, maybe there's a delay
	EVT_STS_RUNNING,  // event is running
	EVT_STS_FINISHED  // event_finished
} event_status_type;


//************* object and event definitions **************

typedef struct OBJECT_COLORTRANSITION {
	T_COLOR_HSV hsv_from;
	T_COLOR_HSV hsv_to;
} T_OBJECT_COLORTRANSITION;

typedef struct OBJECT_BMP {
	char *url;
//	T_BMP_WORKING w_data;
//	T_PROCESS_BMP p_data;
} T_OBJECT_BMP;

typedef struct OBJECT_DATA {
	int32_t id;
	object_type type;
	int32_t len; // relative start length

	union {
		T_COLOR_HSV hsv;  // when only one color is needed
		T_OBJECT_COLORTRANSITION tr; // color transition
		T_OBJECT_BMP bmp; // data for bmp display
	} para;

	struct OBJECT_DATA *nxt;
} T_DISPLAY_OBJECT_DATA;

typedef struct EVT_OBJECT {
	char oid[LEN_EVT_OID];
	T_DISPLAY_OBJECT_DATA *data;

	struct EVT_OBJECT *nxt;
} T_DISPLAY_OBJECT;


typedef struct EVENT {
	uint32_t id;
	event_type type; // what to do
	//event_status_type status;
	union {
		char svalue[32];
		double value;
	} para;

	struct EVENT *nxt; // next event
} T_EVENT;


typedef enum {
	EVFL_WAIT            = 0x0001, // wait, do not paint something
	EVFL_WAIT_FIRST_DONE = 0x0002,
	EVFL_CLEARPIXEL      = 0x0004,
	EVFL_UNKNOWN         = 0xFFFF
} event_flags;

typedef enum {
	PT_INIT,
	PT_WORK,
	PT_FINAL
} t_processing_type;

#define TEXT2EVFL(c) ( \
		!strcasecmp(c,"wait") ? ET_WAIT : \
		!strcasecmp(c,"clearpixel") ? EVFL_CLEARPIXEL : \
		!strcasecmp(c,"finished") ? EVFL_FINISHED : \
		EVFL_UNKNOWN)

/**
 * parts of an events of a scene
 */
typedef struct EVENT_GROUP {
	char id[LEN_EVT_ID];

	T_EVENT *evt_init_list;
	T_EVENT *evt_work_list;
	T_EVENT *evt_final_list;

	struct EVENT_GROUP *nxt;
} T_EVENT_GROUP;

typedef struct {
	T_BMP_WORKING w_data;
	T_PROCESS_BMP p_data;
} T_W_BMP;

typedef struct TRACK_ELEMENT {
	uint32_t id;
	int repeats;
	int64_t time; // event time
	event_status_type status;

	// working data
	int32_t delta_pos; // +1 or -1
	uint32_t w_repeats;
	uint32_t w_flags;
	double w_pos;
	double w_distance;
	double w_len_factor;
	double w_len_factor_delta;
	double w_speed;
	double w_acceleration;
	double w_brightness;
	double w_brightness_delta;
	int64_t w_wait_time;
	//int64_t w_bmp_remaining_lines;
	T_W_BMP *w_bmp;

	/// id of the object to display
	//char w_object_oid[LEN_EVT_OID];
	T_DISPLAY_OBJECT *w_object;

	event_status_type evt_grp_current_status;
	T_EVENT_GROUP *evtgrp;
	T_EVENT *evt_work_current;

	struct TRACK_ELEMENT *nxt;
} T_TRACK_ELEMENT;

typedef struct TRACK {
	int id;
	event_status_type status;
	T_TRACK_ELEMENT *current_element;
	T_TRACK_ELEMENT *element_list;
} T_TRACK;



#endif /* MAIN_TIMER_EVENTS_H_ */
