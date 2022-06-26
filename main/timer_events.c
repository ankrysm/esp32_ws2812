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

static esp_timer_handle_t s_periodic_timer;
static uint64_t s_timer_period; // in ms
static EventGroupHandle_t s_timer_event_group;

static const int EVENT_BIT_START = BIT0;
static const int EVENT_BIT_STOP = BIT1;
static const int EVENT_BIT_PAUSE = BIT2;
static const int EVENT_BIT_NEW_SCENE = BIT3;

static const uint32_t EVENT_BIT_RESET = BIT0;

static const int EVENT_BITS_ALL = EVENT_BIT_START | \
		EVENT_BIT_STOP | \
		EVENT_BIT_PAUSE | \
		EVENT_BIT_NEW_SCENE \
		;

static volatile run_status_type s_run_status = SCENES_NOTHING;
static volatile uint64_t s_scene_time = 0;

static T_EVENT *s_event_list = NULL;

void event_list_free() {
	if ( !s_event_list)
		return;

	T_EVENT *nxt;
	while (s_event_list) {
		nxt = s_event_list->nxt;
//		if ( s_event_list->fade_in) free(s_event_list->fade_in);
//		if ( s_event_list->fade_out) free(s_event_list->fade_out);
		if ( s_event_list->movement) free(s_event_list->movement);
		if ( s_event_list->data) free(s_event_list->data);
		if ( s_event_list->bg_color) free(s_event_list->bg_color);
		free(s_event_list);
		s_event_list = nxt;
	}
	// after this s_event_list is NULL
}

void event_list_add(T_EVENT *evt) {
	if ( s_event_list) {
		// add at the end of the list
		T_EVENT *t;
		for (t=s_event_list; t->nxt; t=t->nxt){}
		evt->lfd = t->lfd +1;
		t->nxt = evt;
	} else {
		// first entry
		evt->lfd = 1;
		s_event_list = evt;
	}
}

static void process_solid(T_EVENT *evt, uint32_t flags) {
	if ( ! evt->data) {
		ESP_LOGE(__func__,"[%d] missing data", evt->lfd);
		return;
	}
	T_SOLID_DATA *d = (T_SOLID_DATA*) evt->data;
	if ( flags & EVENT_BIT_RESET) {
		ESP_LOGI(__func__, "[%d] reset, start %lld, duration %lld", evt->lfd, evt->t_start, evt->duration);
		evt->t = 0;
		d->flags =0;
	}

	if ( s_scene_time <= evt->t_start ) {
		// not yet
		ESP_LOGI(__func__, "[%d] check start %lld <= %lld", evt->lfd, s_scene_time, evt->t_start);
		return;
	}

	if ( d->flags & 2 ) {
		return; // duration ended
	}

	if ( d->flags & 1) {
		// done, check duration
		if ( evt->duration > 0  && evt->t > evt->duration) {
			ESP_LOGI(__func__,"[%d] duration -> stop %lld, %lld", evt->lfd, evt->t, evt->duration);
			strip_set_color(evt->pos, evt->pos+ evt->len, 0, 0, 0);
			d->flags |= 2;
			return;
		}
		ESP_LOGI(__func__,"[%d] duration check %lld, %lld", evt->lfd, evt->t, evt->duration);
		evt->t += s_timer_period;
		return;
	}

	uint32_t pos = evt->pos;
	ESP_LOGI(__func__, "[%d] start at pos %d", evt->lfd, pos);

	int32_t r,g,b,dr,dg,db;
	// inset
	ESP_LOGI(__func__, "[%d] inset %d", evt->lfd, d->inset);
	if ( d->inset > 0 ) {
		double dd = 1.0 / pow(2.0, d->inset);
		r = g = b = 0;
		for (int i=0; i < d->inset; i++) {
			dr = d->fg_color.r * dd;
			dg = d->fg_color.g * dd;
			db = d->fg_color.b * dd;

			r += dr; if ( r > d->fg_color.r ) {r = d->fg_color.r;}
			g += dg; if ( g > d->fg_color.g ) {g = d->fg_color.g;}
			b += db; if ( b > d->fg_color.b ) {b = d->fg_color.b;}
			ESP_LOGI(__func__, "[%d] inset %d/%d, %0.4f RGB=%d,%d,%d", evt->lfd, i, d->inset, dd,r,g,b);

			strip_set_pixel(pos, r, g, b);
			dd = dd*2.0;
			pos++;
		}
	}
	int32_t len = evt->len - d->inset - d->outset;
	if ( len < 0) {
		// outset parameter to big
		len = evt->len - d->inset;
	}
	ESP_LOGI(__func__, "[%d] setcolor %d ... %d", evt->lfd, pos, pos+len);
	strip_set_color(pos, pos+len, d->fg_color.r, d->fg_color.g, d->fg_color.b);
	pos +=len;

	ESP_LOGI(__func__, "[%d] outset %d", evt->lfd, d->outset);
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
			strip_set_pixel(pos, r, g, b);
			ESP_LOGI(__func__, "[%d] outset %d/%d, %0.4f RGB=%d,%d,%d", evt->lfd, i, d->inset, dd,r,g,b);

			pos--;
		}
	}
	ESP_LOGI(__func__, "[%d] done", evt->lfd);

	d->flags |= 1;
	evt->t += s_timer_period;

}


static void periodic_timer_callback(void* arg)
{
	static uint32_t flags= EVENT_BIT_RESET | BIT5;

	if ( flags & BIT5) {
		ESP_LOGI(__func__,"started");
		flags &= ~BIT5;
	}

	EventBits_t uxBits = xEventGroupGetBits(s_timer_event_group);
	if ( uxBits & EVENT_BIT_NEW_SCENE ) {
		ESP_LOGI(__func__, "new scenes NYI");
		// TODO switch to new scenes
	}
	if ( uxBits & EVENT_BIT_START ) {
		if ( s_run_status != SCENES_RUNNING ) {
			ESP_LOGI(__func__, "Start scenes");
			s_run_status = SCENES_RUNNING;
			// TODO do start
		}
	}
	if ( uxBits & EVENT_BIT_STOP ) {
		if ( s_run_status != SCENES_STOPPED ) {
			ESP_LOGI(__func__, "Stop scenes");
			s_run_status = SCENES_STOPPED;
			s_scene_time = 0; // stop resets the time
			flags |= EVENT_BIT_RESET; // set reset bit
		}
	}
	if ( uxBits & EVENT_BIT_PAUSE ) {
		if ( s_run_status != SCENES_PAUSED ) {
			ESP_LOGI(__func__, "Pause scenes");
			s_run_status = SCENES_PAUSED;
			// TODO do pause
		}
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

	/// play scenes
	s_scene_time += s_timer_period;

	for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
		switch(evt->type) {
		case EVT_SOLID:
			process_solid(evt, flags);
			break;
		default:
			ESP_LOGI(__func__,"Event %d NYI", evt->type);
		}
	}

	strip_show();
	flags &= ~EVENT_BIT_RESET; // reset of reset bit


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
	case SCENES_PAUSED: scenes_pause(); break;
	default:
		ESP_LOGE(__func__, "unexpected status %d", new_status);
	}

	return ret;
}

uint64_t get_event_timer_period() {
	return s_timer_period;
//	uint64_t p;
//	// get micro secoinds
//	if (esp_timer_get_period( s_periodic_timer, &p) == ESP_OK ) {
//		return p/1000; // return ms
//	}
//	return 0;
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
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &s_periodic_timer));

	// start timer, time inm microseconds
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_periodic_timer, s_timer_period*1000));
}

