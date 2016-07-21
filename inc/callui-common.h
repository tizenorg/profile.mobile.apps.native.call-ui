/*
 * Copyright (c) 2009-2016 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#ifndef __CALLUI_COMMON_H__
#define __CALLUI_COMMON_H__

#include "callui.h"
#include "callui-common-types.h"
#include "callui-common-defines.h"

typedef enum {
	CALLUI_LOCK_TYPE_UNLOCK = 1,
	CALLUI_LOCK_TYPE_SWIPE_LOCK,
	CALLUI_LOCK_TYPE_SECURITY_LOCK
} callui_idle_lock_type_t;

typedef void (*set_call_duration_time)(struct tm *cur_time, Evas_Object *obj, const char *part);

/**
 * @brief State if headset is conected
 *
 * @param[in] appdata        App data
 *
 * @return state
 *
 */
Eina_Bool _callui_common_is_headset_conected(void *appdata);

/**
 * @brief Get idle lock type
 *
 * @return lock type
 *
 */
callui_idle_lock_type_t _callui_common_get_idle_lock_type(void);

/**
 * @brief Unlock swipe lock
 *
 */
void _callui_common_unlock_swipe_lock(void);

/**
 * @brief Launch bluetooth application
 *
 * @param[in] appdata        App data
 *
 */
void _callui_common_launch_setting_bluetooth(void *appdata);

/**
 * @brief Launch Composer of Message application
 *
 * @param[in] appdata            App data
 * @param[in] number             Phone number
 * @param[in] is_exit_app_needed Set if exit application is needed on Message app launch
 *
 */
void _callui_common_launch_msg_composer(void *appdata, const char *number, bool is_exit_app_needed);

/**
 * @brief Launch Dialer of Phone application
 *
 * @param[in] appdata        App data
 *
 */
void _callui_common_launch_dialer(void *appdata);

/**
 * @brief Launch Contacts application
 *
 * @param[in] appdata        App data
 *
 */
void _callui_common_launch_contacts(void *appdata);

/**
 * @brief Reset main layout text fields
 *
 * @param[in] contents	Parent object
 *
 */
void _callui_common_reset_main_ly_text_fields(Evas_Object *contents);

/**
 * @brief state is volume mode on
 *
 * @param[in] data			Application data
 *
 * @return state
 *
 */
bool _callui_common_is_extra_volume_available(void *data);

/**
 * @brief state is aswering mode on
 *
 * @return state
 *
 */
int _callui_common_is_answering_mode_on(void);

/**
 * @brief state is powerkey mode on
 *
 * @return state
 *
 */
int _callui_common_is_powerkey_mode_on(void);

/**
 * @brief State of earjack connection
 *
 * @return state
 *
 */
Eina_Bool _callui_common_is_earjack_connected(void);

/**
 * @brief Sets callback on lock state changed
 *
 */
void _callui_common_set_lock_state_changed_cb();

/**
 * @brief Unsets callback on lock state changed
 *
 */
void _callui_common_unset_lock_state_changed_cb();

/**
 * @brief Gets call edj file path
 * @return Call edj file path
 */
const char *_callui_common_get_call_edj_path();

/**
 * @brief Gets call theme edj file path
 * @return Call theme edj file path
 */
const char *_callui_common_get_call_theme_path();

/**
 * @brief Gets reject message count
 *
 * @param[out] count	Reject message count
 *
 * @return CALLUI_RESULT_OK on success or CALLUI_RESULT_FAIL otherwise
 */
callui_result_e _callui_common_get_reject_msg_count(int *count);
/**
 * @brief Gets reject message text by index
 * @param[in] index		index of the reject message
 * @return reject message text
 */
char *_callui_common_get_reject_msg_by_index(int index);

/**
 * @brief Makes request on close application
 */
void _callui_common_exit_app();

/**
 * @brief Initializes message framework client
 * @param[in] appdata		application data
 * @return result of operation: CALLUI_RESULT_OK on success;
 * 								CALLUI_RESULT_ALREADY_REGISTERED if client is already registered;
 * 								CALLUI_RESULT_FAIL on error;
 */
callui_result_e _callui_common_init_msg_client(void *appdata);

/**
 * @brief Deinitializes message framework client
 * @param[in] appdata		application data
 */
void _callui_common_deinit_msg_client(void *appdata);

/**
 * @brief Gets last message data if it is available
 * @param[in] appdata		application data
 * @param[in] tel_number	telephone number of message sender
 * @param[in/out] msg_data		last message data of person with @tel_number
 * @return CALLUI_RESULT_OK on success and error result otherwise
 */
callui_result_e _callui_common_get_last_msg_data(void *appdata, const char *tel_number, callui_msg_data_t *msg_data);

/**
 * @brief Sends reject message to incoming call recipient
 * @param[in] appdata		application data
 * @param[in] reject_msg	reject message txt
 * @return result CALLUI_RESULT_OK on success and error result otherwise
 */
int _callui_common_send_reject_msg(void *appdata, const char *reject_msg);

/**
 * @brief Gets audio mode
 * @return @c true when callui is on handsfree mode, otherwise false
 */
bool _callui_is_on_handsfree_mode();

/**
 * @brief Gets background state
 * @return @c true when callui is on background, otherwise false
 */
bool _callui_is_on_background();

/**
 * @brief Set call duration time into text part of Evas object
 * @param[in] cur_time		source time data
 * @param[in] obj			Evas object to set call duration
 * @param[in] part			Evas object text part name
 */
void _callui_common_set_call_duration_time(struct tm *cur_time,
		Evas_Object *obj,
		const char *part);

/**
 * @brief Update call duration data in EvasObjcet text part if it is needed
 * @remark if updaet is needed @cur_time data will be replaced by @comp_time data
 * @param[in] cur_time		current time data
 * @param[in] comp_time		time data to compare
 * @param[in] func			Evas Object part updater function
 * @param[in] obj			Evas object to set call duration
 * @param[in] part			Evas object text part name
 */
void _callui_common_try_update_call_duration_time(struct tm *cur_time,
		struct tm *comp_time,
		set_call_duration_time func,
		Evas_Object *obj,
		const char *part);

/**
 * @brief Get time string from time structure
 * @param[in] time		time structure to process
 * @return string with time (must be free internally)
 */
char *_callui_common_get_duration_time_string(struct tm *time);

/**
 * @brief Gets difference between curent time and @time
 * @param[in] time		time to calculate difference
 * @return time stucture on success with time difference
 */
struct tm *_callui_common_get_current_time_diff_in_tm(long time);

/**
 * @brief Converts time_t time representation into a date string with consideration of system time format
 * @remark returned value is allocated memory so user must free it after use.
 * @param[in]	last_update_time	Time to convert
 * @return Date string on success or NULL otherwise
 */
char *_callui_common_get_date_string_representation(time_t last_update_time);


#endif /*__CALLUI_COMMON_H_ */
