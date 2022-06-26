/*
 * local.h
 *
 *  Created on: 10.05.2022
 *      Author: ankrysm
 */

#ifndef MAIN_LOCAL_H_
#define MAIN_LOCAL_H_

// #include "fastled_application.h"
void init_restservice();
void server_stop();
void initialise_mdns(void);
void initialise_netbios();


esp_err_t strip_init(int numleds);
esp_err_t strip_setup(int numleds);
esp_err_t strip_resize(int numleds);
void strip_set_color(uint32_t start_idx, uint32_t end_idx, uint32_t red, uint32_t green, uint32_t blue);
void strip_set_pixel(uint32_t idx, uint32_t red, uint32_t green, uint32_t blue);
void strip_show();
int strip_initialized();
int strip_numleds();
void strip_clear();
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);

//void strip_rotate(int32_t dir);
//void led_strip_main();

#endif /* MAIN_LOCAL_H_ */
