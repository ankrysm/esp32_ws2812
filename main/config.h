/*
 * config.h
 *
 *  Created on: 12.06.2022
 *      Author: andreas
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define STORAGE_NAMESPACE "storage"
#define STORAGE_KEY_CONFIG "config"


// flag values
#define CFG_AUTOPLAY 0x0001

#define LEN_SCENEFILE 32

typedef struct {
	uint32_t flags;
	uint32_t numleds;
	uint32_t cycle; // Timer cycle in ms
	char autoplayfile[LEN_SCENEFILE];
} T_CONFIG;



#endif /* CONFIG_H_ */
