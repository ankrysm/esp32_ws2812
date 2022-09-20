/*
 * config.h
 *
 *  Created on: 12.06.2022
 *      Author: andreas
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define STORAGE_NAMESPACE "storage"
//#define STORAGE_KEY_CONFIG "config"


// flag values
//#define CFG_PERSISTENCE_MASK 0x00FF
#define CFG_AUTOPLAY         0x0001
#define CFG_SHOW_STATUS      0x0002 // first led will show status, not include in scenes
#define CFG_STRIP_DEMO       0x0004

// transient flags
//#define CFG_TRANSIENT_MASK   0xFF00
#define CFG_WITH_WIFI        0x0100

#define LEN_SCENEFILE 32

/*
typedef struct {
	uint32_t flags;
	uint32_t numleds;
	uint32_t cycle; // Timer cycle in ms
	char autoplayfile[LEN_SCENEFILE];
} T_CONFIG;
*/

#define CFG_KEY_FLAGS "flags"
#define CFG_KEY_NUMLEDS "numleds"
#define CFG_KEY_AUTOPLAY_FILE "autoplayfile"
#define CFG_KEY_CYCLE "cycle"


#endif /* CONFIG_H_ */
