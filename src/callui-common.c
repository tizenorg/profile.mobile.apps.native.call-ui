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
#include <Ecore_Wayland.h>

#include "callui-common.h"
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

static Eina_Bool __callui_common_duration_timer_cb(void *data)
{
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_VALUE_IF_FAIL(ad, ECORE_CALLBACK_CANCEL);

	if ((ad->active) && (ad->incom == NULL) && (ad->active->call_state != CM_CALL_STATE_DIALING)) {
		if (ad->held) {
			_callui_common_update_call_duration(ad->held->start_time);
		} else {
			_callui_common_update_call_duration(ad->active->start_time);
		}
	}
	return ECORE_CALLBACK_RENEW;
}

void _callui_common_set_call_duration(char *time_dur)
{
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);
	Evas_Object *layout = NULL;
	layout = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
	CALLUI_RETURN_IF_FAIL(layout);
	if (_callvm_get_top_view_id(ad->view_manager_handle) == VIEW_INCALL_MULTICALL_SPLIT_VIEW) {
		Evas_Object *one_hold_layout = elm_object_part_content_get(layout, PART_SWALLOW_CALL_INFO);
		Evas_Object *active_layout = elm_object_part_content_get(one_hold_layout, PART_SWALLOW_ACTIVE_INFO);
		elm_object_part_text_set(active_layout, PART_TEXT_STATUS, time_dur);
	} else {
		edje_object_part_text_set(_EDJ(layout), "call_txt_status", _(time_dur));
	}

	if (ad->win_quickpanel && ad->quickpanel_layout) {
		_callui_view_qp_set_call_timer(ad->quickpanel_layout, time_dur);
	}
}

void _callui_common_update_call_duration(long starttime)
{
	callui_app_data_t *ad = _callui_get_app_data();
	long curr_time = 0;
	struct tm loctime;
	long call_time;

	curr_time = _callui_common_get_uptime();
	call_time = curr_time - starttime;
	gmtime_r((const time_t *)&call_time, &loctime);

	ad->current_sec = loctime.tm_sec;
	ad->current_min = loctime.tm_min;
	ad->current_hour = loctime.tm_hour;

	char dur[TIME_BUF_LEN];
	if (ad->current_hour > 0) {
		snprintf(dur, TIME_BUF_LEN, "%02d:%02d:%02d", ad->current_hour, ad->current_min, ad->current_sec);
	} else {
		snprintf(dur, TIME_BUF_LEN, "%02d:%02d", ad->current_min, ad->current_sec);
	}

	_callui_common_set_call_duration(dur);
}

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

void _callui_common_create_duration_timer()
{
	dbg("_callui_common_create_duration_timer..");
	callui_app_data_t *ad = _callui_get_app_data();

	if (ad->duration_timer) {
		ecore_timer_del(ad->duration_timer);
		ad->duration_timer = NULL;
	}
	CALLUI_RETURN_IF_FAIL(ad->active);

	_callui_common_update_call_duration(ad->active->start_time);
	ad->duration_timer = ecore_timer_add(1.0, __callui_common_duration_timer_cb, NULL);
	if (ad->duration_timer == NULL) {
		err("ecore_timer_add returned NULL");
	}
}

void _callui_common_delete_duration_timer()
{
	callui_app_data_t *ad = _callui_get_app_data();
	if (ad->duration_timer) {
		ecore_timer_del(ad->duration_timer);
		ad->duration_timer = NULL;
	}
}

static Eina_Bool __callui_common_ending_timer_expired_cb(void *data)
{
	dbg("__callui_common_ending_timer_expired_cb");
	callui_app_data_t *ad = _callui_get_app_data();

	ad->ending_timer = NULL;
	_callvm_terminate_app_or_view_change(ad);
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __callui_common_ending_timer_blink_cb(void *data)
{
	dbg("__callui_common_ending_timer_blink_cb");
	callui_app_data_t *ad = _callui_get_app_data();

	if ((ad->blink_cnt % 2) == 0) {
		_callui_show_caller_info_status(ad, _("IDS_CALL_BODY_CALL_ENDE_M_STATUS_ABB"));
	} else if ((ad->blink_cnt % 2) == 1) {
		_callui_show_caller_info_status(ad, _(" "));
	}

	ad->blink_cnt++;
	if (ad->blink_cnt == 5) {
		/* Run a timer of 2secs for destroying the end selection menu */
		if (ad->ending_timer) {
			ecore_timer_del(ad->ending_timer);
			ad->ending_timer = NULL;
		}
		ad->ending_timer = ecore_timer_add(2, __callui_common_ending_timer_expired_cb, NULL);

		ad->blink_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}
	return ECORE_CALLBACK_RENEW;
}

void _callui_common_create_ending_timer(call_view_data_t *vd)
{
	CALLUI_RETURN_IF_FAIL(vd);
	callui_app_data_t *ad = _callui_get_app_data();
	ad->blink_cnt = 0;
	if (ad->blink_timer) {
		ecore_timer_del(ad->blink_timer);
		ad->blink_timer = NULL;
	}
	ad->blink_timer = ecore_timer_add(0.5, __callui_common_ending_timer_blink_cb, vd);
}

void _callui_common_delete_ending_timer(void)
{
	callui_app_data_t *ad = _callui_get_app_data();
	if (ad->ending_timer) {
		ecore_timer_del(ad->ending_timer);
		ad->ending_timer = NULL;
	}

	if (ad->blink_timer) {
		ecore_timer_del(ad->blink_timer);
		ad->blink_timer = NULL;
	}
}

char *_callui_common_get_sim_name(void *appdata)
{
	dbg("_callui_common_get_sim_name");
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	char *sim_name = NULL;
	if (ad->sim_slot == CM_SIM_SLOT_1_E) {
		sim_name = vconf_get_str(VCONFKEY_SETAPPL_SIM1_NAME);
	} else if (ad->sim_slot == CM_SIM_SLOT_2_E) {
		sim_name = vconf_get_str(VCONFKEY_SETAPPL_SIM2_NAME);
	} else {
		err("Invalid option for sim slot reached !!!");
	}
	info("sim name is :  %s ", sim_name);
	return sim_name;
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


Eina_Bool _callui_common_is_headset_conected(void)
{
	g_is_headset_connected = false;
	callui_app_data_t *ad  = _callui_get_app_data();
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

long _callui_common_get_uptime(void)
{
	struct sysinfo info;

	if (sysinfo(&info) == 0) {
		return info.uptime;
	}
	return 0;
}

void _callui_common_win_set_noti_type(void *appdata, int bwin_noti)
{
	dbg("_callui_common_win_set_noti_type");
	// TODO: commented until functionality of switch window types will be fixed
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
}

void _callui_common_get_contact_info(int person_id, call_contact_data_t *ct_info)
{
	dbg("_callui_common_get_contact_info");
	contacts_error_e err = CONTACTS_ERROR_NONE;
	contacts_record_h person_record = NULL;

	if (person_id == -1) {
		err("Invalid contact index!!!");
		return;
	}

	if (ct_info == NULL) {
		err("Empty contact info!!!");
		return;
	}

	err = contacts_connect();
	if (CONTACTS_ERROR_NONE != err) {
		err("contacts_connect is error : %d", err);
		return;
	}

	ct_info->person_id = person_id;
	err = contacts_db_get_record(_contacts_person._uri, person_id, &person_record);
	if (CONTACTS_ERROR_NONE != err) {
		err("contacts_db_get_record error %d", err);
	} else {
		char *name = NULL;
		char *img_path = NULL;

		/* Get display name */
		err = contacts_record_get_str(person_record, _contacts_person.display_name, &name);
		if (CONTACTS_ERROR_NONE != err) {
			err("contacts_record_get_str(display name) error %d", err);
		} else {
			g_strlcpy(ct_info->call_disp_name, name, CALLUI_DISPLAY_NAME_LENGTH_MAX);
			free(name);
		}

		/* Get caller id path */
		err = contacts_record_get_str(person_record, _contacts_person.image_thumbnail_path, &img_path);
		if (CONTACTS_ERROR_NONE != err) {
			err("contacts_record_get_str(caller id path) error %d", err);
		} else {
			g_strlcpy(ct_info->caller_id_path, img_path, CALLUI_IMAGE_PATH_LENGTH_MAX);
			free(img_path);
		}
		contacts_record_destroy(person_record, TRUE);
	}

	dbg("contact index:[%d]", ct_info->person_id);
	dbg("display name:[%s]", ct_info->call_disp_name);
	dbg("img path:[%s]", ct_info->caller_id_path);

	contacts_disconnect();
	return;
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

void _callui_common_launch_msg_composer(void *appdata, char *number)
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
	int bPowerkeyMode = 0;
	char str_on[32];
	char str_dim[32];
	char str_holdkey[2];
	char *ar[3];
	int ret = vconf_get_bool(VCONFKEY_CISSAPPL_POWER_KEY_ENDS_CALL_BOOL, &bPowerkeyMode);
	if (ret < 0) {
		err("vconf_get_int failed..[%d]\n", ret);
	}

	dbg("set timeout : %d, powerkeymode : %d", state, bPowerkeyMode);
	if (state == LCD_TIMEOUT_SET) {
		snprintf(str_on, sizeof(str_on), "%d", 10);
		snprintf(str_dim, sizeof(str_dim), "%d", 20);
		snprintf(str_holdkey, sizeof(str_holdkey), "%d", bPowerkeyMode);
	} else if (state == LCD_TIMEOUT_UNSET) {
		snprintf(str_on, sizeof(str_on), "%d", 0);
		snprintf(str_dim, sizeof(str_dim), "%d", 0);
		snprintf(str_holdkey, sizeof(str_holdkey), "%d", bPowerkeyMode);
	} else if (state == LCD_TIMEOUT_LOCKSCREEN_SET) { /*After lock-screen comes in Connected state LCD goes to OFF in 5 secs*/
		snprintf(str_on, sizeof(str_on), "%d", 5);
		snprintf(str_dim, sizeof(str_dim), "%d", 0);
		snprintf(str_holdkey, sizeof(str_holdkey), "%d", bPowerkeyMode);
	} else if (state == LCD_TIMEOUT_KEYPAD_SET) {
		snprintf(str_on, sizeof(str_on), "%d", 3);
		snprintf(str_dim, sizeof(str_dim), "%d", 5);
		snprintf(str_holdkey, sizeof(str_holdkey), "%d", bPowerkeyMode);
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

	return;
}

gboolean _callui_common_is_extra_volume_available(void)
{
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_VALUE_IF_FAIL(ad, EINA_TRUE);
	cm_audio_state_type_e snd_path = CM_AUDIO_STATE_NONE_E;

	cm_get_audio_state(ad->cm_handle, &snd_path);
	dbg("sound path : %d", snd_path);

	if ((snd_path == CM_AUDIO_STATE_BT_E)
		|| ((snd_path == CM_AUDIO_STATE_EARJACK_E)
				&& (_callui_common_is_earjack_connected() == EINA_TRUE))) {
		return EINA_FALSE;
	} else {
		return EINA_TRUE;
	}
}

gboolean _callui_common_is_answering_mode_on(void)
{
	gboolean bAnswerMode = EINA_FALSE;
	Eina_Bool ret = EINA_FALSE;

	ret = vconf_get_bool(VCONFKEY_CISSAPPL_ANSWERING_KEY_BOOL, &bAnswerMode);
	if (0 == ret) {
		dbg("bAnswerMode = [%d] \n", bAnswerMode);
	} else {
		dbg("vconf_get_int failed..[%d]\n", ret);
	}

	return bAnswerMode;
}

gboolean _callui_common_is_powerkey_mode_on(void)
{
	gboolean bPowerkeyMode = EINA_FALSE;
	Eina_Bool ret = EINA_FALSE;

	ret = vconf_get_bool(VCONFKEY_CISSAPPL_POWER_KEY_ENDS_CALL_BOOL, &bPowerkeyMode);
	if (0 == ret) {
		dbg("bPowerkeyMode = [%d] \n", bPowerkeyMode);
	} else {
		dbg("vconf_get_int failed..[%d]\n", ret);
	}

	return bPowerkeyMode;
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

void _callui_common_set_lock_state_changed_cb()
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
