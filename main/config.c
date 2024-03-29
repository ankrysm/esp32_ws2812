/*
 * config.c
 *
 *  Created on: 12.06.2022
 *      Author: andreas
 */


#include "esp32_ws2812.h"

extern uint32_t cfg_flags;
extern uint32_t cfg_trans_flags;
extern uint32_t cfg_numleds;
extern uint32_t cfg_cycle;
extern char *cfg_autoplayfile;
extern char *cfg_timezone;
extern char *cfg_ota_url;
extern char *cfg_name;
extern char *compile_date;
extern uint32_t extended_log;
extern char last_loaded_file[];

char sha256_hash_boot_partition[HASH_TEXT_LEN];
char sha256_hash_run_partition[HASH_TEXT_LEN];


esp_vfs_spiffs_conf_t fs_conf = {
  .base_path = "/spiffs",
  .partition_label = NULL,
  .max_files = 5,
  .format_if_mount_failed = true
};

nvs_handle_t my_handle=0;

void add_base_path(char *filename, size_t sz_filename) {
	char *f = strdup(filename);
	snprintf(filename,sz_filename,"%s/%s", fs_conf.base_path, f);
	free(f);
}

/**
 * store the gConfig blob into storage
 */
esp_err_t store_config() {

	nvs_handle_t my_handle=0;
	esp_err_t ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_open() failed (%s)", esp_err_to_name(ret));
		return ret;
	}
	/*
	size_t size = sizeof(gConfig);
	ret = nvs_set_blob(my_handle, STORAGE_KEY_CONFIG, &gConfig, size);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_set_blob() failed (%s)", esp_err_to_name(ret));
		return ret;
	}
	 */
	nvs_set_u32(my_handle, CFG_KEY_FLAGS, cfg_flags);
	nvs_set_u32(my_handle, CFG_KEY_NUMLEDS, cfg_numleds);
	nvs_set_u32(my_handle, CFG_KEY_CYCLE, cfg_cycle);

	nvs_set_str(my_handle, CFG_KEY_AUTOPLAY_FILE, cfg_autoplayfile ? cfg_autoplayfile : "");
	nvs_set_str(my_handle, CFG_KEY_TIMEZONE, cfg_timezone ? cfg_timezone : "");
	nvs_set_str(my_handle, CFG_KEY_OTA_URL, cfg_ota_url ? cfg_ota_url : "");
	nvs_set_str(my_handle, CFG_KEY_NAME, cfg_name ? cfg_name : "");

	nvs_set_u32(my_handle, CFG_KEY_EXTENDED_LOG, extended_log);

	ret = nvs_commit(my_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_commit() failed (%s)", esp_err_to_name(ret));
		return ret;
	}

	nvs_close(my_handle);

    ESP_LOGI(__func__, "done.");

	return ESP_OK;
}

esp_err_t load_config() {
	nvs_handle_t my_handle=0;
	bool store_needed = false;
	esp_err_t ret = ESP_FAIL;

	ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(__func__, "nvs_open() failed (%s)", esp_err_to_name(ret));
    	return ret;
    }

    do {

    	ret = nvs_get_u32(my_handle, CFG_KEY_FLAGS, &cfg_flags);
    	if (ret == ESP_OK) {
            ESP_LOGI(__func__, "retrieve '%s' successful: 0x%04x", CFG_KEY_FLAGS, cfg_flags);
    	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
    		store_needed = true;
    		cfg_flags = CFG_SHOW_STATUS;
            ESP_LOGI(__func__, "retrieve '%s' not found, initial value: 0x%04x", CFG_KEY_FLAGS, cfg_flags);
    	} else {
            ESP_LOGI(__func__, "retrieve '%s' failed, ret=%d", CFG_KEY_FLAGS, ret);
    		break;
    	}

    	ret = nvs_get_u32(my_handle, CFG_KEY_NUMLEDS, &cfg_numleds);
    	if (ret == ESP_OK) {
            ESP_LOGI(__func__, "retrieve '%s' successful: %d", CFG_KEY_NUMLEDS, cfg_numleds);
    	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
    		store_needed = true;
    		cfg_numleds = 60;
            ESP_LOGI(__func__, "retrieve '%s' not found, initial value: %d", CFG_KEY_NUMLEDS, cfg_numleds);
    	} else {
            ESP_LOGI(__func__, "retrieve '%s' failed, ret=%d", CFG_KEY_NUMLEDS, ret);
    		break;
    	}

    	ret = nvs_get_u32(my_handle, CFG_KEY_CYCLE, &cfg_cycle);
    	if (ret == ESP_OK) {
            ESP_LOGI(__func__, "retrieve '%s' successful: %d", CFG_KEY_CYCLE, cfg_cycle);
    	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
    		store_needed = true;
    		cfg_cycle = 50;
            ESP_LOGI(__func__, "retrieve '%s' not found, initial value: %d", CFG_KEY_CYCLE, cfg_cycle);
    	} else {
            ESP_LOGI(__func__, "retrieve '%s' failed, ret=%d", CFG_KEY_CYCLE, ret);
    		break;
    	}

    	size_t len;

    	// ***** autoplay file ********************************
    	if ( cfg_autoplayfile) {
    		free(cfg_autoplayfile);
    		cfg_autoplayfile = NULL;
    	}
    	ret = nvs_get_str(my_handle, CFG_KEY_AUTOPLAY_FILE, NULL, &len); ///call for length
        ESP_LOGI(__func__, "retrieve '%s' len = %d'", CFG_KEY_AUTOPLAY_FILE, len);

    	if (ret == ESP_OK) {
    		cfg_autoplayfile = calloc(len+1, sizeof(char));
        	nvs_get_str(my_handle, CFG_KEY_AUTOPLAY_FILE, cfg_autoplayfile, &len); // call for value
            ESP_LOGI(__func__, "retrieve '%s' successful: '%s'", CFG_KEY_AUTOPLAY_FILE, cfg_autoplayfile?cfg_autoplayfile:"");
    	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
    		// it is ok missing it
            ESP_LOGI(__func__, "retrieve '%s' not found", CFG_KEY_AUTOPLAY_FILE);
    	} else {
            ESP_LOGI(__func__, "retrieve '%s' failed, ret=%d", CFG_KEY_AUTOPLAY_FILE, ret);
    		break;
    	}

    	// ***** ota url ********************************
    	if ( cfg_ota_url) {
    		free(cfg_ota_url);
    		cfg_ota_url = NULL;
    	}
    	ret = nvs_get_str(my_handle, CFG_KEY_OTA_URL, NULL, &len); ///call for length
        ESP_LOGI(__func__, "retrieve '%s' len = %d'", CFG_KEY_OTA_URL, len);

    	if (ret == ESP_OK) {
    		cfg_ota_url = calloc(len+1, sizeof(char));
        	nvs_get_str(my_handle, CFG_KEY_OTA_URL, cfg_ota_url, &len); // call for value
            ESP_LOGI(__func__, "retrieve '%s' successful: '%s'", CFG_KEY_OTA_URL, cfg_ota_url?cfg_ota_url:"");
    	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
    		// it is ok missing it
            ESP_LOGI(__func__, "retrieve '%s' not found", CFG_KEY_OTA_URL);
    	} else {
            ESP_LOGI(__func__, "retrieve '%s' failed, ret=%d", CFG_KEY_OTA_URL, ret);
    		break;
    	}

    	// ****** time zone *************************
    	if ( cfg_timezone) {
    		free(cfg_timezone);
    		cfg_timezone = NULL;
    	}
    	ret = nvs_get_str(my_handle, CFG_KEY_TIMEZONE, NULL, &len); ///call for length
        ESP_LOGI(__func__, "retrieve '%s' len = %d'", CFG_KEY_TIMEZONE, len);

    	if (ret == ESP_OK) {
    		cfg_timezone = calloc(len+1, sizeof(char));
        	nvs_get_str(my_handle, CFG_KEY_TIMEZONE, cfg_timezone, &len); // call for value
            ESP_LOGI(__func__, "retrieve '%s' successful: '%s'", CFG_KEY_TIMEZONE, cfg_timezone?cfg_timezone:"");

    	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
    		// it is ok missing it
            ESP_LOGI(__func__, "retrieve '%s' not found", CFG_KEY_TIMEZONE);
    	} else {
            ESP_LOGI(__func__, "retrieve '%s' failed, ret=%d", CFG_KEY_TIMEZONE, ret);
    		break;
    	}

    	// ****** name *************************
    	if ( cfg_name) {
    		free(cfg_name);
    		cfg_name = NULL;
    	}
    	ret = nvs_get_str(my_handle, CFG_KEY_NAME, NULL, &len); ///call for length
        ESP_LOGI(__func__, "retrieve '%s' len = %d'", CFG_KEY_NAME, len);

    	if (ret == ESP_OK) {
    		cfg_name = calloc(len+1, sizeof(char));
        	nvs_get_str(my_handle, CFG_KEY_NAME, cfg_name, &len); // call for value
            ESP_LOGI(__func__, "retrieve '%s' successful: '%s'", CFG_KEY_NAME, cfg_name?cfg_name:"");

    	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
    		// it is ok missing it
            ESP_LOGI(__func__, "retrieve '%s' not found", CFG_KEY_NAME);
    	} else {
            ESP_LOGI(__func__, "retrieve '%s' failed, ret=%d", CFG_KEY_NAME, ret);
    		break;
    	}

    	// ********** extended_log ******************************
       	ret = nvs_get_u32(my_handle, CFG_KEY_EXTENDED_LOG, &extended_log);
        	if (ret == ESP_OK) {
                ESP_LOGI(__func__, "retrieve '%s' successful: %d", CFG_KEY_EXTENDED_LOG, extended_log);
        		global_set_extended_log(extended_log);
        	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        		store_needed = true;
        		global_set_extended_log(0);
                ESP_LOGI(__func__, "retrieve '%s' not found, initial value: %d", CFG_KEY_EXTENDED_LOG, extended_log);
        	} else {
                ESP_LOGI(__func__, "retrieve '%s' failed, ret=%d", CFG_KEY_EXTENDED_LOG, ret);
        		break;
        	}

    } while(0);

    // close handle immediately, if it's necessary to open it again, it will be done later
    nvs_close(my_handle);

    if ( store_needed) {
    	ret = store_config();
    	if (ret != ESP_OK) {
    		ESP_LOGE(__func__, "store_config() failed (%s)", esp_err_to_name(ret));
    		return ret;
    	}
    }

    return ESP_OK;

}

char *config2txt(char *txt, size_t sz) {
	snprintf(txt, sz,
			"config:\n" \
			"numleds=%d\n" \
			"autoplayfile=%s\n" \
			"timezone=%s\n" \
			"cfg_flags=0x%04x\n" \
			"  autoplay=%s\n" \
			"  showstatus=%s\n" \
			"cfg_trans_flags=0x%04x\n" \
			"  with_wifi=%s\n" \
			"  autoplay file loaded=%s\n" \
			"  autoplay started=%s\n" \
			"cycle=%d\n" \
			"ota_url=%s\n" \
			"extended_log=%d\n" \
			"name=%s\n",
			cfg_numleds,
			cfg_autoplayfile ? cfg_autoplayfile:"",
			cfg_timezone ? cfg_timezone:"",
			cfg_flags,
			(cfg_flags & CFG_AUTOPLAY ? "true" : "false"),
			(cfg_flags & CFG_SHOW_STATUS ? "true" : "false"),
			cfg_trans_flags,
			(cfg_trans_flags & CFG_WITH_WIFI ? "true" : "false"),
			(cfg_trans_flags & CFG_AUTOPLAY_LOADED ? "true" : "false" ),
			(cfg_trans_flags & CFG_AUTOPLAY_STARTED ? "true" : "false" ),
			cfg_cycle,
			cfg_ota_url ? cfg_ota_url : "",
			extended_log,
			cfg_name?cfg_name:""
	);
	return txt;
}


void add_config_informations(cJSON *root) {

	// persistent data
	//cJSON *root = cJSON_AddObjectToObject(element,"config");

	cJSON_AddNumberToObject(root, "numleds", cfg_numleds);
	cJSON_AddNumberToObject(root, "cycle", cfg_cycle);
	cJSON_AddStringToObject(root, "autoplay_file", cfg_autoplayfile && strlen(cfg_autoplayfile) ? cfg_autoplayfile : "");
	cJSON_AddStringToObject(root, "timezone", cfg_timezone && strlen(cfg_timezone) ? cfg_timezone : "");
	cJSON_addBoolean(root,  "autoplay", cfg_flags & CFG_AUTOPLAY );
	cJSON_AddStringToObject(root, "ota_url", cfg_ota_url && strlen(cfg_ota_url) ? cfg_ota_url : "");
	cJSON_addBoolean(root, "show_status", cfg_flags & CFG_SHOW_STATUS);
	cJSON_AddNumberToObject(root, "extended_log", extended_log);
	cJSON_AddStringToObject(root, "name", cfg_name ? cfg_name : "");

	// transient data
	cJSON *var = cJSON_AddObjectToObject(root,"work");

	cJSON_addBoolean(var, "with_wifi", cfg_trans_flags & CFG_WITH_WIFI );
	cJSON_addBoolean(var, "autoplay_file_loaded",cfg_trans_flags & CFG_AUTOPLAY_LOADED);
	cJSON_addBoolean(var, "autoplay_started",cfg_trans_flags & CFG_AUTOPLAY_STARTED);
	cJSON_AddStringToObject(var, "last_loaded_file", last_loaded_file);
}


esp_err_t storage_info(size_t *total, size_t *used) {
	*total=0;
	*used=0;
    esp_err_t ret = esp_spiffs_info(fs_conf.partition_label, total, used);
    return ret;
}

/**
 * Init Filesystem and permanent storage, initialize gConfig if necessary
 */
esp_err_t init_storage() {

    ESP_LOGI(__func__, "Initializing SPIFFS");


    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&fs_conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(__func__, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(__func__, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(__func__, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

#ifdef CONFIG_EXAMPLE_SPIFFS_CHECK_ON_START
    ESP_LOGI(__func__, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(fs_conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(__func__, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(__func__, "SPIFFS_check() successful");
    }
#endif

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(fs_conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(__func__, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(fs_conf.partition_label);
        return ret;
    } else {
        ESP_LOGI(__func__, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partiton size info.
    if (used > total) {
        ESP_LOGW(__func__, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(fs_conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            ESP_LOGE(__func__, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return ret;
        } else {
            ESP_LOGI(__func__, "SPIFFS_check() successful");
        }
    }

    // ** init Config ***

    return ESP_OK;
}

void get_sha256_partition_hashes() {
	get_sha256_of_bootloader_partition(sha256_hash_boot_partition, sizeof(sha256_hash_boot_partition));
	log_info(__func__, "sha256 hash of bootloader: %s", sha256_hash_boot_partition);
	get_sha256_of_running_partition(sha256_hash_run_partition, sizeof(sha256_hash_run_partition));
	log_info(__func__, "sha256 hash of running partition: %s", sha256_hash_run_partition);

}
