/*
 * esp32_ws2812_protos.h
 * all prototypes
 *
 *  Created on: 23.07.2022
 *      Author: ankrysm
 */

#ifndef MAIN_ESP32_WS2812_PROTOS_H_
#define MAIN_ESP32_WS2812_PROTOS_H_

// from util.h
int32_t get_random(int32_t min, uint32_t diff);

// from process_events.c
void process_event(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period);
void reset_event( T_EVENT *evt);
void reset_event_repeats(T_EVENT *evt);
void event2text(T_EVENT *evt, char *buf, size_t sz_buf);

// from config.c
esp_err_t store_config();
esp_err_t init_storage();
char *config2txt(char *txt, size_t sz);
esp_err_t storage_info(size_t *total, size_t *used);

// from color.c
void c_hsv2rgb( T_COLOR_HSV *hsv, T_COLOR_RGB *rgb );
void c_checkrgb(T_COLOR_RGB *rgb, T_COLOR_RGB *rgbmin, T_COLOR_RGB *rgbmax);
void c_checkrgb_abs(T_COLOR_RGB *rgb);
T_NAMED_RGB_COLOR *color4name(char *name);

// from led_strip.c
void strip_set_range(int32_t start_idx, int32_t end_idx,  T_COLOR_RGB *rgb);
void strip_set_pixel(int32_t idx, T_COLOR_RGB *rgb);
void strip_clear();
void strip_show();
void firstled(int red, int green, int blue);
size_t get_numleds();

// from led_strip_util.c
void led_strip_init(uint32_t numleds);
void led_strip_refresh() ;
void led_strip_firstled(int red, int green, int blue);


// from timer_events.c
void init_timer_events();
//void set_timer_cycle(int new_delta_ms);
int set_event_timer_period(int new_timer_period);
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
esp_err_t decode_effect_list(char *param, T_EVENT *event );

/*
// from location_based_events.c
esp_err_t decode_effect_solid(T_LOC_EVENT *evt, uint32_t len, T_COLOR_HSV *hsv);
esp_err_t decode_effect_smooth(
		T_LOC_EVENT *evt,
		uint32_t len,
		uint32_t fade_in,
		uint32_t fade_out,
		T_COLOR_HSV *hsv1,
		T_COLOR_HSV *hsv2,
		T_COLOR_HSV *hsv3
);
void loc_event2string(T_LOC_EVENT *evt, char *buf, size_t sz_buf);
esp_err_t process_loc_event(T_EVENT *evt);

// from move_events.c
esp_err_t decode_effect_fix(T_MOV_EVENT *evt, int32_t start);
esp_err_t decode_effect_rotate(
		T_MOV_EVENT *evt,
		int32_t start,
		uint32_t len,
		int32_t start_pos,
		uint64_t dt,
		int32_t dir,
		T_COLOR_HSV *bg_hsv);

void calc_pos(T_MOV_EVENT *evt, int32_t *pos, int32_t *delta);

esp_err_t process_move_events(T_EVENT *evt, uint64_t timer_period);
void mov_event2string(T_MOV_EVENT *evt, char *buf, size_t sz_buf);
*/
// from create_demo
void build_demo2(
		T_COLOR_RGB *fg_color // foreground color
);

#endif /* MAIN_ESP32_WS2812_PROTOS_H_ */
