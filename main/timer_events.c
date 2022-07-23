/*
 * timer_events.c
 *
 * handle timer events
 *
 *  Created on: 24.06.2022
 *      Author: ankrysm
 */

/*
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
*/

#include "esp32_ws2812.h"

extern T_CONFIG gConfig;

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

static volatile run_status_type s_run_status = RUN_STATUS_IDLE;
static volatile uint64_t s_scene_time = 0;

T_EVENT *s_event_list = NULL;


static void reset_event( T_EVENT *evt) {
	evt->isdirty = 1;
	evt->w_t = 0;
	evt->mov_event.w_pos = evt->mov_event.start;
	evt->mov_event.w_status = SCENE_IDLE;
	evt->mov_event.w_t = 0;
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
		if ( s_run_status != RUN_STATUS_RUNNING ) {
			ESP_LOGI(__func__, "Start scenes");
			s_run_status = RUN_STATUS_RUNNING;
			do_reset = true;
		}
	}
	if ( uxBits & EVENT_BIT_STOP ) {
		if ( s_run_status != RUN_STATUS_STOPPED ) {
			ESP_LOGI(__func__, "Stop scenes");
			s_run_status = RUN_STATUS_STOPPED;
			s_scene_time = 0; // stop resets the time
			//flags |= EVENT_BIT_RESET; // set reset bit
		}
	}
	if ( uxBits & EVENT_BIT_PAUSE ) {
		if ( s_run_status != RUN_STATUS_PAUSED ) {
			ESP_LOGI(__func__, "Pause scenes");
			s_run_status = RUN_STATUS_PAUSED;
			// TODO do pause
		}
	}

	if ( uxBits & EVENT_BIT_RESTART ) {
			ESP_LOGI(__func__, "Restart scenes");
			s_run_status = RUN_STATUS_RUNNING;
			s_scene_time = 0; // stop resets the time
			do_reset = true;
	}

	xEventGroupClearBits(s_timer_event_group, EVENT_BITS_ALL);

	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock on eventlist");
		return;
	}
	if ( !s_event_list) {
		//ESP_LOGI(__func__,"event list is empty");
		// clear the strip except first led
		strip_set_color(1, gConfig.numleds - 1, 0, 0, 0);
		strip_show();
		s_run_status = RUN_STATUS_IDLE;
		s_scene_time = 0;
		release_eventlist_lock();
		return;
	}

	if ( s_run_status != RUN_STATUS_RUNNING ) {
		// nothing to do at this time
		release_eventlist_lock();
		return;
	}

	if ( do_reset) {
		// reset all events, set isdirty on true;
		for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
			reset_event(evt);
		}
	}

	/// play scenes
	s_scene_time += s_timer_period;
	int n_paint=0;
	// first check: is something to paint?
	for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
		// first: move
		process_move_events(evt,s_timer_period);
		if ( evt->isdirty) {
			n_paint++;
			evt->isdirty=0;
		}
		// next: time events
		// TODO
	}

	if ( n_paint > 0) {
		// i have something to paint
		for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
			process_loc_event(evt);
		}
		//ESP_LOGI(__func__, "strip_show");
		strip_show();
	}
	release_eventlist_lock();

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
	case RUN_STATUS_STOPPED: scenes_stop(); break;
	case RUN_STATUS_RUNNING: scenes_start(); break;
	case RUN_STATUS_PAUSED:  scenes_pause(); break;
	case RUN_STATUS_RESTART: scenes_restart(); break;
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

	// initialize eventlist handling
	init_eventlist_utils();

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

