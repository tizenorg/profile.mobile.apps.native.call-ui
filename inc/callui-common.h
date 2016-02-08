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

#define CALLUI_KEY_BACK "XF86Back"
#define CALLUI_KEY_MEDIA "XF86AudioMedia"
#define CALLUI_KEY_SELECT "XF86Phone"
#define CALLUI_KEY_POWER "XF86PowerOff"
#define CALLUI_KEY_HOME "XF86Home"
#define CALLUI_KEY_VOLUMEUP "XF86AudioRaiseVolume"
#define CALLUI_KEY_VOLUMEDOWN "XF86AudioLowerVolume"


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


/**
 * @brief Set call duration
 *
 * @param[in] time_dur       Time duration
 *
 */
void _callui_common_set_call_duration(char *time_dur);

/**
 * @brief Update begin timer
 *
 * @param[in] starttime       Start time
 *
 */
void _callui_common_update_call_duration(long starttime);

/**
 * @brief Create duration timer
 *
 */
void _callui_common_create_duration_timer();

/**
 * @brief Delete duration timer
 *
 */
void _callui_common_delete_duration_timer();

/**
 * @brief create ending timer
 *
 * @param[in] vd            View data
 *
 */
void _callui_common_create_ending_timer(call_view_data_t *vd);

/**
 * @brief Delete ending timer
 *
 */
void _callui_common_delete_ending_timer(void);

/**
 * @brief Get sim name
 *
 * @param[in] appdata        App data
 *
 * @return sim name
 *
 */
char * _callui_common_get_sim_name(void *appdata);

/**
 * @brief State if headset is conected
 *
 * @return state
 *
 */
Eina_Bool _callui_common_is_headset_conected(void);

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
 * @brief Get uptime
 * @return uptime
 *
 */
long _callui_common_get_uptime(void);

/**
 * @brief Set notification type
 *
 * @param[in] person_id      Person id
 * @param[in] bwin_noti      win type
 *
 */
void _callui_common_win_set_noti_type(void *appdata, int bwin_noti);

/**
 * @brief Get contact info
 *
 * @param[in] person_id      Person id
 * @param[out] ct_info       Contacts data
 *
 */
void _callui_common_get_contact_info(int person_id, call_contact_data_t *ct_info);

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
void _callui_common_launch_msg_composer(void *appdata, char *number);

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
 * @return state
 *
 */
gboolean _callui_common_is_extra_volume_available(void);

/**
 * @brief state is aswering mode on
 *
 * @return state
 *
 */
gboolean _callui_common_is_answering_mode_on(void);

/**
 * @brief state is powerkey mode on
 *
 * @return state
 *
 */
gboolean _callui_common_is_powerkey_mode_on(void);

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

#endif //__CALLUI_COMMON_H_
