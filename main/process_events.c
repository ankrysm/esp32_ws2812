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
	/*
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
	*/
}

// time dependend events
// time values in ms
// sets
//  * speed (OK) or
//  * acceleration (OK) or
//  * position or
//  * length or
//  * brightness or
//  * color
void process_event_when(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {
	if (!evt->evt_time_list) {
		return; // no timing events
	}

	uint32_t cnt_evts = 0;
	uint32_t cnt_finished_evts = 0;
	for ( T_EVT_TIME *e = evt->evt_time_list; e; e=e->nxt) {
		cnt_evts++;
		if ( e->w_time >=0 ) {
			int64_t last_w_time = e->w_time;
			e->w_time -= timer_period;
			if ( last_w_time >= 0 && e->w_time < 0 ) {
				// change from >0 to <0, do work
				ESP_LOGI(__func__,"evt.id=%d, tevt.id=%d: timer of %llu ms, type %d, val=%.2f arrived",evt->id, e->id, e->starttime, e->type, e->value);
				cnt_finished_evts++;
				switch(e->type) {
				case ET_SPEED:
					evt->w_speed = e->value;
					break;
				case ET_SPEEDUP:
					evt->w_acceleration = e->value;
					break;
				case ET_BOUNCE:
					evt->w_speed = -evt->w_speed;
					break;
				case ET_CLEAR:
					evt->flags |= EVFL_CLEARPIXEL;
					break;
				case ET_JUMP:
					evt->w_pos = e->value;
					break;
				case ET_REVERSE:
					evt->delta_pos = evt->delta_pos < 0 ? +1 : -1;
					break;
				case ET_SET_BRIGHTNESS:
					evt->w_brightness = e->value;
					break;
				case ET_SET_BRIGHTNESS_DELTA:
					evt->w_brightness_delta = e->value;
					break;
				default:
					ESP_LOGW(__func__,"evt.id=%d, tevt.id=%d: timer of %llu ms, type %d arrived, TYPE UNKNOWN",evt->id, e->id, e->starttime, e->type);
				}
			}
		}
	}

	if ( cnt_evts == cnt_finished_evts ) {
		// all events finished, repeat it?
		if (evt->evt_time_list_repeats > 0 ) {
			if ( evt->w_t_repeats > 0) {
				evt->w_t_repeats--;
			}
		} else {
			evt->w_t_repeats = 1;
		}

		if ( evt->w_t_repeats > 0) {
			// reset event
			evt->w_speed = evt->speed;
			evt->w_acceleration = evt->acceleration;
			// reset all timing vents
			ESP_LOGI(__func__, "evt.id=%d: repeat events (%d/%d)", evt->id, evt->w_t_repeats, evt->evt_time_list_repeats);
			for ( T_EVT_TIME *e = evt->evt_time_list; e; e=e->nxt) {
				e->w_time = e->starttime;
			}
		}
	}

}

// paint the pixel in the calculated range evt->working.pos and evt->working.len.value
void process_event_what(T_EVENT *evt) {

	T_EVT_WHAT *what_list = evt->what_list;
	if ( !what_list || evt->brightness < 0.01 ) {
		return;
	}

	// whole length of all sections
	int32_t len = 0;
	for (T_EVT_WHAT *w = what_list; w; w=w->nxt) {
		len += w->len;
	}
	double flen = len * evt->w_len_factor;

	if ( flen < 1.0 ) {
		return; // nothing left to display
	}

	// display range length:
	//   len/2 - lenf/2 = (len-lenf)/2
	//
	// len   = 10 10   10      11
	// lenf  =  2  3    1      11
	// pos   =  4  3.5  4.5     5.5
	// start =  4  3    4       0
	// end   =  6  6    5      11
	//
	// 0123456789
	// xxxxxxxxxx
	//     XX

	int32_t startpos, endpos;
	if ( evt->delta_pos > 0 ) {
		startpos = evt->w_pos;
		endpos   = evt->w_pos + len;
	} else {
		startpos = evt->w_pos + len;
		endpos   = evt->w_pos;
	}
	double f = evt->w_brightness;

	int32_t dstart = floor(len - flen)/2.0;
	int32_t dend = ceil(dstart + flen);

	int32_t pos = startpos;
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

			if ( pos >=0 && pos < get_numleds() && pos >= dstart && pos <= dend) {
				c_checkrgb_abs(&rgb);
				strip_set_pixel(pos, &rgb);
				evt->flags |= EVFL_ISDIRTY;
			}
			pos += evt->delta_pos;
			if ( pos < startpos || pos > endpos) {
				break; // done.
			}
		}
	}
}




void process_event(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {
	evt->flags = 0;

	//process_event_where(evt,timer_period);
	//process_event_what(evt);

	process_event_when(evt,scene_time, timer_period);
	process_event_what(evt);

	// next timestep
	// calculate speed and length
	// v = a * t
	// Δv = a * Δt
	// speed is leds per ms
	evt->w_speed += evt->w_acceleration;

	evt->w_len_factor += evt->w_len_factor_delta;
	if ( evt->w_len_factor < 0.0 ) {
		evt->w_len_factor = 0.0;
	} else if ( evt->w_len_factor > 1.0 ) {
		evt->w_len_factor = 1.0;
	}

	evt->w_brightness += evt->w_brightness_delta;
	if ( evt->w_brightness < 0.0)
		evt->w_brightness = 0.0;
	else if(evt->w_brightness > 1.0)
		evt->w_brightness = 1.0;

	evt->time += timer_period;
	evt->w_pos += evt->w_speed;

}

// reset sets working->what_list to start->what_list
void reset_event( T_EVENT *evt) {
	evt->w_pos = evt->pos;
	evt->w_len_factor = evt->len_factor;
	evt->w_len_factor_delta = evt->len_factor_delta;
	evt->w_speed = evt->speed;
	evt->w_acceleration = evt->acceleration;
	evt->w_brightness = evt->brightness;
	evt->w_brightness_delta = evt->brightness_delta;
	evt->time = 0;

	for ( T_EVT_TIME *e = evt->evt_time_list; e; e=e->nxt) {
		e->w_time = e->starttime;
	}
	ESP_LOGI(__func__, "reset event %d", evt->id);


}

void event2text(T_EVENT *evt, char *buf, size_t sz_buf) {
	if ( !evt) {
		snprintf(buf, sz_buf, "evt was NULL");
		return;
	}
	snprintf(buf, sz_buf, "Event %d, startpos=%.2f", evt->id, evt->pos);

	if ( evt->what_list) {
		snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),", what=");

		for ( T_EVT_WHAT *w=evt->what_list; w; w=w->nxt) {
			snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),"<id=%d, type=%d, pos=%d, len=%d>",
					w->id, w->type, w->pos, w->len
			);
		}

	} else {
		snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),", no what list");
	}

	if (evt->evt_time_list) {
		snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),", time events:");
		for (T_EVT_TIME *tevt = evt->evt_time_list; tevt; tevt=tevt->nxt) {
			snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf)," [id=%d, starttime=%llu, type=%d, val=%.2f",
					tevt->id, tevt->starttime, tevt->type, tevt->value);


			snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),"]");
		}
	} else {
		snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),", no time events.");
	}
}
