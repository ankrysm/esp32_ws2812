/*
 * timer_events.h
 *
 *  Created on: 25.06.2022
 *      Author: andreas
 */

#ifndef MAIN_TIMER_EVENTS_H_
#define MAIN_TIMER_EVENTS_H_

typedef enum {
	EVT_NOTHING, // nothing to do
	EVT_BLANK, // switch off all leds
	EVT_SOLID, // one color for all
	EVT_BLINKING, // blinking 
	EVT_RAIBNOW, // show a rainbow
	EVT_SPARKLE, // light on random places
	EVT_MOVE
} strip_event_type;

typedef enum {
	SCENES_STOPPED,
	SCENES_RUNNING,
	SCENES_PAUSED
} run_status_type;

void init_timer_events(int delta_ms);
void scenes_start();
void scenes_stop();
void scenes_pause();

#endif /* MAIN_TIMER_EVENTS_H_ */
