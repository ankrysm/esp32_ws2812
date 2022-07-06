/*
 * local.h
 *
 *  Created on: 10.05.2022
 *      Author: ankrysm
 */

#ifndef MAIN_LOCAL_H_
#define MAIN_LOCAL_H_

#include "color.h"
#include "led_strip.h"

void init_restservice();
void server_stop();
void initialise_mdns(void);
void initialise_netbios();


void build_demo2(
		T_COLOR_RGB *fg_color // foreground color
) ;

//void strip_rotate(int32_t dir);
//void led_strip_main();

#endif /* MAIN_LOCAL_H_ */
