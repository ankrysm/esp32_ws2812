/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Update the system time from time stored in NVS.
 *
 */

esp_err_t update_time_from_nvs(void);

/**
 * @brief Fetch the current time from SNTP and stores it in NVS.
 *
 */
esp_err_t fetch_and_store_time_in_nvs(void*);

/**
 * @brief init time service
 */
esp_err_t init_time_service();

/**
 * @brief get current date string
 */
void get_current_timestamp(char *tbuf, size_t sz_tbuf);

/**
 * convert time_t to a verbose date string
 */
void get_time4(time_t seconds, char *tbuf, size_t sz_tbuf);

/**
 * convert time_t to a short date string
 */
void get_shorttime4(time_t now, char *tbuf, size_t sz_tbuf);

/**
 * @brief set the timezone
 */
void set_timezone(char *tz);

/**
 * @brief get current date string
 */
void get_current_timestamp(char *tbuf, size_t sz_tbuf);

/**
 * @brief set the timezone
 */
void set_timezone(char *tz);

#ifdef __cplusplus
}
#endif
