/*
 * led_strip.h
 *
 *  Created on: 06.07.2022
 *      Author: andreas
 */

#ifndef MAIN_LED_STRIP_H_
#define MAIN_LED_STRIP_H_

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
esp_err_t strip_init(int numleds);
esp_err_t strip_resize(int numleds);
esp_err_t strip_setup(int numleds);
void strip_set_color(uint32_t start_idx, uint32_t end_idx, uint32_t red, uint32_t green, uint32_t blue);
void strip_set_pixel(uint32_t idx, uint32_t red, uint32_t green, uint32_t blue);
void strip_set_pixel_lvl(uint32_t idx, uint32_t red, uint32_t green, uint32_t blue, double lvl);
void strip_clear();
void strip_rotate(int32_t dir);
void firstled(int red, int green, int blue);
void strip_show();
uint32_t strip_get_numleds();
int strip_initialized();


#endif /* MAIN_LED_STRIP_H_ */
