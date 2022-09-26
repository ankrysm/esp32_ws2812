/*
 * global_data.c
 *
 *  Created on: 19.09.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812.h"

T_SCENE *s_scene_list = NULL;
T_EVT_OBJECT *s_object_list = NULL;

uint32_t cfg_flags = 0;
uint32_t cfg_trans_flags = 0;
uint32_t cfg_numleds = 60;
uint32_t cfg_cycle = 50;
char *cfg_autoplayfile = NULL;


