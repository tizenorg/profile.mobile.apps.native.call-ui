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
#include "callui-view-manager.h"
#include "callui-view-elements.h"
#include "callui-keypad.h"
#include "callui-view-quickpanel.h"
#include "callui-common.h"

struct _callui_view_dialing {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;
	Evas_Object *btn_ly;
};
typedef struct _callui_view_dialing _callui_view_dialing_t;

static int __callui_view_dialing_oncreate(call_view_data_base_t *view_data, void *appdata);
static int __callui_view_dialing_ondestroy(call_view_data_base_t *view_data);
static int __callui_view_dialing_show(callui_view_dialing_h vd);

static Evas_Object *__callui_view_dialing_create_contents(callui_app_data_t *appdata, char *grpname);
static void __callui_view_dialing_end_btn_cb(void *data, Evas_Object *obj, void *event_info);
static int __callui_view_dialing_draw_screen(callui_view_dialing_h vd);

callui_view_dialing_h _callui_dialing_view_dialing_new()
{
	callui_view_dialing_h dialing_view = calloc(1, sizeof(_callui_view_dialing_t));
	CALLUI_RETURN_NULL_IF_FAIL(dialing_view);

	dialing_view->base_view.onCreate = __callui_view_dialing_oncreate;
	dialing_view->base_view.onUpdate = NULL;
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

	vd->base_view.contents = __callui_view_dialing_create_contents(ad, GRP_MAIN_LY);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", vd->base_view.contents);

	vd->btn_ly = __callui_view_dialing_create_contents(ad, GRP_BUTTON_LAYOUT);
	elm_object_part_content_set(vd->base_view.contents, "btn_region", vd->btn_ly);

	vd->caller_info = __callui_view_dialing_create_contents(ad, GRP_CALLER_INFO);
	elm_object_part_content_set(vd->base_view.contents, "caller_info", vd->caller_info);

	/*create keypad layout*/
	_callui_keypad_create_layout(ad);

	int res = __callui_view_dialing_show(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, CALLUI_RESULT_FAIL);

	_callui_lock_manager_start(ad->lock_handle);

	return CALLUI_RESULT_OK;
}

static int __callui_view_dialing_show(callui_view_dialing_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	int res = __callui_view_dialing_draw_screen(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static int __callui_view_dialing_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_dialing_h vd = (callui_view_dialing_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	/*Delete keypad layout */
	_callui_keypad_delete_layout(ad);

	elm_object_signal_emit(vd->base_view.contents, "HIDE_BTN_LY", "ALLBTN");
	_callui_common_reset_main_ly_text_fields(vd->base_view.contents);

	return CALLUI_RESULT_OK;
}

static Evas_Object *__callui_view_dialing_create_contents(callui_app_data_t *appdata, char *grpname)
{
	return _callui_load_edj(appdata->main_ly, EDJ_NAME, grpname);
}

static void __callui_view_dialing_end_btn_cb(void *data, Evas_Object *obj, void *event_info)
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

static int __callui_view_dialing_draw_screen(callui_view_dialing_h vd)
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

	_callui_create_top_first_button(ad);
	_callui_create_top_second_button(ad);
	_callui_create_top_third_button(ad);
	_callui_create_bottom_first_button_disabled(ad);
	_callui_create_bottom_second_button_disabled(ad);
	_callui_create_bottom_third_button_disabled(ad);

	elm_object_signal_emit(vd->base_view.contents, "SHOW_EFFECT", "ALLBTN");

	_callui_create_end_call_button(vd->base_view.contents, __callui_view_dialing_end_btn_cb, vd);

	if (now_call_data->is_emergency == EINA_TRUE) {
		elm_object_signal_emit(vd->caller_info, "set_emergency_mode", "");
	} else {
		if (strcmp(file_path, "default") != 0) {
			_callui_show_caller_id(vd->caller_info, file_path);
		}
	}

	evas_object_show(vd->base_view.contents);

	return CALLUI_RESULT_OK;
}
