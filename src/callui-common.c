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
#include <device/display.h>
#include <device/power.h>
#include <dbus/dbus.h>
#include <gio/gio.h>
#include <runtime_info.h>
#include <bluetooth.h>
#include <system_settings.h>
#include <efl_util.h>
#include <app_common.h>
#include <msg.h>
#include <msg_transport.h>
//#include <Ecore_Wayland.h>

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

#define CONTACT_PKG			"org.tizen.contacts"
#define PHONE_PKG			"org.tizen.phone"
#define BLUETOOTH_PKG		"ug-bluetooth-efl"
#define BUS_NAME			"org.tizen.system.deviced"
#define OBJECT_PATH			"/Org/Tizen/System/DeviceD"
#define INTERFACE_NAME		BUS_NAME

#define DEVICED_PATH_DISPLAY	OBJECT_PATH"/Display"
#define DEVICED_INTERFACE_DISPLAY	INTERFACE_NAME".display"
#define LCD_ON_SIGNAL_NAME "LCDOnByPowerkey"
#define METHOD_SET_LCDTIMEOUT "setlcdtimeout"

#define TIME_BUF_LEN (16)

#define DBUS_REPLY_TIMEOUT (120 * 1000)

static bool g_is_headset_connected;

#define CALL_EDJ_NAME		"/edje/call.edj"
#define CALL_THEME_EDJ_NAME	"/edje/call_theme.edj"

struct dbus_byte {
	const char *data;
	int size;
};

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

static Eina_Bool __callui_common_ending_timer_expired_cb(void *data)
{
	dbg("__callui_common_ending_timer_expired_cb");
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);
	callui_app_data_t *ad = (callui_app_data_t *)data;

	ad->ending_timer = NULL;
	_callui_common_exit_app();

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __callui_common_ending_timer_blink_cb(void *data)
{
	dbg("__callui_common_ending_timer_blink_cb");
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);
	callui_app_data_t *ad = (callui_app_data_t *)data;

	if ((ad->blink_cnt % 2) == 0) {
		_callui_show_caller_info_status(ad, _("IDS_CALL_BODY_CALL_ENDE_M_STATUS_ABB"));
	} else if ((ad->blink_cnt % 2) == 1) {
		_callui_show_caller_info_status(ad, _(" "));
	}

	ad->blink_cnt++;
	if (ad->blink_cnt == 5) {
		/* Run a timer of 2secs for destroying the end selection menu */
		DELETE_ECORE_TIMER(ad->ending_timer);
		ad->ending_timer = ecore_timer_add(2, __callui_common_ending_timer_expired_cb, ad);

		ad->blink_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}
	return ECORE_CALLBACK_RENEW;
}

void _callui_common_create_ending_timer(void *appdata)
{
	CALLUI_RETURN_IF_FAIL(appdata);
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	ad->blink_cnt = 0;
	DELETE_ECORE_TIMER(ad->blink_timer);
	ad->blink_timer = ecore_timer_add(0.5, __callui_common_ending_timer_blink_cb, ad);
}

void _callui_common_delete_ending_timer(void *appdata)
{
	CALLUI_RETURN_IF_FAIL(appdata);
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	DELETE_ECORE_TIMER(ad->ending_timer);
	DELETE_ECORE_TIMER(ad->blink_timer);
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
	vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);
	return 0;
}

void _callui_common_win_set_noti_type(void *appdata, int bwin_noti)
{
//	dbg("_callui_common_win_set_noti_type START");
//	callui_app_data_t *ad = (callui_app_data_t *)appdata;
//
//	Ecore_Wl_Window *win = elm_win_wl_window_get(ad->win);
//	if (bwin_noti == EINA_FALSE) {
//		dbg("window type: NORMAL");
//		/* Set Normal window */
//		ecore_wl_window_type_set(win, ECORE_WL_WINDOW_TYPE_TOPLEVEL);
//	} else {
//		dbg("window type: NOTI-HIGH");
//		/* Set Notification window */
//		ecore_wl_window_type_set(win, ECORE_WL_WINDOW_TYPE_NOTIFICATION);
//		/* Set Notification's priority to LEVEL_HIGH */
//		efl_util_set_notification_window_level(ad->win, EFL_UTIL_NOTIFICATION_LEVEL_TOP);
//	}
//	dbg("_callui_common_win_set_noti_type END");
//
//	return;
}

void _callui_common_launch_contacts(void *appdata)
{
	dbg("..");
	app_control_h service;
	callui_app_data_t *ad = appdata;
	_callui_lock_manager_stop(ad->lock_handle);
	ad->start_lock_manager_on_resume = true;

	_callui_common_win_set_noti_type(appdata, EINA_FALSE);

	int ret = app_control_create(&service);
	if (ret < 0) {
		err("app_control_create() return error : %d", ret);
		return;
	}

	if (app_control_set_app_id(service, CONTACT_PKG)  != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed");
	} else if (app_control_set_operation(service, APP_CONTROL_OPERATION_DEFAULT)  != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed");
	} else if (app_control_send_launch_request(service, NULL, NULL)  != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed");
	}

	app_control_destroy(service);
}

void _callui_common_launch_bt_app(void *appdata)
{
	dbg("..");
	app_control_h service;
	callui_app_data_t *ad = appdata;
	_callui_lock_manager_stop(ad->lock_handle);
	ad->start_lock_manager_on_resume = true;

	_callui_common_win_set_noti_type(appdata, EINA_FALSE);

	int ret = app_control_create(&service);
	if (ret < 0) {
		err("app_control_create() return error : %d", ret);
		return;
	}

	if (app_control_set_app_id(service, BLUETOOTH_PKG) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed");
	} else if (app_control_set_operation(service, APP_CONTROL_OPERATION_PICK) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed");
	} else if (app_control_send_launch_request(service, NULL, NULL)  != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed");
	}

	app_control_destroy(service);
}

void _callui_common_launch_dialer(void *appdata)
{
	app_control_h service = NULL;
	int ret = APP_CONTROL_ERROR_NONE;
	char *uri = NULL;

	callui_app_data_t *ad = appdata;
	_callui_lock_manager_stop(ad->lock_handle);
	ad->start_lock_manager_on_resume = true;

	_callui_common_win_set_noti_type(appdata, EINA_FALSE);

	ret = app_control_create(&service);
	if (ret < 0) {
		err("app_control_create() return error : %d", ret);
		return;
	}

	uri = (char *)calloc(5, sizeof(char));
	if (uri == NULL) {
		err("memory alloc failed");
		app_control_destroy(service);
		return;
	}
	snprintf(uri, sizeof(char)*5, "%s", "tel:");

	if (app_control_set_app_id(service, PHONE_PKG)  != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed");
	} else if (app_control_set_operation(service, APP_CONTROL_OPERATION_DIAL)  != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed");
	} else if (app_control_set_uri(service, uri)  != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_uri() is failed");
	} else if (app_control_add_extra_data(service, "launch_type", "add_call")  != APP_CONTROL_ERROR_NONE) {
		err("app_control_add_extra_data() is failed");
	} else if (app_control_send_launch_request(service, NULL, NULL)  != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed");
	}

	app_control_destroy(service);
	g_free(uri);

	return;
}

void _callui_common_launch_msg_composer(void *appdata, const char *number)
{
	dbg("..");

	app_control_h service;
	int ret = APP_CONTROL_ERROR_NONE;

	callui_app_data_t *ad = appdata;
	_callui_lock_manager_stop(ad->lock_handle);
	ad->start_lock_manager_on_resume = true;

	_callui_common_win_set_noti_type(appdata, EINA_FALSE);

	ret = app_control_create(&service);
	if (ret != APP_CONTROL_ERROR_NONE) {
		warn("app_control_create() return error : %d", ret);
		return;
	}

	ret = app_control_set_app_id(service, MSG_PKG);
	if (ret != APP_CONTROL_ERROR_NONE) {
		warn("app_control_set_app_id() return error : %d", ret);
		ret = app_control_destroy(service);
		return;
	}

	if (number) {
		ret = app_control_add_extra_data(service, "type", "compose");
		if (ret != APP_CONTROL_ERROR_NONE) {
			warn("app_control_add_extra_data() return error : %d", ret);
			ret = app_control_destroy(service);
			return;
		}

		if (strlen(number) > 0) {
			const char *array[] = { number };
			ret = app_control_add_extra_data_array(service, APP_CONTROL_DATA_TO, array, 1);
			if (ret != APP_CONTROL_ERROR_NONE) {
				warn("app_control_add_extra_data() return error : %d", ret);
				ret = app_control_destroy(service);
				return;
			}
		}
	}

	ret = app_control_send_launch_request(service, NULL, NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		warn("app_control_send_launch_request() is failed : %d", ret);
	}

	app_control_destroy(service);
}

/* LCD api */
void _callui_common_dvc_control_lcd_state(callui_lcd_control_t state)
{
	dbg("[%d]", state);
	int result = -1;
	switch (state) {
	case LCD_ON:
		result = device_display_change_state(DISPLAY_STATE_NORMAL);
		break;

	case LCD_ON_LOCK:
		result = device_display_change_state(DISPLAY_STATE_NORMAL);
		result = device_power_request_lock(POWER_LOCK_DISPLAY, 0);
		break;

	case LCD_ON_UNLOCK:
		result = device_display_change_state(DISPLAY_STATE_NORMAL);
		result = device_power_release_lock(POWER_LOCK_DISPLAY);
		result = device_power_release_lock(POWER_LOCK_CPU);
		break;

	case LCD_UNLOCK:
		result = device_power_release_lock(POWER_LOCK_DISPLAY);
		result = device_power_release_lock(POWER_LOCK_CPU);
		break;

	case LCD_OFF_SLEEP_LOCK:
		result = device_power_request_lock(POWER_LOCK_CPU, 0);
		break;

	case LCD_OFF_SLEEP_UNLOCK:
		result = device_power_release_lock(POWER_LOCK_CPU);
		break;

	case LCD_OFF:
		result = device_display_change_state(DISPLAY_STATE_SCREEN_OFF);
		break;

	default:
		break;
	}
	if (result != DEVICE_ERROR_NONE)
		warn("error during change lcd state");
}

callui_lcd_control_t _callui_common_get_lcd_state()
{
	display_state_e state = DISPLAY_STATE_NORMAL;
	int result = device_display_get_state(&state);
	if (result != DEVICE_ERROR_NONE) {
		warn("error during get lcd state");
	}
	switch (state) {
		case DISPLAY_STATE_SCREEN_OFF:
			return LCD_OFF;
		case DISPLAY_STATE_NORMAL:
		case DISPLAY_STATE_SCREEN_DIM:
		default:
			return LCD_ON;
	}
}

#ifdef _DBUS_DVC_LSD_TIMEOUT_

static int __callui_common_dvc_append_variant(DBusMessageIter *iter, const char *sig, char *param[])
{
	char *ch = NULL;
	int i;
	int int_type;
	uint64_t int64_type;
	DBusMessageIter arr;
	struct dbus_byte *byte;

	if (!sig || !param)
		return 0;

	for (ch = (char *)sig, i = 0; *ch != '\0'; ++i, ++ch) {
		switch (*ch) {
			case 'i':
				int_type = atoi(param[i]);
				dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &int_type);
				break;
			case 'u':
				int_type = strtoul(param[i], NULL, 10);
				dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &int_type);
				break;
			case 't':
				int64_type = atoll(param[i]);
				dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &int64_type);
				break;
			case 's':
				dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param[i]);
				break;
			case 'a':
				if ((ch+1 != NULL) && (*(ch+1) == 'y')) {
					dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &arr);
					byte = (struct dbus_byte *)param[i];
					dbus_message_iter_append_fixed_array(&arr, DBUS_TYPE_BYTE, &(byte->data), byte->size);
					dbus_message_iter_close_container(iter, &arr);
					ch++;
				}
				break;
			default:
				return -EINVAL;
		}
	}
	return 0;
}

static void __callui_common_dvc_dbus_reply_cb(DBusPendingCall *call, gpointer user_data)
{
	//TODO DBus is not supported. Need to move on kdbus or gdbus

	DBusMessage *reply;
	DBusError derr;

	reply = dbus_pending_call_steal_reply(call);
	dbus_error_init(&derr);

	if (dbus_set_error_from_message(&derr, reply)) {
		err("__callui_common_dvc_dbus_reply_cb error: %s, %s",
			derr.name, derr.message);
		dbus_error_free(&derr);
		goto done;
	}
	dbus_pending_call_unref(call);
done:
	dbg("__callui_common_dvc_dbus_reply_cb : -");
	dbus_message_unref(reply);
}

gboolean __callui_common_dvc_invoke_dbus_method_async(const char *dest,
		const char *path,
		const char *interface,
		const char *method, const char *sig, char *param[])
{
	// TODO DBus is not supported. Need to move on kdbus or gdbus

	DBusConnection *conn = NULL;
	DBusMessage *msg = NULL;
	DBusMessageIter iter;
	DBusPendingCall *c = NULL;
	dbus_bool_t ret = FALSE;
	int r = -1;
	conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (!conn) {
		err("dbus bus get error");
		return FALSE;
	}

	msg = dbus_message_new_method_call(dest, path, interface, method);
	if (!msg) {
		err("dbus_message_new_method_call(%s:%s-%s",
				path, interface, method);
		return FALSE;
	}
	dbus_message_iter_init_append(msg, &iter);
	r = __callui_common_dvc_append_variant(&iter, sig, param);
	if (r < 0) {
		err("append_variant error : %d", r);
		goto EXIT;
	}

	ret = dbus_connection_send_with_reply(conn, msg, &c, DBUS_REPLY_TIMEOUT);
	if (!ret) {
		err("dbus_connection_send_ error (False returned)");
	}
	dbus_pending_call_set_notify(c, __callui_common_dvc_dbus_reply_cb,	NULL, NULL);

EXIT:
	dbus_message_unref(msg);

	return ret;
	return true;
}

void _callui_common_dvc_set_lcd_timeout(callui_lcd_timeout_t state)
{
	int powerkey_mode = 0;
	char str_on[32];
	char str_dim[32];
	char str_holdkey[2];
	char *ar[3];
	int ret = vconf_get_bool(VCONFKEY_CISSAPPL_POWER_KEY_ENDS_CALL_BOOL, &powerkey_mode);
	if (ret < 0) {
		err("vconf_get_int failed..[%d]\n", ret);
	}

	dbg("set timeout : %d, powerkeymode : %d", state, powerkey_mode);
	if (state == LCD_TIMEOUT_SET) {
		snprintf(str_on, sizeof(str_on), "%d", 10);
		snprintf(str_dim, sizeof(str_dim), "%d", 20);
		snprintf(str_holdkey, sizeof(str_holdkey), "%d", powerkey_mode);
	} else if (state == LCD_TIMEOUT_UNSET) {
		snprintf(str_on, sizeof(str_on), "%d", 0);
		snprintf(str_dim, sizeof(str_dim), "%d", 0);
		snprintf(str_holdkey, sizeof(str_holdkey), "%d", powerkey_mode);
	} else if (state == LCD_TIMEOUT_LOCKSCREEN_SET) { /*After lock-screen comes in Connected state LCD goes to OFF in 5 secs*/
		snprintf(str_on, sizeof(str_on), "%d", 5);
		snprintf(str_dim, sizeof(str_dim), "%d", 0);
		snprintf(str_holdkey, sizeof(str_holdkey), "%d", powerkey_mode);
	} else if (state == LCD_TIMEOUT_KEYPAD_SET) {
		snprintf(str_on, sizeof(str_on), "%d", 3);
		snprintf(str_dim, sizeof(str_dim), "%d", 5);
		snprintf(str_holdkey, sizeof(str_holdkey), "%d", powerkey_mode);
	} else {
		snprintf(str_on, sizeof(str_on), "%d", 0);
		snprintf(str_dim, sizeof(str_dim), "%d", 0);
		snprintf(str_holdkey, sizeof(str_holdkey), "%d", 0);
	}

	dbg("on(%s), dim(%s), hold(%s)", str_on, str_dim, str_holdkey);

	ar[0] = str_on;
	ar[1] = str_dim;
	ar[2] = str_holdkey;

	__callui_common_dvc_invoke_dbus_method_async(BUS_NAME,
				DEVICED_PATH_DISPLAY,
				DEVICED_INTERFACE_DISPLAY,
				METHOD_SET_LCDTIMEOUT, "iii", ar);
}

#endif

void _callui_common_reset_main_ly_text_fields(Evas_Object *contents)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(contents);
	Evas_Object *caller_info = NULL;

	edje_object_part_text_set(_EDJ(contents), "call_txt_status", "");
	edje_object_part_text_set(_EDJ(contents), "txt_timer", "");

	caller_info = elm_object_part_content_get(contents, "caller_info");
	if (caller_info) {
		edje_object_part_text_set(_EDJ(caller_info), "txt_call_name", "");
		edje_object_part_text_set(_EDJ(caller_info), "txt_phone_num", "");
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
		_callui_common_win_set_noti_type(ad, EINA_FALSE);
	} else {
		_callui_common_win_set_noti_type(ad, EINA_TRUE);
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

	const callui_call_state_data_t *incom = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_TYPE_INCOMING);

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

	// TODO: Need functionality
	int slot_id = 1;
	dbg("msg_sms_send_message() Sim slot [%d]", slot_id);
	if (slot_id != -1) {
		slot_id++;
		msg_set_int_value(msgInfo, MSG_MESSAGE_SIM_INDEX_INT, slot_id);
	}

	/* No setting send option */
	msg_set_bool_value(sendOpt, MSG_SEND_OPT_SETTING_BOOL, FALSE);

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
	//return (ad->speaker_status || ad->headset_status || ad->earphone_status);

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

	char dur[TIME_BUF_LEN];
	if (cur_time->tm_hour > 0) {
		snprintf(dur, TIME_BUF_LEN, "%02d:%02d:%02d", cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
	} else {
		snprintf(dur, TIME_BUF_LEN, "%02d:%02d", cur_time->tm_min, cur_time->tm_sec);
	}
	elm_object_part_text_set(obj, part, _(dur));
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
