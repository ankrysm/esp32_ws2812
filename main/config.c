/*
 * config.c
 *
 *  Created on: 12.06.2022
 *      Author: andreas
 */


#include "esp32_ws2812_basic.h"
#include "config.h"


T_CONFIG gConfig;

static esp_vfs_spiffs_conf_t conf = {
  .base_path = "/spiffs",
  .partition_label = NULL,
  .max_files = 5,
  .format_if_mount_failed = true
};

/**
 * store the gConfig blob into storage
 */
esp_err_t store_config() {

    nvs_handle_t my_handle;

	esp_err_t ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_open() failed (%s)", esp_err_to_name(ret));
		return ret;
	}
	size_t size = sizeof(gConfig);
	ret = nvs_set_blob(my_handle, STORAGE_KEY_CONFIG, &gConfig, size);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_set_blob() failed (%s)", esp_err_to_name(ret));
		return ret;
	}

	ret = nvs_commit(my_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_commit() failed (%s)", esp_err_to_name(ret));
		return ret;
	}

	nvs_close(my_handle);

    ESP_LOGI(__func__, "done.");

	return ESP_OK;
}

esp_err_t storage_info(size_t *total, size_t *used) {
	*total=0;
	*used=0;
    esp_err_t ret = esp_spiffs_info(conf.partition_label, total, used);
    return ret;
}

/**
 * Init Filesystem and permanent storage, initialize gConfig if necessary
 */
esp_err_t init_storage() {

    ESP_LOGI(__func__, "Initializing SPIFFS");


    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

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
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(__func__, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(__func__, "SPIFFS_check() successful");
    }
#endif

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(__func__, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return ret;
    } else {
        ESP_LOGI(__func__, "Partition size: total: %d, used: %d", total, used);
    }


    // Check consistency of reported partiton size info.
    if (used > total) {
        ESP_LOGW(__func__, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
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
    memset(&gConfig, 0, sizeof(gConfig));

    size_t size;

    nvs_handle_t my_handle;
    ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(__func__, "nvs_open() failed (%s)", esp_err_to_name(ret));
    	return ret;
    }

    size = sizeof(gConfig);
    ret = nvs_get_blob(my_handle, STORAGE_KEY_CONFIG, &gConfig, &size);
    // close handle immediately, if it's necessary to open it again, it will be done later
    nvs_close(my_handle);

    if  ( ret == ESP_OK ) {
        ESP_LOGI(__func__, "nvs_get_blob() successful");

    } else if ( ret == ESP_ERR_NVS_NOT_FOUND ) {
    	// has to initialized
    	snprintf(gConfig.autoplayfile, LEN_SCENEFILE, "%s", "autoplay");
    	gConfig.flags = CFG_AUTOPLAY | CFG_SHOW_STATUS;
    	gConfig.cycle = 50;
    	gConfig.numleds = 60;

    	ret = store_config();
        if (ret != ESP_OK) {
            ESP_LOGE(__func__, "store_config() failed (%s)", esp_err_to_name(ret));
        	return ret;
        }

    } else {
        ESP_LOGE(__func__, "nvs_get_blob() failed (%s)", esp_err_to_name(ret));
    	return ret;
    }
    gConfig.flags &= CFG_PERSISTENCE_MASK;
    return ESP_OK;

}

char *config2txt(char *txt, size_t sz) {
	snprintf(txt, sz,
			"config:\n" \
			"numleds=%d\n" \
			"autoplayfile=%s\n" \
			"autoplay=%s\n" \
			"showstatus=%s\n" \
			"with_wifi=%s\n" \
			"cycle=%d\n" ,
			gConfig.numleds,
			gConfig.autoplayfile,
			(gConfig.flags & CFG_AUTOPLAY ? "true" : "false"),
			(gConfig.flags & CFG_SHOW_STATUS ? "true" : "false"),
			(gConfig.flags & CFG_WITH_WIFI ? "true" : "false"),
			gConfig.cycle
	);
	return txt;
}
