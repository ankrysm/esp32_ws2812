/*
 * process_events.c
 *
 *  Created on: 09.08.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

static void reset_timing_events(T_EVT_TIME *tevt);

T_EVT_OBJECT_DATA object_data_clear = {
	.type=WT_CLEAR,
	.pos=0,
	.len=-1,
	.para.hsv={.h=0,.s=0,.v=0},
	.nxt=NULL
};

T_EVT_OBJECT event_clear = {
	.oid="CLEAR",
	.data = &object_data_clear
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
	// TO DO: check a range: is it near the trigger point, which direction
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

// paint the pixel in the calculated range evt->working.pos and evt->working.len.value
void process_object(T_EVENT *evt) {

	if ( evt->w_flags & EVFL_WAIT) {
		return;
	}

	T_EVT_OBJECT *obj = NULL;

	if ( strlen(evt->w_object_oid)) {
		obj = find_object4oid(evt->w_object_oid);
		if ( !obj) {
			ESP_LOGW(__func__, "evt.id='%s', no object found for oid='%s'", evt->id, evt->w_object_oid);
		}
	}
	if (! obj) {
		//ESP_LOGI(__func__, "nothing to paint");
		return;
	}

	int32_t startpos, endpos;
	T_COLOR_RGB rgb = {.r=0,.g=0,.b=0};
	T_COLOR_RGB rgb2 = {.r=0,.g=0,.b=0};

	// whole length of all sections
	int32_t len = 0;
	for (T_EVT_OBJECT_DATA *data = obj->data; data; data=data->nxt) {
		len += data->len;
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
	for (T_EVT_OBJECT_DATA *w = obj->data; w; w=w->nxt) {
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


static void process_event_when_init(T_EVENT *evt) {
	//ESP_LOGI(__func__, "started");
	for(T_EVT_TIME *e = evt->evt_time_init_list; e; e=e->nxt ) {
		if (e->status == TE_FINISHED)
			continue; // init done

		ESP_LOGI(__func__,"INIT evt.id='%s', tevt.id=%d: type %d(%s), val=%.3f, sval='%s'",
				evt->id, e->id, e->type, ET2TEXT(e->type), e->value, e->svalue);

		switch(e->type) {
		case ET_SPEED:
			evt->w_speed = e->value;
			break;
		case ET_SPEEDUP:
			evt->w_acceleration = e->value;
			break;
		case ET_GOTO_POS:
			evt->w_pos = e->value;
			break;
		case ET_SET_BRIGHTNESS:
			evt->w_brightness = e->value;
			break;
		case ET_SET_BRIGHTNESS_DELTA:
			evt->w_brightness_delta = e->value;
			break;

		case ET_SET_OBJECT:
			if (strlen(e->svalue)) {
				strlcpy(evt->w_object_oid, e->svalue, sizeof(evt->w_object_oid));
				ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: new object_oid='%s'", evt->id, e->id, evt->w_object_oid);
			}
			break;
	//	case ET_SET_REPEAT_COUNT:
			// processed in reset_event_repeats
		//	break;

		default:
			break;
		}
		e->status = TE_FINISHED;
	}
}



static void process_event_when_final(T_EVENT *evt) {
	ESP_LOGI(__func__, "started");
	for(T_EVT_TIME *e = evt->evt_time_final_list; e; e=e->nxt ) {
		if (e->status == TE_FINISHED)
			continue; // init done

		ESP_LOGI(__func__,"FINAL evt.id='%s', tevt.id=%d: type %d(%s), val=%.3f, sval='%s'",
				evt->id, e->id, e->type, ET2TEXT(e->type), e->value, e->svalue);

		switch(e->type) {
		case ET_CLEAR:
			evt->w_flags |= EVFL_CLEARPIXEL;
			break;
		case ET_SET_BRIGHTNESS:
			evt->w_brightness = e->value;
			break;
		default:
			break;
		}
		e->status = TE_FINISHED;
	}
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
void  process_event_when(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {
	if (!evt->evt_time_list) {
		return; // no timing events
	}

	evt->delta_pos = evt->w_speed < 0.0 ? -1 : +1;

	// **** INIT events *************
	process_event_when_init(evt);

	// ************ WORK Events **************************
	T_EVT_TIME *tevt = evt->evt_time_list;
	while(tevt) {

		if ( tevt->status == TE_FINISHED ) {
			// finished or is not a paint type
			tevt = tevt->nxt;
			continue;
		}

		evt->w_flags &= ~EVFL_WAIT;

		if ( tevt->status == TE_WAIT_FOR_START) {
			// Timer start
			ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: timer of %llu ms, type %d(%s), val=%.3f, sval='%s' timer START",
					evt->id, tevt->id, tevt->time, tevt->type, ET2TEXT(tevt->type), tevt->value, tevt->svalue);

			// do work at START ********************************************
			switch(tevt->type) {
			case ET_PAUSE:
				evt->w_flags |= EVFL_WAIT;
				break;
			case ET_SPEED:
				evt->w_speed = tevt->value;
				break;
			case ET_SPEEDUP:
				evt->w_acceleration = tevt->value;
				break;
			case ET_SET_BRIGHTNESS:
				evt->w_brightness = tevt->value;
				break;
			case ET_SET_BRIGHTNESS_DELTA:
				evt->w_brightness_delta = tevt->value;
				break;
			default:
				//ESP_LOGW(__func__,"evt.id=%d, tevt.id=%d: timer of %llu ms, type %d expired, NYI", evt->id, tevt->id, tevt->time, tevt->type);
				break;
			}
			// continue with timer running
			tevt->status = TE_RUNNING;
		}

		if ( tevt->status == TE_RUNNING ) {
			if ( tevt->w_time > 0) {
				// do work while timer is RUNNING  *****************************************
				switch(tevt->type) {
				case ET_PAUSE:
					evt->w_flags |= EVFL_WAIT;
					break;
				case ET_CLEAR:
					evt->w_flags |= EVFL_CLEARPIXEL;
					break;
				default:
					break;
				}
				tevt->w_time -= timer_period;
				break; // while

			} else {
				// timer expired
				ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: timer of %llu ms, type %d(%s), val=%.3f, timer EXPIRED",
						evt->id, tevt->id, tevt->time, tevt->type, ET2TEXT(tevt->type), tevt->value);
				tevt->status = TE_FINISHED;
			}
		}

		if ( tevt->status == TE_FINISHED) {
			// do work when timer ENDs *******************************************

			T_EVT_TIME *tevt_next = tevt->nxt;
			bool check_for_repeat = false;
			switch(tevt->type) {
			case ET_SET_OBJECT:
				if (strlen(tevt->svalue)) {
					strlcpy(evt->w_object_oid, tevt->svalue, sizeof(evt->w_object_oid));
					ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: new object_oid='%s'", evt->id, tevt->id, evt->w_object_oid);
				}
				break;
			case ET_BOUNCE:
				evt->w_speed = -evt->w_speed;
				break;
			case ET_GOTO_POS:
				evt->w_pos = tevt->value;
				break;
			case ET_REVERSE:
				evt->delta_pos = evt->delta_pos < 0 ? +1 : -1;
				break;

			case ET_STOP: // end of event chain, check for repeat
				tevt_next = evt->evt_time_list; // go to start of the list
				check_for_repeat = true;
				break;
			case ET_JUMP_MARKER:
				// find event with marker
				tevt_next = find_timer_event4marker (evt->evt_time_list, tevt->marker);
				if ( tevt_next ) {
					// have one
					if (tevt_next == tevt) {
						// found myself
						ESP_LOGE(__func__, "found my self tid=%d, marker='%s'", tevt->id, tevt->marker);
						tevt_next = tevt->nxt;

					} else {
						// found a destination and it is not itselves
						// check for repeat
						check_for_repeat = true;
						ESP_LOGI(__func__, "found destination tid=%d, marker='%s'", tevt_next->id, tevt_next->marker);
					}
				} else {
					ESP_LOGE(__func__, "no event for '%s' found", tevt->marker);
					tevt_next = tevt->nxt;
				}
				break;

			default:
				//ESP_LOGW(__func__,"evt.id=%d, tevt.id=%d: timer of %llu ms, type %d expired, NYI", evt->id, tevt->id, tevt->time, tevt->type);
				break;
			}

			if ( !tevt_next){
				// no more events, start from the beginning
				check_for_repeat = true; // no more timer events, check for repeat
				tevt_next = evt->evt_time_list; // go to start of the list
			}

			if ( check_for_repeat) {
				// all events finished, repeat it?
				if (evt->t_repeats > 0 ) {
					if ( evt->w_t_repeats > 0) {
						evt->w_t_repeats--;
					}
				} else {
					evt->w_t_repeats = 1; // forever
				}

				if ( evt->w_t_repeats > 0) {
					// reset event, next turn
					ESP_LOGI(__func__, "evt.id='%s': repeat events (%d/%d)", evt->id, evt->w_t_repeats, evt->t_repeats);
					reset_event(evt);
					reset_timing_events(evt->evt_time_init_list);
					reset_timing_events(tevt_next);
					ESP_LOGI(__func__, "next event to tid=%d", tevt_next->id );
				} else {
					// done, mark event as finished
					evt->w_flags |= EVFL_FINISHED;
				}
			}

			tevt = tevt_next;
			continue;

		} else {
			// its the running timer
			break;

		} // if finished

	} // while


}



/**
 * process an event, calculate time dependend events,
 * show the pixel
 * calculate parameter for next cycle
 *
 * caller should check EVFL_FINISHED flag
 */
void process_event(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {
	if ( evt->w_flags & EVFL_FINISHED) {
		return;
	}

	//ESP_LOGI(__func__, "start process_event_when evt=%d, t=%llu", evt->id, scene_time);
	process_event_when(evt, scene_time, timer_period);

	// finished is a new status
	if ( evt->w_flags & EVFL_FINISHED ) {
		process_event_when_final(evt);
		process_object(evt);
		return; // not necessary to do more
	}

	//ESP_LOGI(__func__, "start process_event_what evt=%d", evt->id);
	process_object(evt);

	if ( evt->w_flags & EVFL_FINISHED )
		return; // not necessary to do more

	// next timestep
	evt->time += timer_period;

	if ( evt->w_flags & EVFL_WAIT )
		return; // no changes while wait


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

	evt->w_pos += evt->w_speed;
	//ESP_LOGI(__func__, "finished evt=%d", evt->id);
	//return false;

}

/**
 * resets all timing events starting from the given event
 */
static void reset_timing_events(T_EVT_TIME *tevt) {
	if ( !tevt)
		return;

	for ( T_EVT_TIME *e = tevt; e; e=e->nxt) {
		e->w_time = e->time;
		e->status = TE_WAIT_FOR_START;
		ESP_LOGI(__func__, "id=%d: status=0x%04x", e->id, e->status);
	}
}


/**
 *  reset an event, except repeat parameter
 */
void reset_event( T_EVENT *evt) {
	evt->w_flags = 0; //evt->flags;
	evt->w_pos = 0; //evt->pos;
	evt->w_len_factor = 1.0; //evt->len_factor;
	evt->w_len_factor_delta = 0.0; //evt->len_factor_delta;
	evt->w_speed = 0.0; //evt->speed;
	evt->w_acceleration = 0.0; //evt->acceleration;
	evt->w_brightness = 1.0; //evt->brightness;
	evt->w_brightness_delta = 0.0; //evt->brightness_delta;
	evt->time = 0;
	evt->delta_pos = 1;
	memset(evt->w_object_oid,0, LEN_EVT_OID);

	reset_timing_events(evt->evt_time_init_list);

	ESP_LOGI(__func__, "event '%s'", evt->id);

}

/**
 * reset repeat parameter of an event
 * occurs if anything is played, reset also final events
 */
void reset_event_repeats(T_EVENT *evt) {

	evt->w_t_repeats = evt->t_repeats;
	/*
	for(T_EVT_TIME *e = evt->evt_time_init_list; e; e=e->nxt ) {
		switch(e->type) {
		case ET_SET_REPEAT_COUNT:
			evt->w_t_repeats = e->value;
			e->status = TE_FINISHED;
			break;
		default:
			break;
		}
	}
	*/
	reset_timing_events(evt->evt_time_final_list);

	ESP_LOGI(__func__, "event '%s' repeates=%d", evt->id, evt->w_t_repeats);

}

void process_scene(T_SCENE *scene, uint64_t scene_time, uint64_t timer_period) {
	T_EVENT *events = scene->events;
	if ( !events ) {
		scene->flags |= EVFL_FINISHED;
		return;
	}

	if ( !scene->event) {
		// first event
		scene->event=scene->events;
		reset_event(scene->event);
	}

	bool finished = true;
	for ( ; scene->event; scene->event = scene->event->nxt) {
		if (scene->event->w_flags & EVFL_FINISHED)
			continue; // step over

		// not finished, process it
		process_event(scene->event, scene_time, timer_period);
		if ( !(scene->event->w_flags & EVFL_FINISHED))
			finished = false;

		break;
	}

	if (finished) {
		scene->flags |= EVFL_FINISHED;
		ESP_LOGI(__func__, "scene '%s' finished", scene->id);
	}



}


void reset_scene(T_SCENE *scene) {
	ESP_LOGI(__func__, "scene '%s'", scene->id);
	scene->flags = 0;
	for( T_EVENT *evt = scene->events; evt; evt=evt->nxt) {
		reset_event(evt);
		reset_event_repeats(evt);
	}

}



