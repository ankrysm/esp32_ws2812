/*
 * timer_events.c
 *
 * handle timer events
 *
 *  Created on: 24.06.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812.h"

int s_timer_period = 100; // in ms
int s_timer_period_new = 100; // in ms

static esp_timer_handle_t s_periodic_timer;
static EventGroupHandle_t s_timer_event_group;

static const int EVENT_BIT_START = BIT0;
static const int EVENT_BIT_STOP = BIT1;
static const int EVENT_BIT_PAUSE = BIT2;
//static const int EVENT_BIT_NEW_SCENE = BIT3;
//static const int EVENT_BIT_RESTART = BIT4;

static const int EVENT_BITS_ALL = 0xFF;


static volatile run_status_type s_run_status = RUN_STATUS_STOPPED;
static volatile uint64_t s_scene_time = 0;

T_EVENT *s_event_list = NULL;
extern T_CONFIG gConfig;

static void show_status() {
	if ( gConfig.flags & CFG_SHOW_STATUS) {
		if (gConfig.flags & CFG_WITH_WIFI) {
			// green
			firstled(0,32,0);
		} else {
			// red
			firstled(32,0,0);
		}
	}
}

static int logcnt=0;
static int64_t t_sum=0;
static void periodic_timer_callback(void* arg);

// callback with timing
static void periodic_timer_callback_t(void* arg) {

	int64_t t_start = esp_timer_get_time();
	periodic_timer_callback(arg);
	int64_t t_end = esp_timer_get_time();

	int64_t dt=(t_end - t_start);
	t_sum+=dt;
	logcnt++;
	if ( logcnt >= 50 ) {
		//ESP_LOGI(__func__,"processing time %.3f mikrosec on core %d", (1.0*t_sum/logcnt), xPortGetCoreID());
		//ESP_LOGI(__func__,"raw data %lld, %d, %lld", dt, logcnt, t_sum);
		logcnt = 0;
		t_sum=0;
	}
}


/**
 * main timer function
 */
static void periodic_timer_callback(void* arg)
{
/*	static uint32_t flags= BIT5; // Bit 5 for initial logging

	if ( flags & BIT5) {
		ESP_LOGI(__func__,"started");
		flags &= ~BIT5;
	}
*/
	int do_reset = false;

	// discover stauts of playback
	EventBits_t uxBits = xEventGroupGetBits(s_timer_event_group);

	if ( uxBits & EVENT_BIT_START ) {
		//logcnt = 10;
		switch(s_run_status) {
		case RUN_STATUS_PAUSED:
			ESP_LOGI(__func__, "continue scenes");
			s_run_status = RUN_STATUS_RUNNING;
			break;
		case RUN_STATUS_RUNNING:
			ESP_LOGI(__func__, "already running scenes");
			break;
		default:
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
			// stop this timer
			esp_timer_stop(s_periodic_timer);
			return;
		}
	}

	// set run status to pause
	if ( uxBits & EVENT_BIT_PAUSE ) {
		if ( s_run_status != RUN_STATUS_PAUSED ) {
			ESP_LOGI(__func__, "Pause scenes");
			s_run_status = RUN_STATUS_PAUSED;
			// TODO do pause
		}
	}

	xEventGroupClearBits(s_timer_event_group, EVENT_BITS_ALL);

	if ( s_run_status != RUN_STATUS_RUNNING ) {
		return;
	}

	// here always: ...RUNNING

	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock on eventlist");
		return;
	}

	/*if ( !s_event_list) {
		//ESP_LOGI(__func__,"event list is empty");
		// clear the strip
		uint32_t n = get_numleds();
		ESP_LOGI(__func__,"reset numleds=%u", n);
		T_COLOR_RGB bk={.r=0,.g=0,.b=0};
		strip_set_color(0, n - 1, &bk);
		show_status();
		strip_show();
		s_run_status = RUN_STATUS_IDLE;
		s_scene_time = 0;
		release_eventlist_lock();
		return;
	}*/

	s_scene_time += s_timer_period;
	int n_paint=0;

	// handle reset (first after start)
	if ( do_reset) {
		s_scene_time = 0;
		// reset all event data
		if ( s_event_list) {
			for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
				reset_event(evt);
			}
		} else {
			// clear the strip
			uint32_t n = get_numleds();
			ESP_LOGI(__func__,"reset numleds=%u", n);
			T_COLOR_RGB bk={.r=0,.g=0,.b=0};
			strip_set_color(0, n - 1, &bk);
			n_paint++;
		}
	}

	// paint scenes
	if ( s_event_list) {
		for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
			process_event(evt, s_scene_time, s_timer_period);
			if ( evt->flags & EVFL_ISDIRTY) {
				n_paint++;
			}
		}
	}

	if ( n_paint > 0) {
		//ESP_LOGI(__func__, "strip_show");
		show_status();
		strip_show();
	}

	release_eventlist_lock();

    //int64_t time_since_boot = esp_timer_get_time();
    //ESP_LOGI(__func__, "Periodic timer called, time since boot: %lld us", time_since_boot);
}

// start playing by starting the periodic timer
void scenes_start() {
	ESP_LOGI(__func__, "start, periodic time %d ms(old: %d)", s_timer_period_new, s_timer_period);
	logcnt = 10;

    esp_err_t rc = esp_timer_start_periodic(s_periodic_timer, s_timer_period_new*1000);
    if ( rc == ESP_ERR_INVALID_STATE) {
    	// Timer is running, check for speed change
    	if ( s_timer_period_new != s_timer_period ) {
    		// timer period changed, restart timer
    		esp_timer_stop(s_periodic_timer);
    		rc = esp_timer_start_periodic(s_periodic_timer, s_timer_period_new*1000);
    		if ( rc == ESP_OK) {
    	    	ESP_LOGI(__func__,"Timer restarted");
    	        xEventGroupSetBits(s_timer_event_group, EVENT_BIT_START);
    		} else {
    	    	ESP_LOGE(__func__, "cannot restart timer (rc=%d)", rc);
    		}
    	} else {
        	ESP_LOGI(__func__, "Timer already running");
    	}

    } else if (rc == ESP_OK) {
    	ESP_LOGI(__func__,"Timer started");
        xEventGroupSetBits(s_timer_event_group, EVENT_BIT_START);

    } else {
    	ESP_LOGE(__func__, "cannot start timer (rc=%d)", rc);
    }

    s_timer_period = s_timer_period_new;
}

void scenes_stop() {
	// tiomer stops by themselves
	ESP_LOGI(__func__, "stop");
    xEventGroupSetBits(s_timer_event_group, EVENT_BIT_STOP);
}

void scenes_pause() {
	ESP_LOGI(__func__, "pause");
    xEventGroupSetBits(s_timer_event_group, EVENT_BIT_PAUSE);
}


run_status_type get_scene_status() {
	return s_run_status;
}

uint64_t get_scene_time() {
	return s_scene_time;
}



// sets the new scene status, returnes the old one
run_status_type set_scene_status(run_status_type new_status) {
	ESP_LOGI(__func__, "start %d",new_status);
	run_status_type ret = s_run_status;
	switch(new_status) {
	case RUN_STATUS_STOPPED: scenes_stop(); break;
	case RUN_STATUS_RUNNING: scenes_start(); break;
	case RUN_STATUS_PAUSED:  scenes_pause(); break;
	default:
		ESP_LOGE(__func__, "unexpected status %d", new_status);
	}

	return ret;
}

uint64_t get_event_timer_period() {
	return s_timer_period;
}

// sets new timer period, will be used with a scenes_start-call
int set_event_timer_period(int new_timer_period) {
	ESP_LOGI(__func__, "new cycle time %d, old=%d",new_timer_period, s_timer_period);
	s_timer_period_new = new_timer_period;
	return s_timer_period;
}


void init_timer_events(//int delta_ms
		) {
	//ESP_LOGI(__func__, "Start, delta=%d ms", delta_ms);
	ESP_LOGI(__func__, "Started");

	// initialize eventlist handling
	init_eventlist_utils();

	//s_timer_period = delta_ms;
	s_timer_event_group = xEventGroupCreate();
	xEventGroupClearBits(s_timer_event_group, EVENT_BITS_ALL);

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback_t,
            // name is optional, but may help identify the timer when debugging
            .name = "periodic"
    };

	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &s_periodic_timer));

	// start timer, time in microseconds
 //   ESP_ERROR_CHECK(esp_timer_start_periodic(s_periodic_timer, s_timer_period*1000));
  //  ESP_LOGI(__func__, "Timer started");
//	ESP_LOGI(__func__,"##### NO TIMER STARTED FOR TEST");
}

