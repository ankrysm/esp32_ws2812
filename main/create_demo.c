/*
 * create_demo.c
 *
 *  Created on: 26.06.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

void build_demo2(
		T_COLOR_RGB *fg_color // foreground color
) {
/*
	T_EVENT *evt;
	T_EVT_WHAT *w;
	T_EVT_TIME *tevt;

	evt = create_event(1000);
	if ( !evt)
		return;

	// * wait 3 secs,
	// * at position 10 paint 15 pixel in red for 5 secs then finished
	// * brightness 10 %
	evt->pos = 10;
	evt->flags = EVFL_WAIT; // start with wait
	evt->evt_time_list_repeats = 5;

	w = create_what(evt, 2001);
	if (!w)
		return;
	w->pos=0;
	w->len=15;
	w->type = WT_COLOR;
	T_NAMED_RGB_COLOR *nc = color4name("red");
	w->para.hsv.h = nc->hsv.h;
	w->para.hsv.s = nc->hsv.s;
	w->para.hsv.v = nc->hsv.v;


	tevt = create_timing_event(evt,1001);
	if (!tevt)
		return;

	tevt->type = ET_SET_BRIGHTNESS;
	tevt->time = 3000; // 3 sec, in ms
	//tevt->clear_flags = EVFL_WAIT; // clear wait flag
	tevt->value = 0.1; // new brightness

	tevt = create_timing_event(evt,1001);
	if (!tevt)
		return;
	tevt->type = ET_CLEAR;
	tevt->time = 8000; // 3 sec, in ms


	event_list_add(evt);

	//////////////////////

	evt = create_event(2000);
	if ( !evt)
		return;
	evt->speed = 2.0;
	evt->brightness = 0.1;
	evt->evt_time_list_repeats = 3;

	w = create_what(evt, 2001);
	if (!w)
		return;
	w->pos=0;
	w->len=5;
	w->type = WT_COLOR;
	nc = color4name("green");
	w->para.hsv.h = nc->hsv.h;
	w->para.hsv.s = nc->hsv.s;
	w->para.hsv.v = nc->hsv.v;


	w = create_what(evt, 2001);
	if (!w)
		return;
	w->pos=5;
	w->len=10;
	w->type = WT_COLOR_TRANSITION;
	nc = color4name("green");
	w->para.tr.hsv_from.h = nc->hsv.h;
	w->para.tr.hsv_from.s = nc->hsv.s;
	w->para.tr.hsv_from.v = nc->hsv.v;
	nc = color4name("blue");
	w->para.tr.hsv_to.h = nc->hsv.h;
	w->para.tr.hsv_to.s = nc->hsv.s;
	w->para.tr.hsv_to.v = nc->hsv.v;

	tevt = create_timing_event(evt,2101);
	if (!tevt)
		return;
	tevt->id =2101;
	tevt->type = ET_CLEAR;
	tevt->time = 2000; // 2 sec, in ms

	event_list_add(evt);
*/
}








