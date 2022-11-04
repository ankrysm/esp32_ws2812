/*
 * process_events.c
 *
 *  Created on: 09.08.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

extern T_TRACK tracks[];

static int extended_logging = true;


static void process_track_element_init(T_TRACK_ELEMENT *ele) {
	ESP_LOGI(__func__, "started");

	ele->status = EVT_STS_RUNNING; // in most cases
	ele->evt_grp_current_status = EVT_STS_READY;

	for(T_EVENT *evt = ele->evtgrp->evt_init_list; evt; evt=evt->nxt ) {

		if ( extended_logging) {
			char buf[64];
			event2text(evt, buf, sizeof(buf));
			ESP_LOGI(__func__,"INIT ele.id=%d, %s", ele->id, buf);
		}
		switch(evt->type) {

		case ET_WAIT: // wait for Statup
			ele->w_flags |= EVFL_WAIT;
			ele->w_wait_time = evt->para.value;
			ele->status = EVT_STS_STARTING;
			break;

		case ET_WAIT_FIRST: /// wait for first Statup
			if ( ele->w_flags & EVFL_WAIT_FIRST_DONE) {
				break; // not at first init
			}
			ele->w_flags |= EVFL_WAIT;
			ele->w_wait_time = evt->para.value;
			ele->status = EVT_STS_STARTING;
			break;

		case ET_CLEAR:
			ele->w_flags |= EVFL_CLEARPIXEL;
			break;

		case ET_SPEED:
			ele->w_speed = evt->para.value;
			break;

		case ET_SPEEDUP:
			ele->w_acceleration = evt->para.value;
			break;

		case ET_GOTO_POS:
			ele->w_pos = evt->para.value;
			break;

		case ET_SET_BRIGHTNESS:
			ele->w_brightness = evt->para.value;
			break;

		case ET_SET_BRIGHTNESS_DELTA:
			ele->w_brightness_delta = evt->para.value;
			break;

		case ET_SET_OBJECT:
			if (strlen(evt->para.svalue)) {
				strlcpy(ele->w_object_oid, evt->para.svalue, sizeof(ele->w_object_oid));
				if ( extended_logging)
					ESP_LOGI(__func__,"ele.id=%d, tevt.id=%d: new object_oid='%s'", ele->id, evt->id, ele->w_object_oid);
			}
			break;

		case ET_BMP_OPEN:
			bmp_open_url(ele->w_object_oid);
			break;

		default:
			break;
		}
	}
}

static void process_track_element_starting(T_TRACK_ELEMENT *ele, uint64_t scene_time, uint64_t timer_period){
	//ESP_LOGI(__func__, "started");

	int nproc=0;
	for(T_EVENT *evt = ele->evtgrp->evt_init_list; evt; evt=evt->nxt ) {

		switch (evt->type) {
		case ET_WAIT:
		case ET_WAIT_FIRST:
			nproc++;
			ele->w_wait_time -= timer_period;
			if (ele->w_wait_time <=0) {
				// timer ends, event done, reset wait flag
				ele->status = EVT_STS_RUNNING;
				ele->w_flags &= ~EVFL_WAIT;
				if ( extended_logging) {
					char buf[64];
					event2text(evt, buf, sizeof(buf));
					ESP_LOGI(__func__,"finished ele.id=%d, %s'", ele->id, buf);
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
		ele->status = EVT_STS_RUNNING;
	}
}


static void process_track_element_final(T_TRACK_ELEMENT *ele) {
	ESP_LOGI(__func__, "started");
	for(T_EVENT *evt = ele->evtgrp->evt_final_list; evt; evt=evt->nxt ) {

		if ( extended_logging) {
			char buf[64];
			event2text(evt, buf, sizeof(buf));
			ESP_LOGI(__func__,"FINAL ele.id=%d, pos=%.2f, %s", ele->id, ele->w_pos, buf);
		}
		switch(evt->type) {

		case ET_CLEAR:
			ele->w_flags |= EVFL_CLEARPIXEL;
			break;

		case ET_BMP_CLOSE:
			bmp_stop_processing();
			ele->w_bmp_remaining_lines = 0;
			break;

		default:
			break;
		}
	}
}


// process the current working event of a track element,
// if finished, start the next
// time values in ms
// sets several parameters
static void  process_track_element_work(T_TRACK_ELEMENT *ele, uint64_t scene_time, uint64_t timer_period) {
	if (!ele->evt_work_current) {
		ESP_LOGW(__func__, "ele.id=%d: no evt_work_current", ele->id );
		return; // no events
	}

	// ************ WORK Events **************************
	while(ele->evt_work_current) {

		if (ele->evt_grp_current_status == EVT_STS_READY ) {
			// initialize work event
			ESP_LOGI(__func__, "ele.id=%d, evt.id=%d READY", ele->id, ele->evt_work_current->id);
			ele->evt_grp_current_status = EVT_STS_FINISHED; // in most cases

			switch(ele->evt_work_current->type) {
			case ET_WAIT:
				ele->w_flags |= EVFL_WAIT;
				ele->w_wait_time = ele->evt_work_current->para.value;
				ele->evt_grp_current_status = EVT_STS_RUNNING;
				break;
			case ET_PAINT:
				ele->w_wait_time = ele->evt_work_current->para.value;
				ele->evt_grp_current_status = EVT_STS_RUNNING;
				break;
			case ET_DISTANCE:
				ele->w_distance = ele->evt_work_current->para.value;
				ele->evt_grp_current_status = EVT_STS_RUNNING;
				break;
			case ET_SPEED:
				ele->w_speed = ele->evt_work_current->para.value;
				break;
			case ET_SPEEDUP:
				ele->w_acceleration = ele->evt_work_current->para.value;
				break;
			case ET_BOUNCE:
				ele->w_speed = -ele->w_speed;
				break;
			case ET_REVERSE:
				ele->delta_pos = ele->delta_pos < 0 ? +1 : -1;
				break;
			case ET_GOTO_POS:
				ele->w_pos = ele->evt_work_current->para.value;
				break;
			case ET_CLEAR:
				ele->w_flags |= EVFL_CLEARPIXEL;
				break;
			case ET_SET_BRIGHTNESS:
				ele->w_brightness = ele->evt_work_current->para.value;
				break;
			case ET_SET_BRIGHTNESS_DELTA:
				ele->w_brightness_delta = ele->evt_work_current->para.value;
				break;
			case ET_SET_OBJECT:
				if (strlen(ele->evt_work_current->para.svalue)) {
					strlcpy(ele->w_object_oid, ele->evt_work_current->para.svalue, sizeof(ele->w_object_oid));
					if ( extended_logging)
						ESP_LOGI(__func__,"ele.id=%d, evt.id=%d: new object_oid='%s'", ele->id, ele->evt_work_current->id, ele->w_object_oid);
				}
				break;
			case ET_BMP_OPEN:
				bmp_open_url(ele->w_object_oid);
				break;
			case ET_BMP_READ:
				ele->w_bmp_remaining_lines = ele->evt_work_current->para.value;
				ele->evt_grp_current_status = EVT_STS_RUNNING;
				break;
			case ET_BMP_CLOSE:
				bmp_stop_processing();
				ele->w_bmp_remaining_lines = 0;
				break;

			default:
				//ESP_LOGW(__func__,"evt.id=%d, tevt.id=%d: timer of %llu ms, type %d expired, NYI", evt->id, tevt->id, tevt->time, tevt->type);
				break;
			}

		} // if READY

		if ( ele->evt_grp_current_status == EVT_STS_FINISHED) {
			// current event finished, try the next
			if ( extended_logging) {
				char buf[64];
				event2text(ele->evt_work_current, buf, sizeof(buf));
				ESP_LOGI(__func__,"event finished ele.id=%d, %s", ele->id, buf);

			}
			ele->evt_work_current = ele->evt_work_current->nxt;
			ele->evt_grp_current_status = EVT_STS_READY;
			continue;
		}

		// **** here always: EVT_STS_RUNNING ******
		switch (ele->evt_work_current->type) {
		case ET_WAIT:
			ele->w_wait_time -= timer_period;
			if (ele->w_wait_time <=0) {
				// timer ends, event done, reset wait flag
				ele->evt_grp_current_status = EVT_STS_FINISHED;
				ele->w_flags &= ~EVFL_WAIT;
			}
			break;
		case ET_PAINT:
			ele->w_wait_time -= timer_period;
			if (ele->w_wait_time <=0) {
				// timer ends
				ele->evt_grp_current_status = EVT_STS_FINISHED;
			}
			break;
		case ET_DISTANCE:
			ele->w_distance -= fabs(ele->w_speed);
			if ( ele->w_distance <= 0.0) {
				// distance reached
				ele->evt_grp_current_status = EVT_STS_FINISHED;
			}
			break;
		case ET_BMP_READ:
			if ( !get_is_bmp_reading()) {
				ESP_LOGI(__func__,"bmp_read_data: all data read");
				ele->evt_grp_current_status = EVT_STS_FINISHED;
				break;
			}

			// if remaining lines < 0 wait for EOF
			if (ele->w_bmp_remaining_lines > 0) {
				ESP_LOGI(__func__,"remaining lines %lld",ele->w_bmp_remaining_lines);
				(ele->w_bmp_remaining_lines)--;
				if ( ele->w_bmp_remaining_lines == 0) {
					ESP_LOGI(__func__,"bmp_read_data: all lines read");
					bmp_stop_processing();
					ele->evt_grp_current_status = EVT_STS_FINISHED;
				}
			}
			break;
		default:
			// should not happen here
			ele->evt_grp_current_status = EVT_STS_FINISHED;
			break;
		}

		// if a running event finished
		if ( ele->evt_grp_current_status == EVT_STS_FINISHED) {
			ele->evt_work_current = ele->evt_work_current->nxt;
			ele->evt_grp_current_status = EVT_STS_READY;
			continue;
		}
		break; // because of the RUNNING event not finished yet
	} // while

	if ( ! ele->evt_work_current) {
		// no more event, track element finished
		ele->status = EVT_STS_FINISHED;
		ESP_LOGI(__func__, "ele.id=%d: ALL DONE", ele->id);
	}

//	if (!evt) {
//		// no more events
//		check_for_repeat(evtgrp);
//		if ( evtgrp->status == EVT_STS_FINISHED) {
//			// all done
//			ESP_LOGI(__func__, "evt.id='%s': repeat events (%d/%d) ALL DONE", evtgrp->id, evtgrp->w_t_repeats, evtgrp->t_repeats);
//			return;
//		}
//		// next turn, reset events
//		reset_event_group(evtgrp);
//		evtgrp->w_flags |= EVFL_WAIT_FIRST_DONE;
//		// status is set to READY
//		//needs to be RUNNING
//		//evtgrp->status = EVT_STS_RUNNING;
//	}
}

// ************************************************************************************



static void process_track(T_TRACK *track, uint64_t scene_time, uint64_t timer_period) {

	if ( track->status ==  EVT_STS_FINISHED) {
		return; // all elements finished, track completed, nothing to do anymore
	}

	track->status = EVT_STS_RUNNING;

	while ( track->current_element) {

		if ( track->current_element->status == EVT_STS_READY) {
			// **** process INIT events *************
			process_track_element_init(track->current_element);
		}

		if ( track->current_element->status == EVT_STS_STARTING) {
			// **** process STARTING events (wait) ****
			process_track_element_starting(track->current_element, scene_time, timer_period);
			if ( track->current_element->status == EVT_STS_STARTING ) {
				// starting events in progress, enough for now
				return;
			}
		}

		// here the status is always "running"

		//ESP_LOGI(__func__, "start process_event_when evtgrp=%s, t=%llu", evtgrp->id, scene_time);
		process_track_element_work(track->current_element, scene_time, timer_period);

		if ( track->current_element->status == EVT_STS_FINISHED ) {
			process_track_element_final(track->current_element);
			process_object(track->current_element);

			// next track element
			track->current_element = track->current_element->nxt;
			continue;
			//return; // not necessary to do more
		}

		//ESP_LOGI(__func__, "start process_event_what evt=%d", evt->id);
		process_object(track->current_element);

		// next timestep
		track->current_element->time += timer_period;

		if ( track->current_element->w_flags & EVFL_WAIT )
			return; // no changes while wait

		// calculate speed and length factor
		// v = a * t
		// Δv = a * Δt
		// speed is leds per ms
		track->current_element->w_speed += track->current_element->w_acceleration;

		track->current_element->w_len_factor += track->current_element->w_len_factor_delta;
		if ( track->current_element->w_len_factor < 0.0 ) {
			track->current_element->w_len_factor = 0.0;
		} else if ( track->current_element->w_len_factor > 1.0 ) {
			track->current_element->w_len_factor = 1.0;
		}

		track->current_element->w_brightness += track->current_element->w_brightness_delta;
		if ( track->current_element->w_brightness < 0.0)
			track->current_element->w_brightness = 0.0;
		else if(track->current_element->w_brightness > 1.0)
			track->current_element->w_brightness = 1.0;

		track->current_element->w_pos += track->current_element->w_speed;

		// enough for a running track element
		break;
	}

	if ( ! track->current_element) {
		ESP_LOGI(__func__, "track %d finished", track->id);
		track->status = EVT_STS_FINISHED;
	}
}

int process_tracks(uint64_t scene_time, uint64_t timer_period) {
	int active_tracks=0;

	for ( int i=0; i < N_TRACKS; i++) {
		T_TRACK *track = &(tracks[i]);
		if ( ! track->element_list)
			continue; // nothing to process

		process_track(track, scene_time, timer_period);
		if ( track->status != EVT_STS_FINISHED)
			active_tracks++;
	}
	return active_tracks;
}

// ********************** RESET functions *********************************************************


static void reset_track_element(T_TRACK_ELEMENT *ele) {
	ele->status = EVT_STS_READY;
	ele->evt_grp_current_status = EVT_STS_READY;
	ele->w_repeats = ele->repeats;
	ele->w_flags = 0;
	ele->w_pos = 0;
	ele->w_len_factor = 1.0;
	ele->w_len_factor_delta = 0.0;
	ele->w_speed = 0.0;
	ele->w_acceleration = 0.0;
	ele->w_brightness = 1.0;
	ele->w_brightness_delta = 0.0;
	ele->w_distance = 0.0;
	ele->w_wait_time = 0;
	ele->w_bmp_remaining_lines = 0;
	ele->time = 0;
	ele->delta_pos = 1;
	ele->evt_work_current = ele->evtgrp ? ele->evtgrp->evt_work_list : NULL;
	memset(ele->w_object_oid,0, LEN_EVT_OID);
}

void reset_tracks() {
	ESP_LOGI(__func__,"start");
	for ( int i=0; i < N_TRACKS; i++) {
		T_TRACK *track = &(tracks[i]);
		if ( ! track->element_list)
			continue; // nothing to reset
		for( T_TRACK_ELEMENT *ele = track->element_list; ele; ele = ele->nxt) {
			reset_track_element(ele);
		}
		track->current_element = track->element_list;
		track->status = EVT_STS_READY;
	}
}

