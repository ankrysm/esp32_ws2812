/*
 * color.h
 *
 *  Created on: 25.06.2022
 *      Author: ankrysm
 */

#ifndef MAIN_COLOR_H_
#define MAIN_COLOR_H_

typedef struct {
	uint32_t r;
	uint32_t g;
	uint32_t b;
} T_COLOR_RGB;

typedef struct {
	uint32_t h;
	uint32_t s;
	uint32_t v;
} T_COLOR_HSV;

#endif /* MAIN_COLOR_H_ */
