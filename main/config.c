/*
 * config.c
 *
 *  Created on: 12.06.2022
 *      Author: andreas
 */



#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
//#include "protocol_examples_common.h"
//#if CONFIG_EXAMPLE_WEB_DEPLOY_SD
//#include "driver/sdmmc_host.h"
//#endif
#include "local.h"
#include <stdio.h>
#include "config.h"

//static const char *TAG = "config";

T_CONFIG gConfig;

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


/**
 * Init Filesystem and permanent storage, initialize gConfig if necessary
 */
esp_err_t init_storage() {

    ESP_LOGI(__func__, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

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

//    // get WIFI config
//    char *keyname=STORAGE_KEY_WIFI_CONFIG_SSID;
//    size = 64;
//    ret = nvs_get_str(my_handle, keyname, NULL, &size);
//    if ( ret == ESP_OK ) {
//    	// known
//    	gWifiConfig.ssid = malloc(size);
//    	nvs_get_str(my_handle, keyname, gWifiConfig.ssid, &size);
//        ESP_LOGI(__func__, "getting '%s' successful sz=%d: '%s'",keyname, size,gWifiConfig.ssid );
//
//    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
//    	// nothing stored
//        ESP_LOGI(__func__, "nothing stored for '%s'", keyname);
//        gWifiConfig.ssid = strdup("");
//    }
//
//    keyname=STORAGE_KEY_WIFI_CONFIG_PW;
//    size = 64;
//    ret = nvs_get_str(my_handle, keyname, NULL, &size);
//     if ( ret == ESP_OK ) {
//     	// known
//     	gWifiConfig.pw = malloc(size);
//     	nvs_get_str(my_handle, keyname, gWifiConfig.pw, &size);
//        ESP_LOGI(__func__, "getting '%s' successful sz=%d: '%s'",keyname, size,gWifiConfig.pw );
//
//     } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
//     	// nothing stored, store an emtpy string
//         ESP_LOGI(__func__, "nothing stored for '%s'", keyname);
//         gWifiConfig.pw = strdup("");
//     }
//
//     ESP_LOGI(__func__, "stored Wifi-Connection '%s', '%s'",(gWifiConfig.ssid?gWifiConfig.ssid:"<null>"),(gWifiConfig.pw?gWifiConfig.pw:"<null>") );

    size = sizeof(gConfig);
    ret = nvs_get_blob(my_handle, STORAGE_KEY_CONFIG, &gConfig, &size);
    // close handle immediately, if it's necessary to open it again, it will be done later
    nvs_close(my_handle);

    if  ( ret == ESP_OK ) {
        ESP_LOGI(__func__, "nvs_get_blob() successful");

    } else if ( ret == ESP_ERR_NVS_NOT_FOUND ) {
    	// has to initialized
    	snprintf(gConfig.scenefile, LEN_SCENEFILE, "%s", "scenes");
    	gConfig.flags = CFG_AUTOPLAY | CFG_REPEAT;
    	gConfig.cycle = 100;
    	gConfig.numleds = 12;

    	ret = store_config();
        if (ret != ESP_OK) {
            ESP_LOGE(__func__, "store_config() failed (%s)", esp_err_to_name(ret));
        	return ret;
        }

    } else {
        ESP_LOGE(__func__, "nvs_get_blob() failed (%s)", esp_err_to_name(ret));
    	return ret;
    }

    return ESP_OK;

    /*
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(__func__, "Opening file");
    FILE* f = fopen("/spiffs/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(__func__, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello World!\n");
    fclose(f);
    ESP_LOGI(__func__, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/spiffs/foo.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/spiffs/foo.txt");
    }

    // Rename original file
    ESP_LOGI(__func__, "Renaming file");
    if (rename("/spiffs/hello.txt", "/spiffs/foo.txt") != 0) {
        ESP_LOGE(__func__, "Rename failed");
        return;
    }

    // Open renamed file for reading
    ESP_LOGI(__func__, "Reading file");
    f = fopen("/spiffs/foo.txt", "r");
    if (f == NULL) {
        ESP_LOGE(__func__, "Failed to open file for reading");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(__func__, "Read from file: '%s'", line);

    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(__func__, "SPIFFS unmounted");
	*/
}

char *config2txt(char *txt, size_t sz) {
	snprintf(txt, sz,
			"config:\n" \
			"numleds=%d\n" \
			"scenefile=%s\n" \
			"autostart=%s\n" \
			"repeat=%s\n" \
			"cycle=%d\n",
			gConfig.numleds,
			gConfig.scenefile,
			(gConfig.flags & CFG_AUTOPLAY ? "true" : "false"),
			(gConfig.flags & CFG_REPEAT ? "true" : "false"),
			gConfig.cycle
	);
	return txt;
}
