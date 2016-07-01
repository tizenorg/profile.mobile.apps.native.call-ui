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

#include <message_port.h>

#include "callui-indicator.h"
#include "callui.h"
#include "callui-debug.h"
#include "callui-common-defines.h"

#define CALLUI_LOCAL_PORT_NAME		"callui/indicator"
#define CALLUI_REMOTE_PORT_NAME		"indicator/bg/color"

#define CALLUI_KEY_BG				"indicator/bg/color"
#define CALLUI_KEY_VALUE_BG_CALL	"indicator/bg/color/call"

#define CALLUI_INDICATOR_APP		"org.tizen.indicator"

typedef enum {
	CALLUI_INDICATOR_DEFAULT = 0,
	CALLUI_INDICATOR_DURING_CALL,
	CALLUI_INDICATOR_CALL_ON_HOLD,
	CALLUI_INDICATOR_END_CALL,
	CALLUI_INDICATOR_COUNT
} __callui_indicator_type_e;

static const char *color_param[CALLUI_INDICATOR_COUNT] = {
		"call/idle", 		/* Default bg color */
		"call/during_call",	/* Incoming call or during call */
		"call/on_hold",		/* Call on hold */
		"call/end_call"		/* Call ended */
};

struct __callui_indicator {

	int local_port_id;
	bool active;
	callui_app_data_t *ad;
	__callui_indicator_type_e cur_type;
};
typedef struct __callui_indicator __callui_indicator_t;

typedef struct appdata callui_app_data_t;

static callui_result_e __callui_indicator_init(callui_indicator_h indicator, callui_app_data_t *ad);
static void __callui_indicator_deinit(callui_indicator_h indicator);

static void __reset_indicator_color(callui_indicator_h indicator);
static void __refresh_indicator_color(callui_indicator_h indicator);

static callui_result_e __register_local_msg_port(callui_indicator_h indicator, const char *port_name);
static void __unregister_local_msg_port(callui_indicator_h indicator);

static callui_result_e __convert_msg_port_result(int err);
static void __msg_port_cb(int trusted_local_port_id, const char *remote_app_id,
		const char *remote_port, bool trusted_remote_port, bundle *message, void *data);

static bundle *__create_color_change_msg(__callui_indicator_type_e type);
static callui_result_e __send_color_change_msg(callui_indicator_h indicator, bundle *message);

void __state_event_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info)
{
	debug_enter();
	callui_indicator_h indicator = user_data;
	if (indicator->active) {
		__refresh_indicator_color(user_data);
	}
}

static callui_result_e __callui_indicator_init(callui_indicator_h indicator, callui_app_data_t *ad)
{
	indicator->ad = ad;
	indicator->cur_type = CALLUI_INDICATOR_DEFAULT;

	CALLUI_RETURN_VALUE_IF_FAIL(ad->state_provider, CALLUI_RESULT_FAIL);
	callui_result_e res = _callui_stp_add_call_state_event_cb(indicator->ad->state_provider, __state_event_cb, indicator);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	bool port_exists = false;
	res = __convert_msg_port_result(
			message_port_check_trusted_remote_port(CALLUI_INDICATOR_APP, CALLUI_REMOTE_PORT_NAME, &port_exists));
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	if (port_exists) {
		return __register_local_msg_port(indicator, CALLUI_LOCAL_PORT_NAME);
	} else {
		return CALLUI_RESULT_NOT_REGISTERED;
	}
}

static void __reset_indicator_color(callui_indicator_h indicator)
{
	if (indicator->cur_type != CALLUI_INDICATOR_DEFAULT) {
		dbg("Indicator is not default type. Current type [%d]. Need to reset to default", indicator->cur_type);
		indicator->cur_type = CALLUI_INDICATOR_DEFAULT;
		bundle *msg = __create_color_change_msg(indicator->cur_type);
		CALLUI_RETURN_IF_FAIL(msg);
		callui_result_e res = __send_color_change_msg(indicator, msg);
		if (res != CALLUI_RESULT_OK) {
			err("Call __send_color_change_msg() failed. res[%d]", res);
		}
	}
}

static void __callui_indicator_deinit(callui_indicator_h indicator)
{
	_callui_stp_remove_call_state_event_cb(indicator->ad->state_provider, __state_event_cb, indicator);

	if (indicator->local_port_id) {
		__reset_indicator_color(indicator);
		__unregister_local_msg_port(indicator);
	}
}

static callui_result_e __convert_msg_port_result(int err)
{
	int result = CALLUI_RESULT_FAIL;

	switch (err) {
	case MESSAGE_PORT_ERROR_NONE:
		result = CALLUI_RESULT_OK;
		break;
	case MESSAGE_PORT_ERROR_IO_ERROR:
		err("MessagePort error: i/o error");
		break;
	case MESSAGE_PORT_ERROR_OUT_OF_MEMORY:
		err("MessagePort error: out of memory");
		break;
	case MESSAGE_PORT_ERROR_INVALID_PARAMETER:
		err("MessagePort error: invalid parameter");
		break;
	case MESSAGE_PORT_ERROR_PORT_NOT_FOUND:
		err("MessagePort error: message port not found");
		break;
	case MESSAGE_PORT_ERROR_CERTIFICATE_NOT_MATCH:
		err("MessagePort error: certificate not match");
		break;
	case MESSAGE_PORT_ERROR_MAX_EXCEEDED:
		err("MessagePort error: max exceeded");
		break;
	default:
		err("MessagePort error: unknown error");
		break;
	}
	return result;
}

static void __msg_port_cb(int trusted_local_port_id, const char *remote_app_id,
		const char *remote_port, bool trusted_remote_port, bundle *message, void *data)
{
	debug_enter();
}

static callui_result_e __register_local_msg_port(callui_indicator_h indicator, const char *port_name)
{
	int temp_id = message_port_register_trusted_local_port(port_name, __msg_port_cb, NULL);

	if (temp_id < 0) {
		err("Failed to register local message port");
		indicator->local_port_id = 0;
		return __convert_msg_port_result(temp_id);
	}

	dbg("Message port %s registered with id: %d", port_name, temp_id);
	indicator->local_port_id = temp_id;

	return CALLUI_RESULT_OK;
}

static void __unregister_local_msg_port(callui_indicator_h indicator)
{
	callui_result_e res = __convert_msg_port_result(
			message_port_unregister_trusted_local_port(indicator->local_port_id));
	if (res) {
		err("Failed to unregister local message port [%d]", indicator->local_port_id);
	}
}

callui_indicator_h callui_indicator_create(callui_app_data_t *appdata)
{
	CALLUI_RETURN_NULL_IF_FAIL(appdata);

	callui_indicator_h indicator = calloc(1, sizeof(__callui_indicator_t));
	CALLUI_RETURN_NULL_IF_FAIL(indicator);

	callui_result_e res = __callui_indicator_init(indicator, appdata);
	if (res != CALLUI_RESULT_OK) {
		err("Init indicator failed. res[%d]", res);
		__callui_indicator_deinit(indicator);
		FREE(indicator);
	}
	return indicator;
}

void callui_indicator_destroy(callui_indicator_h indicator)
{
	CALLUI_RETURN_IF_FAIL(indicator);

	__callui_indicator_deinit(indicator);

	free(indicator);
}

static void __refresh_indicator_color(callui_indicator_h indicator)
{
	callui_app_data_t *ad = indicator->ad;

	if (!ad->state_provider) {
		err("State provider is NULL");
		return;
	}

	const callui_call_data_t *incom = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_INCOMING);
	const callui_call_data_t *active = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_ACTIVE);
	const callui_call_data_t *held = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_HELD);

	__callui_indicator_type_e type = CALLUI_INDICATOR_DEFAULT;

	if (active || incom) {
		type = CALLUI_INDICATOR_DURING_CALL;
	} else if (held) {
		type = CALLUI_INDICATOR_CALL_ON_HOLD;
	} else if (!incom && !active && !held) {
		type = CALLUI_INDICATOR_END_CALL;
	}

	if (type != indicator->cur_type) {
		indicator->cur_type = type;
		bundle *msg = __create_color_change_msg(type);
		CALLUI_RETURN_IF_FAIL(msg);
		callui_result_e res = __send_color_change_msg(indicator, msg);
		if (res != CALLUI_RESULT_OK) {
			err("Call __send_color_change_msg() failed. res[%d]", res);
		}
	}
}

void callui_indicator_set_active(callui_indicator_h indicator, bool is_active)
{
	CALLUI_RETURN_IF_FAIL(indicator);

	if (indicator->active == is_active) {
		dbg("Already [%s]", is_active ? "[active]" : "[not active]");
		return;
	}

	indicator->active = is_active;

	if (indicator->active) {
		dbg("Indicator is active. Try to refresh indicator color...");
		__refresh_indicator_color(indicator);
	} else {
		dbg("Indicator is not active. Try to reset to default indicator color...");
		__reset_indicator_color(indicator);
	}
}

void callui_indicator_force_deativate(callui_indicator_h indicator)
{
	CALLUI_RETURN_IF_FAIL(indicator);

	if (indicator->active) {
		indicator->active = false;
	}
	__reset_indicator_color(indicator);
}

static bundle *__create_color_change_msg(__callui_indicator_type_e type)
{
	bundle *message = bundle_create();
	CALLUI_RETURN_NULL_IF_FAIL(message);

	dbg("INDICATOR TYPE TO SET [%d]", type);
	bundle_add_str(message, CALLUI_KEY_BG, CALLUI_KEY_VALUE_BG_CALL);
	bundle_add_str(message, CALLUI_KEY_VALUE_BG_CALL, color_param[type]);

	return message;
}

static callui_result_e __send_color_change_msg(callui_indicator_h indicator, bundle *message)
{
	callui_result_e res = __convert_msg_port_result(message_port_send_trusted_message_with_local_port(CALLUI_INDICATOR_APP,
			CALLUI_REMOTE_PORT_NAME, message, indicator->local_port_id));

	bundle_free(message);
	return res;
}
