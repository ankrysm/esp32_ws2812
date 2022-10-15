/*
 * timer_events.h
 *
 *  Created on: 25.06.2022
 *      Author: andreas
 */

#ifndef MAIN_TIMER_EVENTS_H_
#define MAIN_TIMER_EVENTS_H_

#include "color.h"

#define LEN_EVT_MARKER 8+1
#define LEN_EVT_OID 16
#define LEN_EVT_ID 16


typedef enum {
	RES_OK,
	RES_NOT_FOUND,
	RES_NO_VALUE,
	RES_INVALID_DATA_TYPE,
	RES_FINISHED,
	RES_FAILED
} t_result;

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
 * status of an event
 ***********************************************/
typedef enum {
	EVT_STS_READY    = 0x0000, // wait for start
	EVT_STS_RUNNING  = 0x0001, // event is running
	EVT_STS_FINISHED = 0x0002  // event_finished
} event_status_type;

typedef enum {
	OBJT_NOTHING,
	OBJT_CLEAR, // switch of all leds
	OBJT_COLOR,
	OBJT_COLOR_TRANSITION,
	OBJT_RAINBOW,
	OBJT_SPARKLE,
	OBJT_BMP, // handle bitmap file
	OBJT_UNKNOWN
} object_type;

#define TEXT2OBJT(c) ( \
	!strcasecmp(c,"clear") ? OBJT_CLEAR : \
	!strcasecmp(c,"color") ? OBJT_COLOR : \
	!strcasecmp(c,"color_transition") ? OBJT_COLOR_TRANSITION : \
	!strcasecmp(c,"rainbow") ? OBJT_RAINBOW : \
	!strcasecmp(c,"sparkle") ? OBJT_SPARKLE : \
	!strcasecmp(c,"bmp") ? OBJT_BMP : OBJT_UNKNOWN \
)

#define OBJT2TEXT(c) ( \
	c==OBJT_CLEAR ? "clear" : \
	c==OBJT_COLOR ? "color" : \
	c==OBJT_COLOR_TRANSITION ? "color_transition" : \
	c==OBJT_RAINBOW ? "rainbow" : \
	c==OBJT_SPARKLE ? "sparkle" : \
	c==OBJT_BMP ? "bmp" : "unknown" \
)


typedef enum  {
	EVT_PARA_NONE,
	EVT_PARA_TIME,
	EVT_PARA_NUMERIC,
	EVT_PARA_STRING
} event_parameter_type;

// I - init - no timing
// W - at work with timing parameters, s-start, r-running, e-end
// F - finally (all repeats done) - no timing
typedef enum {
	ET_NONE,                 // - - - (-) nothing to do
	ET_WAIT,                 // - W - (time) wait for condition time, condition...
	ET_SPEED,                // I W - (numeric) set speed
	ET_SPEEDUP,              // I W - (numeric) set acceleration
	ET_BOUNCE,               // - W - (-) change direction speed=-speed
	ET_REVERSE,              // - W - (-) change delta_pos to -delta_pos
	ET_GOTO_POS,             // I W - (numeric) goto to position
	ET_MARKER,               // - W - destination marker
	ET_JUMP_MARKER,          // - W - jump to event with the marker
	ET_CLEAR,                // I W F clear pixels
	ET_SET_BRIGHTNESS,       // I W -
	ET_SET_BRIGHTNESS_DELTA, // I W -
	ET_SET_OBJECT,           // I W - oid for object
	ET_BMP_OPEN,             // I W - open internet connection for bmp file
	ET_BMP_READ,             // - W - read bmp data
	ET_BMP_CLOSE,            // - W F close connection for bmp
	ET_UNKNOWN
} event_type;

typedef struct {
	event_type evt_type;
	event_parameter_type evt_para_type;
	char *name;
	char *help;
} T_EVENT_CONFIG;




/*
#define TEXT2ET(c) ( \
	!strcasecmp(c,"wait") ? ET_WAIT : \
	!strcasecmp(c,"speed") ? ET_SPEED : \
	!strcasecmp(c,"speedup") ? ET_SPEEDUP : \
	!strcasecmp(c,"bounce") ? ET_BOUNCE : \
	!strcasecmp(c,"reverse") ? ET_REVERSE : \
	!strcasecmp(c,"goto") ? ET_GOTO_POS : \
	!strcasecmp(c,"marker") ? ET_MARKER : \
	!strcasecmp(c,"jump_marker") ? ET_JUMP_MARKER : \
	!strcasecmp(c,"clear") ? ET_CLEAR : \
	!strcasecmp(c,"brightness") ? ET_SET_BRIGHTNESS : \
	!strcasecmp(c,"brightness_delta") ? ET_SET_BRIGHTNESS_DELTA : \
	!strcasecmp(c,"object") ? ET_SET_OBJECT : \
	!strcasecmp(c,"bmp_open") ? ET_BMP_OPEN : \
	!strcasecmp(c,"bmp_read") ? ET_BMP_READ : \
	!strcasecmp(c,"bmp_close") ? ET_BMP_CLOSE : \
	ET_UNKNOWN \
)

#define ET2TEXT(c) ( \
	c==ET_WAIT ? "wait" : \
	c==ET_SPEED ? "speed" : \
	c==ET_SPEEDUP ? "speedup" : \
	c==ET_BOUNCE ? "bounce" : \
	c==ET_REVERSE ? "reverse" : \
	c==ET_GOTO_POS ? "goto" : \
	c==ET_MARKER ? "marker" : \
	c==ET_JUMP_MARKER ? "jump_marker" : \
	c==ET_CLEAR ? "clear" : \
	c==ET_SET_BRIGHTNESS ? "brightness" : \
	c==ET_SET_BRIGHTNESS_DELTA ? "brightness_delta" : \
	c==ET_SET_OBJECT ? "object" : \
	c==ET_BMP_OPEN ? "bmp_open" : \
	c==ET_BMP_READ ? "bmp_read" : \
	c==ET_BMP_CLOSE ? "bmp_close" : \
	"unknown" \
)
*/

//************* object and event definitions **************

typedef struct OBJECT_COLORTRANSITION {
	T_COLOR_HSV hsv_from;
	T_COLOR_HSV hsv_to;
} T_OBJECT_COLORTRANSITION;

typedef struct OBJECT_DATA {
	int32_t id;
	object_type type;
	//int32_t pos; // relative start position
	int32_t len; // relative start length

	union {
		T_COLOR_HSV hsv;  // when only one color is needed
		T_OBJECT_COLORTRANSITION tr; // color transition
		char *url;
	} para;

	struct OBJECT_DATA *nxt;
} T_OBJECT_DATA;

typedef struct EVT_OBJECT {
	char oid[LEN_EVT_OID];
	T_OBJECT_DATA *data;

	struct EVT_OBJECT *nxt;
} T_EVT_OBJECT;

typedef struct {
	uint64_t time; // initial duration, when 0 execute immediately
	int64_t w_time; // working time, count doun from 'time'
} T_EVENT_PARA_WAIT;

typedef struct EVENT {
	uint32_t id;
	event_type type; // what to do

	//	char marker[LEN_EVT_MARKER]; // destination for jump, value for ET_ JUMP_MARKER
//	uint64_t time; // initial duration, when 0 execute immediately
//	int64_t w_time; // working time, count doun from 'time'
	event_status_type status;

	union {
		T_EVENT_PARA_WAIT wait;
		char svalue[32];
		double value;
	} para;

	struct EVENT *nxt; // next event
} T_EVENT;


typedef enum {
	EVFL_WAIT      = 0x0001, // wait, do not paint something
	EVFL_CLEARPIXEL= 0x0002,
	EVFL_FINISHED  = 0x0004,
	EVFL_BMP_OPEN  = 0x0100,
	EVFL_BMP_READ  = 0x0200, // TO DO really needed?
	EVFL_BMP_CLOSE = 0x0400,
	EVFL_BMP_MASK  = 0x0F00,
	EVFL_UNKNOWN   = 0xFFFF
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
 * events of a scene
 */
typedef struct EVENT_GROUP {
	char id[LEN_EVT_ID];

	event_status_type status;

	int64_t time; // event time
	uint32_t w_flags;
	double w_pos;
	double w_len_factor;
	double w_len_factor_delta;
	double w_speed;
	double w_acceleration;
	double w_brightness;
	double w_brightness_delta;
	int32_t delta_pos; // +1 or -1
	char w_object_oid[LEN_EVT_OID];

	// time dependend events,
	uint32_t t_repeats; // 0=for evener
	uint32_t w_t_repeats;

	T_EVENT *evt_init_list;
	T_EVENT *evt_work_list;
	T_EVENT *evt_final_list;


	struct EVENT_GROUP *nxt;
} T_EVENT_GROUP;

typedef struct SCENE {
	char id[LEN_EVT_ID];
	event_status_type status;

	//uint32_t flags;
	T_EVENT_GROUP *event_group; // actual working event
	T_EVENT_GROUP *event_groups;
	struct SCENE *nxt;
} T_SCENE;


#endif /* MAIN_TIMER_EVENTS_H_ */
