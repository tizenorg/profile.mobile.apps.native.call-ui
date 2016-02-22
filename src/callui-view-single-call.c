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

#include "callui.h"
#include "callui-view-single-call.h"
#include "callui-view-elements.h"
#include "callui-keypad.h"
#include "callui-common.h"

#define	 VIEW_SINGLE_CALL_STATUS_TXT_LEN 129

struct _call_view_single_call {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;
};
typedef struct _call_view_single_call _call_view_single_call_t;

static int __callui_view_single_call_oncreate(call_view_data_base_t *view_data, void *appdata);
static int __callui_view_single_call_onupdate(call_view_data_base_t *view_data);
static int __callui_view_single_call_ondestroy(call_view_data_base_t *view_data);

static int __create_main_content(call_view_single_call_h vd);
static int __update_displayed_data(call_view_single_call_h vd);

static void __more_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);

call_view_single_call_h _callui_view_single_call_new()
{
	call_view_single_call_h single_call_view = calloc(1, sizeof(_call_view_single_call_t));
	CALLUI_RETURN_NULL_IF_FAIL(single_call_view);

	single_call_view->base_view.onCreate = __callui_view_single_call_oncreate;
	single_call_view->base_view.onUpdate = __callui_view_single_call_onupdate;
	single_call_view->base_view.onDestroy = __callui_view_single_call_ondestroy;

	return single_call_view;
}

static int __callui_view_single_call_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	call_view_single_call_h vd = (call_view_single_call_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	int res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	_callui_lock_manager_start(ad->lock_handle);

	return __update_displayed_data(vd);
}

static int __callui_view_single_call_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	return __update_displayed_data((call_view_single_call_h)view_data);
}

static int __callui_view_single_call_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	call_view_single_call_h vd = (call_view_single_call_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	if (ad->ctxpopup) {
		elm_ctxpopup_dismiss(ad->ctxpopup);
		ad->ctxpopup = NULL;
	}

	/*Delete keypad layout*/
	_callui_keypad_delete_layout(ad);

	if (vd->base_view.contents) {
		eext_object_event_callback_del(vd->base_view.contents, EEXT_CALLBACK_MORE, __more_btn_click_cb);
		_callui_common_reset_main_ly_text_fields(vd->base_view.contents);
	}
	elm_object_signal_emit(vd->base_view.contents, "HIDE_BTN_LY", "ALLBTN");

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

	int ret = cm_end_call(ad->cm_handle, 0, CALL_RELEASE_TYPE_ALL_CALLS);
	if (ret != CM_ERROR_NONE) {
		err("cm_end_call() is failed");
	}
}

static int __update_displayed_data(call_view_single_call_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;
	call_data_t *call_data = NULL;
	char *file_path = NULL;
	Eina_Bool is_held = EINA_FALSE;
	char status_txt[VIEW_SINGLE_CALL_STATUS_TXT_LEN] = { 0 };

	if (ad->active) {
		call_data = ad->active;
		is_held = EINA_FALSE;
	} else {
		call_data = ad->held;
		is_held = EINA_TRUE;
	}
	CALLUI_RETURN_VALUE_IF_FAIL(call_data, CALLUI_RESULT_FAIL);

	file_path = call_data->call_ct_info.caller_id_path;

	sec_dbg("file_path: %s", file_path);

	if (call_data->is_emergency == EINA_TRUE) {
		elm_object_signal_emit(vd->caller_info, "set_emergency_mode", "");
	} else {
		if (strcmp(file_path, "default") != 0) {
			_callui_show_caller_id(vd->caller_info, file_path);
		}
	}

	if (is_held) {
		dbg("====== HOLD ======");
		snprintf(status_txt, sizeof(status_txt), _("IDS_CALL_BODY_ON_HOLD_ABB"));
		_callui_show_caller_info_status(ad, status_txt);
	} else {
		dbg("====== UNHOLD ======");
		_callui_common_update_call_duration(call_data->start_time);
	}

	char *call_name = call_data->call_ct_info.call_disp_name;
	char *disp_number = NULL;

	if (strlen(call_data->call_disp_num) > 0) {
		disp_number = call_data->call_disp_num;
	} else {
		disp_number = call_data->call_num;
	}

	if (call_data->is_emergency == EINA_TRUE) {
		call_name = _("IDS_COM_BODY_EMERGENCY_NUMBER");
		disp_number = "";
	}

	if (strlen(call_name) == 0) {
		_callui_show_caller_info_name(ad, disp_number);
		elm_object_signal_emit(vd->caller_info, "1line", "caller_name");
	} else if (call_data->is_emergency == EINA_TRUE) {
		_callui_show_caller_info_name(ad, call_name);
		elm_object_signal_emit(vd->caller_info, "1line", "caller_name");
	} else {
		_callui_show_caller_info_name(ad, call_name);
		_callui_show_caller_info_number(ad, disp_number);
		elm_object_signal_emit(vd->caller_info, "2line", "caller_name");
	}

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_top_third_button(ad), CALLUI_RESULT_FAIL);
	if (is_held) {
		CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_top_second_button_disabled(ad), CALLUI_RESULT_FAIL);
		CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_bottom_second_button_disabled(ad), CALLUI_RESULT_FAIL);

	} else {
		CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_top_second_button(ad), CALLUI_RESULT_FAIL);
		CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_bottom_second_button(ad), CALLUI_RESULT_FAIL);
	}
	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_bottom_first_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_top_first_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_bottom_third_button(ad), CALLUI_RESULT_FAIL);

	elm_object_signal_emit(vd->base_view.contents, "SHOW_EFFECT", "ALLBTN");

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static int __create_main_content(call_view_single_call_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;
	CALLUI_RETURN_VALUE_IF_FAIL(ad->main_ly, CALLUI_RESULT_FAIL);

	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME, GRP_MAIN_LY);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", vd->base_view.contents);

	// TODO: replace this into view manager in nearest future
	eext_object_event_callback_add(vd->base_view.contents, EEXT_CALLBACK_MORE, __more_btn_click_cb, ad);

	vd->caller_info = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_CALLER_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->caller_info, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, "caller_info", vd->caller_info);

	Evas_Object *btn_region = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_BUTTON_LAYOUT);
	CALLUI_RETURN_VALUE_IF_FAIL(btn_region, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, "btn_region", btn_region);

	/*create keypad layout*/
	int res = _callui_keypad_create_layout(ad);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_end_call_button(vd->base_view.contents, __end_call_btn_click_cb, vd),
			CALLUI_RESULT_ALLOCATION_FAIL);

	return res;
}
