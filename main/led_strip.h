/*
 * led_strip.h
 *
 *  Created on: 14.06.2022
 *      Author: andreas
 */

#ifndef LED_STRIP_H_
#define LED_STRIP_H_

esp_err_t strip_init(int numleds);
esp_err_t strip_setup(int numleds);
void strip_set_color(uint32_t start_idx, uint32_t end_idx, uint32_t red, uint32_t green, uint32_t blue);
void strip_show();
int strip_initialized();
int strip_numleds();
void strip_clear();
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);

//void strip_rotate(int32_t dir);
//void led_strip_main();



#endif /* LED_STRIP_H_ */
