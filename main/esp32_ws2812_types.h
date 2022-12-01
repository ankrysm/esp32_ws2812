/*
 * esp32_ws2812_types.h
 *
 *  Created on: 31.10.2022
 *      Author: ankrysm
 */

#ifndef MAIN_ESP32_WS2812_TYPES_H_
#define MAIN_ESP32_WS2812_TYPES_H_

typedef enum {
	RES_OK,
	RES_NOT_FOUND,
	RES_NO_VALUE,
	RES_NO_DATA,
	RES_INVALID_DATA_TYPE,
	RES_OUT_OF_RANGE,
	RES_NOT_ACTIVE,
	RES_FINISHED,
	RES_FAILED
} t_result;


// **** for web/rest-Services

// kind of urls for REST-Service
typedef enum {
	HP_STATUS,
	HP_LIST,
	HP_LIST_ERR,
	HP_FILE_LIST,
	HP_FILE_STORE,
	HP_FILE_GET,
	HP_FILE_LOAD,
	HP_FILE_DELETE,
	HP_CLEAR,
	HP_RUN,
	HP_STOP,
	HP_PAUSE,
	HP_BLANK,
	HP_ASK,
	HP_CONFIG_GET,
	HP_CONFIG_SET,
	HP_RESET,
	HP_CFG_RESET,
	HP_CFG_OTA_CHECK,
	HP_CFG_OTA_UPDATE,
	HP_CFG_OTA_STATUS,
	HP_HELP,
	HP_LOAD,
	HP_CLEAR_ERR,
	HP_TEST,
	// End of list
	HP_END_OF_LIST
} t_http_processing;

// hints for process REST-Requests
typedef enum {
	HPF_PATH_FROM_URL = 0x0001,
	HPF_POST          = 0x0002
} t_http_processing_flag;

// entries for REST service configuration tab
typedef struct {
	char  *path;
	int  flags;
	t_http_processing todo;
	char *help;
} T_HTTP_PROCCESSING_TYPE;

// objects howto show LEDs
typedef enum {
	OBJT_CLEAR, // switch of all leds
	OBJT_COLOR,
	OBJT_COLOR_TRANSITION,
	OBJT_RAINBOW,
	OBJT_BMP, // handle bitmap file
	OBJT_SPARKLE,
	OBJT_EOT
} object_type;

typedef enum {
	OBJATTR_TYPE       = 0x00000001,
	OBJATTR_LEN        = 0x00000002,
	OBJATTR_URL        = 0x00000004,
	OBJATTR_COLOR      = 0x00000008,
	OBJATTR_COLOR_FROM = 0x00000010,
	OBJATTR_COLOR_TO   = 0x00000020,
	OBJATTR_HSV        = 0x00000040,
	OBJATTR_HSV_FROM   = 0x00000080,
	OBJATTR_HSV_TO     = 0x00000100,
	OBJATTR_RGB        = 0x00000200,
	OBJATTR_RGB_FROM   = 0x00000400,
	OBJATTR_RGB_TO     = 0x00000800,
	OBJATTR_EOT        = 0x10000000
} object_attr_type;

#define OBJATTR_GROUP_MANDATORY (OBJATTR_TYPE|OBJATTR_LEN)
#define OBJATTR_GROUP_COLOR (OBJATTR_COLOR|OBJATTR_HSV|OBJATTR_RGB)
#define OBJATTR_GROUP_COLOR_FROM (OBJATTR_COLOR_FROM|OBJATTR_HSV_FROM|OBJATTR_RGB_FROM)
#define OBJATTR_GROUP_COLOR_TO (OBJATTR_COLOR_TO|OBJATTR_HSV_TO|OBJATTR_RGB_TO)


typedef struct {
	object_attr_type type;
	char *name;
	char *help;
} T_OBJECT_ATTR_CONFIG;

typedef struct {
	object_type type;
	char *name;
	object_attr_type attr_group1;
	object_attr_type attr_group2;
	char *help;
} T_OBJECT_CONFIG;



typedef enum  {
	EVT_PARA_NONE     = 0x00,
	EVT_PARA_NUMERIC  = 0x01,
	EVT_PARA_STRING   = 0x02,
	EVT_PARA_OPTIONAL = 0x80  // additional to EVT_PARA...
} event_parameter_type;

// I - init - no timing
// W - at work with timing parameters, s-start, r-running, e-end
// F - finally (all repeats done) - no timing
typedef enum {
	ET_NONE,                 // - - - (-) nothing to do
	ET_WAIT,                 // I W - (numeric) wait n ms
	ET_WAIT_FIRST,           // I - - (numeric) wait n ms only during first init
	ET_PAINT,                // - W - (numeric) paint pixel with the given parameter
	ET_DISTANCE,             // - W - (numeric) paint until has moved n leds
	ET_SPEED,                // I W - (numeric) set speed
	ET_SPEEDUP,              // I W - (numeric) set acceleration
	ET_BOUNCE,               // - W - (-) change direction speed=-speed
	ET_REVERSE,              // - W - (-) change delta_pos to -delta_pos
	ET_GOTO_POS,             // I W - (numeric) goto to position
	ET_CLEAR,                // I W F clear pixels
	ET_SET_BRIGHTNESS,       // I W -
	ET_SET_BRIGHTNESS_DELTA, // I W -
	ET_SET_OBJECT,           // I W - oid for object
	ET_BMP_OPEN,             // I W - open internet connection for bmp file
	ET_BMP_READ,             // - W - (numeric) read n lines from bmp data (-1 = until bmp ends)
	ET_BMP_CLOSE,            // - W F close connection for bmp
	ET_TRESHOLD,             // I W - ignore pixels when rgb < treshold
	ET_UNKNOWN
} event_type;

typedef struct {
	event_type evt_type;
	event_parameter_type evt_para_type;
	char *name;
	char *help;
	char *parahelp;
} T_EVENT_CONFIG;


#endif /* MAIN_ESP32_WS2812_TYPES_H_ */
