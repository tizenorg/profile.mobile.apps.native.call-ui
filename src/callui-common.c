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

#include <app.h>
#include <app_control.h>
#include <vconf.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <contacts.h>
#include <runtime_info.h>
#include <network/bluetooth.h>
#include <system_settings.h>
#include <efl_util.h>
#include <msg.h>
#include <msg_transport.h>
#include <msg_storage.h>
#include <utils_i18n_ulocale.h>
#include <utils_i18n.h>
#include <glib.h>

#include "callui-common.h"
#include "callui-debug.h"
#include "callui-view-elements.h"
#include "callui.h"
#include "callui-sound-manager.h"
#include "callui-state-provider.h"

#define CALLUI_CSTM_I18N_UDATE_IGNORE	-2 /* Used temporarily since there is no substitute of UDATE_IGNORE in base-utils */
#define CALLUI_TIME_STRING_BUFF_SIZE	512
#define CALLUI_PAUSE_LOCK_TIMEOUT_LIMIT	0.35

#define CALLUI_TIME_FORMAT_12		"hm"
#define CALLUI_TIME_FORMAT_24		"Hm"
#define CALLUI_DATETIME_FORMAT_12	"yMdhm"
#define CALLUI_DATETIME_FORMAT_24	"yMdHm"

#define CALLUI_BLUETOOTH_PKG	"ug-bluetooth-efl"
#define CALLUI_CONTACTS_PKG		"org.tizen.contacts"

#define CALLUI_PHONE_TELEPHONE_URI			"tel:"
#define CALLUI_PHONE_LAUNCH_TYPE_PARAM_NAME	"launch_type"
#define CALLUI_PHONE_LAUNCH_TYPE_VALUE		"add_call"
#define CALLUI_MESSAGE_SMS_URI				"sms:"

static bool g_is_headset_connected;

static bool __bt_device_connected_profile(bt_profile_e profile, void *user_data);
static bool __bt_adapter_bonded_device_cb(bt_device_info_s *device_info, void *user_data);
static void __reset_visibility_properties(callui_app_data_t *ad);
static void __lock_state_changed_cb (system_settings_key_e key, void *user_data);
static const char *__get_res_path();
static const char *__get_resource(const char *res_name);
static char *__vconf_get_str(const char *in_key);
static char *__parse_vconf_string(char *input_string);
static void __send_reject_msg_status_cb(msg_handle_t Handle, msg_struct_t pStatus, void *pUserParam);
static callui_result_e __init_msg_client(void *appdata);
static i18n_udatepg_h __create_pattern_generator();
static void __destroy_pattern_generator(i18n_udatepg_h pattern_generator);
static bool __check_date_on_today(const time_t req_time);
static bool __check_date_on_yesterday(const time_t req_time);
static void __generate_best_pattern(i18n_udatepg_h pattern_generator, const char *locale, i18n_uchar *skeleton, char *formatted_string, time_t *time);
static char *__get_date_text(i18n_udatepg_h pattern_generator, const char *locale, char *skeleton, time_t *time);
static void __app_launch_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data);
static void __update_params_according_lockstate(callui_app_data_t *ad);

Eina_Bool _callui_common_is_earjack_connected(void)
{
	int result = EINA_FALSE;
	int earjack_status = -1;

	int ret = runtime_info_get_value_int(RUNTIME_INFO_KEY_AUDIO_JACK_STATUS, &earjack_status);
	if (RUNTIME_INFO_ERROR_NONE == ret) {
		dbg("earjack_status:[%d]", earjack_status);
		if (RUNTIME_INFO_AUDIO_JACK_STATUS_UNCONNECTED != earjack_status) {
			result = EINA_TRUE;
		}
	} else {
		err("runtime_info_get_value_int failed.. Error: %d", ret);
	}

	return result;
}

static bool __bt_device_connected_profile(bt_profile_e profile, void *user_data)
{
	dbg("..");
	if ((profile == BT_PROFILE_A2DP) || (profile == BT_PROFILE_HSP)) {
		dbg("found connected bluetooth headset device");
		g_is_headset_connected = true;
		return false;
	}
	return true;
}

static bool __bt_adapter_bonded_device_cb(bt_device_info_s *device_info, void *user_data)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)user_data;
	if (device_info->is_connected) {
		bt_device_foreach_connected_profiles(device_info->remote_address, __bt_device_connected_profile, ad);
	}
	return true;
}


Eina_Bool _callui_common_is_headset_conected(void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, EINA_FALSE);

	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	g_is_headset_connected = false;
	bt_adapter_foreach_bonded_device(__bt_adapter_bonded_device_cb, ad);

	return g_is_headset_connected;
}

callui_idle_lock_type_t _callui_common_get_idle_lock_type(void)
{
	int lock_state = -1;
	int lock_type = SETTING_SCREEN_LOCK_TYPE_NONE;
	int ret = 0;
	callui_idle_lock_type_t ret_val = CALLUI_LOCK_TYPE_UNLOCK;

	ret = system_settings_get_value_int(SYSTEM_SETTINGS_KEY_LOCK_STATE, &lock_state);
	if (ret < 0) {
		err("system_settings_get_value_int failed with code %d", ret);
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, &lock_type);
	if (ret < 0) {
		err("vconf_get_int error");
	}

	if (lock_state == SYSTEM_SETTINGS_LOCK_STATE_LOCK) {
		if (lock_type == SETTING_SCREEN_LOCK_TYPE_SIMPLE_PASSWORD
			|| lock_type == SETTING_SCREEN_LOCK_TYPE_PASSWORD) {
			ret_val = CALLUI_LOCK_TYPE_SECURITY_LOCK;
		} else {
			ret_val = CALLUI_LOCK_TYPE_SWIPE_LOCK;
		}
	} else {
		ret_val = CALLUI_LOCK_TYPE_UNLOCK;
	}

	info("Lock state : %d", ret_val);
	return ret_val;
}

callui_result_e _callui_common_unlock_swipe_lock(void)
{
	debug_enter();

	int res = vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);
	if (res != 0) {
		err("Set flag IDLE_UNLOCK failed");
		return CALLUI_RESULT_FAIL;
	}
	return CALLUI_RESULT_OK;
}

static void __reset_visibility_properties(callui_app_data_t *ad)
{
	if (_callui_lock_manager_is_started(ad->lock_handle)) {
		_callui_lock_manager_stop(ad->lock_handle);
		ad->start_lock_manager_on_resume = true;
	}
}

static void __update_params_according_lockstate(callui_app_data_t *ad)
{
	callui_idle_lock_type_t type = _callui_common_get_idle_lock_type();
	if (type == CALLUI_LOCK_TYPE_SECURITY_LOCK) {
		_callui_window_set_above_lockscreen_mode(ad->window, false);
	}
	_callui_common_unlock_swipe_lock();
}

static void __app_launch_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	if (result == APP_CONTROL_RESULT_APP_STARTED) {
		callui_app_data_t *ad = user_data;
		ad->on_background = true;
		__update_params_according_lockstate(ad);
	}
}

void _callui_common_launch_setting_bluetooth(void *appdata)
{
	CALLUI_RETURN_IF_FAIL(appdata);

	callui_app_data_t *ad = appdata;

	app_control_h app_control = NULL;
	int ret;
	if ((ret = app_control_create(&app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_create() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_app_id(app_control, CALLUI_BLUETOOTH_PKG)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_PICK)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_enable_app_started_result_event(app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_enable_app_started_result_event() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, __app_launch_reply_cb, ad)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed. ret[%d]", ret);
	} else {
		__reset_visibility_properties(ad);
	}
	if (app_control) {
		app_control_destroy(app_control);
	}
}

void _callui_common_launch_dialer(void *appdata)
{
	CALLUI_RETURN_IF_FAIL(appdata);

	callui_app_data_t *ad = appdata;

	app_control_h app_control = NULL;
	int ret;
	if ((ret = app_control_create(&app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_create() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_DIAL)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_uri(app_control, CALLUI_PHONE_TELEPHONE_URI)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_uri() is failed. ret[%d]", ret);
	} else if ((ret = app_control_add_extra_data(app_control, CALLUI_PHONE_LAUNCH_TYPE_PARAM_NAME, CALLUI_PHONE_LAUNCH_TYPE_VALUE)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_add_extra_data() is failed. ret[%d]", ret);
	} else if ((ret = app_control_enable_app_started_result_event(app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_enable_app_started_result_event() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, __app_launch_reply_cb, ad)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed. ret[%d]", ret);
	} else {
		__reset_visibility_properties(ad);
	}
	if (app_control) {
		app_control_destroy(app_control);
	}
}

void _callui_common_launch_contacts(void *appdata)
{
	CALLUI_RETURN_IF_FAIL(appdata);

	callui_app_data_t *ad = appdata;

	app_control_h app_control = NULL;
	int ret;
	if ((ret = app_control_create(&app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_create() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_app_id(app_control, CALLUI_CONTACTS_PKG)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_enable_app_started_result_event(app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_enable_app_started_result_event() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, __app_launch_reply_cb, ad)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed. ret[%d]", ret);
	} else {
		__reset_visibility_properties(ad);
	}
	if (app_control) {
		app_control_destroy(app_control);
	}
}

void _callui_common_launch_msg_composer(void *appdata, const char *number)
{
	CALLUI_RETURN_IF_FAIL(appdata);

	callui_app_data_t *ad = appdata;

	char str[CALLUI_BUFF_SIZE_SML];
	snprintf(str, sizeof(str), "%s%s", CALLUI_MESSAGE_SMS_URI, number);

	app_control_h app_control = NULL;
	int ret;
	if ((ret = app_control_create(&app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_create() failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_COMPOSE)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_uri(app_control, str)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_uri() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed. ret[%d]", ret);
	} else {
		__reset_visibility_properties(ad);
	}
	if (app_control) {
		app_control_destroy(app_control);
	}
}

void _callui_common_reset_main_ly_text_fields(Evas_Object *contents)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(contents);
	Evas_Object *caller_info = NULL;

	elm_object_part_text_set(contents, "call_txt_status", "");
	elm_object_part_text_set(contents, "txt_timer", "");

	caller_info = elm_object_part_content_get(contents, "swallow.caller_info");
	if (caller_info) {
		elm_object_part_text_set(caller_info, "contact_name", "");
		elm_object_part_text_set(caller_info, "phone_number", "");
	}
}

bool _callui_common_is_extra_volume_available(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, true);

	callui_app_data_t *ad = (callui_app_data_t*)data;

	callui_audio_state_type_e audio_state =
			_callui_sdm_get_audio_state(ad->sound_manager);

	dbg("sound path : %d", audio_state);

	if ((audio_state == CALLUI_AUDIO_STATE_BT)
		|| ((audio_state == CALLUI_AUDIO_STATE_EARJACK)
				&& (_callui_common_is_earjack_connected() == EINA_TRUE))) {
		return false;
	} else {
		return true;
	}
}

int _callui_common_is_answering_mode_on(void)
{
	int answerMode = 0;
	int ret = 0;

	ret = vconf_get_bool(VCONFKEY_CISSAPPL_ANSWERING_KEY_BOOL, &answerMode);
	if (!ret) {
		dbg("answerMode = [%d] \n", answerMode);
	} else {
		dbg("vconf_get_int failed..[%d]\n", ret);
	}

	return answerMode;
}

int _callui_common_is_powerkey_mode_on(void)
{
	int powerkey_mode = 0;
	int ret = 0;

	ret = vconf_get_bool(VCONFKEY_CISSAPPL_POWER_KEY_ENDS_CALL_BOOL, &powerkey_mode);
	if (!ret) {
		dbg("powerkey_mode = [%d] \n", powerkey_mode);
	} else {
		dbg("vconf_get_int failed..[%d]\n", ret);
	}
	return powerkey_mode;
}

static void __lock_state_changed_cb(system_settings_key_e key, void *user_data)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_app_data_t *ad = user_data;

	if (_callui_common_get_idle_lock_type() == CALLUI_LOCK_TYPE_UNLOCK) {
		dbg("Device lock state [UNLOCKED]");
		_callui_window_set_above_lockscreen_mode(ad->window, false);
		if (ad->need_win_minimize) {
			ad->need_win_minimize = false;
			_callui_window_minimize(ad->window);
		}
	} else {
		dbg("Device lock state [LOCKED]");
		if (!ad->on_background) {
			_callui_window_set_above_lockscreen_mode(ad->window, true);
		} else {
			double time_diff = ecore_time_get() - ad->app_pause_time;
			if (time_diff <= CALLUI_PAUSE_LOCK_TIMEOUT_LIMIT) {
				dbg("App_pause -> lock_device time diff [%ld]", time_diff);
				_callui_window_set_above_lockscreen_mode(ad->window, true);
				ad->app_pause_time = 0.0;
			}
			if (_callui_lock_manager_is_started(ad->lock_handle)) {
				_callui_lock_manager_stop(ad->lock_handle);
				ad->start_lock_manager_on_resume = true;
			}
		}
	}
}

void _callui_common_set_lock_state_changed_cb(void *user_data)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCK_STATE, __lock_state_changed_cb, user_data);
}

void _callui_common_unset_lock_state_changed_cb()
{
	system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCK_STATE);
}

static const char *__get_res_path()
{
	static char res_folder_path[PATH_MAX] = {'\0'};
	if (res_folder_path[0] == '\0') {
		char *res_path_buff = app_get_resource_path();
		strncpy(res_folder_path, res_path_buff, PATH_MAX-1);
		free(res_path_buff);
	}
	return res_folder_path;
}

static const char *__get_resource(const char *res_name)
{
	if (res_name == NULL) {
		err("res_name is NULL");
		return NULL;
	}

	static char res_path[PATH_MAX] = {'\0'};
	snprintf(res_path, PATH_MAX, "%s%s", __get_res_path(), res_name);
	return res_path;
}

const char *_callui_common_get_call_edj_path()
{
	return __get_resource(CALLUI_CALL_EDJ_NAME);
}

const char *_callui_common_get_call_theme_path()
{
	return __get_resource(CALLUI_CALL_THEME_EDJ_NAME);
}

static char *__vconf_get_str(const char *in_key)
{
	char *result = vconf_get_str(in_key);
	if (result == NULL) {
		err("vconf get error : %s", in_key);
	}
	return result;
}

static char *__parse_vconf_string(char *input_string)
{
	if (NULL == input_string) {
		err("Input string is NULL");
		return NULL;
	}
	int i;
	char *parsed_message = NULL;
	int input_string_len = strlen(input_string);

	parsed_message = (char *)calloc(input_string_len + 1, sizeof(char));
	if (NULL == parsed_message) {
		err("Parsed message is NULL");
		return NULL;
	}

	for (i = 0; i < input_string_len; i++) {
		if (input_string[i] == '<' && input_string[i+1] == 'b' &&
			input_string[i+2] == 'r' && input_string[i+3] == '/' &&
			input_string[i+4] == '>') {
			i = i + 4;
		} else {
			int j = 0;
			for (; i < input_string_len; i++) {
				if (input_string[i] == '<' && input_string[i+1] == 'b' &&
					input_string[i+2] == 'r' && input_string[i+3] == '/' &&
					input_string[i+4] == '>') {
					if (parsed_message[j-1] != ' ') {
						parsed_message[j] = ' ';
						j++;
					}
					i = i + 4;
				} else {
					parsed_message[j] = input_string[i];
					j++;
				}
			}
		}
	}
	return parsed_message;
}

callui_result_e _callui_common_get_reject_msg_count(int *count)
{
	int res = vconf_get_int(VCONFKEY_CISSAPPL_REJECT_CALL_MSG_INT, count);
	if (res != 0) {
		*count = 0;
		return CALLUI_RESULT_FAIL;
	}
	return CALLUI_RESULT_OK;
}

char *_callui_common_get_reject_msg_by_index(int index)
{
	char *message = NULL;
	char *markup_converted_message = NULL;
	char *return_str = NULL;
	char *parsed_message = NULL;

	switch (index) {
	case 0:
		message = __vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG1_STR);
		break;
	case 1:
		message = __vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG2_STR);
		break;
	case 2:
		message = __vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG3_STR);
		break;
	case 3:
		message = __vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG4_STR);
		break;
	case 4:
		message = __vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG5_STR);
		break;
	case 5:
		message = __vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG6_STR);
		break;
	default:
		return NULL;
	}

	if (NULL == message) {
		return NULL;
	}

	markup_converted_message = elm_entry_utf8_to_markup(message);
	parsed_message = __parse_vconf_string(markup_converted_message);
	return_str = strdup(_(parsed_message));

	free(parsed_message);
	free(message);
	free(markup_converted_message);

	return return_str;
}

void _callui_common_exit_app()
{
	ui_app_exit();
}

static callui_result_e __init_msg_client(void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_app_data_t *ad = appdata;

	CALLUI_RETURN_VALUE_IF_FAIL(!ad->msg_handle, CALLUI_RESULT_ALREADY_REGISTERED);

	msg_error_t err = msg_open_msg_handle(&ad->msg_handle);
	if (err != MSG_SUCCESS) {
		dbg("msg_open_msg_handle() failed. err[%d]", err);
		return CALLUI_RESULT_FAIL;
	}
	return CALLUI_RESULT_OK;
}

callui_result_e _callui_common_init_msg_client(void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	return __init_msg_client(appdata);
}

void _callui_common_deinit_msg_client(void *appdata)
{
	CALLUI_RETURN_IF_FAIL(appdata);

	callui_app_data_t *ad = appdata;

	if (ad->msg_handle) {
		msg_close_msg_handle(&ad->msg_handle);
		ad->msg_handle = NULL;
	}
}

callui_result_e _callui_common_get_last_msg_data(void *appdata, const char *tel_number, callui_msg_data_t *msg_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(tel_number, CALLUI_RESULT_INVALID_PARAM);

	callui_app_data_t *ad = appdata;

	callui_result_e res = __init_msg_client(appdata);
	CALLUI_RETURN_VALUE_IF_FAIL((res == CALLUI_RESULT_OK) || (res == CALLUI_RESULT_ALREADY_REGISTERED), res);

	/* Create msg info to get address list handle */
    msg_struct_t msg_info = msg_create_struct(MSG_STRUCT_MESSAGE_INFO);
	if (!msg_info) {
		dbg("msg_create_struct() failed");
		return CALLUI_RESULT_ALLOCATION_FAIL;
	}

	msg_struct_t temp_msg = NULL;
	msg_error_t err = msg_list_add_item(msg_info, MSG_MESSAGE_ADDR_LIST_HND, &temp_msg);
	if (err != MSG_SUCCESS) {
		dbg("msg_list_add_item() failed. err[%d]", err);
		msg_release_struct(&msg_info);
		return CALLUI_RESULT_FAIL;
	}

	dbg("tel_number [%s]", tel_number);

	msg_set_int_value(temp_msg, MSG_ADDRESS_INFO_ADDRESS_TYPE_INT, MSG_ADDRESS_INFO_ADDRESS_TYPE_INT);
	msg_set_int_value(temp_msg, MSG_ADDRESS_INFO_RECIPIENT_TYPE_INT, MSG_RECIPIENTS_TYPE_TO);
	msg_set_str_value(temp_msg, MSG_ADDRESS_INFO_ADDRESS_VALUE_STR, tel_number, strlen(tel_number));

	msg_list_handle_t addr_list = NULL;
	err = msg_get_list_handle(msg_info, MSG_MESSAGE_ADDR_LIST_HND, (void **)&addr_list);
	if (err != MSG_SUCCESS) {
		dbg("msg_get_list_handle() failed. err[%d]", err);
		msg_release_struct(&msg_info);
		return CALLUI_RESULT_FAIL;
	}

	/* Get thead id by address handle */
	msg_thread_id_t thread_id = -1;
	err = msg_get_thread_id_by_address2(ad->msg_handle, addr_list, &thread_id);
	if (err != MSG_SUCCESS) {
		dbg("msg_get_thread_id_by_address() failed. err[%d]", err);
		msg_release_struct(&msg_info);
		return CALLUI_RESULT_FAIL;
	}
	dbg("thread_id [%d]", thread_id);

	msg_release_struct(&msg_info);

	/* Get conversation list by thread id */
	msg_struct_list_s msg_conv_list;
	err = msg_get_conversation_view_list(ad->msg_handle, thread_id, &msg_conv_list);
	if (err != MSG_SUCCESS) {
		dbg("msg_get_conversation_view_list() failed. err[%d]", err);
		return CALLUI_RESULT_FAIL;
	}

	dbg("conversation - msg count [%d]", msg_conv_list.nCount);

	char msg_txt[MAX_MSG_TEXT_LEN + 1] = { '\0' };
	int msg_txt_size;
	int msg_direct;
	int msg_time;

	int i = msg_conv_list.nCount - 1;
	for (; i >= 0 ; --i) {
		msg_direct = -1;
		msg_get_int_value(msg_conv_list.msg_struct_info[i], MSG_CONV_MSG_DIRECTION_INT, &msg_direct);
		dbg("conversation - msg[%d], direction [%d]", i, msg_direct);

		if (msg_direct == MSG_DIRECTION_TYPE_MT) {
			msg_get_int_value(msg_conv_list.msg_struct_info[i], MSG_CONV_MSG_DISPLAY_TIME_INT, &msg_time);
			msg_get_str_value(msg_conv_list.msg_struct_info[i], MSG_CONV_MSG_TEXT_STR, msg_txt, MAX_MSG_TEXT_LEN);
			msg_get_int_value(msg_conv_list.msg_struct_info[i], MSG_CONV_MSG_TEXT_SIZE_INT, &msg_txt_size);
			break;
		}
	}
	msg_release_list_struct(&msg_conv_list);

	if (msg_txt[0] != '\0') {
		dbg("last incoming msg found - txt[%s], txt_size[%d], time[%d]", msg_txt, msg_txt_size, msg_time);
		snprintf(msg_data->text, msg_txt_size, "%s", msg_txt);
		msg_data->date = msg_time;
		return CALLUI_RESULT_OK;
	}
	return CALLUI_RESULT_FAIL;
}

static void __send_reject_msg_status_cb(msg_handle_t Handle, msg_struct_t pStatus, void *pUserParam)
{
	CALLUI_RETURN_IF_FAIL(pStatus != NULL);
	int status = MSG_NETWORK_SEND_FAIL;

	msg_get_int_value(pStatus, MSG_SENT_STATUS_NETWORK_STATUS_INT, &status);
	dbg("status:[%d]", status);
}

int _callui_common_send_reject_msg(void *appdata, const char *reject_msg)
{
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(!STRING_EMPTY(reject_msg), CALLUI_RESULT_INVALID_PARAM);

	callui_app_data_t *ad = appdata;

	const callui_call_data_t *incom = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);
	CALLUI_RETURN_VALUE_IF_FAIL(incom, CALLUI_RESULT_FAIL);

	callui_result_e res = __init_msg_client(ad);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK || res == CALLUI_RESULT_ALREADY_REGISTERED, res);

	msg_error_t err = msg_reg_sent_status_callback(ad->msg_handle, __send_reject_msg_status_cb, NULL);
	if (err != MSG_SUCCESS) {
		dbg("msg_reg_sent_status_callback() failed. err[%d]", err);
		return res;
	}

	msg_struct_t msg_info = msg_create_struct(MSG_STRUCT_MESSAGE_INFO);
	msg_struct_t send_opt = msg_create_struct(MSG_STRUCT_SENDOPT);
	msg_struct_t req = msg_create_struct(MSG_STRUCT_REQUEST_INFO);

	/* Set message type to SMS reject*/
	msg_set_int_value(msg_info, MSG_MESSAGE_TYPE_INT, MSG_TYPE_SMS_REJECT);

	int slot_id = CALLUI_SIM_SLOT_DEFAULT;
	dbg("msg_sms_send_message() Sim slot [%d]", slot_id);
	slot_id++;
	msg_set_int_value(msg_info, MSG_MESSAGE_SIM_INDEX_INT, slot_id);

	/* No setting send option */
	msg_set_bool_value(send_opt, MSG_SEND_OPT_SETTING_BOOL, false);

	/* Set message body */
	if (msg_set_str_value(msg_info, MSG_MESSAGE_SMS_DATA_STR, reject_msg, strlen(reject_msg)) != MSG_SUCCESS) {
		err("msg_set_str_value() failed");
	} else {
		/* Create address list*/
		msg_struct_list_s *addr_list;
		msg_get_list_handle(msg_info, MSG_MESSAGE_ADDR_LIST_STRUCT, (void **)&addr_list);
		msg_struct_t addr_info = addr_list->msg_struct_info[0];
		const char *call_number = incom->call_num;

		/* Set message address */
		msg_set_int_value(addr_info, MSG_ADDRESS_INFO_RECIPIENT_TYPE_INT, MSG_RECIPIENTS_TYPE_TO);
		msg_set_str_value(addr_info, MSG_ADDRESS_INFO_ADDRESS_VALUE_STR, call_number, strlen(call_number));
		addr_list->nCount = 1;

		/* Set message struct to Request*/
		msg_set_struct_handle(req, MSG_REQUEST_MESSAGE_HND, msg_info);
		msg_set_struct_handle(req, MSG_REQUEST_SENDOPT_HND, send_opt);

		/* Send message */
		err = msg_sms_send_message(ad->msg_handle, req);
		if (err != MSG_SUCCESS) {
			err("msg_sms_send_message() failed. err[%d]", err);
		} else {
			dbg("Sending...");
			res = CALLUI_RESULT_OK;
		}
	}
	msg_release_struct(&req);
	msg_release_struct(&msg_info);
	msg_release_struct(&send_opt);

	return res;
}

bool _callui_is_on_handsfree_mode()
{
	callui_app_data_t *ad = _callui_get_app_data();

	callui_audio_state_type_e type = _callui_sdm_get_audio_state(ad->sound_manager);
	return (type != CALLUI_AUDIO_STATE_RECEIVER && type != CALLUI_AUDIO_STATE_NONE);
}

bool _callui_is_on_background()
{
	callui_app_data_t *ad = _callui_get_app_data();
	return ad->on_background;
}

void _callui_common_set_call_duration_time(struct tm *cur_time,
		Evas_Object *obj,
		const char *part)
{
	CALLUI_RETURN_IF_FAIL(cur_time);
	CALLUI_RETURN_IF_FAIL(obj);
	CALLUI_RETURN_IF_FAIL(part);

	char *tmp = _callui_common_get_duration_time_string(cur_time);
	elm_object_part_text_set(obj, part, tmp);
	free(tmp);
}

char *_callui_common_get_duration_time_string(struct tm *time)
{
	char *tm_string = calloc(1, CALLUI_BUFF_SIZE_TINY);

	if (time->tm_hour > 0) {
		snprintf(tm_string, CALLUI_BUFF_SIZE_TINY, "%02d:%02d:%02d", time->tm_hour, time->tm_min, time->tm_sec);
	} else {
		snprintf(tm_string, CALLUI_BUFF_SIZE_TINY, "%02d:%02d", time->tm_min, time->tm_sec);
	}
	return tm_string;
}

void _callui_common_try_update_call_duration_time(
		struct tm *cur_time,
		struct tm *comp_time,
		set_call_duration_time func,
		Evas_Object *obj,
		const char *part)
{
	CALLUI_RETURN_IF_FAIL(cur_time);
	CALLUI_RETURN_IF_FAIL(comp_time);
	CALLUI_RETURN_IF_FAIL(func);
	CALLUI_RETURN_IF_FAIL(obj);
	CALLUI_RETURN_IF_FAIL(part);

	int sec_diff = comp_time->tm_sec - cur_time->tm_sec;

	if (sec_diff != 0) {
		memcpy(cur_time, comp_time, sizeof(struct tm));
		func(cur_time, obj, part);
	}
}

struct tm *_callui_common_get_current_time_diff_in_tm(long time)
{
	struct tm *time_tm = calloc(1, sizeof (struct tm));
	CALLUI_RETURN_NULL_IF_FAIL(time_tm);

	long curr_time = 0;
	struct sysinfo info;
	if (sysinfo(&info) == 0) {
		curr_time = info.uptime;
	} else {
		free(time_tm);
		return NULL;
	}

	long call_time = curr_time - time;
	gmtime_r((const time_t *)&call_time, time_tm);

	return time_tm;
}

static i18n_udatepg_h __create_pattern_generator()
{
	i18n_error_code_e status = I18N_ERROR_NONE;

	status = i18n_ulocale_set_default(getenv("LC_TIME"));
	CALLUI_RETURN_NULL_IF_FAIL(status == I18N_ERROR_NONE);

	const char *locale = NULL;
	i18n_ulocale_get_default(&locale);

	/* remove ".UTF-8" in locale */
	char locale_tmp[CALLUI_BUFF_SIZE_SML] = { '\0' };
	strncpy(locale_tmp, locale, sizeof(locale_tmp) - 1);
	char *p = g_strstr_len(locale_tmp, strlen(locale_tmp), ".UTF-8");
	if (p) {
		*p = 0;
	}

	i18n_udatepg_h pattern_generator = NULL;
	status = i18n_udatepg_create(locale_tmp, &pattern_generator);
	CALLUI_RETURN_NULL_IF_FAIL(pattern_generator && status == I18N_ERROR_NONE);

	return pattern_generator;
}

static void __destroy_pattern_generator(i18n_udatepg_h pattern_generator)
{
	if (pattern_generator) {
		i18n_udatepg_destroy(pattern_generator);
		pattern_generator = NULL;
	}
}

static bool __check_date_on_today(const time_t req_time)
{
	time_t now_time = time(NULL);

	struct tm tm_buff = { '\0' };

	struct tm *dummy = localtime_r(&now_time, &tm_buff);
	CALLUI_RETURN_VALUE_IF_FAIL(dummy, false);

	struct tm now_tm;
	memcpy(&now_tm, dummy, sizeof(struct tm));

	dummy = localtime_r(&req_time, &tm_buff);
	CALLUI_RETURN_VALUE_IF_FAIL(dummy, false);
	struct tm req_tm;
	memcpy(&req_tm, dummy, sizeof(struct tm));

	return (req_tm.tm_year == now_tm.tm_year && req_tm.tm_yday == now_tm.tm_yday);
}

static bool __check_date_on_yesterday(const time_t req_time)
{
	time_t now_time = time(NULL);

	struct tm tm_buff = { '\0' };

	struct tm *dummy = localtime_r(&now_time, &tm_buff);
	CALLUI_RETURN_VALUE_IF_FAIL(dummy, false);
	struct tm now_tm;
	memcpy(&now_tm, dummy, sizeof(struct tm));

	dummy = localtime_r(&req_time, &tm_buff);
	CALLUI_RETURN_VALUE_IF_FAIL(dummy, false);
	struct tm req_tm;
	memcpy(&req_tm, dummy, sizeof(struct tm));

	if (now_tm.tm_yday == 0) { /* It is the first day of year */
		return (req_tm.tm_year == now_tm.tm_year - 1 && req_tm.tm_mon == 12 && req_tm.tm_mday == 31);
	} else {
		return (req_tm.tm_year == now_tm.tm_year && req_tm.tm_yday == now_tm.tm_yday - 1);
	}
}

static void __generate_best_pattern(i18n_udatepg_h pattern_generator,
		const char *locale,
		i18n_uchar *skeleton,
		char *formatted_string,
		time_t *time)
{
	i18n_uchar best_pattern[CALLUI_BUFF_SIZE_MID] = { '\0' };
	int32_t best_pattern_capacity = (int32_t) (sizeof(best_pattern) / sizeof(best_pattern[0]));
	int32_t skeleton_len = i18n_ustring_get_length(skeleton);

	int32_t best_pattern_len;
	i18n_error_code_e status = i18n_udatepg_get_best_pattern(pattern_generator, skeleton, skeleton_len, best_pattern, best_pattern_capacity, &best_pattern_len);
	CALLUI_RETURN_IF_FAIL(status == I18N_ERROR_NONE)

	i18n_udate_format_h formatter;
	status = i18n_udate_create(CALLUI_CSTM_I18N_UDATE_IGNORE, CALLUI_CSTM_I18N_UDATE_IGNORE, locale, NULL, -1, best_pattern, -1, &formatter);
	CALLUI_RETURN_IF_FAIL(status == I18N_ERROR_NONE)

	i18n_uchar formatted[CALLUI_BUFF_SIZE_MID] = {'\0'};
	int32_t formatted_capacity = (int32_t) (sizeof(formatted) / sizeof(formatted[0]));
	i18n_udate date = (i18n_udate)(*time) * 1000;
	int32_t formatted_length;
	status = i18n_udate_format_date(formatter, date, formatted, formatted_capacity, NULL, &formatted_length);
	CALLUI_RETURN_IF_FAIL(status == I18N_ERROR_NONE)

	i18n_ustring_copy_au_n(formatted_string, formatted, CALLUI_BUFF_SIZE_BIG);
	i18n_udate_destroy(formatter);
}

static char *__get_date_text(i18n_udatepg_h pattern_generator, const char *locale, char *skeleton, time_t *time)
{
	debug_enter();
	char formatted_string[CALLUI_BUFF_SIZE_BIG] = { '\0' };
	i18n_uchar custom_skeleton[CALLUI_BUFF_SIZE_MID] = { '\0' };
	int skeletonLength = strlen(skeleton);

	i18n_ustring_copy_ua_n(custom_skeleton, skeleton, skeletonLength);
	__generate_best_pattern(pattern_generator, locale, custom_skeleton, formatted_string, time);

	CALLUI_RETURN_NULL_IF_FAIL(formatted_string[0] != '\0');
	return strdup(formatted_string);
}

char *_callui_common_get_date_string_representation(time_t last_update_time)
{
	char update_time[CALLUI_TIME_STRING_BUFF_SIZE] = { '\0' };
	bool is_format_24h;

	int ret = system_settings_get_value_bool( SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &is_format_24h);
	CALLUI_RETURN_NULL_IF_FAIL(ret == SYSTEM_SETTINGS_ERROR_NONE);

	i18n_udatepg_h pattern_generator = __create_pattern_generator();
	CALLUI_RETURN_NULL_IF_FAIL(pattern_generator);

	const char *icu_locale = NULL;
	ret = i18n_ulocale_get_default(&icu_locale);
	if (ret != I18N_ERROR_NONE) {
		__destroy_pattern_generator(pattern_generator);
		return NULL;
	}

	char *skeleton = NULL;
	char *formatted_text = NULL;
	if (__check_date_on_today(last_update_time)) {
		skeleton = (is_format_24h) ? CALLUI_TIME_FORMAT_24 : CALLUI_TIME_FORMAT_12;
		formatted_text = __get_date_text(pattern_generator, icu_locale, skeleton, &last_update_time);
		snprintf(update_time, sizeof(update_time), "%s %s", "Today" , formatted_text);		// TODO: need IDS string
	} else if (__check_date_on_yesterday(last_update_time)) {
		skeleton = (is_format_24h) ? CALLUI_TIME_FORMAT_24 : CALLUI_TIME_FORMAT_12;
		formatted_text = __get_date_text(pattern_generator, icu_locale, skeleton, &last_update_time);
		snprintf(update_time, sizeof(update_time), "%s %s", "Yesterday", formatted_text);	// TODO: need IDS string
	} else {
		skeleton = (is_format_24h) ? CALLUI_DATETIME_FORMAT_24 : CALLUI_DATETIME_FORMAT_12;
		formatted_text = __get_date_text(pattern_generator, icu_locale, skeleton, &last_update_time);
		snprintf(update_time, sizeof(update_time), "%s", formatted_text);
	}
	FREE(formatted_text);

	char *result_txt = calloc(1, CALLUI_TIME_STRING_BUFF_SIZE);
	if (!result_txt) {
		__destroy_pattern_generator(pattern_generator);
		return NULL;
	}

	memcpy(result_txt, update_time, CALLUI_TIME_STRING_BUFF_SIZE);
	__destroy_pattern_generator(pattern_generator);

	return result_txt;
}
