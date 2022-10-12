/*
 * process_events.c
 *
 *  Created on: 09.08.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

static void reset_timing_events(T_EVENT *tevt);

T_OBJECT_DATA object_data_clear = {
	.type=OBJT_CLEAR,
	.pos=0,
	.len=-1,
	.para.hsv={.h=0,.s=0,.v=0},
	.nxt=NULL
};

T_EVT_OBJECT event_clear = {
	.oid="CLEAR",
	.data = &object_data_clear
};

static int exentended_logging = false;

// position dependend events
// sets
//  * speed or
//  * acceleration or
//  * position or
//  * length or
//  * brightness or
//  * color
// void process_event_where(T_EVENT_GROUP *evt, uint64_t timer_period) {
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
// }

// paint the pixel in the calculated range evt->working.pos and evt->working.len.value
void process_object(T_EVENT_GROUP *evtgrp) {

	if ( evtgrp->w_flags & EVFL_WAIT) {
		return;
	}

	T_EVT_OBJECT *obj = NULL;

	if ( strlen(evtgrp->w_object_oid)) {
		obj = find_object4oid(evtgrp->w_object_oid);
		if ( !obj) {
			ESP_LOGW(__func__, "evt.id='%s', no object found for oid='%s'", evtgrp->id, evtgrp->w_object_oid);
		}
	}
	if (! obj) {
		//ESP_LOGI(__func__, "nothing to paint");
		return;
	}

	int32_t startpos, endpos;
	T_COLOR_RGB rgb = {.r=0,.g=0,.b=0};
	T_COLOR_RGB rgb2 = {.r=0,.g=0,.b=0};
	T_COLOR_HSV hsv;
	double dh;

	// whole length of all sections
	int32_t len = 0;
	for (T_OBJECT_DATA *data = obj->data; data; data=data->nxt) {
		len += data->len;
	}
	double flen = len * evtgrp->w_len_factor;

	if ( flen < 1.0 ) {
		return; // nothing left to display
	}

	if ( evtgrp->w_flags & EVFL_CLEARPIXEL) {
		evtgrp->w_flags &= ~EVFL_CLEARPIXEL; // reset the flag
		startpos = floor(evtgrp->w_pos);
		endpos   = ceil(evtgrp->w_pos + len);
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

	startpos = evtgrp->w_pos;
	endpos   = evtgrp->w_pos + len;
	int pos = startpos;
	if ( evtgrp->delta_pos > 0 ) {
		pos = startpos;
	} else {
		pos = endpos;
	}
	double f = evtgrp->w_brightness;

	int32_t dstart = startpos + floor(len - flen)/2.0;
	int32_t dend = ceil(dstart + flen);

	//ESP_LOGI(__func__, "pos=%d..%d, +%d d=%d..%d", startpos, endpos, evt->delta_pos, dstart, dend);
	double r,g,b;

	// for color transition
	double df,dr,dg,db;

	bool ende = false;
	for (T_OBJECT_DATA *data = obj->data; data; data=data->nxt) {
		for ( int data_pos=0; data_pos < data->len; data_pos++) {
			switch (data->type) {
			case OBJT_COLOR:
				if ( data_pos == 0 ) {
					// initialize
					c_hsv2rgb(&(data->para.hsv), &rgb);
					r = f*rgb.r;
					g = f*rgb.g;
					b = f*rgb.b;
					rgb.r = r;
					rgb.g = g;
					rgb.r = r;
				}
				break;
			case OBJT_COLOR_TRANSITION: // linear from one color to another
				// if lvl2-lvl1 = 100 % and len = 4
				// use 25% 50% 75% 100%, not start with 0%
				if (data_pos ==0) {
					df = 1.0 / data->len;
					c_hsv2rgb(&(data->para.tr.hsv_from), &rgb);
					c_hsv2rgb(&(data->para.tr.hsv_to), &rgb2);
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

			case OBJT_RAINBOW:
				if ( data_pos==0) {
					// init
					hsv.h=0;
					hsv.s=100;
					hsv.v=100;
					dh = 360.0 / data->len;
				}
				c_hsv2rgb(&hsv, &rgb);
				rgb.r = f * rgb.r;
				rgb.g = f * rgb.g;
				rgb.b = f * rgb.b;
				hsv.h += dh;
				if ( hsv.h > 360)
					hsv.h=360;
				break;

			case OBJT_SPARKLE:
				break;

			case OBJT_BMP:
				// TODO get the next pixel as RGB
				if ( get_is_bmp_reading()) {
					bmp_read_data(pos, &rgb);
				}
				break;
			default:
				ESP_LOGW(__func__,"what type %d NYI", data->type);
			}

			//ESP_LOGI(__func__,"paint id=%d, type=%d, len=%d, pos=%d, dpos=%d, startpos=%d, endpos=%d, dstart=%d, dend=%d",
			//		w->id, w->type, w->len, pos, evt->delta_pos, startpos, endpos, dstart, dend);

			if ( pos >=0 && pos < get_numleds() && pos >= dstart && pos <= dend) {
				c_checkrgb_abs(&rgb);
				strip_set_pixel(pos, &rgb);
			}
			pos += evtgrp->delta_pos;
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


static void process_event_when_init(T_EVENT_GROUP *evtgrp) {
	//ESP_LOGI(__func__, "started");
	for(T_EVENT *evt = evtgrp->evt_time_init_list; evt; evt=evt->nxt ) {
		if (evt->status == TE_STS_FINISHED)
			continue; // init done

		if ( exentended_logging)
			ESP_LOGI(__func__,"INIT evt.id='%s', tevt.id=%d: type %d(%s), val=%.3f, sval='%s'",	evtgrp->id, evt->id, evt->type, ET2TEXT(evt->type), evt->value, evt->svalue);

		switch(evt->type) {
		case ET_SPEED:
			evtgrp->w_speed = evt->value;
			break;
		case ET_SPEEDUP:
			evtgrp->w_acceleration = evt->value;
			break;
		case ET_GOTO_POS:
			evtgrp->w_pos = evt->value;
			break;
		case ET_SET_BRIGHTNESS:
			evtgrp->w_brightness = evt->value;
			break;
		case ET_SET_BRIGHTNESS_DELTA:
			evtgrp->w_brightness_delta = evt->value;
			break;

		case ET_SET_OBJECT:
			if (strlen(evt->svalue)) {
				strlcpy(evtgrp->w_object_oid, evt->svalue, sizeof(evtgrp->w_object_oid));
				if ( exentended_logging)
					ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: new object_oid='%s'", evtgrp->id, evt->id, evtgrp->w_object_oid);
			}
			break;

		case ET_BMP_OPEN:
			if ( bmp_open_connection(evt->svalue) != ESP_OK) {
				ESP_LOGE(__func__,"INIT FAILED evt.id='%s', tevt.id=%d: type %d(%s), sval='%s'",
						evtgrp->id, evt->id, evt->type, ET2TEXT(evt->type), evt->svalue);
			}
			break;
		default:
			break;
		}
		evt->status = TE_STS_FINISHED;
	}
}



static void process_event_when_final(T_EVENT_GROUP *evtgrp) {
	ESP_LOGI(__func__, "started");
	for(T_EVENT *evt = evtgrp->evt_time_final_list; evt; evt=evt->nxt ) {
		if (evt->status == TE_STS_FINISHED)
			continue; // init done

		if ( exentended_logging) {
			ESP_LOGI(__func__,"FINAL evt.id='%s', tevt.id=%d: type %d(%s), val=%.3f, sval='%s'",
				evtgrp->id, evt->id, evt->type, ET2TEXT(evt->type), evt->value, evt->svalue);
			ESP_LOGI(__func__,"FINAL evt.id='%s', pos=%.2f", evtgrp->id, evtgrp->w_pos);
		}
		switch(evt->type) {
		case ET_CLEAR:
			evtgrp->w_flags |= EVFL_CLEARPIXEL;
			break;
		case ET_SET_BRIGHTNESS:
			evtgrp->w_brightness = evt->value;
			break;
		default:
			break;
		}
		evt->status = TE_STS_FINISHED;
	}
}

// time dependend events
// time values in ms
// sets several parameters
void  process_event_when(T_EVENT_GROUP *evtgrp, uint64_t scene_time, uint64_t timer_period) {
	if (!evtgrp->evt_time_list) {
		return; // no timing events
	}

	//evt->delta_pos = evt->w_speed < 0.0 ? -1 : +1;

	// **** INIT events *************
	process_event_when_init(evtgrp);

	// ************ WORK Events **************************
	T_EVENT *evt = evtgrp->evt_time_list;
	while(evt) {

		if ( evt->status == TE_STS_FINISHED ) {
			// finished or is not a paint type
			evt = evt->nxt;
			continue;
		}

		evtgrp->w_flags &= ~EVFL_WAIT;

		if ( evt->status == TE_STS_WAIT_FOR_START) {
			// Timer start
			if ( exentended_logging)
				ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: timer of %llu ms, type %d(%s), val=%.3f, sval='%s' timer START",
					evtgrp->id, evt->id, evt->time, evt->type, ET2TEXT(evt->type), evt->value, evt->svalue);

			// do work at START ********************************************
			switch(evt->type) {
			case ET_PAUSE:
				evtgrp->w_flags |= EVFL_WAIT;
				break;
			case ET_SPEED:
				evtgrp->w_speed = evt->value;
				break;
			case ET_SPEEDUP:
				evtgrp->w_acceleration = evt->value;
				break;
			case ET_SET_BRIGHTNESS:
				evtgrp->w_brightness = evt->value;
				break;
			case ET_SET_BRIGHTNESS_DELTA:
				evtgrp->w_brightness_delta = evt->value;
				break;
			case ET_BMP_OPEN:
				if ( bmp_open_connection(evt->svalue) != ESP_OK) {
					ESP_LOGE(__func__,"INIT FAILED evt.id='%s', tevt.id=%d: type %d(%s), sval='%s'",
							evtgrp->id, evt->id, evt->type, ET2TEXT(evt->type), evt->svalue);
				}
				break;
			default:
				//ESP_LOGW(__func__,"evt.id=%d, tevt.id=%d: timer of %llu ms, type %d expired, NYI", evt->id, tevt->id, tevt->time, tevt->type);
				break;
			}
			// continue with timer running
			evt->status = TE_STS_RUNNING;
		}

		if ( evt->status == TE_STS_RUNNING ) {
			if ( evt->w_time > 0) {
				// do work while timer is RUNNING  *****************************************
				switch(evt->type) {
				case ET_PAUSE:
					evtgrp->w_flags |= EVFL_WAIT;
					break;
				case ET_CLEAR:
					evtgrp->w_flags |= EVFL_CLEARPIXEL;
					break;
				default:
					break;
				}
				evt->w_time -= timer_period;
				break; // while

			} else {
				// timer expired
				if ( exentended_logging)
					ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: timer of %llu ms, type %d(%s), val=%.3f, timer EXPIRED",
						evtgrp->id, evt->id, evt->time, evt->type, ET2TEXT(evt->type), evt->value);
				evt->status = TE_STS_FINISHED;
			}
		}

		if ( evt->status == TE_STS_FINISHED) {
			// do work when timer ENDs *******************************************

			T_EVENT *tevt_next = evt->nxt;
			bool check_for_repeat = false;
			switch(evt->type) {
			case ET_SET_OBJECT:
				if (strlen(evt->svalue)) {
					strlcpy(evtgrp->w_object_oid, evt->svalue, sizeof(evtgrp->w_object_oid));
					if ( exentended_logging)
						ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: new object_oid='%s'", evtgrp->id, evt->id, evtgrp->w_object_oid);
				}
				break;

			case ET_BOUNCE:
				evtgrp->w_speed = -evtgrp->w_speed;
				break;

			case ET_GOTO_POS:
				evtgrp->w_pos = evt->value;
				break;

			case ET_REVERSE:
				evtgrp->delta_pos = evtgrp->delta_pos < 0 ? +1 : -1;
				break;

			case ET_STOP: // end of event chain, check for repeat
				tevt_next = evtgrp->evt_time_list; // go to start of the list
				check_for_repeat = true;
				break;

			case ET_JUMP_MARKER:
				// find event with marker
				tevt_next = find_timer_event4marker (evtgrp->evt_time_list, evt->marker);
				if ( tevt_next ) {
					// have one
					if (tevt_next == evt) {
						// found myself
						ESP_LOGE(__func__, "found my self tid=%d, marker='%s'", evt->id, evt->marker);
						tevt_next = evt->nxt;

					} else {
						// found a destination and it is not itselves
						// check for repeat
						check_for_repeat = true;
						if ( exentended_logging)
							ESP_LOGI(__func__, "found destination tid=%d, marker='%s'", tevt_next->id, tevt_next->marker);
					}
				} else {
					ESP_LOGE(__func__, "no event for '%s' found", evt->marker);
					tevt_next = evt->nxt;
				}
				break;

			default:
				//ESP_LOGW(__func__,"evt.id=%d, tevt.id=%d: timer of %llu ms, type %d expired, NYI", evt->id, tevt->id, tevt->time, tevt->type);
				break;
			}

			if ( !tevt_next){
				// no more events, start from the beginning
				check_for_repeat = true; // no more timer events, check for repeat
				tevt_next = evtgrp->evt_time_list; // go to start of the list
			}

			// check for repeat ********************************************************************
			if ( check_for_repeat) {
				// all events finished, repeat it?
				if (evtgrp->t_repeats > 0 ) {
					if ( evtgrp->w_t_repeats > 0) {
						evtgrp->w_t_repeats--;
					}
				} else {
					evtgrp->w_t_repeats = 1; // forever
				}

				if ( evtgrp->w_t_repeats > 0) {
					// have to be repeated **************************************************************
					// reset event, next turn
					if ( exentended_logging)
						ESP_LOGI(__func__, "evt.id='%s': repeat events (%d/%d)", evtgrp->id, evtgrp->w_t_repeats, evtgrp->t_repeats);

					reset_event(evtgrp);
					reset_timing_events(tevt_next);
					// process init events
					process_event_when_init(evtgrp);

					if ( exentended_logging)
						ESP_LOGI(__func__, "next event to tid=%d", tevt_next->id );
				} else {
					// done, mark event as finished
					evtgrp->w_flags |= EVFL_FINISHED;
				}
			}

			evt = tevt_next;
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
void process_event(T_EVENT_GROUP *evt, uint64_t scene_time, uint64_t timer_period) {
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
static void reset_timing_events(T_EVENT *tevt) {
	if ( !tevt)
		return;

	for ( T_EVENT *e = tevt; e; e=e->nxt) {
		e->w_time = e->time;
		e->status = TE_STS_WAIT_FOR_START;
		//ESP_LOGI(__func__, "id=%d: status=0x%04x", e->id, e->status);
	}
}


/**
 *  reset an event,
 *  except repeat parameter and working events
 */
void reset_event( T_EVENT_GROUP *evt) {
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

	if ( exentended_logging)
		ESP_LOGI(__func__, "event '%s'", evt->id);
}

/**
 * reset repeat parameter of an event
 * occurs if anything is played, reset also final events
 */
void reset_event_repeats(T_EVENT_GROUP *evt) {

	evt->w_t_repeats = evt->t_repeats;
	reset_timing_events(evt->evt_time_final_list);

	if ( exentended_logging)
		ESP_LOGI(__func__, "event '%s' repeates=%d", evt->id, evt->w_t_repeats);

}

void process_scene(T_SCENE *scene, uint64_t scene_time, uint64_t timer_period) {
	T_EVENT_GROUP *events = scene->events;
	if ( !events ) {
		scene->flags |= EVFL_FINISHED;
		return;
	}

	if ( scene->flags & EVFL_FINISHED)
		return;

	if ( !scene->event) {
		// first event
		scene->event=scene->events;
		reset_event(scene->event);
	}

	bool finished = true;
	for ( ; scene->event; scene->event = scene->event->nxt) {
		if (scene->event->w_flags & EVFL_FINISHED) {
			if ( exentended_logging)
				ESP_LOGI(__func__,"scene '%s', event '%s' already finished", scene->id, scene->event->id );
			continue; // step over
		}
		// not finished, process it
		process_event(scene->event, scene_time, timer_period);
		if ( !(scene->event->w_flags & EVFL_FINISHED)) {
			finished = false;
		} else {
			if ( exentended_logging)
				ESP_LOGI(__func__,"scene '%s', event '%s' just finished", scene->id, scene->event->id );
		}

		break;
	}

	if (finished) {
		scene->flags |= EVFL_FINISHED;
		if ( exentended_logging)
			ESP_LOGI(__func__, "scene '%s' finished", scene->id);
	}
}


void reset_scene(T_SCENE *scene) {
	if ( exentended_logging)
		ESP_LOGI(__func__, "scene '%s'", scene->id);
	scene->flags = 0;
	scene->event = NULL;
	for( T_EVENT_GROUP *evt = scene->events; evt; evt=evt->nxt) {
		reset_event(evt);
		// additional reset working events
		reset_timing_events(evt->evt_time_list);
		reset_event_repeats(evt);
	}

}



