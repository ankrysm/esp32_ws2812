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
			1,
			20,
			&c,
			NULL,
			5,
			5,
			1000,
			5000,
			1000,
			1000,
			0
			);

	create_blank(
			1,
			-1,
			0,
			0,
			0
			);

	c.r=64;
	c.g=64;
	c.b=0;
	create_solid(
			20,
			20,
			&c,
			NULL,
			5,
			5,
			3000,
			4000,
			1000,
			1000,
			0
			);
	create_blank(
			1,
			-1,
			12000,
			0,
			0
			);

}
