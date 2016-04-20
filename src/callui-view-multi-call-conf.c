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
#include <efl_extension.h>

#include "callui-view-multi-call-conf.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-common.h"
#include "callui-view-elements.h"
#include "callui-view-layout.h"
#include "callui-keypad.h"
#include "callui-state-provider.h"

#define CALLUI_BUF_STATUS_SIZE 129

struct _callui_view_mc_conf {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;
	callui_keypad_h keypad;

} callui_view_mc_list;

typedef struct _callui_view_mc_conf _callui_view_mc_conf_t;

static callui_result_e __callui_view_multi_call_conf_oncreate(call_view_data_base_t *view_data, void *appdata);
static callui_result_e __callui_view_multi_call_conf_onupdate(call_view_data_base_t *view_data);
static callui_result_e __callui_view_multi_call_conf_ondestroy(call_view_data_base_t *view_data);

static callui_result_e __create_main_content(callui_view_mc_conf_h vd);
static callui_result_e __update_displayed_data(callui_view_mc_conf_h vd);

static void __manage_calls_btn_clicked_cb(void *data, Evas_Object *o, const char *emission, const char *source);
static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __more_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility);
static Eina_Bool __call_duration_timer_cb(void* data);

callui_view_mc_conf_h _callui_view_multi_call_conf_new()
{
	callui_view_mc_conf_h mc_list_conf = calloc(1, sizeof(_callui_view_mc_conf_t));
	CALLUI_RETURN_NULL_IF_FAIL(mc_list_conf);

	mc_list_conf->base_view.create = __callui_view_multi_call_conf_oncreate;
	mc_list_conf->base_view.update = __callui_view_multi_call_conf_onupdate;
	mc_list_conf->base_view.destroy = __callui_view_multi_call_conf_ondestroy;

	return mc_list_conf;
}

static callui_result_e __create_main_content(callui_view_mc_conf_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	/* Main Layout */
	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME, GRP_VIEW_MAIN_LY);
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
	elm_object_part_content_set(vd->caller_info, "manage_calls_btn", manage_calls_ly);
	edje_object_signal_callback_add(_EDJ(manage_calls_ly), "mouse,clicked,1", "btn", __manage_calls_btn_clicked_cb, vd);

	_callui_action_bar_show(ad->action_bar);

	_callui_keypad_clear_input(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, __keypad_show_state_change_cd, vd);

	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_end_call_button(vd->base_view.contents, __end_call_btn_click_cb, vd),
			CALLUI_RESULT_ALLOCATION_FAIL);

	return CALLUI_RESULT_OK;
}

static callui_result_e __callui_view_multi_call_conf_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)view_data;
	view_data->ad = (callui_app_data_t *)appdata;

	callui_result_e res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return __update_displayed_data(vd);
}

static callui_result_e __update_nonetranslatable_elements(callui_view_mc_conf_h vd)
{
	char buf[CALLUI_BUF_MEMBER_SIZE] = { 0 };
	callui_app_data_t *ad = vd->base_view.ad;

	const callui_call_data_t *call_data =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	if (!call_data) {
		call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
	}
	CALLUI_RETURN_VALUE_IF_FAIL(call_data, CALLUI_RESULT_FAIL);

	char *status = _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB");
	snprintf(buf, CALLUI_BUF_MEMBER_SIZE, status, call_data->conf_member_count);
	_callui_show_caller_info_number(ad, buf);

	return CALLUI_RESULT_OK;
}

static callui_result_e __callui_view_multi_call_conf_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)view_data;
	callui_result_e res = CALLUI_RESULT_FAIL;
	if (vd->base_view.update_flags & CALLUI_UF_DATA_REFRESH) {
		res = __update_displayed_data(vd);
	} else if (vd->base_view.update_flags & CALLUI_UF_LANG_CHANGE) {
		res = __update_nonetranslatable_elements(vd);
	}
	return res;
}

static Eina_Bool __call_duration_timer_cb(void* data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_mc_conf_h vd = data;

	struct tm *new_tm = _callui_stp_get_call_duration(vd->base_view.ad->state_provider,
			CALLUI_CALL_DATA_ACTIVE);
	if (!new_tm) {
		vd->base_view.call_duration_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	_callui_common_try_update_call_duration_time(vd->base_view.call_duration_tm,
			new_tm,
			_callui_common_set_call_duration_time,
			vd->base_view.contents,
			"call_txt_status");

	free(new_tm);

	return ECORE_CALLBACK_RENEW;
}

static callui_result_e __update_displayed_data(callui_view_mc_conf_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	char buf[CALLUI_BUF_MEMBER_SIZE] = { 0 };

	Eina_Bool is_held = EINA_FALSE;

	const callui_call_data_t *call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	if (!call_data) {
		call_data = _callui_stp_get_call_data(ad->state_provider,
				CALLUI_CALL_DATA_HELD);
		is_held = EINA_TRUE;
	}
	CALLUI_RETURN_VALUE_IF_FAIL(call_data, CALLUI_RESULT_FAIL);

	DELETE_ECORE_TIMER(vd->base_view.call_duration_timer);
	FREE(vd->base_view.call_duration_tm);

	if (is_held) {
		elm_object_signal_emit(vd->caller_info, "hide_manage_calls_btn", "");
		_callui_show_caller_info_status(ad, "IDS_CALL_BODY_ON_HOLD_ABB");

	} else {
		elm_object_signal_emit(vd->caller_info, "show_manage_calls_btn", "");

		vd->base_view.call_duration_tm = _callui_stp_get_call_duration(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
		CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.call_duration_tm, CALLUI_RESULT_ALLOCATION_FAIL);

		_callui_common_set_call_duration_time(vd->base_view.call_duration_tm, vd->base_view.contents, "call_txt_status");

		vd->base_view.call_duration_timer = ecore_timer_add(0.1, __call_duration_timer_cb, vd);
		CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.call_duration_timer, CALLUI_RESULT_ALLOCATION_FAIL);
	}

	elm_object_signal_emit(vd->caller_info, "set_conference_mode", "");
	_callui_show_caller_info_name(ad, "IDS_CALL_BODY_CONFERENCE");

	char *status = _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB");
	snprintf(buf, CALLUI_BUF_MEMBER_SIZE, status, call_data->conf_member_count);
	_callui_show_caller_info_number(ad, buf);

	elm_object_signal_emit(vd->base_view.contents, "SHOW_NO_EFFECT", "ALLBTN");

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static callui_result_e __callui_view_multi_call_conf_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)view_data;
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

static void __manage_calls_btn_clicked_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	_callui_vm_change_view(ad->view_manager, CALLUI_VIEW_MULTICALL_LIST);
	return;
}

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_view_mc_conf_h vd = (callui_view_mc_conf_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	callui_result_e res = _callui_manager_end_call(ad->call_manager,
			0, CALLUI_CALL_RELEASE_ALL);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_end_call() failed. res[%d]", res);
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
