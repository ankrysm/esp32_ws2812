/*
 * move_events.h
 *
 *  Created on: 22.07.2022
 *      Author: ankrysm
 */

#ifndef MAIN_MOVE_EVENTS_H_
#define MAIN_MOVE_EVENTS_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "config.h"
#include "timer_event_types.h"

typedef enum {
	MOV_EVENT_NOTHING, // no move
	MOV_EVENT_ROTATE,
	MOV_EVENT_SHIFT,
	MOV_EVENT_BOUNCE
} mov_event_type;

typedef struct MOV_EVENT {
	mov_event_type type;
	int32_t start; // start position
	uint32_t len;  // length of range (number of pixels)
	uint64_t dt;  // position delta per cycle
	// working data
	scene_status_type w_status;
	int32_t w_pos; // actual position
	uint64_t w_t; // for timing, contains some ms
} T_MOV_EVENT;

esp_err_t decode_effect_no_move(T_MOV_EVENT *evt, int32_t start);
esp_err_t decode_effect_rotate(T_MOV_EVENT *evt, int32_t start, uint32_t len, uint64_t dt);
void calc_pos(T_MOV_EVENT *evt, int32_t *pos, int32_t *delta);
void mov_event2string(T_MOV_EVENT *evt, char *buf, size_t sz_buf);


#endif /* MAIN_MOVE_EVENTS_H_ */
