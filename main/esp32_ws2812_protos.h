/*
 * esp32_ws2812_protos.h
 * all prototypes
 *
 *  Created on: 23.07.2022
 *      Author: ankrysm
 */

#ifndef MAIN_ESP32_WS2812_PROTOS_H_
#define MAIN_ESP32_WS2812_PROTOS_H_

// from color.c
void c_hsv2rgb(T_COLOR_HSV *hsv, T_COLOR_RGB *rgb);
void c_rgb2hsv(T_COLOR_RGB *rgb, T_COLOR_HSV *hsv);
void c_checkrgb(T_COLOR_RGB *rgb, T_COLOR_RGB *rgbmin, T_COLOR_RGB *rgbmax);
void c_checkrgb_abs(T_COLOR_RGB *rgb);
T_NAMED_RGB_COLOR *color4name(char *name);

// from global_data.c
void global_data_init();
void global_set_extended_log(uint32_t p_extended_log);

// from config.c
esp_err_t store_config();
esp_err_t load_config();
esp_err_t init_storage();
char *config2txt(char *txt, size_t sz);
esp_err_t storage_info(size_t *total, size_t *used);
void add_base_path(char *filename, size_t sz_filename);
void add_config_informations(cJSON *element);
void get_sha256_partition_hashes();

// from create_config.c
esp_err_t decode_json4config_root(char *content, char *errmsg, size_t sz_errmsg);

// from create_events.c
esp_err_t decode_json_list_of_events(cJSON *element, char *errmsg, size_t sz_errmsg);

// from create_objects.c
esp_err_t decode_json4event_object_list(cJSON *element, char *errmsg, size_t sz_errmsg);

// from create_tracks.c
esp_err_t decode_json_list_of_tracks(cJSON *element, char *errmsg, size_t sz_errmsg);

// from decode_json.c
esp_err_t decode_json4event_root(char *content, char *errmsg, size_t sz_errmsg);
esp_err_t load_events_from_file(char *filename, char *errmsg, size_t sz_errmsg);
esp_err_t store_events_to_file(char *filename, char *content, char *errmsg, size_t sz_errmsg);
esp_err_t load_autostart_file();

// from event_util.c
void delete_event_group(T_EVENT_GROUP *evt);
esp_err_t event_list_add(T_EVENT_GROUP *evt);
esp_err_t obtain_eventlist_lock();
esp_err_t release_eventlist_lock();
void init_eventlist_utils();
T_EVENT_GROUP *create_event_group(char *id);
T_EVENT_GROUP *find_event_group(char *id);
T_EVENT *create_event_init(T_EVENT_GROUP *evt, uint32_t id);
T_EVENT *create_event_work(T_EVENT_GROUP *evt, uint32_t id);
T_EVENT *create_event_final(T_EVENT_GROUP *evt, uint32_t id);
T_EVENT_CONFIG *find_event_config(char *name);
void event2text(T_EVENT *evt, char *buf, size_t sz_buf);
char *eventype2text(event_type type);
T_OBJECT_ATTR_CONFIG *object_attr4type_id(int id);
T_OBJECT_ATTR_CONFIG *object_attr4type_name(char *name);
void object_attr_group2text(object_attr_type attr_group, char *text, size_t sz_text);
T_OBJECT_CONFIG *object4type_name(char *name);
char *object_attrtype2text(int id);
T_TRACK_ELEMENT *create_track_element(int tidx, int id);
esp_err_t clear_tracks();
esp_err_t clear_event_group_list();
esp_err_t clear_object_list();
void delete_object(T_DISPLAY_OBJECT *obj);
esp_err_t delete_object_by_oid(char *oid);
T_DISPLAY_OBJECT *find_object4oid(char *oid);
T_DISPLAY_OBJECT *create_object(char *oid) ;
T_DISPLAY_OBJECT_DATA *create_object_data(T_DISPLAY_OBJECT *obj, uint32_t id);
esp_err_t object_list_add(T_DISPLAY_OBJECT *obj);
esp_err_t clear_data(char *msg, size_t sz_msg, run_status_type new_status);

// from json_util.c
t_result evt_get_bool(cJSON *element, char *attr, bool *val, char *errmsg, size_t sz_errmsg);
t_result evt_get_number(cJSON *element, char *attr, double *val, char *errmsg, size_t sz_errmsg);
t_result evt_get_string(cJSON *element, char *attr, char *sval, size_t sz_sval, char *errmsg, size_t sz_errmsg);
t_result evt_get_list(cJSON *element, char *attr, cJSON **found, int *array_size, char *errmsg, size_t sz_errmsg);
t_result decode_json_getcolor_by_name(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg);
t_result decode_json_getcolor_as_hsv(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg);
t_result decode_json_getcolor_as_rgb(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg);
void cJSON_addBoolean(cJSON *element, char *attribute_name, bool flag);
t_result decode_json_getcolor(cJSON *element, object_attr_type attr4colorname, object_attr_type attr4hsv, object_attr_type attr4rgb, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg);
char *object_type2text(object_type type);
t_result decode_object_attr_string(cJSON *element, object_attr_type attr_type, char *sval, size_t sz_sval, char *errmsg, size_t sz_errmsg);
t_result decode_object_attr_numeric(cJSON *element, object_attr_type attr_type, double *val, char *errmsg, size_t sz_errmsg);

// from led_strip.c
void strip_set_range(int32_t start_idx, int32_t end_idx,  T_COLOR_RGB *rgb);
void strip_set_pixel(int32_t idx, T_COLOR_RGB *rgb);
void strip_clear();
void strip_show(bool forced);
void firstled(int red, int green, int blue);


// from process_bmp.c
t_result get_is_bmp_reading(T_TRACK_ELEMENT *ele);
void bmp_stop_processing(T_TRACK_ELEMENT *ele);
t_result bmp_open_url(T_TRACK_ELEMENT *ele);
void process_object_bmp(int32_t pos, T_TRACK_ELEMENT *ele, double brightness);

// from process_events.c
int process_tracks(uint64_t scene_time, uint64_t timer_period);
void reset_tracks();
void process_stop_all_tracks();

// from process_objects.c
void process_object(T_TRACK_ELEMENT *ele);

// from restserver.c
void get_handler_list(httpd_req_t *req);

// from timer_events.c
void init_timer_events();
int set_event_timer_period(int new_timer_period);
void scenes_start();
void scenes_autostart();
void scenes_stop(bool flag_blank);
void scenes_pause();
void scenes_restart();
run_status_type get_scene_status();
run_status_type set_scene_status(run_status_type new_status);
uint64_t get_event_timer_period();
uint64_t get_scene_time();

// from webserver.c
esp_err_t get_handler_html(httpd_req_t *req);

// from wifi_config.c
void initialise_wifi();
esp_err_t waitforConnect();
wifi_status_type wifi_connect_status();
char *wifi_connect_status2text(wifi_status_type status);

// from rest_server_main.c
void init_restservice();
void server_stop();
void initialise_mdns(void);
void initialise_netbios();

// from rest_server_post.c
esp_err_t post_handler_load(httpd_req_t *req, char *content);
esp_err_t post_handler_file_store(httpd_req_t *req, char *content, char *fname, size_t sz_fname);
esp_err_t post_handler_config_set(httpd_req_t *req, char *buf);

// from rest_server_get.c
void get_handler_list_err(httpd_req_t *req);
void get_handler_clear_err(httpd_req_t *req);
void get_handler_list(httpd_req_t *req);
esp_err_t get_handler_file_list(httpd_req_t *req);
void get_handler_status_current(httpd_req_t *req);
void get_handler_scene_new_status(httpd_req_t *req, run_status_type new_status);
esp_err_t get_handler_clear(httpd_req_t *req);
void get_handler_restart(httpd_req_t *req);
void get_handler_reset(httpd_req_t *req);
void get_handler_config(httpd_req_t *req, char *msg);
esp_err_t get_handler_file_load(httpd_req_t *req, char *fname, size_t sz_fname);
esp_err_t get_handler_file_delete(httpd_req_t *req, char *fname, size_t sz_fname);
esp_err_t get_handler_file_get(httpd_req_t *req, char *fname, size_t sz_fname);
esp_err_t get_handler_test(httpd_req_t *req, char *fname, size_t sz_fname);

// from rest_server_ota.c
esp_err_t get_handler_ota_check(httpd_req_t *req);
esp_err_t get_handler_ota_update(httpd_req_t *req);
esp_err_t get_handler_ota_status(httpd_req_t *req);
char *update_status_to_text();

#endif /* MAIN_ESP32_WS2812_PROTOS_H_ */
