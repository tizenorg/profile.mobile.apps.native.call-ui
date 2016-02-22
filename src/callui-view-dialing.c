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
#include "callui.h"
#include "callui-view-elements.h"
#include "callui-keypad.h"
#include "callui-common.h"

struct _callui_view_dialing {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;
};
typedef struct _callui_view_dialing _callui_view_dialing_t;

static int __callui_view_dialing_oncreate(call_view_data_base_t *view_data, void *appdata);
static int __callui_view_dialing_onupdate(call_view_data_base_t *view_data);
static int __callui_view_dialing_ondestroy(call_view_data_base_t *view_data);

static int __create_main_content(callui_view_dialing_h vd);

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static int __update_displayed_data(callui_view_dialing_h vd);

callui_view_dialing_h _callui_dialing_view_dialing_new()
{
	callui_view_dialing_h dialing_view = calloc(1, sizeof(_callui_view_dialing_t));
	CALLUI_RETURN_NULL_IF_FAIL(dialing_view);

	dialing_view->base_view.onCreate = __callui_view_dialing_oncreate;
	dialing_view->base_view.onUpdate = __callui_view_dialing_onupdate;
	dialing_view->base_view.onDestroy = __callui_view_dialing_ondestroy;

	return dialing_view;
}

static int __callui_view_dialing_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_dialing_h vd = (callui_view_dialing_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	int res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, CALLUI_RESULT_FAIL);

	_callui_lock_manager_start(ad->lock_handle);

	return __update_displayed_data(vd);
}

static int __create_main_content(callui_view_dialing_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME, GRP_MAIN_LY);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", vd->base_view.contents);

	Evas_Object *btn_layout = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_BUTTON_LAYOUT);
	CALLUI_RETURN_VALUE_IF_FAIL(btn_layout, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, "btn_region", btn_layout);

	vd->caller_info = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_CALLER_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->caller_info, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, "caller_info", vd->caller_info);

	int res = _callui_keypad_create_layout(ad);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_end_call_button(vd->base_view.contents, __end_call_btn_click_cb, vd),
			CALLUI_RESULT_ALLOCATION_FAIL);

	return res;
}

static int __callui_view_dialing_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	return __update_displayed_data((callui_view_dialing_h)view_data);
}

static int __callui_view_dialing_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_dialing_h vd = (callui_view_dialing_h)view_data;

	_callui_keypad_delete_layout(vd->base_view.ad);

	DELETE_EVAS_OBJECT(vd->base_view.contents);

	free(vd);

	return CALLUI_RESULT_OK;
}

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_dialing_h vd = (callui_view_dialing_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	if (ad->active) {
		int ret = cm_end_call(ad->cm_handle, ad->active->call_id, CALL_RELEASE_TYPE_BY_CALL_HANDLE);
		if (ret != CM_ERROR_NONE) {
			err("cm_end_call() is failed");
		}
	}
}

static int __update_displayed_data(callui_view_dialing_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;
	call_data_t *now_call_data = ad->active;
	CALLUI_RETURN_VALUE_IF_FAIL(now_call_data, CALLUI_RESULT_FAIL);

	char *file_path = now_call_data->call_ct_info.caller_id_path;
	char *call_name = now_call_data->call_ct_info.call_disp_name;
	char *disp_number = NULL;

	if (strlen(now_call_data->call_disp_num) > 0) {
		disp_number = now_call_data->call_disp_num;
	} else {
		disp_number = now_call_data->call_num;
	}

	if (now_call_data->is_emergency) {
		call_name = _("IDS_COM_BODY_EMERGENCY_NUMBER");
		disp_number = "";
	}

	if (strlen(call_name) == 0) {
		_callui_show_caller_info_name(ad, disp_number);
		elm_object_signal_emit(vd->caller_info, "1line", "caller_name");
	} else if (now_call_data->is_emergency == EINA_TRUE) {
		_callui_show_caller_info_name(ad, call_name);
		elm_object_signal_emit(vd->caller_info, "1line", "caller_name");
	} else {
		_callui_show_caller_info_name(ad, call_name);
		_callui_show_caller_info_number(ad, disp_number);
		elm_object_signal_emit(vd->caller_info, "2line", "caller_name");
	}

	_callui_show_caller_info_status(ad, _("IDS_CALL_POP_DIALLING"));

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_top_first_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_top_second_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_top_third_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_bottom_first_button_disabled(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_bottom_second_button_disabled(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_bottom_third_button_disabled(ad), CALLUI_RESULT_FAIL);

	elm_object_signal_emit(vd->base_view.contents, "SHOW_EFFECT", "ALLBTN");

	if (now_call_data->is_emergency == EINA_TRUE) {
		elm_object_signal_emit(vd->caller_info, "set_emergency_mode", "");
	} else {
		if (strcmp(file_path, "default") != 0) {
			_callui_show_caller_id(vd->caller_info, file_path);
		}
	}

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}
