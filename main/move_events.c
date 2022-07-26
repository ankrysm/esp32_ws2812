/*
 * move_events.c
 *
 *  Created on: 22.07.2022
 *      Author: ankrysm
 */
/*
#include "timer_events.h"
#include "led_strip_proto.h"
*/
#include "esp32_ws2812.h"

/**
 * fix position
 */
esp_err_t decode_effect_fix(T_MOV_EVENT *evt, int32_t start) {
	memset(evt, 0, sizeof(T_MOV_EVENT));
	evt->type = MOV_EVENT_FIX;
	evt->start = start;

	evt->w_pos = evt->start;
	evt->w_status = SCENE_IDLE;

	ESP_LOGI(__func__,"start=%d", evt->start);
	return ESP_OK;
}

/**
 * roation
 */
esp_err_t decode_effect_rotate(T_MOV_EVENT *evt, int32_t start, uint32_t len, uint64_t dt, int32_t dir, T_COLOR_HSV *bg_hsv) {
	memset(evt, 0, sizeof(T_MOV_EVENT));
	evt->type = MOV_EVENT_ROTATE;
	evt->start = start;
	evt->len = len;
	evt->dt = dt; // in ms
	evt->dir = dir > 0 ? 1 : -1;
	c_hsv2rgb( bg_hsv, &(evt->bg_rgb));

	evt->w_pos = evt->start;
	evt->w_status = SCENE_IDLE;

	ESP_LOGI(__func__,"start=%d, lden=%d, dt=%lld", evt->start, evt->len, evt->dt);
	return ESP_OK;
}


void mov_event2string(T_MOV_EVENT *evt, char *buf, size_t sz_buf) {
	switch(evt->type) {
	case MOV_EVENT_FIX: // no move
		snprintf(buf, sz_buf, "'fix' start=%d",evt->start);
		break;
	case MOV_EVENT_ROTATE:
		snprintf(buf, sz_buf, "'rotate' start=%d, len=%d, dt=%lld", evt->start, evt->len, evt->dt);
		break;
	case MOV_EVENT_SHIFT:
		snprintf(buf, sz_buf, "'shift' (NYI) start=%d",evt->start);
		break;
	case MOV_EVENT_BOUNCE:
		snprintf(buf, sz_buf, "'bounce' (NYI) start=%d",evt->start);
		break;
	default:
		snprintf(buf, sz_buf,"NYI: %d", evt->type);
	}

}

/*
 * calculates the position according to the moving type
 */
void calc_pos(T_MOV_EVENT *evt, int32_t *pos, int32_t *delta) {
	*pos += *delta;

	switch(evt->type) {
	case MOV_EVENT_FIX: // fix, no move
		break;
	case MOV_EVENT_ROTATE:
		if ( *pos > evt->start + evt->len) {
			*pos = evt->start;
			break;
		}
		if ( *pos < evt->start ) {
			*pos = evt->start + evt->len;
			break;
		}
		break;
	case MOV_EVENT_SHIFT:
		if ( *pos > evt->start + evt->len) {
			*pos = -1;
		}
		if ( *pos < evt->start ) {
			*pos = -1;
		}
		break;
	case MOV_EVENT_BOUNCE:
		if ( *pos > evt->start + evt->len) {
			*delta = - *delta;
			*pos = evt->start + evt->len;
		}
		if ( *pos < evt->start ) {
			*delta = - *delta;
			*pos = evt->start;
		}
		break;
	}
}

static esp_err_t process_effect_fix(T_EVENT *evt, uint64_t timer_period) {
	//ESP_LOGI(__func__,"pos=%d", evt->mov_event.w_pos);
	T_MOV_EVENT *mevt = &(evt->mov_event);
	mevt->w_status = SCENE_UP;
	if ( evt->isdirty) {
		T_COLOR_RGB bk={.r=0,.g=0,.b=0};
		strip_set_color(mevt->start, mevt->start + mevt->len - 1, &bk);
	}
	return ESP_OK;
}

static esp_err_t process_effect_rotate(T_EVENT *evt, uint64_t timer_period) {

	T_MOV_EVENT *mevt = &(evt->mov_event);
	mevt->w_status = SCENE_UP;

	//ESP_LOGI(__func__,"pos=%d", mevt->w_pos);

	mevt->w_t += timer_period;
	if ( mevt->w_t >= mevt->dt) {
		// need new positions
		mevt->w_t = 0;
		evt->isdirty = 1;
		mevt->w_pos += mevt->dir;
		if ( mevt->w_pos < mevt->start) {
			// jump to the end
			mevt->w_pos = mevt->start+mevt->len;
		} else if ( mevt->w_pos > mevt->start+mevt->len) {
			// jump to start
			mevt->w_pos = mevt->start;
		} else {
			// new position is ok
		}
	}
	if ( evt->isdirty) {
		strip_set_color(mevt->start, mevt->start + mevt->len, &(mevt->bg_rgb));
	}
	return ESP_OK;
}

static esp_err_t process_effect_shift(T_EVENT *evt, uint64_t timer_period) {
	ESP_LOGI(__func__,"pos=%d", evt->mov_event.w_pos);
	return ESP_OK;
}

static esp_err_t process_effect_bounce(T_EVENT *evt, uint64_t timer_period) {
	ESP_LOGI(__func__,"pos=%d", evt->mov_event.w_pos);
	return ESP_OK;
}

esp_err_t process_move_events(T_EVENT *evt, uint64_t timer_period) {
	switch(evt->mov_event.type) {
	case MOV_EVENT_FIX: // no move
		return process_effect_fix(evt, timer_period);
		break;
	case MOV_EVENT_ROTATE:
		return process_effect_rotate(evt, timer_period);
		break;
	case MOV_EVENT_SHIFT:
		return process_effect_shift(evt, timer_period);
		break;
	case MOV_EVENT_BOUNCE:
		return process_effect_bounce(evt,timer_period);
		break;
	}
	ESP_LOGW(__func__,"%d NYI", evt->mov_event.type);
	return ESP_FAIL;
}

