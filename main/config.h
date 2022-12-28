/*
 * config.h
 *
 *  Created on: 12.06.2022
 *      Author: andreas
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define STORAGE_NAMESPACE "storage"


// persistent flag values
#define CFG_AUTOPLAY         0x0001
#define CFG_SHOW_STATUS      0x0002 // first led will show status, not include in scenes
//#define CFG_STRIP_DEMO       0x0004

// transient flags
#define CFG_WITH_WIFI        0x0100
#define CFG_AUTOPLAY_LOADED  0x0200
#define CFG_AUTOPLAY_STARTED 0x0400

#define LEN_SCENEFILE 32

#define CFG_KEY_FLAGS "flags"
#define CFG_KEY_NUMLEDS "numleds"
#define CFG_KEY_AUTOPLAY_FILE "autoplayfile"
#define CFG_KEY_CYCLE "cycle"
#define CFG_KEY_OTA_URL "ota_url"
#define CFG_KEY_TIMEZONE "timezone"
#define CFG_KEY_EXTENDED_LOG "extended_log"
#define CFG_KEY_NAME "name"


#endif /* CONFIG_H_ */
