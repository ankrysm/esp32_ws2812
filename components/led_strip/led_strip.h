/*
 * led_strip.h
 *
 *  Created on: 20.09.2022
 *      Author: ankrysm
 */

#ifndef COMPONENTS_LED_STRIP_LED_STRIP_H_
#define COMPONENTS_LED_STRIP_LED_STRIP_H_


void led_strip_init(uint32_t numleds);
void led_strip_refresh();
void led_strip_firstled(int red, int green, int blue);
void led_strip_clear();
void led_strip_set_pixel(int32_t idx, uint8_t r, uint8_t g, uint8_t b);
void led_strip_memcpy(int32_t idx, uint8_t *buf, uint32_t nbuf);

size_t get_numleds();
esp_err_t set_numleds(uint32_t numleds);
uint32_t get_led_strip_data_hash();
bool is_led_strip_initialized();
void led_strip_demo(char *msg);

#endif /* COMPONENTS_LED_STRIP_LED_STRIP_H_ */
