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

#include <gio/gio.h>
#include <vconf.h>
#include <stdlib.h>
#include <efl_util.h>
#include <device/power.h>
#include <device/display.h>

#include "callui.h"
#include "callui-display.h"
#include "callui-debug.h"
#include "callui-common-def.h"

struct __callui_display {
	GDBusConnection *conn;
	GCancellable *cancel_obj;
	callui_app_data_t *ad;
};
typedef struct __callui_display __callui_display_t;

#define BUS_NAME					"org.tizen.system.deviced"
#define OBJECT_PATH					"/Org/Tizen/System/DeviceD"
#define INTERFACE_NAME				BUS_NAME
#define DEVICED_PATH_DISPLAY		OBJECT_PATH"/Display"
#define DEVICED_INTERFACE_DISPLAY	INTERFACE_NAME".display"
#define METHOD_SET_DISPLAY_TIMEOUT	"setlcdtimeout"

#define TIMEOUT_PARAMS_COUNT	3
#define DBUS_REPLY_TIMEOUT		(120 * 1000)

static callui_result_e _callui_display_init(callui_display_h display, callui_app_data_t *appdata);
static void _callui_display_deinit(callui_display_h display);
static callui_result_e __send_gdbus_msg_async(callui_display_h display, int *param);
static void __gdbus_reply_msg_async_cb(GObject *source_object, GAsyncResult *res, gpointer user_data);

static callui_result_e _callui_display_init(callui_display_h display, callui_app_data_t *appdata)
{
	display->ad = appdata;

	GError *err = NULL;
	display->conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (err) {
		err("Connection error occurred. [%d]-[%s]", err->code, err->message);
		g_error_free(err);
		return CALLUI_RESULT_FAIL;
	}

	display->cancel_obj = g_cancellable_new();
	CALLUI_RETURN_VALUE_IF_FAIL(display->cancel_obj, CALLUI_RESULT_FAIL);

	return CALLUI_RESULT_OK;
}

static void _callui_display_deinit(callui_display_h display)
{
	g_cancellable_cancel(display->cancel_obj);

	g_object_unref (display->cancel_obj);

	g_object_unref(display->conn);
}

callui_display_h _callui_display_create(callui_app_data_t *appdata)
{
	CALLUI_RETURN_NULL_IF_FAIL(appdata);

	callui_display_h contr = calloc(1, sizeof(__callui_display_t));
	CALLUI_RETURN_NULL_IF_FAIL(contr);

	callui_result_e res = _callui_display_init(contr, appdata);
	if (res != CALLUI_RESULT_OK) {
		_callui_display_deinit(contr);
		FREE(contr);
	}
	return contr;
}

void _callui_display_destroy(callui_display_h display)
{
	CALLUI_RETURN_IF_FAIL(display);

	_callui_display_deinit(display);

	free(display);
}

static void __gdbus_reply_msg_async_cb(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	GError *err = NULL;
	GDBusMessage *reply_msg =
			g_dbus_connection_send_message_with_reply_finish(G_DBUS_CONNECTION(source_object), res, &err);

	if (err) {
		err("Local in-process error occurred. [%d]-[%s]", err->code, err->message);
		g_error_free(err);
	}

	if (reply_msg) {
		dbg("Reply message was received");
		if (g_dbus_message_to_gerror(reply_msg, &err)) {
			dbg("Reply is an error. [%d]-[%s]", err->code, err->message);
			g_error_free(err);
		}
	} else {
		err("There is no reply message");
	}

	g_object_unref(reply_msg);

	debug_leave();
}

static callui_result_e __send_gdbus_msg_async(callui_display_h display, int *param)
{
	if (!display->conn) {
		return CALLUI_RESULT_FAIL;
	}

	GDBusMessage *msg = g_dbus_message_new_method_call(BUS_NAME,
			DEVICED_PATH_DISPLAY,
			DEVICED_INTERFACE_DISPLAY,
			METHOD_SET_DISPLAY_TIMEOUT);
	if (!msg) {
		err("g_dbus_message_new_method_call() failed");
		return CALLUI_RESULT_FAIL;
	}

	GVariant *arguments = g_variant_new("(iii)", param[0], param[1], param[2]);
	if (!arguments) {
		err("g_variant_new() failed.");
		g_object_unref(msg);
		return CALLUI_RESULT_FAIL;
	}

	g_dbus_message_set_body(msg, arguments);

	g_dbus_connection_send_message_with_reply(
			display->conn,
			msg,
			G_DBUS_SEND_MESSAGE_FLAGS_NONE,
			DBUS_REPLY_TIMEOUT,
			NULL,
			display->cancel_obj,
			__gdbus_reply_msg_async_cb,
			NULL);

	g_object_unref (msg);

	return CALLUI_RESULT_OK;
}

callui_result_e _callui_display_set_timeout(callui_display_h display, callui_display_timeout_e timeout_type)
{
	debug_enter();

	CALLUI_RETURN_VALUE_IF_FAIL(display, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL((timeout_type >= CALLUI_DISPLAY_TIMEOUT_DEFAULT) &&
				(timeout_type <= CALLUI_DISPLAY_TIMEOUT_LS_SET), CALLUI_RESULT_INVALID_PARAM);

	int powerkey_mode = 0;
	int res = vconf_get_bool(VCONFKEY_CISSAPPL_POWER_KEY_ENDS_CALL_BOOL, &powerkey_mode);
	if (res < 0) {
		err("vconf_get_int failed. res[%d]", res);
	}

	dbg("timeout type [%d], powerkeymode [%d]", timeout_type, powerkey_mode);

	int timeout_params[TIMEOUT_PARAMS_COUNT] = { 0 };
	timeout_params[2] = powerkey_mode;

	switch(timeout_type) {
	case CALLUI_DISPLAY_TIMEOUT_SET:
		timeout_params[0] = 10;
		timeout_params[1] = 20;
		break;
	case CALLUI_DISPLAY_TIMEOUT_UNSET:
		break;
	case CALLUI_DISPLAY_TIMEOUT_LS_SET: /*After lock-screen comes in Connected state LCD goes to OFF in 5 secs*/
		timeout_params[0] = 5;
		timeout_params[1] = 0;
		break;
	case CALLUI_DISPLAY_TIMEOUT_DEFAULT:
		timeout_params[2] = 0;
		break;
	default:
		break;
	}

	return __send_gdbus_msg_async(display, timeout_params);
}

callui_result_e _callui_display_set_control_state(callui_display_h display, callui_display_control_e state)
{
	debug_enter();

	CALLUI_RETURN_VALUE_IF_FAIL(display, CALLUI_RESULT_INVALID_PARAM);

	dbg("[%d]", state);
	int result = -1;
	switch (state) {
	case CALLUI_DISPLAY_ON:
		result = device_display_change_state(DISPLAY_STATE_NORMAL);
		CALLUI_RETURN_VALUE_IF_FAIL(result == DEVICE_ERROR_NONE, CALLUI_RESULT_FAIL);
		break;
	case CALLUI_DISPLAY_ON_LOCK:
		result = device_display_change_state(DISPLAY_STATE_NORMAL);
		CALLUI_RETURN_VALUE_IF_FAIL(result == DEVICE_ERROR_NONE, CALLUI_RESULT_FAIL);
		result = efl_util_set_window_screen_mode(display->ad->win, EFL_UTIL_SCREEN_MODE_ALWAYS_ON);
		CALLUI_RETURN_VALUE_IF_FAIL(result == EFL_UTIL_ERROR_NONE, CALLUI_RESULT_FAIL);
		break;

	case CALLUI_DISPLAY_ON_UNLOCK:
		result = device_display_change_state(DISPLAY_STATE_NORMAL);
		CALLUI_RETURN_VALUE_IF_FAIL(result == DEVICE_ERROR_NONE, CALLUI_RESULT_FAIL);
		result = efl_util_set_window_screen_mode(display->ad->win, EFL_UTIL_SCREEN_MODE_DEFAULT);
		CALLUI_RETURN_VALUE_IF_FAIL(result == EFL_UTIL_ERROR_NONE, CALLUI_RESULT_FAIL);
		result = device_power_release_lock(POWER_LOCK_CPU);
		CALLUI_RETURN_VALUE_IF_FAIL(result == DEVICE_ERROR_NONE, CALLUI_RESULT_FAIL);
		break;

	case CALLUI_DISPLAY_UNLOCK:
		result = efl_util_set_window_screen_mode(display->ad->win, EFL_UTIL_SCREEN_MODE_DEFAULT);
		CALLUI_RETURN_VALUE_IF_FAIL(result == EFL_UTIL_ERROR_NONE, CALLUI_RESULT_FAIL);
		result = device_power_release_lock(POWER_LOCK_CPU);
		CALLUI_RETURN_VALUE_IF_FAIL(result == EFL_UTIL_ERROR_NONE, CALLUI_RESULT_FAIL);
		break;

	case CALLUI_DISPLAY_OFF_SLEEP_LOCK:
		result = device_power_request_lock(POWER_LOCK_CPU, 0);
		CALLUI_RETURN_VALUE_IF_FAIL(result == EFL_UTIL_ERROR_NONE, CALLUI_RESULT_FAIL);
		break;

	case CALLUI_DISPLAY_OFF_SLEEP_UNLOCK:
		result = device_power_release_lock(POWER_LOCK_CPU);
		CALLUI_RETURN_VALUE_IF_FAIL(result == EFL_UTIL_ERROR_NONE, CALLUI_RESULT_FAIL);
		break;

	case CALLUI_DISPLAY_OFF:
		result = device_display_change_state(DISPLAY_STATE_SCREEN_OFF);
		CALLUI_RETURN_VALUE_IF_FAIL(result == EFL_UTIL_ERROR_NONE, CALLUI_RESULT_FAIL);
		break;

	default:
		break;
	}
	return CALLUI_RESULT_OK;
}

callui_result_e _callui_display_get_control_state(callui_display_h display, callui_display_control_e *state)
{
	CALLUI_RETURN_VALUE_IF_FAIL(display, CALLUI_RESULT_INVALID_PARAM);

	display_state_e disp_state = DISPLAY_STATE_NORMAL;
	int res = device_display_get_state(&disp_state);
	if (res != DEVICE_ERROR_NONE) {
		err("device_display_get_state() failed. res[%d]", res);
		return CALLUI_RESULT_FAIL;
	}

	switch (disp_state) {
	case DISPLAY_STATE_SCREEN_OFF:
		*state = CALLUI_DISPLAY_OFF;
		break;
	case DISPLAY_STATE_NORMAL:
	case DISPLAY_STATE_SCREEN_DIM:
	default:
		*state = CALLUI_DISPLAY_ON;
		break;
	}

	return CALLUI_RESULT_OK;
}
