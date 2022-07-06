/*
 * event_util.c
 *
 *  Created on: 05.07.2022
 *      Author: andreas
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"
#include "timer_events.h"
#include "math.h"
#include "color.h"
#include "led_strip.h"
#include "led_strip_proto.h"


extern T_EVENT *s_event_list;

/**
 * calculates the real values for pos and len
 */
void real_pos_len(T_EVENT *evt, int32_t *pos, int32_t *len) {
	int32_t p_end = evt->w_pos + evt->w_len;
	*len = -1;
	*pos = evt->w_pos < 0 ? 0 : evt->w_pos > strip_get_numleds() ? -1 : evt->w_pos;
	if ( *pos < 0 || p_end < 0)
		return ; // out of range
	*len = *pos - p_end;
}

// clear strip a needed
void check_clear_strip(T_EVENT *evt) {
	if ( evt->w_flags & EVENT_FLAG_BIT_STRIP_CLEARED)
		return; // already cleared
	strip_set_color(0, strip_get_numleds(), 0, 0, 0);
	evt->w_flags |= EVENT_FLAG_BIT_STRIP_SHOW_NEEDED | EVENT_FLAG_BIT_STRIP_CLEARED;
}

/**
 * frees scene parameter list
 */
void scene_parameter_list_free(T_SCENE_PARAMETER_LIST **list) {
	if ( !*list)
		return;

	T_SCENE_PARAMETER *nxt, *sp;
	sp = (*list)->sp_list;
	if (sp) {
		while (sp) {
			nxt =sp->nxt;
			free(sp);
			sp = nxt;
		}
	}

	free(*list);
	*list = NULL;
}

T_SCENE_PARAMETER_LIST *scene_parameter_list_create(int32_t pos, int32_t len, int32_t repeat) {
	T_SCENE_PARAMETER_LIST *list= calloc(1, sizeof(T_SCENE_PARAMETER_LIST));
	list->pos = pos;
	list->len = len;
	list->repeat = repeat;
	return list;
}

void scene_parameter_list_add_parameter(T_SCENE_PARAMETER_LIST *list, T_SCENE_PARAMETER *sp) {
	if ( list->sp_list) {
		T_SCENE_PARAMETER *t;
		for( t = list->sp_list; t->nxt; t=t->nxt){}
		t->nxt = sp;
	} else {
		list->sp_list = sp;
	}
}


/**
 * frees the event list
 */
void event_list_free() {
	if ( !s_event_list)
		return;

	T_EVENT *nxt;
	while (s_event_list) {
		nxt = s_event_list->nxt;
		//if ( s_event_list->movement) free(s_event_list->movement);
		scene_parameter_list_free(&(s_event_list->l_brightness));
		scene_parameter_list_free(&(s_event_list->l_color));
		scene_parameter_list_free(&(s_event_list->t_moving));
		scene_parameter_list_free(&(s_event_list->t_brightness));
		scene_parameter_list_free(&(s_event_list->t_color));
		scene_parameter_list_free(&(s_event_list->t_size));
		if ( s_event_list->data) free(s_event_list->data);
		//if ( s_event_list->bg_color) free(s_event_list->bg_color);
		free(s_event_list);
		s_event_list = nxt;
	}
	// after this s_event_list is NULL
}

/**
 * adds an event
 */
void event_list_add(T_EVENT *evt) {
	if ( s_event_list) {
		// add at the end of the list
		T_EVENT *t;
		for (t=s_event_list; t->nxt; t=t->nxt){}
		evt->lfd = t->lfd +1;
		t->nxt = evt;
	} else {
		// first entry
		evt->lfd = 1;
		s_event_list = evt;
	}
}

/**
 * process location base brightness
 */
void process_l_brightness(T_EVENT *evt ) {

}

