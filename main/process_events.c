/*
 * process_events.c
 *
 *  Created on: 09.08.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

static void reset_events(T_EVENT *events, char *msg);

T_OBJECT_DATA object_data_clear = {
	.type=OBJT_CLEAR,
	//.pos=0,
	.len=-1,
	.para.hsv={.h=0,.s=0,.v=0},
	.nxt=NULL
};

T_EVT_OBJECT event_clear = {
	.oid="CLEAR",
	.data = &object_data_clear
};

static int extended_logging = true;


// paint the pixel in the calculated range evt->working.pos and evt->working.len.value
void process_object(T_EVENT_GROUP *evtgrp) {

	//if ( extended_logging)
	//	ESP_LOGI(__func__, "startetd evtgrp '%s', flags 0x%x, sts=%d, pos=%d", evtgrp->id, evtgrp->w_flags, evtgrp->status, evtgrp->w_pos);

	if ( evtgrp->w_flags & EVFL_WAIT) {
		return; // do nothing, wait
	}

	T_EVT_OBJECT *obj = NULL;

	if ( strlen(evtgrp->w_object_oid)) {
		obj = find_object4oid(evtgrp->w_object_oid);
		if ( !obj) {
			ESP_LOGW(__func__, "evt.id='%s', no object found for oid='%s'", evtgrp->id, evtgrp->w_object_oid);
		}
	}
	if (! obj) {
		ESP_LOGI(__func__, "nothing to paint");
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
				// TODO
				if ( evtgrp->w_flags & EVFL_BMP_OPEN) {
					if ( bmp_open_connection(data->para.url) != ESP_OK) {
						ESP_LOGE(__func__,"BMP_OPEN FAILED evt.id='%s', url='%s'",
								evtgrp->id, data->para.url);
					}
				} else if (evtgrp->w_flags & EVFL_BMP_CLOSE) {
					bmp_stop_processing();

				} else {
					// get the next pixel as RGB
					if ( get_is_bmp_reading()) {
						// returns RES_OK while reading data and RES_FINISHED when done
						t_result res = bmp_read_data(pos, &rgb);
					}
				}
				evtgrp->w_flags &= ~EVFL_BMP_MASK; // reset the bmp-flags
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


static void process_event_group_init(T_EVENT_GROUP *evtgrp) {
	ESP_LOGI(__func__, "started");

	evtgrp->status = EVT_STS_RUNNING; // in most cases

	for(T_EVENT *evt = evtgrp->evt_init_list; evt; evt=evt->nxt ) {

		if ( extended_logging) {
			char buf[64];
			event2text(evt, buf, sizeof(buf));
			ESP_LOGI(__func__,"INIT evt.id='%s', %s", evtgrp->id, buf);
		}
		switch(evt->type) {

		case ET_WAIT: /// wait for Statup
			evtgrp->w_flags |= EVFL_WAIT;
			evtgrp->w_wait_time = evt->para.value;
			evtgrp->status = EVT_STS_STARTING;
			break;

		case ET_WAIT_FIRST: /// wait for first Statup
			if ( evtgrp->w_flags & EVFL_WAIT_FIRST_DONE) {
				break; // not at first init
			}
			evtgrp->w_flags |= EVFL_WAIT;
			evtgrp->w_wait_time = evt->para.value;
			evtgrp->status = EVT_STS_STARTING;
			break;

		case ET_CLEAR:
			evtgrp->w_flags |= EVFL_CLEARPIXEL;
			break;

		case ET_SPEED:
			evtgrp->w_speed = evt->para.value;
			break;

		case ET_SPEEDUP:
			evtgrp->w_acceleration = evt->para.value;
			break;

		case ET_GOTO_POS:
			evtgrp->w_pos = evt->para.value;
			break;

		case ET_SET_BRIGHTNESS:
			evtgrp->w_brightness = evt->para.value;
			break;

		case ET_SET_BRIGHTNESS_DELTA:
			evtgrp->w_brightness_delta = evt->para.value;
			break;

		case ET_SET_OBJECT:
			if (strlen(evt->para.svalue)) {
				strlcpy(evtgrp->w_object_oid, evt->para.svalue, sizeof(evtgrp->w_object_oid));
				if ( extended_logging)
					ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: new object_oid='%s'", evtgrp->id, evt->id, evtgrp->w_object_oid);
			}
			break;

		case ET_BMP_OPEN:
			evtgrp->w_flags |= EVFL_BMP_OPEN;
			break;

		default:
			break;
		}
	}
}

static void process_event_group_starting(T_EVENT_GROUP *evtgrp, uint64_t scene_time, uint64_t timer_period){
	//ESP_LOGI(__func__, "started");

	int nproc=0;
	for(T_EVENT *evt = evtgrp->evt_init_list; evt; evt=evt->nxt ) {

		switch (evt->type) {
		case ET_WAIT:
		case ET_WAIT_FIRST:
			nproc++;
			evtgrp->w_wait_time -= timer_period;
			if (evtgrp->w_wait_time <=0) {
				// timer ends, event done, reset wait flag
				evtgrp->status = EVT_STS_RUNNING;
				evtgrp->w_flags &= ~EVFL_WAIT;
				if ( extended_logging) {
					char buf[64];
					event2text(evt, buf, sizeof(buf));
					ESP_LOGI(__func__,"finished evt.id='%s, %s'", evtgrp->id, buf);
				}
			}
			break;

		default:
			break;
		}
	}

	if (!nproc) {
		// no starting events
		ESP_LOGW(__func__, "no starting events");
		evtgrp->status = EVT_STS_RUNNING;
	}
}


static void process_event_group_final(T_EVENT_GROUP *evtgrp) {
	ESP_LOGI(__func__, "started");
	for(T_EVENT *evt = evtgrp->evt_final_list; evt; evt=evt->nxt ) {

		if ( extended_logging) {
			char buf[64];
			event2text(evt, buf, sizeof(buf));
			ESP_LOGI(__func__,"FINAL evt.id='%s', pos=%.2f, %s", evtgrp->id, evtgrp->w_pos, buf);
		}
		switch(evt->type) {

		case ET_CLEAR:
			evtgrp->w_flags |= EVFL_CLEARPIXEL;
			break;

		case ET_BMP_CLOSE:
			evtgrp->w_flags |= EVFL_BMP_CLOSE;
			break;

		default:
			break;
		}
	}
}


static void check_for_repeat(T_EVENT_GROUP *evtgrp) {
	if (evtgrp->t_repeats > 0 ) {
		if ( evtgrp->w_t_repeats > 0) {
			evtgrp->w_t_repeats--;
		}
	} else {
		evtgrp->w_t_repeats = 1; // forever
	}

	if ( evtgrp->w_t_repeats == 0 ) {
		evtgrp->status = EVT_STS_FINISHED;
		if ( extended_logging)
			ESP_LOGI(__func__, "evt.id='%s': repeat events (%d/%d) FINISHED", evtgrp->id, evtgrp->w_t_repeats, evtgrp->t_repeats);
	} else {
		if ( extended_logging)
			ESP_LOGI(__func__, "evt.id='%s': repeat events (%d/%d) CONTINUE", evtgrp->id, evtgrp->w_t_repeats, evtgrp->t_repeats);
	}
}

//	if ( evtgrp->w_t_repeats > 0) {
//		// have to be repeated **************************************************************
//		// reset event, next turn
//		if ( exentended_logging)
//			ESP_LOGI(__func__, "evt.id='%s': repeat events (%d/%d)", evtgrp->id, evtgrp->w_t_repeats, evtgrp->t_repeats);
//
//		reset_event_group(evtgrp);
//		reset_events(tevt_next);
//		// process init events
//		process_event_group_init(evtgrp);
//
//		if ( exentended_logging)
//			ESP_LOGI(__func__, "next event to tid=%d", tevt_next->id );
//	} else {
//		// done, mark event as finished
//		evtgrp->w_flags |= EVFL_FINISHED;
//	}

//}

// working events
// time values in ms
// sets several parameters
void  process_event_group_work(T_EVENT_GROUP *evtgrp, uint64_t scene_time, uint64_t timer_period) {
	if (!evtgrp->evt_work_list) {
		return; // no events
	}

	// ************ WORK Events **************************
	//bool check_for_repeat = false;
	T_EVENT *evt = evtgrp->evt_work_list;
	T_EVENT *evt_next;
	while(evt) {

		if ( evt->status == EVT_STS_FINISHED ) {
			// already finished
			evt = evt->nxt;
			continue;
		}

		if ( evt->status == EVT_STS_READY ) {
			// initialize work event
			evt->status = EVT_STS_FINISHED; // in most cases

			switch(evt->type) {
			case ET_WAIT:
				evtgrp->w_flags |= EVFL_WAIT;
				evtgrp->w_wait_time = evt->para.value;
				evt->status = EVT_STS_RUNNING;
				break;
			case ET_PAINT:
				evtgrp->w_wait_time = evt->para.value;
				evt->status = EVT_STS_RUNNING;
				break;
			case ET_DISTANCE:
				evtgrp->w_distance = evt->para.value;
				evt->status = EVT_STS_RUNNING;
				break;
			case ET_SPEED:
				evtgrp->w_speed = evt->para.value;
				break;
			case ET_SPEEDUP:
				evtgrp->w_acceleration = evt->para.value;
				break;
			case ET_BOUNCE:
				evtgrp->w_speed = -evtgrp->w_speed;
				break;
			case ET_REVERSE:
				evtgrp->delta_pos = evtgrp->delta_pos < 0 ? +1 : -1;
				break;
			case ET_GOTO_POS:
				evtgrp->w_pos = evt->para.value;
				break;
			case ET_JUMP_MARKER:
				// find event with marker
				evt_next = find_event4marker(evtgrp->evt_work_list, evt->para.svalue);
				if ( evt_next ) {
					// found a destination, check for repeat
					check_for_repeat(evtgrp);
					if ( evtgrp->status == EVT_STS_FINISHED) {
						if ( extended_logging)
							ESP_LOGI(__func__, "found destination next-id=%d, marker='%s' FINISHED", evt_next->id, evt_next->para.svalue);
						// all remaining events set to finished
						for (;evt; evt = evt->nxt) {
							evt->status = EVT_STS_FINISHED;
						}
						return;
					}
					if ( extended_logging)
						ESP_LOGI(__func__, "found destination tid=%d, marker='%s' jump to", evt_next->id, evt_next->para.svalue);

					evt = evt_next;
					reset_events(evt,"WORK(JUMP)");

				} else {
					ESP_LOGE(__func__, "no event for '%s' found", evt->para.svalue);
				}
				break;
			case ET_CLEAR:
				evtgrp->w_flags |= EVFL_CLEARPIXEL;
				break;
			case ET_SET_BRIGHTNESS:
				evtgrp->w_brightness = evt->para.value;
				break;
			case ET_SET_BRIGHTNESS_DELTA:
				evtgrp->w_brightness_delta = evt->para.value;
				break;
			case ET_SET_OBJECT:
				if (strlen(evt->para.svalue)) {
					strlcpy(evtgrp->w_object_oid, evt->para.svalue, sizeof(evtgrp->w_object_oid));
					if ( extended_logging)
						ESP_LOGI(__func__,"evt.id='%s', tevt.id=%d: new object_oid='%s'", evtgrp->id, evt->id, evtgrp->w_object_oid);
				}
				break;
			case ET_BMP_OPEN:
				evtgrp->w_flags |= EVFL_BMP_OPEN;
				break;
			case ET_BMP_READ:
				evtgrp->w_flags |= EVFL_BMP_READ;
				break;
			case ET_BMP_CLOSE:
				evtgrp->w_flags |= EVFL_BMP_CLOSE;
				break;

			default:
				//ESP_LOGW(__func__,"evt.id=%d, tevt.id=%d: timer of %llu ms, type %d expired, NYI", evt->id, tevt->id, tevt->time, tevt->type);
				break;
			}

		} // if READY

		if ( evt->status == EVT_STS_FINISHED) {
			if ( extended_logging) {
				char buf[64];
				event2text(evt, buf, sizeof(buf));
				ESP_LOGI(__func__,"event finished evt.id='%s', %s", evtgrp->id, buf);

			}
			evt = evt->nxt;
			continue;
		}

		// here always: EVT_STS_RUNNING
		switch (evt->type) {
		case ET_WAIT:
			evtgrp->w_wait_time -= timer_period;
			if (evtgrp->w_wait_time <=0) {
				// timer ends, event done, reset wait flag
				evt->status = EVT_STS_FINISHED;
				evtgrp->w_flags &= ~EVFL_WAIT;
			}
			break;
		case ET_PAINT:
			evtgrp->w_wait_time -= timer_period;
			if (evtgrp->w_wait_time <=0) {
				// timer ends
				evt->status = EVT_STS_FINISHED;
			}
			break;
		case ET_DISTANCE:
			evtgrp->w_distance -= abs(evtgrp->w_speed);
			if ( evtgrp->w_distance <= 0.0) {
				// distance reached
				evt->status = EVT_STS_FINISHED;
			}
			break;
		default:
			// should not happen here
			evt->status = EVT_STS_FINISHED;
			break;
		}

		// if a running event finished
		if ( evt->status == EVT_STS_FINISHED) {
			evt = evt->nxt;
			continue;
		}
		break; // because of the RUNNING event not finished yet
	} // while

	if (!evt) {
		// no more events
		check_for_repeat(evtgrp);
		if ( evtgrp->status == EVT_STS_FINISHED) {
			// all done
			ESP_LOGI(__func__, "evt.id='%s': repeat events (%d/%d) ALL DONE", evtgrp->id, evtgrp->w_t_repeats, evtgrp->t_repeats);
			return;
		}
		// next turn, reset events
		reset_event_group(evtgrp);
		evtgrp->w_flags |= EVFL_WAIT_FIRST_DONE;
		// status is set to READY
		//needs to be RUNNING
		//evtgrp->status = EVT_STS_RUNNING;
	}
}



/**
 * main function
 * process an event group, calculate time dependend events,
 * show the pixel
 * calculate parameter for next cycle
 */
void process_event_group_main(T_EVENT_GROUP *evtgrp, uint64_t scene_time, uint64_t timer_period) {

	if ( evtgrp->status ==  EVT_STS_FINISHED) {
		return; // nothing to do anymore
	}

	if ( evtgrp->status ==  EVT_STS_READY) {
		// **** process INIT events *************
		process_event_group_init(evtgrp);
	}

	if ( evtgrp->status == EVT_STS_STARTING) {
		// **** process STARTINFG events (wait) ****
		process_event_group_starting(evtgrp, scene_time, timer_period);
		if ( evtgrp->status == EVT_STS_STARTING ) {
			// starting event not finished yet
			return;
		}
	}

	// here the status is always "running"

	//ESP_LOGI(__func__, "start process_event_when evtgrp=%s, t=%llu", evtgrp->id, scene_time);
	process_event_group_work(evtgrp, scene_time, timer_period);

	if ( evtgrp->status == EVT_STS_FINISHED ) {
		process_event_group_final(evtgrp);
		process_object(evtgrp);
		return; // not necessary to do more
	}

	//ESP_LOGI(__func__, "start process_event_what evt=%d", evt->id);
	process_object(evtgrp);

//	if ( evtgrp->w_flags & EVFL_FINISHED )
//		return; // not necessary to do more

	// next timestep
	evtgrp->time += timer_period;

	if ( evtgrp->w_flags & EVFL_WAIT )
		return; // no changes while wait


	// calculate speed and length
	// v = a * t
	// Δv = a * Δt
	// speed is leds per ms
	evtgrp->w_speed += evtgrp->w_acceleration;

	evtgrp->w_len_factor += evtgrp->w_len_factor_delta;
	if ( evtgrp->w_len_factor < 0.0 ) {
		evtgrp->w_len_factor = 0.0;
	} else if ( evtgrp->w_len_factor > 1.0 ) {
		evtgrp->w_len_factor = 1.0;
	}

	evtgrp->w_brightness += evtgrp->w_brightness_delta;
	if ( evtgrp->w_brightness < 0.0)
		evtgrp->w_brightness = 0.0;
	else if(evtgrp->w_brightness > 1.0)
		evtgrp->w_brightness = 1.0;

	evtgrp->w_pos += evtgrp->w_speed;

	//ESP_LOGI(__func__, "finished evt=%d", evt->id);
	//return false;

}

// ************************************************************************************

void process_scene(T_SCENE *scene, uint64_t scene_time, uint64_t timer_period) {

	T_EVENT_GROUP *event_groups = scene->event_groups;

	if ( !event_groups ) {
		scene->status = EVT_STS_FINISHED;
		return;
	}

	if ( scene->status == EVT_STS_FINISHED)
		return;

	if ( scene->status == EVT_STS_READY) {
		reset_scene(scene);
		scene->status = EVT_STS_RUNNING;
	}

	// scene status is EVT_STS_RUNNING here
	bool finished = true;
	for ( ; scene->event_group; scene->event_group = scene->event_group->nxt) {

		if (scene->event_group->status == EVT_STS_FINISHED) {
			if ( extended_logging)
				ESP_LOGI(__func__,"scene '%s', event group '%s' already finished", scene->id, scene->event_group->id );
			continue; // step over
		}

		// not finished, process it
		process_event_group_main(scene->event_group, scene_time, timer_period);

		if ( scene->event_group->status == EVT_STS_FINISHED) {
			if ( extended_logging)
				ESP_LOGI(__func__,"scene '%s', event '%s' just finished", scene->id, scene->event_group->id );
		} else {
			finished = false;
		}

		break;
	}

	if (finished) {
		scene->status = EVT_STS_FINISHED;
		if ( extended_logging)
			ESP_LOGI(__func__, "scene '%s' finished", scene->id);
	}
}

// ********************** RESET functions *********************************************************

/**
 * resets all timing events starting from the given event
 */
static void reset_events(T_EVENT *events, char *msg) {
	if ( !events)
		return;

	for ( T_EVENT *evt = events; evt; evt=evt->nxt) {
		evt->status = EVT_STS_READY;
		if (extended_logging)
			ESP_LOGI(__func__, "%s: id=%d: status=%d, event '%s'", msg, evt->id, evt->status, eventype2text(evt->type));
	}
}


/**
 *  reset an event,
 *  except repeat parameter and working events
 */
void reset_event_group( T_EVENT_GROUP *evtgrp) {
	evtgrp->status = EVT_STS_READY;
	evtgrp->w_flags = 0;
	evtgrp->w_pos = 0;
	evtgrp->w_len_factor = 1.0;
	evtgrp->w_len_factor_delta = 0.0;
	evtgrp->w_speed = 0.0;
	evtgrp->w_acceleration = 0.0;
	evtgrp->w_brightness = 1.0;
	evtgrp->w_brightness_delta = 0.0;
	evtgrp->w_distance = 0.0;
	evtgrp->w_wait_time = 0;
	evtgrp->time = 0;
	evtgrp->delta_pos = 1;
	memset(evtgrp->w_object_oid,0, LEN_EVT_OID);

	reset_events(evtgrp->evt_init_list, "INIT");
	reset_events(evtgrp->evt_work_list, "WORK");
	reset_events(evtgrp->evt_final_list,"FINAL");

	if ( extended_logging)
		ESP_LOGI(__func__, "event '%s'", evtgrp->id);
}

/**
 * reset repeat parameter of an event
 * occurs if anything is played, reset also final events
 */
void reset_event_repeats(T_EVENT_GROUP *evt) {

	evt->w_t_repeats = evt->t_repeats;

	if ( extended_logging)
		ESP_LOGI(__func__, "event '%s' repeates=%d", evt->id, evt->w_t_repeats);

}

void reset_scene(T_SCENE *scene) {
	if ( extended_logging)
		ESP_LOGI(__func__, "scene '%s'", scene->id);
	scene->event_group = scene->event_groups;

	// reset event groups
	for( T_EVENT_GROUP *evtgrp = scene->event_groups; evtgrp; evtgrp=evtgrp->nxt) {
		reset_event_group(evtgrp);
		reset_event_repeats(evtgrp);
	}

}


