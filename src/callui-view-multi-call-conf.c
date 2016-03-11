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

#include "callui-view-multi-call-conf.h"
#include "callui.h"
#include "callui-common.h"
#include "callui-view-elements.h"
#include "callui-view-layout.h"
#include "callui-keypad.h"

#define CALLUI_BUF_MEMBER_SIZE 512
#define CALLUI_BUF_STATUS_SIZE 129

struct _callui_view_mc_conf {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;
	callui_keypad_h keypad;

} callui_view_mc_list;

typedef struct _callui_view_mc_conf _callui_view_mc_conf_t;

static int __callui_view_multi_call_conf_oncreate(call_view_data_base_t *view_data, void *appdata);
static int __callui_view_multi_call_conf_onupdate(call_view_data_base_t *view_data);
static int __callui_view_multi_call_conf_ondestroy(call_view_data_base_t *view_data);

static int __create_main_content(callui_view_mc_conf_h vd);
static int __update_displayed_data(callui_view_mc_conf_h vd);

static void __manage_calls_btn_clicked_cb(void *data, Evas_Object *o, const char *emission, const char *source);
static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __more_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility);

callui_view_mc_conf_h _callui_view_multi_call_conf_new()
{
	callui_view_mc_conf_h mc_list_conf = calloc(1, sizeof(_callui_view_mc_conf_t));
	CALLUI_RETURN_NULL_IF_FAIL(mc_list_conf);

	mc_list_conf->base_view.onCreate = __callui_view_multi_call_conf_oncreate;
	mc_list_conf->base_view.onUpdate = __callui_view_multi_call_conf_onupdate;
	mc_list_conf->base_view.onDestroy = __callui_view_multi_call_conf_ondestroy;

	return mc_list_conf;
}

static int __create_main_content(callui_view_mc_conf_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	/* Main Layout */
	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME, GRP_MAIN_LY);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", vd->base_view.contents);

	// TODO: replace this into view manager in nearest future
	eext_object_event_callback_add(vd->base_view.contents, EEXT_CALLBACK_MORE, __more_btn_click_cb, ad);

	/* Caller info layout */
	vd->caller_info = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_CALLER_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->caller_info, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, "caller_info", vd->caller_info);

	/* Manage button Layout */
	Evas_Object *manage_calls_ly = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_MANAGE_CALLS);
	CALLUI_RETURN_VALUE_IF_FAIL(manage_calls_ly, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->caller_info, "manage_calls_icon_swallow", manage_calls_ly);
	edje_object_signal_callback_add(_EDJ(manage_calls_ly), "mouse,clicked,1", "btn", __manage_calls_btn_clicked_cb, vd);

	Evas_Object *btn_layout = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_BUTTON_LAYOUT);
	CALLUI_RETURN_VALUE_IF_FAIL(btn_layout, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, "btn_region", btn_layout);

	_callui_keypad_clear_input(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, __keypad_show_state_change_cd, vd);

	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_end_call_button(vd->base_view.contents, __end_call_btn_click_cb, vd),
			CALLUI_RESULT_ALLOCATION_FAIL);

	return CALLUI_RESULT_OK;
}

static int __callui_view_multi_call_conf_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)view_data;
	view_data->ad = (callui_app_data_t *)appdata;

	int res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return __update_displayed_data(vd);
}

static int __callui_view_multi_call_conf_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)view_data;

	return __update_displayed_data(vd);
}

static int __update_displayed_data(callui_view_mc_conf_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	char buf[CALLUI_BUF_MEMBER_SIZE] = { 0 };
	char status_txt[CALLUI_BUF_STATUS_SIZE] = { 0 };
	call_data_t *call_data = NULL;
	Eina_Bool is_held;

	if (ad->active) {
		call_data = ad->active;
		is_held = EINA_FALSE;
	} else if (ad->held) {
		call_data = ad->held;
		is_held = EINA_TRUE;
	}
	CALLUI_RETURN_VALUE_IF_FAIL(call_data, CALLUI_RESULT_FAIL);

	if (is_held) {
		snprintf(status_txt, sizeof(status_txt), _("IDS_CALL_BODY_ON_HOLD_ABB"));
		_callui_show_caller_info_status(ad, status_txt);
		elm_object_signal_emit(vd->caller_info, "set-hold-state", "call-screen");
	} else {
		elm_object_signal_emit(vd->caller_info, "set-unhold-state", "call-screen");
	}

	elm_object_signal_emit(vd->caller_info, "set_conference_mode", "");
	_callui_show_caller_info_name(ad, _("IDS_CALL_BODY_CONFERENCE"));
	char *status = _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB");
	snprintf(buf, CALLUI_BUF_MEMBER_SIZE, status, call_data->member_count);
	_callui_show_caller_info_number(ad, buf);

	if (is_held) {
		CALLUI_RETURN_VALUE_IF_FAIL(
				_callui_create_top_second_button_disabled(ad), CALLUI_RESULT_FAIL);
		CALLUI_RETURN_VALUE_IF_FAIL(
				_callui_create_bottom_second_button_disabled(ad), CALLUI_RESULT_FAIL);
	} else {
		if (_callui_keypad_get_show_status(vd->keypad)) {
			_callui_keypad_hide(vd->keypad);
		}
		CALLUI_RETURN_VALUE_IF_FAIL(
				_callui_create_top_second_button(ad), CALLUI_RESULT_FAIL);
		CALLUI_RETURN_VALUE_IF_FAIL(
				_callui_create_bottom_second_button(ad), CALLUI_RESULT_FAIL);
	}
	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_top_third_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_bottom_first_button(ad), CALLUI_RESULT_FAIL);

	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_top_first_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_bottom_third_button(ad), CALLUI_RESULT_FAIL);

	elm_object_signal_emit(vd->base_view.contents, "SHOW_NO_EFFECT", "ALLBTN");

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static int __callui_view_multi_call_conf_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	if (ad->ctxpopup) {
		elm_ctxpopup_dismiss(ad->ctxpopup);
		ad->ctxpopup = NULL;
	}

	_callui_keypad_hide_immediately(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, NULL, NULL);

	eext_object_event_callback_del(vd->base_view.contents, EEXT_CALLBACK_MORE, __more_btn_click_cb);

	DELETE_EVAS_OBJECT(vd->base_view.contents);

	free(vd);

	return CALLUI_RESULT_OK;
}

static void __manage_calls_btn_clicked_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	_callui_vm_change_view(ad->view_manager_handle, VIEW_TYPE_MULTICALL_LIST);
	return;
}

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	int ret = cm_end_call(ad->cm_handle, 0, CALL_RELEASE_TYPE_ALL_CALLS);
	if (ret != CM_ERROR_NONE) {
		err("cm_end_call() is failed");
	}
}

static void __more_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	_callui_load_more_option(data);
}

static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility)
{
	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)data;

	if (visibility) {
		elm_object_signal_emit(vd->base_view.contents, "SHOW", "KEYPAD_BTN");
	} else {
		elm_object_signal_emit(vd->base_view.contents, "HIDE", "KEYPAD_BTN");
	}
}