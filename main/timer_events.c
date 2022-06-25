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



static esp_timer_handle_t s_periodic_timer;
static EventGroupHandle_t s_timer_event_group;

static const int EVENT_BIT_START = BIT0;
static const int EVENT_BIT_STOP = BIT1;
static const int EVENT_BIT_PAUSE = BIT2;
static const int EVENT_BIT_NEW_SCENE = BIT3;

static const int EVENT_BITS_ALL = EVENT_BIT_START | \
		EVENT_BIT_STOP | \
		EVENT_BIT_PAUSE | \
		EVENT_BIT_NEW_SCENE \
		;

static run_status_type s_run_status = SCENES_STOPPED;
static int64_t s_scene_time = 0;

static void periodic_timer_callback(void* arg)
{
	EventBits_t uxBits = xEventGroupGetBits(s_timer_event_group);
	if ( uxBits & EVENT_BIT_START ) {
		ESP_LOGI(__func__, "new scenes");
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

	if ( s_run_status != SCENES_RUNNING ) {
		// nothing to do at this time
		return;
	}


    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(__func__, "Periodic timer called, time since boot: %lld us", time_since_boot);
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


void init_timer_events(int delta_ms) {
	ESP_LOGI(__func__, "Start, delta=%d ms", delta_ms);

	s_timer_event_group = xEventGroupCreate();
	xEventGroupClearBits(s_timer_event_group, EVENT_BITS_ALL);


    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &s_periodic_timer));

	// start timer, time inm microseconds
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_periodic_timer, delta_ms*1000));
}

