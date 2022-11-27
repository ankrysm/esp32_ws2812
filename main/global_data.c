/*
 * global_data.c
 *
 *  Created on: 19.09.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812.h"

T_LOG_ENTRY logtable[N_LOG_ENTRIES];
size_t sz_logtable = sizeof(logtable);
int log_write_idx=0;

T_TRACK tracks[N_TRACKS];
size_t sz_tracks = sizeof(tracks);

T_DISPLAY_OBJECT *s_object_list = NULL;
T_EVENT_GROUP *s_event_group_list = NULL;

uint32_t cfg_flags = 0;
uint32_t cfg_trans_flags = 0;
uint32_t cfg_numleds = 60;
uint32_t cfg_cycle = 50;
char *cfg_autoplayfile = NULL;
char *cfg_timezone = NULL;
char *cfg_ota_url = NULL;

char last_loaded_file[LEN_PATH_MAX];
size_t sz_last_loaded_file = sizeof(last_loaded_file);

uint32_t extended_log=0;

void global_set_extended_log(uint32_t p_extended_log) {
	extended_log = p_extended_log;
	bmp_set_extended_log(extended_log);
}

void global_data_init() {
	memset(last_loaded_file, 0, sizeof(last_loaded_file));
	memset(tracks, 0, sizeof(tracks));
	memset(logtable, 0, sizeof(logtable));
	log_write_idx=0;
	global_set_extended_log(0);
}

T_HTTP_PROCCESSING_TYPE http_processing[] = {
		{"/r",         0,                          HP_RUN,         "run"},
		{"/s",         0,                          HP_STOP,        "stop"},
		{"/p",         0,                          HP_PAUSE,       "pause"},
		{"/b",         0,                          HP_BLANK,       "stop and blank strip"},
		{"/i",         0,                          HP_ASK,         "run status"},
		{"/sts",       0,                          HP_STATUS,      "status info"},
		{"/cl",        0,                          HP_CLEAR,       "clear event list"},
		{"/li",        0,                          HP_LIST,        "list events"},
		{"/err",       0,                          HP_LIST_ERR,    "list last errors"},
		{"/clerr",     0,                          HP_CLEAR_ERR,   "clear last errors"},
		{"/lo",        HPF_POST,                   HP_LOAD,        "load events, replaces data in memory"},
		{"/f/list",    0,                          HP_FILE_LIST,   "list stored files"},
		{"/f/store/",  HPF_PATH_FROM_URL|HPF_POST, HP_FILE_STORE,  "store JSON event lists into flash memory as &quot;fname&quot;"},
		{"/f/get/",    HPF_PATH_FROM_URL,          HP_FILE_GET,    "get content of stored file &quot;fname&quot;"},
		{"/f/load/",   HPF_PATH_FROM_URL,          HP_FILE_LOAD,   "load JSON event list stored as &quot;fname&quot; into memory"},
		{"/f/delete/", HPF_PATH_FROM_URL,          HP_FILE_DELETE, "delete file &quot;fname&quot;"},
		{"/cfg/get",   0,                          HP_CONFIG_GET,  "show config"},
		{"/cfg/set",   HPF_POST,                   HP_CONFIG_SET,  "set config"},
		{"/cfg/restart", 0,                        HP_RESET,       "restart the controller"},
		{"/cfg/ota/check", 0,                      HP_CFG_OTA_CHECK, "check for new firmware" },
		{"/cfg/ota/update", 0,                     HP_CFG_OTA_UPDATE, "ota update" },
		{"/cfg/tabula_rasa", 0,                    HP_CFG_RESET,   "reset all data to default"},
		{"/test/",     HPF_PATH_FROM_URL,          HP_TEST,        "set leds to color, values via path &lt;r&gt;/&lt;g&gt;/&lt;b&gt;/[len]/[pos]"},
		{"/help",      0,                          HP_HELP,        "API help"},
		{"",           0,                          HP_END_OF_LIST, ""}
};

T_EVENT_CONFIG event_config_tab[] = {
		{ET_WAIT, EVT_PARA_NUMERIC, "wait", "wait for n ms", "waiting time in ms"},
		{ET_WAIT_FIRST, EVT_PARA_NUMERIC, "wait_first", "wait for n ms at first init", "waiting time in ms"},
		{ET_PAINT, EVT_PARA_NUMERIC, "paint", "paint leds with the given parameter", "execution time in ms"},
		{ET_DISTANCE, EVT_PARA_NUMERIC, "distance", "paint until object has moved n leds", "number of leds"},
		{ET_SPEED, EVT_PARA_NUMERIC, "speed", "move with given speed", "speed in leds per second"},
		{ET_SPEEDUP, EVT_PARA_NUMERIC, "speedup", "change speed", "delta speed per display cycle"},
		{ET_BOUNCE, EVT_PARA_NONE, "bounce", "reverse speed", ""},
		{ET_REVERSE, EVT_PARA_NONE, "reverse", "reverse paint direction", ""},
		{ET_GOTO_POS, EVT_PARA_NUMERIC, "goto", "go to led position","new position"},
		{ET_CLEAR,EVT_PARA_NONE, "clear", "blank the strip",""},
		{ET_SET_BRIGHTNESS, EVT_PARA_NUMERIC,"brightness", "set brightness","brightness factor 0.0 .. 1.0"},
		{ET_SET_BRIGHTNESS_DELTA, EVT_PARA_NUMERIC,"brightness_delta", "change brightness", "brightness delta per display cycle"},
		{ET_SET_OBJECT, EVT_PARA_STRING, "object","set object to display from object table","object id"},
		{ET_BMP_OPEN, EVT_PARA_NONE, "bmp_open", "open BMP stream, defined by 'bmp' object",""},
		{ET_BMP_READ, EVT_PARA_NUMERIC | EVT_PARA_OPTIONAL, "bmp_read","read BMP data line by line and display it", "execution time in ms, -1 all lines until end (default)"},
		{ET_BMP_CLOSE, EVT_PARA_NONE, "bmp_close", "close BMP stream",""},
		{ET_TRESHOLD, EVT_PARA_NUMERIC, "treshold", "ignore pixel when r,g,b is lower than treshold","treshold 0 .. 255"},
		{ET_NONE, EVT_PARA_NONE, "", "",""} // end of table
};

T_OBJECT_ATTR_CONFIG object_attr_config_tab[] = {
		{OBJATTR_TYPE, "type", "type of an object, mandatory"},
		{OBJATTR_LEN, "len", "numerical attribute, number of leds for show the object, -1 : whole strip, mandatory"},
		{OBJATTR_URL, "url", "url to get data from a web site"},
		{OBJATTR_COLOR,"color", "color as a name"},
		{OBJATTR_HSV, "hsv", "color as hsv values, format: \"h,s,v\""},
		{OBJATTR_RGB, "rgb", "color as rgb values, format: \"r,g,b\""},
		{OBJATTR_COLOR_FROM,"color_from", "start color transition, color name"},
		{OBJATTR_HSV_FROM,"hsv_from", "start of color transition, color as \"h,s,v\""},
		{OBJATTR_RGB_FROM,"rgb_from", "start of color transition, color as \"r,g,b\""},
		{OBJATTR_COLOR_TO,"color_to", "end of color transition, color name"},
		{OBJATTR_HSV_TO,"hsv_to", "end of color transition, color as \"h,s,v\""},
		{OBJATTR_RGB_TO,"rgb_to", "end of color transition, color as \"r,g,b\""},
		{OBJATTR_EOT,"",""}
};

T_OBJECT_CONFIG object_config_tab[] = {
		{OBJT_CLEAR, "clear", 0,0, "clear all pixels in range"},
		{OBJT_COLOR, "color", OBJATTR_GROUP_COLOR,0,"constant color in range"},
		{OBJT_COLOR_TRANSITION, "color_transition", OBJATTR_GROUP_COLOR_FROM, OBJATTR_GROUP_COLOR_TO,"color transition start with ..from and ends with ...to over a led range"},
		{OBJT_RAINBOW,"rainbow", 0,0, "show rainbow colors in range"},
		{OBJT_BMP, "bmp", OBJATTR_URL,0,"bitmap file from a web site" },
		{OBJT_EOT,"", 0, 0, ""}
};

