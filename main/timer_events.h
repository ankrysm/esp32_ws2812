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
 * timing status of a single scene
 ***********************************************/
typedef enum {
	TE_STS_WAIT_FOR_START = 0x0000, // wait for start
	TE_STS_RUNNING        = 0x0001, // timer is running, wait for expire
	TE_STS_FINISHED       = 0x0002 // timer expired
} timer_event_status_type;

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

// I - init - no timing
// W - at work with timing parameters, s-start, r-running, e-end
// F - finally (all repeats done) - no timing
typedef enum {
	ET_NONE,                 // - -- - nothing to do
	ET_PAUSE,                // - Wsr - do nothing
	ET_GO_ON,                // - Wr - continue with actual parameter
	ET_SPEED,                // I Ws - set speed
	ET_SPEEDUP,              // I Ws - set acceleration
	ET_BOUNCE,               // - We - change direction speed=-speed
	ET_REVERSE,              // - We - change delta_pos to -delta_pos
	ET_GOTO_POS,             // I We - goto to position
	ET_JUMP_MARKER,          // - We - jump to event with the marker
	ET_CLEAR,                // I Wr F clear pixels
	ET_STOP,                 // - We - end of event
	ET_SET_BRIGHTNESS,       // I Ws F
	ET_SET_BRIGHTNESS_DELTA, // I Ws -
	ET_SET_OBJECT,           // I We - oid for object
	ET_BMP_OPEN,             // I Ws - open internet connection for bmp file
	ET_BMP_READ,             // - Wr - read bmp data
	ET_BMP_CLOSE,            // - We F close connection for bmp
	ET_UNKNOWN
} event_type;


#define TEXT2ET(c) ( \
	!strcasecmp(c,"pause") ? ET_PAUSE : \
	!strcasecmp(c,"go") ? ET_GO_ON : \
	!strcasecmp(c,"speed") ? ET_SPEED : \
	!strcasecmp(c,"speedup") ? ET_SPEEDUP : \
	!strcasecmp(c,"bounce") ? ET_BOUNCE : \
	!strcasecmp(c,"reverse") ? ET_REVERSE : \
	!strcasecmp(c,"goto") ? ET_GOTO_POS : \
	!strcasecmp(c,"jump_marker") ? ET_JUMP_MARKER : \
	!strcasecmp(c,"stop") ? ET_STOP : \
	!strcasecmp(c,"clear") ? ET_CLEAR : \
	!strcasecmp(c,"brightness") ? ET_SET_BRIGHTNESS : \
	!strcasecmp(c,"brightness_delta") ? ET_SET_BRIGHTNESS_DELTA : \
	!strcasecmp(c,"object") ? ET_SET_OBJECT : \
	!strcasecmp(c,"bmp_open") ? ET_BMP_OPEN : \
	!strcasecmp(c,"bmp_read") ? ET_BMP_READ : \
	!strcasecmp(c,"bmp_clos") ? ET_BMP_CLOSE : \
	ET_UNKNOWN \
)

#define ET2TEXT(c) ( \
	c==ET_PAUSE ? "pause" : \
	c==ET_GO_ON ? "go" : \
	c==ET_SPEED ? "speed" : \
	c==ET_SPEEDUP ? "speedup" : \
	c==ET_BOUNCE ? "bounce" : \
	c==ET_REVERSE ? "reverse" : \
	c==ET_GOTO_POS ? "goto" : \
	c==ET_JUMP_MARKER ? "jump_marker" : \
	c==ET_STOP ? "stop" : \
	c==ET_CLEAR ? "clear" : \
	c==ET_SET_BRIGHTNESS ? "brightness" : \
	c==ET_SET_BRIGHTNESS_DELTA ? "brightness_delta" : \
	c==ET_SET_OBJECT ? "object" : \
	c==ET_BMP_OPEN ? "bmp_open" : \
	c==ET_BMP_READ ? "bmp_read" : \
	c==ET_BMP_CLOSE ? "bmp_close" : \
	"unknown" \
)

//************* object and event definitions **************

typedef struct OBJECT_COLORTRANSITION {
	T_COLOR_HSV hsv_from;
	T_COLOR_HSV hsv_to;
} T_OBJECT_COLORTRANSITION;

typedef struct OBJECT_BMP {
	bool is_open;
} T_OBJECT_BMP;

typedef struct OBJECT_DATA {
	int32_t id;
	object_type type;
	int32_t pos; // relative start position
	int32_t len; // relative start length

	union {
		T_COLOR_HSV hsv;  // when only one color is needed
		T_OBJECT_COLORTRANSITION tr; // color transition
		T_OBJECT_BMP bmp; // BMP handling
	} para;

	struct OBJECT_DATA *nxt;
} T_OBJECT_DATA;

typedef struct EVT_OBJECT {
	char oid[LEN_EVT_OID];
	T_OBJECT_DATA *data;

	struct EVT_OBJECT *nxt;
} T_EVT_OBJECT;

//  *** when will something happens ***
typedef struct EVENT {
	uint32_t id;
	event_type type; // what to do
	char marker[LEN_EVT_MARKER]; // destination for jump, value for ET_ JUMP_MARKER
	uint64_t time; // initial duration, when 0 execute immediately
	int64_t w_time; // working time, count doun from 'time'
	timer_event_status_type status;

	char svalue[32];
	double value;

	struct EVENT *nxt; // next event
} T_EVENT;

// where will it happen
typedef struct EVT_WHERE {
	uint32_t id;
	event_type type; // what to do
	double pos;	// at which position
	// TODO which direction?

	struct EVT_WHERE *nxt;
} T_EVT_WHERE;

typedef enum {
	EVFL_WAIT      =  0x0001, // wait, do not paint something
	EVFL_CLEARPIXEL=  0x0002,
	EVFL_FINISHED  =  0x0004,
	EVFL_UNKNOWN   =  0xFFFF
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

	T_EVENT *evt_time_init_list;
	T_EVENT *evt_time_list;
	T_EVENT *evt_time_final_list;

	// location based events, example
	//T_EVT_WHERE *evt_where_list;

	struct EVENT_GROUP *nxt;
} T_EVENT_GROUP;

typedef struct SCENE {
	char id[LEN_EVT_ID];
	uint32_t flags;
	T_EVENT_GROUP *event; // actual working event
	T_EVENT_GROUP *events;
	struct SCENE *nxt;
} T_SCENE;


#endif /* MAIN_TIMER_EVENTS_H_ */
