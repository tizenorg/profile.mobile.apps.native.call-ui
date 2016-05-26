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

#include "callui-common.h"
#include "callui-debug.h"
#include "callui-view-elements.h"
#include "callui.h"
#include "callui-view-dialing.h"
#include "callui-view-single-call.h"
#include "callui-view-callend.h"
#include "callui-view-manager.h"
#include "callui-view-multi-call-list.h"
#include "callui-view-multi-call-split.h"
#include "callui-view-multi-call-conf.h"
#include "callui-view-quickpanel.h"
#include "callui-view-caller-info-defines.h"
#include "callui-sound-manager.h"
#include "callui-state-provider.h"

#define BLUETOOTH_PKG	"ug-bluetooth-efl"
#define CONTACTS_PKG		"org.tizen.contacts"

#define PHONE_TELEPHONE_URI				"tel:"
#define PHONE_LAUNCH_TYPE_PARAM_NAME	"launch_type"
#define PHONE_LAUNCH_TYPE_VALUE			"add_call"

#define MESSAGE_SMS_URI					"sms:"

#define TIME_BUF_LEN (16)
#define CONTACT_NUMBER_BUF_LEN 32

static bool g_is_headset_connected;

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

static bool __callui_common_bt_device_connected_profile(bt_profile_e profile, void *user_data)
{
	dbg("..");
	if ((profile == BT_PROFILE_A2DP) || (profile == BT_PROFILE_HSP)) {
		dbg("found connected bluetooth headset device");
		g_is_headset_connected = true;
		return false;
	}
	return true;
}

static bool __callui_common_bt_adapter_bonded_device_cb(bt_device_info_s *device_info, void *user_data)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)user_data;
	if (device_info->is_connected) {
		bt_device_foreach_connected_profiles(device_info->remote_address, __callui_common_bt_device_connected_profile, ad);
	}
	return true;
}


Eina_Bool _callui_common_is_headset_conected(void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, EINA_FALSE);

	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	g_is_headset_connected = false;
	bt_adapter_foreach_bonded_device(__callui_common_bt_adapter_bonded_device_cb, ad);

	return g_is_headset_connected;
}

callui_idle_lock_type_t _callui_common_get_idle_lock_type(void)
{
	int lock_state = -1;
	int lock_type = SETTING_SCREEN_LOCK_TYPE_NONE;
	int ret = 0;
	callui_idle_lock_type_t ret_val = LOCK_TYPE_UNLOCK;

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
			ret_val = LOCK_TYPE_SECURITY_LOCK;
		} else {
			ret_val = LOCK_TYPE_SWIPE_LOCK;
		}
	} else {
		ret_val = LOCK_TYPE_UNLOCK;
	}

	info("Lock state : %d", ret_val);
	return ret_val;
}

int _callui_common_unlock_swipe_lock(void)
{
	int res = vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);
	if (res != 0) {
		err("Set flag IDLE_UNLOCK failed");
	}
	return 0;
}

void _callui_common_win_set_noti_type(void *appdata, bool win_noti)
{
	CALLUI_RETURN_IF_FAIL(appdata);

	callui_app_data_t *ad = appdata;
	if (win_noti) {
		dbg("window type: NOTIFICATION");
		efl_util_set_notification_window_level(ad->win, EFL_UTIL_NOTIFICATION_LEVEL_TOP);
	} else {
		dbg("window type: NORMAL");
		efl_util_set_notification_window_level(ad->win, EFL_UTIL_NOTIFICATION_LEVEL_NONE);
	}
}

static void __reset_visibility_properties(callui_app_data_t *ad)
{
	_callui_lock_manager_stop(ad->lock_handle);
	ad->start_lock_manager_on_resume = true;

	_callui_common_win_set_noti_type(ad, false);
}

void _callui_common_launch_setting_bluetooth(void *appdata)
{
	CALLUI_RETURN_IF_FAIL(appdata);

	callui_app_data_t *ad = appdata;

	app_control_h app_control = NULL;
	int ret;
	if ((ret = app_control_create(&app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_create() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_app_id(app_control, BLUETOOTH_PKG)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_PICK)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
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
	} else if ((ret = app_control_set_uri(app_control, PHONE_TELEPHONE_URI)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_uri() is failed. ret[%d]", ret);
	} else if ((ret = app_control_add_extra_data(app_control, PHONE_LAUNCH_TYPE_PARAM_NAME, PHONE_LAUNCH_TYPE_VALUE)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_add_extra_data() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed. ret[%d]", ret);
	} else {
		__reset_visibility_properties(ad);
	}
	if (app_control){
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
	} else if ((ret = app_control_set_app_id(app_control, CONTACTS_PKG)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
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

	char str[CONTACT_NUMBER_BUF_LEN];
	snprintf(str, sizeof(str), "%s%s", MESSAGE_SMS_URI, number);

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

	caller_info = elm_object_part_content_get(contents, "caller_info");
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

static void __callui_common_lock_state_cb (system_settings_key_e key, void *user_data)
{
	callui_app_data_t *ad = _callui_get_app_data();
	if (_callui_common_get_idle_lock_type() == LOCK_TYPE_UNLOCK) {
		_callui_common_win_set_noti_type(ad, false);
	} else {
		_callui_common_win_set_noti_type(ad, true);
	}
}

void _callui_common_set_lock_state_changed_cb(void *user_data)
{
	system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCK_STATE, __callui_common_lock_state_cb, NULL);
}

void _callui_common_unset_lock_state_changed_cb()
{
	system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCK_STATE);
}

static const char *__callui_common_get_res_path()
{
	static char res_folder_path[PATH_MAX] = {'\0'};
	if (res_folder_path[0] == '\0') {
		char *res_path_buff = app_get_resource_path();
		strncpy(res_folder_path, res_path_buff, PATH_MAX-1);
		free(res_path_buff);
	}
	return res_folder_path;
}

static const char *__callui_common_get_resource(const char *res_name)
{
	if (res_name == NULL) {
		err("res_name is NULL");
		return NULL;
	}

	static char res_path[PATH_MAX] = {'\0'};
	snprintf(res_path, PATH_MAX, "%s%s", __callui_common_get_res_path(), res_name);
	return res_path;
}

const char *_callui_common_get_call_edj_path()
{
	return __callui_common_get_resource(CALL_EDJ_NAME);
}

const char *_callui_common_get_call_theme_path()
{
	return __callui_common_get_resource(CALL_THEME_EDJ_NAME);
}

static char *__callui_common_vconf_get_str(const char *in_key)
{
	char *result = vconf_get_str(in_key);
	if (result == NULL) {
		err("vconf get error : %s", in_key);
	}
	return result;
}

static char *__callui_common_parse_vconf_string(char *input_string)
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

char *_callui_common_get_reject_msg_by_index(int index)
{
	char *message = NULL;
	char *markup_converted_message = NULL;
	char *return_str = NULL;
	char *parsed_message = NULL;

	switch (index) {
	case 0:
		message = __callui_common_vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG1_STR);
		break;
	case 1:
		message = __callui_common_vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG2_STR);
		break;
	case 2:
		message = __callui_common_vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG3_STR);
		break;
	case 3:
		message = __callui_common_vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG4_STR);
		break;
	case 4:
		message = __callui_common_vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG5_STR);
		break;
	case 5:
		message = __callui_common_vconf_get_str(VCONFKEY_CISSAPPL_USER_CREATE_MSG6_STR);
		break;
	default:
		return NULL;
	}

	if (NULL == message) {
		return NULL;
	}

	markup_converted_message = elm_entry_utf8_to_markup(message);
	parsed_message = __callui_common_parse_vconf_string(markup_converted_message);
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

static void ___callui_common_send_reject_msg_status_cb(msg_handle_t Handle, msg_struct_t pStatus, void *pUserParam)
{
	CALLUI_RETURN_IF_FAIL(pStatus != NULL);
	int status = MSG_NETWORK_SEND_FAIL;

	msg_get_int_value(pStatus, MSG_SENT_STATUS_NETWORK_STATUS_INT, &status);
	dbg("status:[%d]", status);
}

int _callui_common_send_reject_msg(void *appdata, char *reject_msg)
{
	CALLUI_RETURN_VALUE_IF_FAIL(reject_msg, CALLUI_RESULT_INVALID_PARAM);

	int res = CALLUI_RESULT_FAIL;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	const callui_call_data_t *incom = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);

	CALLUI_RETURN_VALUE_IF_FAIL(incom, CALLUI_RESULT_FAIL);

	if (strlen(reject_msg) == 0) {
		err("Is not reject with message case");
		return res;
	}

	msg_handle_t msgHandle = NULL;
	msg_error_t err = msg_open_msg_handle(&msgHandle);
	if (err != MSG_SUCCESS) {
		dbg("msg_open_msg_handle()- failed [%d]", err);
		return res;
	}

	err = msg_reg_sent_status_callback(msgHandle, ___callui_common_send_reject_msg_status_cb, NULL);
	if (err != MSG_SUCCESS) {
		dbg("msg_reg_sent_status_callback()- failed [%d]", err);
		msg_close_msg_handle(&msgHandle);
		return res;
	}

	msg_struct_t msgInfo = msg_create_struct(MSG_STRUCT_MESSAGE_INFO);
	msg_struct_t sendOpt = msg_create_struct(MSG_STRUCT_SENDOPT);
	msg_struct_t pReq = msg_create_struct(MSG_STRUCT_REQUEST_INFO);

	/* Set message type to SMS reject*/
	msg_set_int_value(msgInfo, MSG_MESSAGE_TYPE_INT, MSG_TYPE_SMS_REJECT);

	int slot_id = CALLUI_SIM_SLOT_DEFAULT;
	dbg("msg_sms_send_message() Sim slot [%d]", slot_id);
	slot_id++;
	msg_set_int_value(msgInfo, MSG_MESSAGE_SIM_INDEX_INT, slot_id);

	/* No setting send option */
	msg_set_bool_value(sendOpt, MSG_SEND_OPT_SETTING_BOOL, false);

	/* Set message body */
	if (msg_set_str_value(msgInfo, MSG_MESSAGE_SMS_DATA_STR, reject_msg, strlen(reject_msg)) != MSG_SUCCESS) {
		err("msg_set_str_value() - failed");
	} else {
		/* Create address list*/
		msg_struct_list_s *addr_list;
		msg_get_list_handle(msgInfo, MSG_MESSAGE_ADDR_LIST_STRUCT, (void **)&addr_list);
		msg_struct_t addr_info = addr_list->msg_struct_info[0];
		const char *call_number = incom->call_num;

		/* Set message address */
		msg_set_int_value(addr_info, MSG_ADDRESS_INFO_RECIPIENT_TYPE_INT, MSG_RECIPIENTS_TYPE_TO);
		msg_set_str_value(addr_info, MSG_ADDRESS_INFO_ADDRESS_VALUE_STR, call_number, strlen(call_number));
		addr_list->nCount = 1;

		/* Set message struct to Request*/
		msg_set_struct_handle(pReq, MSG_REQUEST_MESSAGE_HND, msgInfo);
		msg_set_struct_handle(pReq, MSG_REQUEST_SENDOPT_HND, sendOpt);

		/* Send message */
		err = msg_sms_send_message(msgHandle, pReq);
		if (err != MSG_SUCCESS) {
			err("msg_sms_send_message() - failed [%d]", err);
		} else {
			dbg("Sending...");
			res = CALLUI_RESULT_OK;
		}
	}
	msg_close_msg_handle(&msgHandle);
	msg_release_struct(&pReq);
	msg_release_struct(&msgInfo);
	msg_release_struct(&sendOpt);

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

	char *tmp = _callui_common_get_time_string(cur_time);
	elm_object_part_text_set(obj, part, tmp);
	free(tmp);
}

char *_callui_common_get_time_string(struct tm *time)
{
	char *tm_string = calloc(1, TIME_BUF_LEN);

	if (time->tm_hour > 0) {
		snprintf(tm_string, TIME_BUF_LEN, "%02d:%02d:%02d", time->tm_hour, time->tm_min, time->tm_sec);
	} else {
		snprintf(tm_string, TIME_BUF_LEN, "%02d:%02d", time->tm_min, time->tm_sec);
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
	}

	long call_time = curr_time - time;
	gmtime_r((const time_t *)&call_time, time_tm);

	return time_tm;
}
