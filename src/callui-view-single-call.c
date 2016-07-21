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

#include <Elementary.h>
#include <efl_extension.h>

#include "callui-view-single-call.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-view-elements.h"
#include "callui-keypad.h"
#include "callui-common.h"
#include "callui-state-provider.h"
#include "callui-action-bar.h"

struct _call_view_single_call {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;
};
typedef struct _call_view_single_call _call_view_single_call_t;

static callui_result_e __callui_view_single_call_oncreate(call_view_data_base_t *view_data, Evas_Object *parent, void *appdata);
static callui_result_e __callui_view_single_call_onupdate(call_view_data_base_t *view_data);
static callui_result_e __callui_view_single_call_ondestroy(call_view_data_base_t *view_data);

static callui_result_e __create_main_content(call_view_single_call_h vd, Evas_Object *parent);
static callui_result_e __update_displayed_data(call_view_single_call_h vd);

static void __more_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility);
static Eina_Bool __call_duration_timer_cb(void* data);

call_view_single_call_h _callui_view_single_call_new()
{
	call_view_single_call_h single_call_view = calloc(1, sizeof(_call_view_single_call_t));
	CALLUI_RETURN_NULL_IF_FAIL(single_call_view);

	single_call_view->base_view.create = __callui_view_single_call_oncreate;
	single_call_view->base_view.update = __callui_view_single_call_onupdate;
	single_call_view->base_view.destroy = __callui_view_single_call_ondestroy;

	return single_call_view;
}

static callui_result_e __callui_view_single_call_oncreate(call_view_data_base_t *view_data, Evas_Object *parent, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(parent, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	call_view_single_call_h vd = (call_view_single_call_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	callui_result_e res = __create_main_content(vd, parent);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	_callui_lock_manager_start(ad->lock_handle);

	return __update_displayed_data(vd);
}

static callui_result_e __callui_view_single_call_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	return __update_displayed_data((call_view_single_call_h)view_data);
}

static callui_result_e __callui_view_single_call_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	call_view_single_call_h vd = (call_view_single_call_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	_callui_action_bar_hide(ad->action_bar);

	_callui_keypad_hide_immediately(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, NULL, NULL);

	DELETE_ECORE_TIMER(vd->base_view.call_duration_timer);
	free(vd->base_view.call_duration_tm);

	eext_object_event_callback_del(vd->base_view.contents, EEXT_CALLBACK_MORE, __more_btn_click_cb);

	evas_object_del(ad->ctxpopup);

	DELETE_EVAS_OBJECT(vd->base_view.contents);

	free(vd);

	return CALLUI_RESULT_OK;
}

static void __more_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	_callui_load_more_option((callui_app_data_t *)data);
}

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	call_view_single_call_h vd = (call_view_single_call_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	callui_result_e res = _callui_manager_end_call(ad->call_manager,
			0, CALLUI_CALL_RELEASE_ALL);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_end_call() is failed. res[%d]", res);
	}
}

static Eina_Bool __call_duration_timer_cb(void* data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	call_view_single_call_h vd = data;

	struct tm *new_tm = _callui_stp_get_call_duration(vd->base_view.ad->state_provider,
			CALLUI_CALL_DATA_ACTIVE);
	if (!new_tm) {
		vd->base_view.call_duration_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	_callui_common_try_update_call_duration_time(
			vd->base_view.call_duration_tm,
			new_tm,
			_callui_common_set_call_duration_time,
			vd->base_view.contents,
			"call_txt_status");

	free(new_tm);

	return ECORE_CALLBACK_RENEW;
}

static callui_result_e __update_displayed_data(call_view_single_call_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;
	Eina_Bool is_held = EINA_FALSE;

	const callui_call_data_t *call_data = _callui_stp_get_call_data(ad->state_provider,
					CALLUI_CALL_DATA_ACTIVE);
	if (!call_data) {
		call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
		is_held = EINA_TRUE;
	}
	CALLUI_RETURN_VALUE_IF_FAIL(call_data, CALLUI_RESULT_FAIL);

	DELETE_ECORE_TIMER(vd->base_view.call_duration_timer);
	FREE(vd->base_view.call_duration_tm);

	if (is_held) {
		_callui_show_caller_info_status(ad, "IDS_CALL_BODY_ON_HOLD_ABB");
	} else {
		vd->base_view.call_duration_tm = _callui_stp_get_call_duration(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
		CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.call_duration_tm, CALLUI_RESULT_ALLOCATION_FAIL);

		_callui_common_set_call_duration_time(vd->base_view.call_duration_tm, vd->base_view.contents, "call_txt_status");

		vd->base_view.call_duration_timer = ecore_timer_add(0.1, __call_duration_timer_cb, vd);
		CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.call_duration_timer, CALLUI_RESULT_ALLOCATION_FAIL);
	}

	const char *call_name = call_data->call_ct_info.call_disp_name;
	const char *disp_number = NULL;

	if (strlen(call_data->call_disp_num) > 0) {
		disp_number = call_data->call_disp_num;
	} else {
		disp_number = call_data->call_num;
	}
	if (call_data->is_emergency) {
		call_name = _("IDS_COM_BODY_EMERGENCY_NUMBER");
		disp_number = "";
	}

	if (strlen(call_name) == 0) {
		_callui_show_caller_info_name(ad, disp_number);
		elm_object_signal_emit(vd->caller_info, "1line", "caller_name");
	} else if (call_data->is_emergency) {
		_callui_show_caller_info_name(ad, call_name);
		elm_object_signal_emit(vd->caller_info, "1line", "caller_name");
	} else {
		_callui_show_caller_info_name(ad, call_name);
		_callui_show_caller_info_number(ad, disp_number);
		elm_object_signal_emit(vd->caller_info, "2line", "caller_name");
	}

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_show_caller_id(vd->caller_info, call_data), CALLUI_RESULT_FAIL);

	evas_object_show(vd->base_view.contents);

	return CALLUI_RESULT_OK;
}

static callui_result_e __create_main_content(call_view_single_call_h vd, Evas_Object *parent)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(parent, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_VIEW_MAIN_LY);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(parent, "elm.swallow.content", vd->base_view.contents);

	// TODO: replace this into view manager in nearest future
	eext_object_event_callback_add(vd->base_view.contents, EEXT_CALLBACK_MORE, __more_btn_click_cb, ad);

	vd->caller_info = _callui_load_edj(vd->base_view.contents, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_CALLER_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->caller_info, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, "swallow.caller_info", vd->caller_info);

	_callui_action_bar_show(ad->action_bar);

	_callui_keypad_clear_input(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, __keypad_show_state_change_cd, vd);

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_end_call_button(vd->base_view.contents, __end_call_btn_click_cb, vd),
			CALLUI_RESULT_ALLOCATION_FAIL);

	return CALLUI_RESULT_OK;
}

static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility)
{
	call_view_single_call_h vd = (call_view_single_call_h)data;

	if (visibility) {
		elm_object_signal_emit(vd->base_view.contents, "hide_caller_info", "view_main_ly");
	} else {
		elm_object_signal_emit(vd->base_view.contents, "show_caller_info", "view_main_ly");
	}
}
