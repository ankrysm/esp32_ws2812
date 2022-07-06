/*
 * timer_events.c
 *
 * handle timer events
 *
 *  Created on: 24.06.2022
 *      Author: ankrysm
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"
#include "timer_events.h"
#include "local.h"
#include "math.h"
#include "led_strip_proto.h"

static esp_timer_handle_t s_periodic_timer;
static uint64_t s_timer_period; // in ms
static EventGroupHandle_t s_timer_event_group;

static const int EVENT_BIT_START = BIT0;
static const int EVENT_BIT_STOP = BIT1;
static const int EVENT_BIT_PAUSE = BIT2;
static const int EVENT_BIT_NEW_SCENE = BIT3;
static const int EVENT_BIT_RESTART = BIT4;

// processing flags
//static const uint32_t EVENT_BIT_STRIP_SHOW_NEEDED = BIT0;
//static const uint32_t EVENT_BIT_END_OF_SCENE = BIT1;


static const int EVENT_BITS_ALL = EVENT_BIT_START | \
		EVENT_BIT_STOP | \
		EVENT_BIT_PAUSE | \
		EVENT_BIT_NEW_SCENE | \
		EVENT_BIT_RESTART \
		;

static volatile run_status_type s_run_status = SCENES_NOTHING;
static volatile uint64_t s_scene_time = 0;

T_EVENT *s_event_list = NULL;



/*
static void process_solid_set_pixel(T_EVENT *evt, double lvl) {
	T_SOLID_DATA *d = (T_SOLID_DATA*) evt->data;
	int32_t evt_pos, evt_len;

	real_pos_len(evt, &evt_pos, &evt_len);

	check_clear_strip(evt);

	if ( evt_len < 1) {
		return; // nothing visible
	}

	int32_t r,g,b,dr,dg,db;
	uint32_t pos = evt_pos;
	double f = lvl < 0.0 ? 0.0 : lvl > 1.0 ? 1.0 : lvl;

	if ( d->inset > 0 ) {
		// better to use a quadratic funkction instead of linear
		double dd = 1.0 / pow(2.0, d->inset);
		r = g = b = 0;
		for (int i=0; i < d->inset; i++) {
			dr = d->fg_color.r * dd;
			dg = d->fg_color.g * dd;
			db = d->fg_color.b * dd;

			r += dr; if ( r > d->fg_color.r ) {r = d->fg_color.r;}
			g += dg; if ( g > d->fg_color.g ) {g = d->fg_color.g;}
			b += db; if ( b > d->fg_color.b ) {b = d->fg_color.b;}

			strip_set_pixel_lvl(pos, r, g, b, f);
			dd = dd*2.0;
			pos++;
		}
	}
	int32_t len = evt_len - d->inset - d->outset;
	if ( len < 0) {
		// outset parameter to big
		len = evt->len - d->inset;
	}

	r = d->fg_color.r * f;
	g = d->fg_color.g * f;
	b = d->fg_color.b * f;
	strip_set_color(pos, pos+len, r, g, b);
	pos +=len;

	if ( d->outset > 0 ) {
		double dd = 1.0 / pow(2.0, d->outset);
		r = g = b = 0;

		pos = evt->pos + evt->len;
		for (int i=0; i < d->inset; i++) {
			dr = d->fg_color.r * dd;
			dg = d->fg_color.g * dd;
			db = d->fg_color.b * dd;

			r += dr; if ( r > d->fg_color.r ) {r = d->fg_color.r;}
			g += dg; if ( g > d->fg_color.g ) {g = d->fg_color.g;}
			b += db; if ( b > d->fg_color.b ) {b = d->fg_color.b;}
			strip_set_pixel_lvl(pos, r, g, b, f);
			//ESP_LOGI(__func__, "[%d] outset %d/%d, %0.4f RGB=%d,%d,%d", evt->lfd, i, d->inset, dd,r,g,b);
			pos--;
		}
	}
	evt->w_flags |= EVENT_FLAG_BIT_STRIP_SHOW_NEEDED;

}
*/

static void process_solid(T_EVENT *evt) {
	//T_SOLID_DATA *d = (T_SOLID_DATA*) evt->data;
	/*
	double p;

	switch (evt->status) {
	case SCENE_IDLE:
		if ( s_scene_time <= evt->t_start ) {
			// not yet
			//ESP_LOGI(__func__, "[%d] check start %lld <= %lld", evt->lfd, s_scene_time, evt->t_start);
			break;
		}
		ESP_LOGI(__func__, "[%d] started", evt->lfd);
		evt->w_t = 0;
		if ( evt->t_fade_in > 0 ) {
			//have to fade in
			evt->status = SCENE_STARTED;
		} else {
			evt->status = SCENE_UP;
			process_solid_set_pixel(evt, 1.0);
		}
		break;

	case SCENE_STARTED:
		p = 1.0 * evt->w_t / evt->t_fade_in;
		process_solid_set_pixel(evt, p);
		if ( p > 1.0 ) {
			evt->status = SCENE_UP;
			break;
		}
		break;
	case SCENE_UP:
		if ( evt->w_t < evt->duration) {
			// not yet
			break;
		}
		ESP_LOGI(__func__,"[%d] duration -> stop %lld, %lld", evt->lfd, evt->w_t, evt->duration);
		evt->w_t = 0;
		if ( evt->t_fade_out > 0 ) {
			evt->status = SCENE_ENDED;
		} else {
			evt->status = SCENE_FINISHED;
		}
		break;
	case SCENE_ENDED:
		p = 1.0 - (1.0 * evt->w_t / evt->t_fade_out);
		process_solid_set_pixel(evt, p);
		if ( p < 0.0 ) {
			evt->status = SCENE_FINISHED;
			strip_set_color(evt->pos, evt->pos+ evt->len, 0, 0, 0);
			evt->w_flags |= EVENT_FLAG_BIT_STRIP_SHOW_NEEDED;

			ESP_LOGI(__func__, "[%d] done", evt->lfd);
			break;
		}
		break;
	case SCENE_FINISHED:
		break;
	}
	evt->w_t += s_timer_period;
*/
}

static void process_blank(T_EVENT *evt) {
	/*

	int32_t evt_len, evt_pos, evt_end;
	switch (evt->status) {
	case SCENE_IDLE:
		if ( s_scene_time <= evt->t_start ) {
			// not yet
			//ESP_LOGI(__func__, "[%d] check start %lld <= %lld", evt->lfd, s_scene_time, evt->t_start);
			break;
		}
		// scene starts
		check_clear_strip(evt);
		real_pos_len(evt, &evt_pos, &evt_len);
		if ( evt_len > 0) {
			strip_set_color(evt->pos, evt_len, 0, 0, 0);
			evt->w_flags |= EVENT_FLAG_BIT_STRIP_SHOW_NEEDED;
		}
		evt->status = SCENE_UP;
		ESP_LOGI(__func__, "[%d] started", evt->lfd);
		break;
	case SCENE_STARTED:
		// do movement
		break;
	case SCENE_UP:
		if ( evt->w_t < evt->duration) {
			// not yet
			break;
		}
		ESP_LOGI(__func__,"[%d] duration -> stop %lld, %lld", evt->lfd, evt->w_t, evt->duration);
		//evt->w_flags |= EVENT_FLAG_BIT_STRIP_SHOW_NEEDED;
		evt->status = SCENE_FINISHED;
		ESP_LOGI(__func__, "[%d] done", evt->lfd);
		break;
	case SCENE_ENDED:
		// nothing to do
		break;
	case SCENE_FINISHED:
		// nothing to do
		break;
	}
	evt->w_t += s_timer_period;
*/

}

/**
 * main timer function
 */
static void periodic_timer_callback(void* arg)
{
	static uint32_t flags= BIT5; // Bit 5 for initial logging

	if ( flags & BIT5) {
		ESP_LOGI(__func__,"started");
		flags &= ~BIT5;
	}
	int do_reset = false;

	EventBits_t uxBits = xEventGroupGetBits(s_timer_event_group);
	if ( uxBits & EVENT_BIT_NEW_SCENE ) {
		ESP_LOGI(__func__, "new scenes NYI");
		// TODO switch to new scenes
	}
	if ( uxBits & EVENT_BIT_START ) {
		if ( s_run_status != SCENES_RUNNING ) {
			ESP_LOGI(__func__, "Start scenes");
			s_run_status = SCENES_RUNNING;
			do_reset = true;
		}
	}
	if ( uxBits & EVENT_BIT_STOP ) {
		if ( s_run_status != SCENES_STOPPED ) {
			ESP_LOGI(__func__, "Stop scenes");
			s_run_status = SCENES_STOPPED;
			s_scene_time = 0; // stop resets the time
			//flags |= EVENT_BIT_RESET; // set reset bit
		}
	}
	if ( uxBits & EVENT_BIT_PAUSE ) {
		if ( s_run_status != SCENES_PAUSED ) {
			ESP_LOGI(__func__, "Pause scenes");
			s_run_status = SCENES_PAUSED;
			// TODO do pause
		}
	}

	if ( uxBits & EVENT_BIT_RESTART ) {
			ESP_LOGI(__func__, "Restart scenes");
			s_run_status = SCENES_RUNNING;
			s_scene_time = 0; // stop resets the time
			do_reset = true;
	}

	xEventGroupClearBits(s_timer_event_group, EVENT_BITS_ALL);

	if ( !s_event_list) {
		//ESP_LOGI(__func__,"event list is empty");
		s_run_status = SCENES_NOTHING;
		s_scene_time = 0;
		return;
	}

	if ( s_run_status != SCENES_RUNNING ) {
		// nothing to do at this time
		return;
	}

	if ( do_reset) {
		// reset all events
		for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
			evt->w_flags = 0;
			evt->w_t = 0;
			evt->status = SCENE_IDLE;
			evt->w_pos = evt->pos;
			evt->w_len = evt->len > 0 ? evt->len : strip_get_numleds();
		}
	}

	/// play scenes
	s_scene_time += s_timer_period;
	int n_paint=0;
	int n_cleared = 0;
	for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
		evt->w_flags &= ~ EVENT_FLAG_BIT_STRIP_SHOW_NEEDED;
		evt->w_flags |= n_cleared;
		switch(evt->type) {
		case EVT_SOLID:
			process_solid(evt);
			break;
		case EVT_BLANK:
			process_blank(evt);
			break;
		default:
			ESP_LOGI(__func__,"Event %d NYI", evt->type);
		}
		n_paint |= evt->w_flags & EVENT_FLAG_BIT_STRIP_SHOW_NEEDED ? 1 : 0;
		n_cleared |= evt->w_flags & EVENT_FLAG_BIT_STRIP_CLEARED ? 1 : 0;

	}
	if (n_paint ) {
		ESP_LOGI(__func__, "strip_show");
		strip_show();
	}

    //int64_t time_since_boot = esp_timer_get_time();
    //ESP_LOGI(__func__, "Periodic timer called, time since boot: %lld us", time_since_boot);
}

void scenes_start() {
	ESP_LOGI(__func__, "start");
    xEventGroupSetBits(s_timer_event_group, EVENT_BIT_START);
}

void scenes_stop() {
	ESP_LOGI(__func__, "start");
    xEventGroupSetBits(s_timer_event_group, EVENT_BIT_STOP);
}

void scenes_pause() {
	ESP_LOGI(__func__, "start");
    xEventGroupSetBits(s_timer_event_group, EVENT_BIT_PAUSE);
}

void scenes_restart() {
	ESP_LOGI(__func__, "restart");
    xEventGroupSetBits(s_timer_event_group, EVENT_BIT_RESTART);
}

run_status_type get_scene_status() {
	return s_run_status;
}

uint64_t get_scene_time() {
	return s_scene_time;
}

run_status_type set_scene_status(run_status_type new_status) {
	ESP_LOGI(__func__, "start %d",new_status);
	run_status_type ret = s_run_status;
	switch(new_status) {
	case SCENES_STOPPED: scenes_stop(); break;
	case SCENES_RUNNING: scenes_start(); break;
	case SCENES_PAUSED:  scenes_pause(); break;
	case SCENES_RESTART: scenes_restart(); break;
	default:
		ESP_LOGE(__func__, "unexpected status %d", new_status);
	}

	return ret;
}

uint64_t get_event_timer_period() {
	return s_timer_period;
}

void set_timer_cycle(int new_delta_ms) {
	ESP_LOGI(__func__, "started, new delta=%d", new_delta_ms);
	esp_timer_stop(s_periodic_timer);
	s_timer_period = new_delta_ms;
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_periodic_timer, s_timer_period*1000));

}

void init_timer_events(int delta_ms) {
	ESP_LOGI(__func__, "Start, delta=%d ms", delta_ms);

	s_timer_period = delta_ms;
	s_timer_event_group = xEventGroupCreate();
	xEventGroupClearBits(s_timer_event_group, EVENT_BITS_ALL);


    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            // name is optional, but may help identify the timer when debugging
            .name = "periodic"
    };

	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &s_periodic_timer));

	// start timer, time in microseconds
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_periodic_timer, s_timer_period*1000));
}

