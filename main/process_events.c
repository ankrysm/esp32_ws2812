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
//  * speed (ET_SPEED, ET_BOUNCE) or
//  * acceleration (ET_SPEEDUP) or
//  * position (ET_JUMP. ET_REVERSE) or
//  * length or
//  * brightness (ET_BRIGHTNESS, ET_BRIGHTNESS_DELTA) or
//  * color (ET_CLEAR)
// return 1 when all timer events arrived
bool  process_event_when(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {
	if (!evt->evt_time_list) {
		return true; // no timing events
	}

	evt->delta_pos = evt->speed < 0.0 ? -1 : +1;
	uint32_t cnt_evts = 0;
	uint32_t cnt_finished_evts = 0;
	for ( T_EVT_TIME *e = evt->evt_time_list; e; e=e->nxt) {
		cnt_evts++;
		if ( e->w_time >=0 ) {
			int64_t last_w_time = e->w_time;
			e->w_time -= timer_period;
			if ( last_w_time >= 0 && e->w_time < 0 ) {
				// change from >0 to <0, do work
				// timer arrived ...
				//ESP_LOGI(__func__,"evt.id=%d, tevt.id=%d: timer of %llu ms, type %d, val=%.2f arrived",evt->id, e->id, e->starttime, e->type, e->value);

				if ( e->clear_flags) {
					evt->w_flags &= ~ e->clear_flags;
				}
				if ( e->set_flags) {
					evt->w_flags |= e->set_flags;
				}
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
					evt->w_flags |= EVFL_CLEARPIXEL;
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
				cnt_finished_evts++;  // just finished
			}
		} else {
			cnt_finished_evts++; // already finished
		}
	}

	if ( cnt_evts == cnt_finished_evts ) {
		return true;
	}
	return false;
}

// paint the pixel in the calculated range evt->working.pos and evt->working.len.value
void process_event_what(T_EVENT *evt) {

	if ( evt->w_flags & EVFL_WAIT) {
		return;
	}

	T_EVT_WHAT *what_list = evt->what_list;
	if ( !what_list ) {
		ESP_LOGI(__func__, "nothing to paint");
		return;
	}

	int32_t startpos, endpos;
	T_COLOR_RGB rgb = {.r=0,.g=0,.b=0};
	T_COLOR_RGB rgb2 = {.r=0,.g=0,.b=0};

	// whole length of all sections
	int32_t len = 0;
	for (T_EVT_WHAT *w = what_list; w; w=w->nxt) {
		len += w->len;
	}
	double flen = len * evt->w_len_factor;

	if ( flen < 1.0 ) {
		return; // nothing left to display
	}

	if ( evt->w_flags & EVFL_CLEARPIXEL) {
		evt->w_flags &= ~EVFL_CLEARPIXEL; // reset the flag
		startpos = evt->w_pos;
		endpos   = evt->w_pos + len;
		strip_set_range(startpos, endpos, &rgb);
		ESP_LOGI(__func__, "clear pixel %d .. %d", startpos, endpos);
		return;
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

	startpos = evt->w_pos;
	endpos   = evt->w_pos + len;
	int32_t pos = startpos;
	if ( evt->delta_pos > 0 ) {
		pos = startpos;
	} else {
		pos = endpos;
	}
	double f = evt->w_brightness;

	int32_t dstart = startpos + floor(len - flen)/2.0;
	int32_t dend = ceil(dstart + flen);

	//ESP_LOGI(__func__, "pos=%d..%d, +%d d=%d..%d", startpos, endpos, evt->delta_pos, dstart, dend);
	double r,g,b;

	// for color transition
	double df,dr,dg,db;

	bool ende = false;
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
					/*ESP_LOGI(__func__,"color transition rgb %d/%d/%d -> %d/%d/%d, drgb=%.1f/%.1f/%.1f, f=%.2f "
							,rgb.r, rgb.g, rgb.b
							,rgb2.r, rgb2.g, rgb2.b
							,dr,dg,db
							,f
							);*/
				}
				r+=dr;
				g+=dg;
				b+=db;
				rgb.r = f * r;
				rgb.g = f * g;
				rgb.b = f * b;
				/*ESP_LOGI(__func__,"pos=%d, color transition rgb %d/%d/%d"
						,pos,rgb.r, rgb.g, rgb.b
						);*/
				break;

			case WT_RAINBOW:
				break;

			case WT_SPARKLE:
				break;

			default:
				ESP_LOGW(__func__,"what type %d NYI", w->type);
			}

			//ESP_LOGI(__func__,"paint id=%d, type=%d, len=%d, pos=%d, dpos=%d, startpos=%d, endpos=%d, dstart=%d, dend=%d",
			//		w->id, w->type, w->len, pos, evt->delta_pos, startpos, endpos, dstart, dend);

			if ( pos >=0 && pos < get_numleds() && pos >= dstart && pos <= dend) {
				c_checkrgb_abs(&rgb);
				strip_set_pixel(pos, &rgb);
			}
			pos += evt->delta_pos;
			if ( pos < startpos || pos > endpos) {
				ende = true;
				break; // done.
			}
		}
		if ( ende)
			break;
	}
	// ESP_LOGI(__func__, "ENDE");
}


/**
 * process an event, calculate time dependend events,
 * show the pixel
 * calculate parameter for next cycle
 * return:
 *  1 - all events finished
 *  0 - do more work
 */
bool process_event(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {
	if ( evt->w_flags & EVFL_FINISHED) {
		return true;
	}

	//ESP_LOGI(__func__, "start process_event_when evt=%d, t=%llu", evt->id, scene_time);
	bool finished = process_event_when(evt,scene_time, timer_period);

	//ESP_LOGI(__func__, "start process_event_what evt=%d", evt->id);
	process_event_what(evt);

	if ( finished) {
		evt->w_flags |= EVFL_FINISHED;

		// all events finished, repeat it?
		if (evt->evt_time_list_repeats > 0 ) {
			if ( evt->w_t_repeats > 0) {
				evt->w_t_repeats--;
			}
		} else {
			evt->w_t_repeats = 1; // forever
		}

		if ( evt->w_t_repeats > 0) {
			// reset event, next turn
			reset_event(evt);
			ESP_LOGI(__func__, "evt.id=%d: repeat events (%d/%d)", evt->id, evt->w_t_repeats, evt->evt_time_list_repeats);
			return false;
		}
		return true;
	}

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
	//ESP_LOGI(__func__, "finished evt=%d", evt->id);
	return false;

}

/**
 *  reset an event, except repeat parameter
 */
void reset_event( T_EVENT *evt) {
	evt->w_flags = evt->flags;
	evt->w_pos = evt->pos;
	evt->w_len_factor = evt->len_factor;
	evt->w_len_factor_delta = evt->len_factor_delta;
	evt->w_speed = evt->speed;
	evt->w_acceleration = evt->acceleration;
	evt->w_brightness = evt->brightness;
	evt->w_brightness_delta = evt->brightness_delta;
	evt->time = 0;
	evt->delta_pos = 1;

	for ( T_EVT_TIME *e = evt->evt_time_list; e; e=e->nxt) {
		e->w_time = e->starttime;
	}
	ESP_LOGI(__func__, "event %d", evt->id);

}

/**
 * reset repeat parameter of an event
 */
void reset_event_repeats(T_EVENT *evt) {
	evt->w_t_repeats = evt->evt_time_list_repeats;
	ESP_LOGI(__func__, "event %d", evt->id);

}

/**
 * for logging ...
 */
void event2text(T_EVENT *evt, char *buf, size_t sz_buf) {
	if ( !evt) {
		snprintf(buf, sz_buf, "evt was NULL");
		return;
	}
	snprintf(buf, sz_buf, "\nEvent %d, startpos=%.2f", evt->id, evt->pos);
	snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),", flags=0x%04x, len_f=%.1f, len_f_delta=%.1f, v=%.1f, v_delta=%.1f, brightn.=%.1f, brightn.delta=%1f"
			, evt->flags, evt->len_factor, evt->len_factor_delta, evt->speed, evt->acceleration, evt->brightness, evt->brightness_delta);

	if ( evt->what_list) {
		snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),"\n  what=");

		for ( T_EVT_WHAT *w=evt->what_list; w; w=w->nxt) {
			snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),"\n    id=%d, type=%d, pos=%d, len=%d>",
					w->id, w->type, w->pos, w->len
			);
		}

	} else {
		snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),"\n  no what list");
	}

	if (evt->evt_time_list) {
		snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),"\n  time events:");
		for (T_EVT_TIME *tevt = evt->evt_time_list; tevt; tevt=tevt->nxt) {
			snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),"\n    id=%d, starttime=%llu, type=%d, val=%.2f",
					tevt->id, tevt->starttime, tevt->type, tevt->value);

		}
	} else {
		snprintf(&(buf[strlen(buf)]), sz_buf-strlen(buf),"\n  no time events.");
	}
}
