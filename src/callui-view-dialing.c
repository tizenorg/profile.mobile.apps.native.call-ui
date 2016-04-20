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

#include <Elementary.h>

#include "callui-view-dialing.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-view-elements.h"
#include "callui-keypad.h"
#include "callui-common.h"
#include "callui-view-caller-info-defines.h"
#include "callui-state-provider.h"

struct _callui_view_dialing {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;
};
typedef struct _callui_view_dialing _callui_view_dialing_t;

static callui_result_e __callui_view_dialing_oncreate(call_view_data_base_t *view_data, void *appdata);
static callui_result_e __callui_view_dialing_onupdate(call_view_data_base_t *view_data);
static callui_result_e __callui_view_dialing_ondestroy(call_view_data_base_t *view_data);

static callui_result_e __create_main_content(callui_view_dialing_h vd);

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static callui_result_e __update_displayed_data(callui_view_dialing_h vd);
static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility);

callui_view_dialing_h _callui_dialing_view_dialing_new()
{
	callui_view_dialing_h dialing_view = calloc(1, sizeof(_callui_view_dialing_t));
	CALLUI_RETURN_NULL_IF_FAIL(dialing_view);

	dialing_view->base_view.create = __callui_view_dialing_oncreate;
	dialing_view->base_view.update = __callui_view_dialing_onupdate;
	dialing_view->base_view.destroy = __callui_view_dialing_ondestroy;

	return dialing_view;
}

static callui_result_e __callui_view_dialing_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_dialing_h vd = (callui_view_dialing_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	callui_result_e res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, CALLUI_RESULT_FAIL);

	_callui_lock_manager_start(ad->lock_handle);

	return __update_displayed_data(vd);
}

static callui_result_e __create_main_content(callui_view_dialing_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME, GRP_VIEW_MAIN_LY);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", vd->base_view.contents);

	vd->caller_info = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_CALLER_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->caller_info, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, "caller_info", vd->caller_info);

	_callui_action_bar_show(ad->action_bar);

	_callui_keypad_clear_input(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, __keypad_show_state_change_cd, vd);

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_end_call_button(vd->base_view.contents, __end_call_btn_click_cb, vd),
			CALLUI_RESULT_ALLOCATION_FAIL);

	return CALLUI_RESULT_OK;
}

static callui_result_e __callui_view_dialing_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	return __update_displayed_data((callui_view_dialing_h)view_data);
}

static callui_result_e __callui_view_dialing_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_dialing_h vd = (callui_view_dialing_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	_callui_action_bar_hide(ad->action_bar);

	_callui_keypad_hide_immediately(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, NULL, NULL);

	DELETE_EVAS_OBJECT(vd->base_view.contents);

	free(vd);

	return CALLUI_RESULT_OK;
}

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_dialing_h vd = (callui_view_dialing_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	const callui_call_data_t *call_state =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);

	if (call_state) {
		callui_result_e res = _callui_manager_end_call(ad->call_manager,
				call_state->call_id, CALLUI_CALL_RELEASE_BY_CALL_HANDLE);
		if (res != CALLUI_RESULT_OK) {
			err("_callui_manager_end_call() failed. res[%d]", res);
		}
	}
}

static callui_result_e __update_displayed_data(callui_view_dialing_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;
	const callui_call_data_t *now_call_data = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_ACTIVE);
	CALLUI_RETURN_VALUE_IF_FAIL(now_call_data, CALLUI_RESULT_FAIL);

	const char *file_path = now_call_data->call_ct_info.caller_id_path;
	const char *call_name = now_call_data->call_ct_info.call_disp_name;
	const char *disp_number = NULL;

	if (strlen(now_call_data->call_disp_num) > 0) {
		disp_number = now_call_data->call_disp_num;
	} else {
		disp_number = now_call_data->call_num;
	}

	if (now_call_data->is_emergency) {
		call_name = "IDS_COM_BODY_EMERGENCY_NUMBER";
		disp_number = "";
	}

	if (strlen(call_name) == 0) {
		_callui_show_caller_info_name(ad, disp_number);
		elm_object_signal_emit(vd->caller_info, "1line", "caller_name");
	} else if (now_call_data->is_emergency) {
		_callui_show_caller_info_name(ad, call_name);
		elm_object_signal_emit(vd->caller_info, "1line", "caller_name");
	} else {
		_callui_show_caller_info_name(ad, call_name);
		_callui_show_caller_info_number(ad, disp_number);
		elm_object_signal_emit(vd->caller_info, "2line", "caller_name");
	}

	_callui_show_caller_info_status(ad, "IDS_CALL_POP_DIALLING");

	if (now_call_data->is_emergency) {
		elm_object_signal_emit(vd->caller_info, "set_emergency_mode", "");
	} else {
		if (strcmp(file_path, "default") != 0) {
			_callui_show_caller_id(vd->caller_info, file_path);
		}
	}

	elm_object_signal_emit(vd->base_view.contents, "SHOW_EFFECT", "ALLBTN");

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility)
{
	callui_view_dialing_h vd = (callui_view_dialing_h)data;

	if (visibility) {
		elm_object_signal_emit(vd->base_view.contents, "SHOW", "KEYPAD_BTN");
	} else {
		elm_object_signal_emit(vd->base_view.contents, "HIDE", "KEYPAD_BTN");
	}
}
