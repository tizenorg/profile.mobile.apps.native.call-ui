/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __CALLUI_COMMON_H_
#define __CALLUI_COMMON_H_

#include "callui-view-manager.h"
#include "callui.h"
#include "callui-common-types.h"
#include "callui-common-def.h"

typedef enum {
	LOCK_TYPE_UNLOCK = 1,
	LOCK_TYPE_SWIPE_LOCK,
	LOCK_TYPE_SECURITY_LOCK
} callui_idle_lock_type_t;

typedef enum {
	LCD_TIMEOUT_SET = 1,
	LCD_TIMEOUT_UNSET,
	LCD_TIMEOUT_LOCKSCREEN_SET, /*After lock-screen comes in Connected state LCD goes to OFF in 5 secs*/
	LCD_TIMEOUT_KEYPAD_SET, /*When Keypad is ON, LCD goes to DIM in 3 secs then goes to OFF in 5 secs*/
	LCD_TIMEOUT_DEFAULT
} callui_lcd_timeout_t;

typedef enum {
	LCD_ON,
	LCD_ON_LOCK,
	LCD_ON_UNLOCK,
	LCD_UNLOCK,
	LCD_OFF_SLEEP_LOCK,
	LCD_OFF_SLEEP_UNLOCK,
	LCD_OFF
} callui_lcd_control_t;

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
 * @return error
 *
 */
int _callui_common_unlock_swipe_lock(void);

/**
 * @brief Set notification type
 *
 * @param[in] person_id      Person id
 * @param[in] bwin_noti      win type
 *
 */
void _callui_common_win_set_noti_type(void *appdata, int bwin_noti);

/**
 * @brief Launch contacts application
 *
 * @param[in] appdata        App data
 *
 */
void _callui_common_launch_contacts(void *appdata);

/**
 * @brief Launch bluetooth application
 *
 * @param[in] appdata        App data
 *
 */
void _callui_common_launch_bt_app(void *appdata);

/**
 * @brief Launch dialer application
 *
 * @param[in] appdata        App data
 *
 */
void _callui_common_launch_dialer(void *appdata);

/**
 * @brief Launch message application
 *
 * @param[in] appdata        App data
 * @param[in] number         Phone number
 *
 */
void _callui_common_launch_msg_composer(void *appdata, const char *number);

/**
 * @brief Reset main layout text fields
 *
 * @param[in] contents	Parent object
 *
 */
void _callui_common_reset_main_ly_text_fields(Evas_Object *contents);

#ifdef _DBUS_DVC_LSD_TIMEOUT_
/**
 * @brief Set lcd timeout
 *
 * @param[in] state          Lcd timer state
 *
 */
void _callui_common_dvc_set_lcd_timeout(callui_lcd_timeout_t state);
#endif

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
 * @brief Changed lcd state
 *
 * @param[in] state           Lcd control state
 *
 */
void _callui_common_dvc_control_lcd_state(callui_lcd_control_t state);

/**
 * @brief Gets lcd state
 *
 * return Lcd control state
 *
 */
callui_lcd_control_t _callui_common_get_lcd_state();

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
 * @brief Sends reject message to incoming call recipient
 * @param[in] appdata		application data
 * @param[in] reject_msg	reject message txt
 * @return result CALLUI_RESULT_OK on success and error result otherwise
 */
int _callui_common_send_reject_msg(void *appdata, char *reject_msg);

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
char *_callui_common_get_time_string(struct tm *time);

/**
 * @brief Gets difference between curent time and @time
 * @param[in] time		time to calculate difference
 * @return time stucture on success with time difference
 */
struct tm *_callui_common_get_current_time_diff_in_tm(long time);

/**
 * @brief Set the text for an object's part, marking it as translatable for call app domain
 * @param[in] obj		Evas object containing the text part
 * @param[in] part		name of the part to set
 * @param[in] text		the original, non-translated text to set
 */
void _callui_common_eo_txt_part_set_translatable_text(Evas_Object *obj,
		const char *part,
		const char *ids_string);

#endif //__CALLUI_COMMON_H_
