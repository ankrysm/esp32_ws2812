/*
 * process_events.c
 *
 *  Created on: 09.08.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

// **************************************************************************
// timing
// **************************************************************************

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


// **************************************************************************
// moving
// **************************************************************************

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

// **************************************************************************

// *** main function ******
// times in ms,
// scene_time useful for start a scene
// timer_period useful for startup and shutdown
void process_event(T_EVENT *evt, uint64_t scene_time, uint64_t timer_period) {

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

void reset_event( T_EVENT *evt) {
	evt->status = SCENE_IDLE;
	evt->sp_status = MOVE_IDLE;
}

