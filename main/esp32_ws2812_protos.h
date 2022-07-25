/*
 * esp32_ws2812_protos.h
 * all prototypes
 *
 *  Created on: 23.07.2022
 *      Author: ankrysm
 */

#ifndef MAIN_ESP32_WS2812_PROTOS_H_
#define MAIN_ESP32_WS2812_PROTOS_H_

// from config.c
esp_err_t store_config();
esp_err_t init_storage();
char *config2txt(char *txt, size_t sz);

// from color.c
void c_hsv2rgb( T_COLOR_HSV *hsv, T_COLOR_RGB *rgb );
void c_checkrgb(T_COLOR_RGB *rgb, T_COLOR_RGB *rgbmin, T_COLOR_RGB *rgbmax);
T_NAMED_RGB_COLOR *color4name(char *name);

// from led_strip.c
esp_err_t strip_init(int numleds);
void strip_set_color(uint32_t start_idx, uint32_t end_idx, uint32_t red, uint32_t green, uint32_t blue);
void strip_set_pixel(uint32_t idx, uint32_t red, uint32_t green, uint32_t blue);
void strip_set_pixel_lvl(uint32_t idx, uint32_t red, uint32_t green, uint32_t blue, double lvl);
void strip_clear();
void strip_rotate(int32_t dir);
void firstled(int red, int green, int blue);
void strip_show();
uint32_t strip_get_numleds();
int strip_initialized();
void strip_set_pixel_rgb(uint32_t idx, T_COLOR_RGB *rgb);
void strip_set_color_rgb(uint32_t start_idx, uint32_t end_idx, T_COLOR_RGB *rgb);

// from timer_events.c
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
esp_err_t decode_effect_rotate(T_MOV_EVENT *evt, int32_t start, uint32_t len, uint64_t dt, int32_t dir, T_COLOR_HSV *bg_hsv);

void calc_pos(T_MOV_EVENT *evt, int32_t *pos, int32_t *delta);

esp_err_t process_move_events(T_EVENT *evt, uint64_t timer_period);
void mov_event2string(T_MOV_EVENT *evt, char *buf, size_t sz_buf);

// from create_demo
void build_demo2(
		T_COLOR_RGB *fg_color // foreground color
);

#endif /* MAIN_ESP32_WS2812_PROTOS_H_ */
