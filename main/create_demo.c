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

/*	create_blank(
			1,     // start position
			-1,    // len
			0,     // t_start
			0,     // duration
			0,     // once
			NULL   // movement
			);
*/

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

	/*
	create_blank(
			1,       // start position
			-1,      // len
			12000,  // t_start
			0,       // duration
			0,       // once
			NULL     // no movement
			);
	*/
}
