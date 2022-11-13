/*
 * create_config.c
 *
 *  Created on: 20.09.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812.h"

extern uint32_t cfg_flags;
extern uint32_t cfg_numleds;
extern uint32_t cfg_cycle;
extern char *cfg_autoplayfile;
extern char *cfg_timezone;

esp_err_t decode_json4config_root(char *content, char *errmsg, size_t sz_errmsg) {
	cJSON *tree = NULL;
	esp_err_t rc = ESP_FAIL;

	ESP_LOGI(__func__, "content='%s'", content);
	memset(errmsg, 0, sz_errmsg);

	char *attr;
	char sval[64];
	double val;
	bool bval;
	int ival;

	bool store_it = false;
	t_result lrc;
	do {
		tree = cJSON_Parse(content);
		if ( !tree) {
			snprintf(errmsg, sz_errmsg, "could not decode JSON-Data");
			break;
		}

		attr="numleds";
		lrc = evt_get_number(tree, attr, &val, errmsg, sz_errmsg);
		if ( lrc == RES_OK) {
			ival = val;
			if ( ival == cfg_numleds) {
				ESP_LOGI(__func__, "%s=%d not changed", attr, cfg_numleds);
			} else {
				if (set_numleds(ival) != ESP_OK) {
					snprintf(errmsg, sz_errmsg, "set_numleds(%d) failed ",ival);
					break;
				}
				cfg_numleds = ival;
				store_it = true;
				ESP_LOGI(__func__, "%s=%d changed", attr, cfg_numleds);
			}
		} else if ( lrc != RES_NOT_FOUND) {
			ESP_LOGE(__func__, "parse attribute '%s' failed: %s", attr, errmsg);
			break;
		}

		attr="cycle";
		lrc = evt_get_number(tree, attr, &val, errmsg, sz_errmsg);
		if (lrc == RES_OK) {
			ival = val;
			if ( ival == cfg_cycle) {
				ESP_LOGI(__func__, "%s=%d not changed", attr, cfg_cycle);
			} else {
				cfg_cycle = ival;
				store_it = true;
				ESP_LOGI(__func__, "%s=%d changed", attr, cfg_cycle);
			}
		} else if ( lrc != RES_NOT_FOUND) {
			ESP_LOGE(__func__, "parse attribute '%s' failed: %s", attr, errmsg);
			break;
		}

		attr="show_status";
		lrc = evt_get_bool(tree, attr, &bval, errmsg, sz_errmsg);
		if (lrc== RES_OK) {
			if ( bval == (cfg_flags &CFG_SHOW_STATUS)) {
				ESP_LOGI(__func__, "%s=%s not changed",
						attr, (cfg_flags &CFG_SHOW_STATUS)?"true":"false");
			} else {
				cfg_flags &= ~CFG_SHOW_STATUS;
				if ( bval)
					cfg_flags |= CFG_SHOW_STATUS;
				store_it = true;
				ESP_LOGI(__func__, "%s=%s changed",
						attr, (cfg_flags &CFG_SHOW_STATUS)?"true":"false");
			}
		} else if ( lrc != RES_NOT_FOUND) {
			ESP_LOGE(__func__, "parse attribute '%s' failed: %s", attr, errmsg);
			break;
		}

		attr="autoplay";
		lrc = evt_get_bool(tree, attr, &bval, errmsg, sz_errmsg);
		if (lrc== RES_OK) {
			if ( bval == (cfg_flags & CFG_AUTOPLAY)) {
				ESP_LOGI(__func__, "%s=%s not changed",
						attr, (cfg_flags & CFG_AUTOPLAY)?"true":"false");
			} else {
				cfg_flags &= ~CFG_AUTOPLAY;
				if ( bval)
					cfg_flags |= CFG_AUTOPLAY;
				store_it = true;
				ESP_LOGI(__func__, "%s=%s changed",
						attr, (cfg_flags & CFG_AUTOPLAY)?"true":"false");
			}
		} else if ( lrc != RES_NOT_FOUND) {
			ESP_LOGE(__func__, "parse attribute '%s' failed: %s", attr, errmsg);
			break;
		}

		attr="autoplay_file";
		lrc = evt_get_string(tree, attr, sval, sizeof(sval), errmsg, sz_errmsg);
		if ( lrc == RES_OK || lrc == RES_NO_VALUE) {
			if (!strcmp(cfg_autoplayfile?cfg_autoplayfile:"", sval)) {
				ESP_LOGI(__func__, "%s='%s' not changed",
						attr, cfg_autoplayfile?cfg_autoplayfile:"");
			} else {
				ESP_LOGI(__func__, "%s='%s' changed",
						attr, cfg_autoplayfile?cfg_autoplayfile:"");
				if ( cfg_autoplayfile)
					free(cfg_autoplayfile);
				cfg_autoplayfile = strlen(sval) ? strdup(sval) : NULL;
				store_it = true;
			}
		} else if ( lrc != RES_NOT_FOUND) {
			ESP_LOGE(__func__, "parse attribute '%s' failed: %s", attr, errmsg);
			break;
		}

		attr="timezone";
		lrc = evt_get_string(tree, attr, sval, sizeof(sval), errmsg, sz_errmsg);
		if ( lrc == RES_OK || lrc == RES_NO_VALUE) {
			if (!strcmp(cfg_timezone ? cfg_timezone:"", sval)) {
				ESP_LOGI(__func__, "%s='%s' not changed",
						attr, cfg_timezone ? cfg_timezone : "");
			} else {
				ESP_LOGI(__func__, "%s='%s' changed",
						attr, cfg_timezone ? cfg_timezone : "");
				if ( cfg_timezone)
					free(cfg_timezone);
				cfg_timezone = strlen(sval) ? strdup(sval) : NULL;
				store_it = true;
				set_timezone(cfg_timezone);
				log_current_time();
			}
		} else if ( lrc != RES_NOT_FOUND) {
			ESP_LOGE(__func__, "parse attribute '%s' failed: %s", attr, errmsg);
			break;
		}

		attr="strip_demo";
		lrc = evt_get_bool(tree, attr, &bval, errmsg, sz_errmsg);
		if (lrc== RES_OK) {
			if ( bval == (cfg_flags & CFG_STRIP_DEMO)) {
				ESP_LOGI(__func__, "%s=%s not changed",
						attr, (cfg_flags &CFG_SHOW_STATUS)?"true":"false");
			} else {
				cfg_flags &= ~CFG_STRIP_DEMO;
				if ( bval)
					cfg_flags |= CFG_STRIP_DEMO;
				store_it = true;
				ESP_LOGI(__func__, "%s=%s changed",
						attr, (cfg_flags & CFG_STRIP_DEMO)?"true":"false");
			}
		} else if ( lrc != RES_NOT_FOUND) {
			ESP_LOGE(__func__, "parse attribute '%s' failed: %s", attr, errmsg);
			break;
		}

		snprintf(errmsg,sz_errmsg,"success.");

		rc = ESP_OK;

	} while(false);

	if ( tree)
		cJSON_Delete(tree);

	if ( store_it) {
		set_event_timer_period(cfg_cycle);
		store_config();
		ESP_LOGI(__func__, "changed config stored");
	}

	ESP_LOGI(__func__, "done: %s", errmsg);
	return rc;
}


