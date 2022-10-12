/*
 * esp32_ws2812_protos.h
 * all prototypes
 *
 *  Created on: 23.07.2022
 *      Author: ankrysm
 */

#ifndef MAIN_ESP32_WS2812_PROTOS_H_
#define MAIN_ESP32_WS2812_PROTOS_H_

// from global_data.c
void global_data_init();

// from process_events.c
void reset_event( T_EVENT_GROUP *evt);
void reset_event_repeats(T_EVENT_GROUP *evt);
void process_scene(T_SCENE *scene, uint64_t scene_time, uint64_t timer_period);
void reset_scene(T_SCENE *scene);

// from json_util.c
t_result evt_get_bool(cJSON *element, char *attr, bool *val, char *errmsg, size_t sz_errmsg);
t_result evt_get_number(cJSON *element, char *attr, double *val, char *errmsg, size_t sz_errmsg);
t_result evt_get_string(cJSON *element, char *attr, char *sval, size_t sz_sval, char *errmsg, size_t sz_errmsg);
t_result evt_get_list(cJSON *element, char *attr, cJSON **found, int *array_size, char *errmsg, size_t sz_errmsg);
t_result decode_json_getcolor_by_name(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg);
t_result decode_json_getcolor_as_hsv(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg);
t_result decode_json_getcolor_as_rgb(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg);
t_result decode_json_getcolor(cJSON *element, char *attr4colorname, char *attr4hsv, char *attr4rgb, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg);
void cJSON_addBoolean(cJSON *element, char *attribute_name, bool flag);


// from config.c
esp_err_t store_config();
esp_err_t load_config();
esp_err_t init_storage();
char *config2txt(char *txt, size_t sz);
esp_err_t storage_info(size_t *total, size_t *used);
void add_base_path(char *filename, size_t sz_filename);
void add_config_informations(cJSON *element);

// from color.c
void c_hsv2rgb(T_COLOR_HSV *hsv, T_COLOR_RGB *rgb);
void c_rgb2hsv(T_COLOR_RGB *rgb, T_COLOR_HSV *hsv);
void c_checkrgb(T_COLOR_RGB *rgb, T_COLOR_RGB *rgbmin, T_COLOR_RGB *rgbmax);
void c_checkrgb_abs(T_COLOR_RGB *rgb);
T_NAMED_RGB_COLOR *color4name(char *name);

// from led_strip.c
void strip_set_range(int32_t start_idx, int32_t end_idx,  T_COLOR_RGB *rgb);
void strip_set_pixel(int32_t idx, T_COLOR_RGB *rgb);
void strip_clear();
void strip_show(bool forced);
void firstled(int red, int green, int blue);

// from led_strip_util.c
void led_strip_init(uint32_t numleds);
void led_strip_refresh() ;
void led_strip_firstled(int red, int green, int blue);


// from timer_events.c
void init_timer_events();
int set_event_timer_period(int new_timer_period);
void scenes_start();
void scenes_autostart();
void scenes_stop();
void scenes_blank();
void scenes_pause();
void scenes_restart();
run_status_type get_scene_status();
run_status_type set_scene_status(run_status_type new_status);
uint64_t get_event_timer_period();
uint64_t get_scene_time();

// from event_util.c
void delete_event(T_EVENT_GROUP *evt);
//esp_err_t event_list_free();
esp_err_t event_list_add(T_SCENE *scene,T_EVENT_GROUP *evt);
esp_err_t obtain_eventlist_lock();
esp_err_t release_eventlist_lock();
void init_eventlist_utils();
T_EVENT_GROUP *create_event(char *id);
T_EVENT_GROUP *find_event(char *id);
T_EVENT *find_timer_event4marker(T_EVENT *tevt_list, char *marker);
T_EVENT *create_timing_event(T_EVENT_GROUP *evt, uint32_t id);
T_EVENT *create_timing_event_init(T_EVENT_GROUP *evt, uint32_t id);
T_EVENT *create_timing_event_final(T_EVENT_GROUP *evt, uint32_t id);
T_SCENE *create_scene(char *id);
void delete_scene(T_SCENE *obj);
esp_err_t scene_list_add(T_SCENE *obj);
esp_err_t scene_list_free();


void delete_object(T_EVT_OBJECT *obj);
esp_err_t delete_object_by_oid(char *oid);
esp_err_t object_list_free();
T_EVT_OBJECT *find_object4oid(char *oid);
T_EVT_OBJECT *create_object(char *oid) ;
T_OBJECT_DATA *create_object_data(T_EVT_OBJECT *obj, uint32_t id);
esp_err_t object_list_add(T_EVT_OBJECT *obj);

// from wifi_vonfig.c
void initialise_wifi();
esp_err_t waitforConnect();
wifi_status_type wifi_connect_status();
char *wifi_connect_status2text(wifi_status_type status);

void init_restservice();
void server_stop();
void initialise_mdns(void);
void initialise_netbios();


// from create_events.c
esp_err_t decode_json4event_root(char *content, char *errmsg, size_t sz_errmsg);
esp_err_t load_events_from_file(char *filename, char *errmsg, size_t sz_errmsg);
esp_err_t store_events_to_file(char *filename, char *content, char *errmsg, size_t sz_errmsg);
esp_err_t load_autostart_file();


// from create_config.c
esp_err_t decode_json4config_root(char *content, char *errmsg, size_t sz_errmsg);

// from bmp.c
void bmp_init();

// from process_bmp.c
esp_err_t bmp_open_connection(char *url);
esp_err_t bmp_read_data();
bool get_is_bmp_reading();
void bmp_stop_processing();

// from create_demo
void build_demo2(
		T_COLOR_RGB *fg_color // foreground color
);

#endif /* MAIN_ESP32_WS2812_PROTOS_H_ */
