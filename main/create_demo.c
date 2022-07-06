/*
 * create_demo.c
 *
 *  Created on: 26.06.2022
 *      Author: ankrysm
 */



#include <stdio.h>
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_log.h"
#include "local.h"
#include "color.h"
#include "config.h"
#include "timer_events.h"
#include "create_events.h"

/*
void start_demo1() {

	T_COLOR_RGB c = {32,0,0};
	create_solid(
			1,    // start position
			20,   // len
			&c,   // fg color
			NULL, // bg_color
			5,    // inset
			5,    // outset
			1000, // t_start
			5000, // duration
			1000, // fade in time
			1000, // fade out time
			-1,
			NULL    // movement
			);

/ *	create_blank(
			1,     // start position
			-1,    // len
			0,     // t_start
			0,     // duration
			0,     // once
			NULL   // movement
			);
* /

	c.r=64;
	c.g=64;
	c.b=0;
	create_solid(
			40,     // start position
			15,     // len
			&c,     // fg color
			NULL,   // bg_color
			3,      // inset
			3,      // outset
			3000,   // t_start
			2000,   // duration
			200,    // fade in time
			500,    // fade out time
			3,
			NULL    // movement
	);

	/ *
	create_blank(
			1,       // start position
			-1,      // len
			12000,  // t_start
			0,       // duration
			0,       // once
			NULL     // no movement
			);
	* /
}
// */

void build_demo2(
		T_COLOR_RGB *fg_color // foreground color
) {
	int pos = 1;
	int len = 20;

	// build then event
	T_EVENT *evt = calloc(1, sizeof(T_EVENT));
	event_list_add(evt);

	evt->status = SCENE_IDLE;
	evt->type = EVT_SOLID;
	evt->pos = pos;
	evt->len = len;

	// if no color specified assume white
	evt->fg_color.r = fg_color? fg_color->r : 255;
	evt->fg_color.g = fg_color? fg_color->g : 255;
	evt->fg_color.b = fg_color? fg_color->b : 255;


	// location based effects
	T_SCENE_PARAMETER_LIST *l_br = calloc(1,sizeof(T_SCENE_PARAMETER_LIST));
	evt->l_brightness = l_br;

	T_SCENE_PARAMETER *sp1,*sp2, *sp3;

	sp1 = calloc(1, sizeof(T_SCENE_PARAMETER));
	l_br->sp_list = sp1;

	/* Brightness
	   /------\
	  /        \
	 /          \
	*/
	// ramp up
	sp1->v_start = 0;
	sp1->v_end = 100;
	sp1->length = 20; // in percent
	sp1->type_t = TR_EXPONENTIAL;
	sp1->type_a = 0; // doesn't matter here
	sp1->type_m = 0; // doen't matter here

	// constant middle
	sp2 = calloc(1, sizeof(T_SCENE_PARAMETER));
	sp1->nxt = sp2;

	sp1->v_start = 100;
	sp1->v_end = 100;
	sp1->length = 40; // in percent
	sp1->type_t = 0; // doesn't matter here
	sp1->type_a = 0; // doesn't matter here
	sp1->type_m = 0; // doesn't matter here

	sp3 = calloc(1, sizeof(T_SCENE_PARAMETER));
	sp2->nxt = sp3;

	// ramp down
	sp1->v_start = 100;
	sp1->v_end = 0;
	sp1->length = 40; // in percent
	sp1->type_t = TR_EXPONENTIAL;
	sp1->type_a = 0;// doesn't matter here
	sp1->type_m = 0; // doesn't matter here


}








