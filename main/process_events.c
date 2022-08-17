/*
 * process_events.c
 *
 *  Created on: 09.08.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

T_EVT_WHAT event_clear = {
		.id=-1,
		.type=WT_CLEAR,
		.pos=0,
		.len=-1,
		.para.hsv={.h=0,.s=0,.v=0},
		.nxt=NULL
	};


// **************************************************************************
// timing
// **************************************************************************
/*
static void event_init(T_EVENT *evt) {
	ESP_LOGI(__func__,"event %d: init", evt->id);
	evt->w_t_start = evt->t_start;
	evt->w_t = 0;
	evt->status = SCENE_INITIALIZED;
	evt->flags = 0;

	// TODO more initialisations
}

static void event_check4start(T_EVENT *evt, uint64_t scene_time) {
	if ( scene_time < evt->w_t_start) {
		evt->flags |= EVFL_DONE;
		return; // not yet
	}
	ESP_LOGI(__func__,"event %d: started", evt->id);
	evt->status = SCENE_STARTED;
	evt->w_t = 0;
}


static void event_check4up(T_EVENT *evt, uint64_t timer_period) {
	if (evt->dt_startup > timer_period ) {
		evt->flags |= EVFL_DONE; // every time it is done
		if ( evt->w_t < evt->dt_startup ) {
			// paint "STARTUP" scenario
			// TODO

			return;
		}
		ESP_LOGI(__func__,"event %d: startup ended", evt->id);
	} else {
		ESP_LOGI(__func__,"event %d: no startup", evt->id);
	}
	// startup has ended
	evt->status = SCENE_UP;
	evt->w_t = 0;

}

static void event_check4end(T_EVENT *evt, uint64_t timer_period) {

	if ( evt->dt_up > timer_period) {
		evt->flags |= EVFL_DONE; // every time it is done
		if ( evt->dt_up == 0 || evt->w_t < evt->dt_up ) {
			// for ever or end of up time not reached
			// paint up scenario
			// TODO

			return;
		}
		ESP_LOGI(__func__,"event %d: up ended", evt->id);
	} else {
		ESP_LOGI(__func__,"event %d: no up", evt->id);
	}

	// up has ended
	evt->status = SCENE_ENDED;
	evt->w_t = 0;

}

static void event_check4finish(T_EVENT *evt, uint64_t timer_period) {
	if ( evt->dt_finish > timer_period) {
		evt->flags |= EVFL_DONE; // every time it is done
		if ( evt->w_t < evt->dt_finish ) {
			// paint finish scenario
			// TODO

			return;
		}
		ESP_LOGI(__func__,"event %d: finish ended", evt->id);
	} else {
		ESP_LOGI(__func__,"event %d: no finish", evt->id);
	}
	// up has ended
	evt->status = SCENE_FINISHED;
	evt->w_t = 0;

}


static void event_finished(T_EVENT *evt) {
	// TODO
	// if nothing planned for the future stay in this status
	evt->flags |= EVFL_DONE;
}
*/

// **************************************************************************
// moving
// **************************************************************************

/*
static void event_sp_init(T_EVENT *evt) {
	ESP_LOGI(__func__,"event %d: init", evt->id);
	evt->w_pos = evt->pos;
	evt->sp_status = MOVE_INITIALIZED;
	evt->w_sp_speed = evt->sp_start;
	evt->w_sp_t = 0;
	evt->w_sp_delta_speed = 0.0;
}

static void event_sp_check4start(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {
	if ( scene_time < evt->sp_t_start) {
		evt->flags |= EVFL_SP_DONE;
		return; // not yet
	}
	ESP_LOGI(__func__,"event %d: started", evt->id);
	evt->sp_status = MOVE_STARTED;
	evt->w_sp_delta_speed = evt->sp_acc_startup * timer_period;
	evt->w_sp_t = 0;
}


static void event_sp_check4up(T_EVENT *evt, uint64_t timer_period) {
	if (evt->sp_dt_startup > timer_period ) {
		evt->flags |= EVFL_SP_DONE; // every time it is done
		if ( evt->w_sp_t < evt->sp_dt_startup ) {
			return;
		}
		ESP_LOGI(__func__,"event %d: startup ended", evt->id);
	} else {
		ESP_LOGI(__func__,"event %d: no startup", evt->id);
	}
	// startup has ended
	evt->sp_status = MOVE_UP;
	evt->w_sp_delta_speed = evt->sp_acc_up * timer_period;
	evt->w_sp_t = 0;

}

static void event_sp_check4end(T_EVENT *evt, uint64_t timer_period) {

	if ( evt->sp_dt_up > timer_period) {
		evt->flags |= EVFL_SP_DONE; // every time it is done
		if ( evt->sp_dt_up == 0 || evt->w_sp_t < evt->sp_dt_up ) {
			return;
		}
		ESP_LOGI(__func__,"event %d: up ended", evt->id);
	} else {
		ESP_LOGI(__func__,"event %d: no up", evt->id);
	}

	// up has ended
	evt->sp_status = MOVE_ENDED;
	evt->w_sp_t = 0;
	evt->w_sp_delta_speed = evt->sp_acc_finish * timer_period;

}

static void event_sp_check4finish(T_EVENT *evt, uint64_t timer_period) {
	if ( evt->sp_dt_finish > timer_period) {
		evt->flags |= EVFL_SP_DONE; // every time it is done
		if ( evt->w_sp_t < evt->sp_dt_finish ) {
			return;
		}
		ESP_LOGI(__func__,"event %d: finish ended", evt->id);
	} else {
		ESP_LOGI(__func__,"event %d: no finish", evt->id);
	}
	// up has ended
	evt->sp_status = MOVE_FINISHED;
	evt->w_sp_t = 0;
	evt->w_sp_delta_speed = 0.0;
}


static void event_sp_finished(T_EVENT *evt) {
	// TODO
	// if nothing planned for the future stay in this status
	evt->flags |= EVFL_SP_DONE;
}
*/
// **************************************************************************

// *** main function ******
// times in ms,
// scene_time useful for start a scene
// timer_period useful for startup and shutdown
/*void process_event(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {

	evt->flags &= ~(EVFL_DONE | EVFL_SP_DONE | EVFL_ISDIRTY);

	// speed
	while ( !(evt->flags & EVFL_SP_DONE)) {
		switch ( evt->sp_status ) {
		case MOVE_IDLE:  // initialization needed
			event_sp_init(evt);
			break;
		case MOVE_INITIALIZED: // check if start speed up has to start
			event_sp_check4start(evt, scene_time, timer_period);
			break;
		case MOVE_STARTED:  // check if up speed up has to start
			event_sp_check4up(evt, timer_period);
			break;
		case MOVE_UP:      // check if end speed up has to start
			event_sp_check4end(evt, timer_period);
			break;
		case MOVE_ENDED:   // check if end speed up has finished
			event_sp_check4finish(evt, timer_period);
			break;
		case MOVE_FINISHED: // check if it stays finished or restart moving
			event_sp_finished(evt);
			break;
		default:
			ESP_LOGE(__func__, "unexpected speed status %d", evt->sp_status);
		}
	}

	// v = a * t
	// Δv = a * Δt
	// speed is leds per ms
	evt->w_sp_speed += evt->w_sp_delta_speed;
	evt->w_pos += evt->w_sp_speed;

	// timing
	while ( !(evt->flags & EVFL_DONE)) {
		// check the timing status of a scene
		switch ( evt->status) {
		case SCENE_IDLE: // it has to be initialized
			event_init(evt);
			break;
		case SCENE_INITIALIZED: // check if it has to start
			event_check4start(evt, scene_time);
			break;
		case SCENE_STARTED: // check if it is up
			event_check4up(evt, timer_period);
			break;
		case SCENE_UP: // check if it has ended
			event_check4end(evt, timer_period);
			break;
		case SCENE_ENDED: // check if it has finished
			event_check4finish(evt, timer_period);
			break;
		case SCENE_FINISHED: // check if it stays finished or restart the scene
			event_finished(evt);
			break;
		default:
			ESP_LOGE(__func__, "unexpected status %d", evt->status);
		}
	}
	evt->w_t += timer_period;
	evt->w_sp_t += timer_period;
}
*/
// #######################################################################################
// #######################################################################################
// #######################################################################################


// position dependend events
// sets
//  * speed or
//  * acceleration or
//  * position or
//  * length or
//  * brightness or
//  * color
void process_event_where(T_EVENT *evt, uint64_t timer_period) {
	T_EVT_WHERE *wevt = evt->w_evt_where;
	//T_EVT_WHAT *what_list = evt->working.what_list;
	if ( !wevt ){
		return; // no changes
	}
	// TODO: check a range: is it near the trigger point, which direction
	if (evt->working.pos.value < wevt->pos ) {
		return; // not here
	}

	evt->working.speed.value = 0.0;
	evt->working.speed.delta = 0.0;
	evt->working.len.delta = 0.0;
	evt->working.what_list = NULL;

	T_EVT_WHERE *nxt = wevt->nxt;
	evt->w_evt_where = nxt;

	if (nxt) {
		if ( nxt->para.set_flags & EP_SET_ACCELERATION )
			evt->working.speed.delta = nxt->para.acceleration * timer_period;

		if ( nxt->para.set_flags & EP_SET_SPEED )
			evt->working.speed.value = nxt->para.speed;

		if ( nxt->para.set_flags & EP_SET_POSITION )
			evt->working.pos.value = nxt->para.position;

		if ( nxt->para.set_flags & EP_SET_SHRINK_RATE )
			evt->working.len.delta = nxt->para.shrink_rate * timer_period;

		if ( nxt->para.set_flags & EP_SET_LEN )
			evt->working.len.value = nxt->para.len;

		if (nxt->what_list ) {
			evt->working.what_list = nxt->what_list;
		}
	}

	switch (wevt->type) {
	case ET_REPEAT:
		// default: from the beginning, if id is specified, find this event
		evt->w_evt_where = evt->evt_where_list;
		if ( nxt->para.set_flags & EP_SET_ID ) {
			// search for id
			for (T_EVT_WHERE *e = evt->evt_where_list; e; e=e->nxt) {
				if ( e->id == nxt->para.id) {
					evt->w_evt_where = e;
					break;
				}
			}
		}
		break;
	case ET_BOUNCE:
		evt->working.speed.delta = -evt->working.speed.delta;
		break;

	case ET_STOP: // check if end position reached, leave lights on
		// position reached
		evt->working.speed.value = 0.0;
		evt->working.speed.delta = 0.0;
		evt->working.len.delta = 0.0;
		// finished. should be last event in list, next location based event is: nothing
		evt->w_evt_where = NULL;
		break;
	case ET_STOP_CLEAR: // check if end position reached, clears range
		// position reached
		evt->working.speed.value = 0.0;
		evt->working.speed.delta = 0.0;
		evt->working.len.delta = 0.0;
		// finished. should be last event in list, next location based event is: nothing
		evt->w_evt_where = NULL;
		evt->working.what_list = &event_clear;
		break;
	default:
		ESP_LOGW(__func__,"event type %d NYI", wevt->type);
	}
}

// time dependend events
// time values in ms
// sets
//  * speed or
//  * acceleration or
//  * position or
//  * length or
//  * brightness or
//  * color
void process_event_when(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {
	T_EVT_TIME *tevt= evt->w_evt_time;
	if (!tevt) {
		return; // no changes
	}

	switch(tevt->status) {
	case SCENE_IDLE:
		if (evt->time < tevt->starttime) {
			return; // not yet
		}
		tevt->status = SCENE_UP;
		tevt->w_time = 0;

		// initialize working data
		memset(&(evt->working), 0, sizeof(T_EVENT_DATA));

		// what to change in working data depend onb the next timing event
		if ( tevt->para.set_flags & EP_SET_ACCELERATION )
			evt->working.speed.delta = tevt->para.acceleration * timer_period;

		if ( tevt->para.set_flags & EP_SET_SPEED )
			evt->working.speed.value = tevt->para.speed;

		if ( tevt->para.set_flags & EP_SET_POSITION )
			evt->working.pos.value = tevt->para.position;

		if ( tevt->para.set_flags & EP_SET_SHRINK_RATE )
			evt->working.len.delta = tevt->para.shrink_rate * timer_period;

		if ( tevt->para.set_flags & EP_SET_LEN )
			evt->working.len.value = tevt->para.len;

		if ( tevt->para.set_flags & EP_SET_BRIGHTNESS )
			evt->working.brightness.value = tevt->para.brightness;

		if ( tevt->para.set_flags & EP_SET_BRIGHTNESS_CHANGE )
			evt->working.brightness.delta = tevt->para.brightness_change * timer_period;

		if (tevt->what_list ) {
			evt->working.what_list = tevt->what_list;
		}

		// if too short immediately to finished (examples: STOP- REPEAT-Events)
		if (tevt->duration < timer_period) {
			tevt->status = SCENE_FINISHED;
			// next event
			evt->w_evt_time = tevt->nxt;
		}
		break;
	case SCENE_UP:
		if ( tevt->w_time < tevt->duration ) {
			break;
		}
		tevt->status = SCENE_FINISHED;
		// next event
		evt->w_evt_time = tevt->nxt;
		break;
	case SCENE_FINISHED:
		return;
	}

	switch (tevt->type) {
	case ET_REPEAT:
		// default: from the beginning, if id is specified, find this event
		evt->w_evt_time = evt->evt_time_list;
		if ( tevt->para.set_flags & EP_SET_ID ) {
			// search for id
			for (T_EVT_TIME *e = evt->evt_time_list; e; e=e->nxt) {
				if ( e->id == tevt->para.id) {
					// found
					evt->w_evt_time = e;
					break;
				}
			}
		}
		// set all events in list to IDLE
		for (T_EVT_TIME *e = evt->w_evt_time; e; e=e->nxt) {
			e->status = SCENE_IDLE;
		}
		break;

	case ET_BOUNCE:
		evt->working.speed.delta = -evt->working.speed.delta;
		break;

	case ET_STOP: // check if end position reached, leave lights on
		// position reached
		// finished. should be last event in list, next location based event is: nothing
		evt->w_evt_time = NULL;
		break;
	case ET_STOP_CLEAR: // check if end position reached, clears range
		// position reached
		// finished. should be last event in list, next location based event is: nothing
		evt->w_evt_time = NULL;
		evt->working.what_list = &event_clear;
		break;
	default:
		ESP_LOGW(__func__,"event type %d NYI", tevt->type);
	}

}

// paint the pixel in the calculated range evt->working.pos and evt->working.len.value
void process_event_what(T_EVENT *evt) {

	T_EVT_WHAT *what_list = evt->working.what_list;
	if ( !what_list) {
		return;
	}
	int32_t startpos, endpos, len;
	int32_t delta_pos = evt->working.pos.delta > 0.0 ? 1 : -1;
	len = evt->working.len.value;
	int32_t pos = evt->working.pos.value;
	if ( delta_pos > 0 ) {
		startpos = pos;
		endpos = pos+len;
	} else {
		startpos = pos+len;
		endpos = pos;
	}
	double f = evt->working.brightness.value;

	pos = startpos;
	T_COLOR_RGB rgb = {.r=0,.g=0,.b=0};
	T_COLOR_RGB rgb2 = {.r=0,.g=0,.b=0};

	double r,g,b;

	// for color transition
	double df,dr,dg,db;

	for (T_EVT_WHAT *w = what_list; w; w=w->nxt) {
		for ( int len=0; len < w->len; len++) {
			switch (w->type) {
			case WT_COLOR:
				if ( len == 0 ) {
					// initialize
					c_hsv2rgb(&(w->para.hsv), &rgb);
					r = f*rgb.r;
					g = f*rgb.g;
					b = f*rgb.b;
					rgb.r = r;
					rgb.g = g;
					rgb.r = r;
				}
				break;
			case WT_COLOR_TRANSITION: // linear from one color to another
				// if lvl2-lvl1 = 100 % and len = 4
				// use 25% 50% 75% 100%, not start with 0%
				if (len ==0) {
					df = 1.0 / w->len;
					c_hsv2rgb(&(w->para.tr.hsv_from), &rgb);
					c_hsv2rgb(&(w->para.tr.hsv_to), &rgb2);
					dr = 1.0 *(rgb2.r - rgb.r) * df;
					dg = 1.0 *(rgb2.g - rgb.g) * df;
					db = 1.0 *(rgb2.b - rgb.b) * df;
					r=rgb.r;
					g=rgb.g;
					b=rgb.b;
				}
				r+=dr;
				g+=dg;
				b+=db;
				rgb.r = f * r;
				rgb.g = f * g;
				rgb.r = f * r;
				break;

			case WT_RAINBOW:
				break;

			case WT_SPARKLE:
				break;

			default:
				ESP_LOGW(__func__,"what type %d NYI", w->type);
			}

			if ( pos >=0 && pos <get_numleds()) {
				c_checkrgb_abs(&rgb);
				strip_set_pixel(pos, &rgb);
				evt->flags |= EVFL_ISDIRTY;
			}
			pos += delta_pos;
			if ( pos < startpos || pos > endpos) {
				break; // done.
			}
		}
	}
}




void process_event(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {
	process_event_where(evt,timer_period);
	process_event_what(evt);
	process_event_when(evt,scene_time, timer_period);
	process_event_what(evt);

	// next timestep
	// calculate speed and length
	// v = a * t
	// Δv = a * Δt
	// speed is leds per ms
	evt->working.speed.value += evt->working.speed.delta;
	evt->working.len.value += evt->working.len.delta;

	evt->working.brightness.value += evt->working.brightness.delta;
	if ( evt->working.brightness.value < 0.0)
		evt->working.brightness.value = 0.0;
	else if(evt->working.brightness.value > 1.0)
		evt->working.brightness.value = 1.0;

	evt->time += timer_period;
	evt->working.pos.value += evt->working.speed.value;

}

// reset sets working->what_list to start->what_list
void reset_event( T_EVENT *evt) {
	//evt->status = SCENE_IDLE;
	//evt->sp_status = MOVE_IDLE;
	evt->w_evt_where = evt->evt_where_list;
	evt->w_evt_time = evt->evt_time_list;
	memcpy(&(evt->working), &(evt->start), sizeof(T_EVENT_DATA));
	if (!evt->working.what_list) {
		evt->working.what_list = &event_clear;
	}
	evt->time = 0;

}

