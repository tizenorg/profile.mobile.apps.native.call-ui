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

#include "callui-view-multi-call-split.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-keypad.h"
#include "callui-common.h"
#include "callui-view-elements.h"
#include "callui-state-provider.h"

#define CALLUI_GROUP_SPLIT					"multi_call_split"
#define CALLUI_GROUP_MERGE_BTN				"merge_btn"
#define CALLUI_GROUP_SWAP_BTN				"swap_btn"
#define CALLUI_GROUP_ACTIVE_INFO			"split_active_info"
#define CALLUI_GROUP_HOLD_INFO				"split_hold_info"
#define CALLUI_GROUP_CALLERS_INFO			"split_callers_info"

#define CALLUI_PART_TEXT_MERGE_SWAP_BTN		"text"
#define CALLUI_PART_TEXT_STATUS				"text.status"
#define CALLUI_PART_TEXT_MAIN				"text.main"
#define CALLUI_PART_TEXT_SUB				"text.sub"
#define CALLUI_PART_TEXT_CALL_DURATION		"call.duration"
#define CALLUI_PART_SWALLOW_CALLER_ID		"swallow.caller_id"
#define CALLUI_PART_SWALLOW_HOLD_INFO		"swallow.hold_info"
#define CALLUI_PART_SWALLOW_MERGE			"swallow.merge"
#define CALLUI_PART_SWALLOW_SWAP			"swallow.swap"
#define CALLUI_PART_SWALLOW_ACTIVE_INFO		"swallow.active_info"
#define CALLUI_PART_SWALLOW_END_BTN			"swallow.end_btn"
#define CALLUI_PART_SWALLOW_CALL_INFO		"swallow.call_info"

#define CALLUI_BUF_SIZE 256

struct _callui_view_mc_split {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;

	Evas_Object *hold_layout;
	Evas_Object *active_layout;
};
typedef struct _callui_view_mc_split _callui_view_mc_split_t;

static callui_result_e _callui_view_multi_call_split_oncreate(call_view_data_base_t *view_data, Evas_Object *parent, void *appdata);
static callui_result_e _callui_view_multi_call_split_onupdate(call_view_data_base_t *view_data);
static callui_result_e _callui_view_multi_call_split_ondestroy(call_view_data_base_t *view_data);

static callui_result_e __update_nonetranslatable_elements(callui_view_mc_split_h vd);

static callui_result_e __create_main_content(callui_view_mc_split_h vd, Evas_Object *parent);
static callui_result_e __update_displayed_data(callui_view_mc_split_h vd);

static Evas_Object *__create_merge_swap_btn(Evas_Object *parent, const char *name, const char *text);

static void __update_hold_active_layout(Evas_Object *layout, const callui_call_data_t *call_data);
static void __fill_one_contact_layout(Evas_Object *parent, const callui_call_data_t *call_data);
static void __fill_conference_layout(Evas_Object *parent, const callui_call_data_t *call_data);
static void __set_hold_info(Evas_Object *parent, Evas_Object *content);
static void __set_active_info(Evas_Object *parent, Evas_Object *content, callui_app_data_t *ad);

static callui_result_e __create_merge_swap_btns(Evas_Object *parent, callui_app_data_t *ad);

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __mng_callers_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src);
static void __merge_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src);
static void __swap_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src);
static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility);
static Eina_Bool __call_duration_timer_cb(void* data);
static callui_result_e __init_call_duration_timer(callui_view_mc_split_h vd);
static void __update_call_duration_time(callui_view_mc_split_h vd, struct tm *comp_time);

callui_view_mc_split_h _callui_view_multi_call_split_new()
{
	callui_view_mc_split_h mc_split_view = calloc(1, sizeof(_callui_view_mc_split_t));
	CALLUI_RETURN_NULL_IF_FAIL(mc_split_view);

	mc_split_view->base_view.create = _callui_view_multi_call_split_oncreate;
	mc_split_view->base_view.update = _callui_view_multi_call_split_onupdate;
	mc_split_view->base_view.destroy = _callui_view_multi_call_split_ondestroy;

	return mc_split_view;
}

static callui_result_e __update_nonetranslatable_elements(callui_view_mc_split_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	const callui_call_data_t *active = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	CALLUI_RETURN_VALUE_IF_FAIL(active, CALLUI_RESULT_FAIL);

	const callui_call_data_t *held = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
	CALLUI_RETURN_VALUE_IF_FAIL(held, CALLUI_RESULT_FAIL);

	char buff[CALLUI_BUFF_SIZE_LAR] = { 0 };
	const char *fmt = _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB");

	if (held->conf_member_count > 1) {
		snprintf(buff, CALLUI_BUFF_SIZE_LAR, fmt, held->conf_member_count);
		elm_object_part_text_set(vd->hold_layout, CALLUI_PART_TEXT_SUB, buff);
	}

	if (active->conf_member_count > 1) {
		snprintf(buff, CALLUI_BUFF_SIZE_LAR, fmt, active->conf_member_count);
		elm_object_part_text_set(vd->active_layout, CALLUI_PART_TEXT_SUB, buff);
	}

	return CALLUI_RESULT_OK;
}

static callui_result_e _callui_view_multi_call_split_oncreate(call_view_data_base_t *view_data, Evas_Object *parent, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(parent, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_split_h vd = (callui_view_mc_split_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	callui_result_e res = __create_main_content(vd, parent);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	_callui_lock_manager_start(ad->lock_handle);

	return __update_displayed_data(vd);
}

static callui_result_e _callui_view_multi_call_split_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_split_h vd = (callui_view_mc_split_h)view_data;

	_callui_lock_manager_start(vd->base_view.ad->lock_handle);

	callui_result_e res = CALLUI_RESULT_FAIL;
	if (vd->base_view.update_flags & CALLUI_UF_DATA_REFRESH) {
		res = __update_displayed_data(vd);
	} else if (vd->base_view.update_flags & CALLUI_UF_LANG_CHANGE) {
		res = __update_nonetranslatable_elements(vd);
	}
	return res;
}

static callui_result_e _callui_view_multi_call_split_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_split_h vd = (callui_view_mc_split_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	_callui_action_bar_hide(ad->action_bar);

	_callui_keypad_hide_immediately(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, NULL, NULL);

	DELETE_ECORE_TIMER(vd->base_view.call_duration_timer);

	free(vd->base_view.call_duration_tm);

	DELETE_EVAS_OBJECT(vd->base_view.contents);

	free(vd);

	return CALLUI_RESULT_OK;
}


static callui_result_e __create_main_content(callui_view_mc_split_h vd, Evas_Object *parent)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(parent, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_SPLIT);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(parent, "elm.swallow.content", vd->base_view.contents);

	vd->caller_info = _callui_load_edj(vd->base_view.contents, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_CALLERS_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->caller_info, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, CALLUI_PART_SWALLOW_CALL_INFO, vd->caller_info);

	vd->hold_layout = _callui_load_edj(vd->caller_info, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_HOLD_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->hold_layout, CALLUI_RESULT_ALLOCATION_FAIL);

	vd->active_layout = _callui_load_edj(vd->caller_info, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_ACTIVE_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->active_layout, CALLUI_RESULT_ALLOCATION_FAIL);

	callui_result_e res = __create_merge_swap_btns(vd->caller_info, ad);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	_callui_action_bar_show(ad->action_bar);

	_callui_keypad_clear_input(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, __keypad_show_state_change_cd, vd);

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_end_call_button(vd->base_view.contents,
			__end_call_btn_click_cb, vd), CALLUI_RESULT_ALLOCATION_FAIL);

	return res;
}

static Evas_Object *__create_merge_swap_btn(Evas_Object *parent, const char *name, const char *text)
{
	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, CALLUI_CALL_EDJ_PATH, name);

	elm_object_translatable_part_text_set(layout, CALLUI_PART_TEXT_MERGE_SWAP_BTN, text);

	return layout;
}

static void __update_hold_active_layout(Evas_Object *layout, const callui_call_data_t *call_data)
{
	if (call_data->conf_member_count == 1) {
		__fill_one_contact_layout(layout, call_data);
	} else {
		__fill_conference_layout(layout, call_data);
	}

	_callui_show_caller_id(layout, call_data);
}

static void __fill_one_contact_layout(Evas_Object *parent, const callui_call_data_t *call_data)
{
	const char *main_text = call_data->call_ct_info.call_disp_name;
	const char *sub_text = call_data->call_num;

	if (call_data->is_emergency) {
		elm_object_translatable_part_text_set(parent, CALLUI_PART_TEXT_MAIN, "IDS_COM_BODY_EMERGENCY_NUMBER");
		elm_object_part_text_set(parent, CALLUI_PART_TEXT_SUB, "");
	} else if (!STRING_EMPTY(main_text)) {
		elm_object_part_text_set(parent, CALLUI_PART_TEXT_MAIN, main_text);
		elm_object_part_text_set(parent, CALLUI_PART_TEXT_SUB, sub_text);
	} else if (!STRING_EMPTY(sub_text)) {
		elm_object_part_text_set(parent, CALLUI_PART_TEXT_MAIN, sub_text);
		elm_object_part_text_set(parent, CALLUI_PART_TEXT_SUB, "");
	} else {
		elm_object_translatable_part_text_set(parent, CALLUI_PART_TEXT_MAIN, "IDS_CALL_BODY_UNKNOWN");
		elm_object_part_text_set(parent, CALLUI_PART_TEXT_SUB, "");
	}
}

static void __fill_conference_layout(Evas_Object *parent, const callui_call_data_t *call_data)
{
	elm_object_translatable_part_text_set(parent, CALLUI_PART_TEXT_MAIN, "IDS_CALL_BODY_CONFERENCE");

	char buffer[CALLUI_BUF_SIZE] = { 0 };
	const char *fmt = _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB");
	snprintf(buffer, CALLUI_BUF_SIZE, fmt, call_data->conf_member_count);
	elm_object_part_text_set(parent, CALLUI_PART_TEXT_SUB, buffer);
}

static void __set_hold_info(Evas_Object *parent, Evas_Object *content)
{
	elm_object_translatable_part_text_set(content,
			CALLUI_PART_TEXT_STATUS, "IDS_CALL_BODY_ON_HOLD_ABB");

	elm_object_part_content_set(parent, CALLUI_PART_SWALLOW_HOLD_INFO, content);
}

static void __set_active_info(Evas_Object *parent, Evas_Object *content, callui_app_data_t *ad)
{
	elm_object_translatable_part_text_set(content,
			CALLUI_PART_TEXT_STATUS, "IDS_CALL_BODY_CONNECTED_M_STATUS_ABB");

	elm_object_part_content_set(parent, CALLUI_PART_SWALLOW_ACTIVE_INFO, content);

	const callui_call_data_t *active = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_ACTIVE);

	if (active) {
		if (active->conf_member_count > 1) {
			elm_object_signal_emit(content, "show_manage_calls_btn", "");
			elm_object_signal_callback_add(content, "mouse,up,*", "arrow", __mng_callers_btn_click_cb, ad);
		} else {
			elm_object_signal_emit(content, "hide_manage_calls_btn", "");
			elm_object_signal_callback_del(content, "mouse,up,*", "arrow", __mng_callers_btn_click_cb);
		}
	}
}

static callui_result_e __create_merge_swap_btns(Evas_Object *parent, callui_app_data_t *ad)
{
	Evas_Object *merge = __create_merge_swap_btn(parent, CALLUI_GROUP_MERGE_BTN, "IDS_CALL_BODY_MERGE_T_CALL");
	CALLUI_RETURN_VALUE_IF_FAIL(merge, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(parent, CALLUI_PART_SWALLOW_MERGE, merge);
	elm_object_signal_callback_add(merge, "mouse,clicked,*", "*", __merge_btn_click_cb, ad);

	Evas_Object *swap = __create_merge_swap_btn(parent, CALLUI_GROUP_SWAP_BTN, "IDS_CALL_SK_MULTICALL_SWAP");
	CALLUI_RETURN_VALUE_IF_FAIL(merge, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(parent, CALLUI_PART_SWALLOW_SWAP, swap);
	elm_object_signal_callback_add(swap, "mouse,clicked,*", "*", __swap_btn_click_cb, ad);

	return CALLUI_RESULT_OK;
}

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_mc_split_h vd = (callui_view_mc_split_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	callui_result_e res = _callui_manager_end_call(ad->call_manager,
			0, CALLUI_CALL_RELEASE_ALL_ACTIVE);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_end_call() failed. res[%d]", res);
	}
}

static void __update_call_duration_time(callui_view_mc_split_h vd, struct tm *comp_time)
{
	struct tm *cur_time = vd->base_view.call_duration_tm;
	int sec_diff = comp_time->tm_sec - cur_time->tm_sec;

	if (sec_diff != 0) {
		memcpy(cur_time, comp_time, sizeof(struct tm));

		char *tmp = _callui_common_get_duration_time_string(cur_time);

		elm_object_part_text_set(vd->active_layout, CALLUI_PART_TEXT_STATUS, tmp);
		elm_object_part_text_set(vd->base_view.contents, CALLUI_PART_TEXT_CALL_DURATION, tmp);

		free(tmp);
	}
}

static Eina_Bool __call_duration_timer_cb(void* data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_mc_split_h vd = data;

	struct tm *new_tm = _callui_stp_get_call_duration(vd->base_view.ad->state_provider,
			CALLUI_CALL_DATA_ACTIVE);
	if (!new_tm) {
		vd->base_view.call_duration_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	__update_call_duration_time(vd, new_tm);

	free(new_tm);

	return ECORE_CALLBACK_RENEW;
}

static callui_result_e __init_call_duration_timer(callui_view_mc_split_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	DELETE_ECORE_TIMER(vd->base_view.call_duration_timer);
	FREE(vd->base_view.call_duration_tm);

	vd->base_view.call_duration_tm = _callui_stp_get_call_duration(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.call_duration_tm, CALLUI_RESULT_ALLOCATION_FAIL);

	_callui_common_set_call_duration_time(vd->base_view.call_duration_tm, vd->active_layout, CALLUI_PART_TEXT_STATUS);

	vd->base_view.call_duration_timer = ecore_timer_add(0.1, __call_duration_timer_cb, vd);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.call_duration_timer, CALLUI_RESULT_ALLOCATION_FAIL);

	return CALLUI_RESULT_OK;
}

static callui_result_e __update_displayed_data(callui_view_mc_split_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	const callui_call_data_t *active = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	CALLUI_RETURN_VALUE_IF_FAIL(active, CALLUI_RESULT_FAIL);
	const callui_call_data_t *held = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
	CALLUI_RETURN_VALUE_IF_FAIL(held, CALLUI_RESULT_FAIL);

	__update_hold_active_layout(vd->hold_layout, held);
	__set_hold_info(vd->caller_info, vd->hold_layout);

	__update_hold_active_layout(vd->active_layout, active);
	__set_active_info(vd->caller_info, vd->active_layout, ad);

	callui_result_e res = __init_call_duration_timer(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	evas_object_show(vd->base_view.contents);

	return res;
}

static void __mng_callers_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;

	_callui_vm_change_view(ad->view_manager, CALLUI_VIEW_MULTICALL_LIST);
}

static void __merge_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;

	callui_result_e res = _callui_manager_join_call(ad->call_manager);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_join_call() failed. res[%d]", res);
	}
}

static void __swap_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;

	callui_result_e res = _callui_manager_swap_call(ad->call_manager);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_swap_call() failed. res[%d]", res);
	}
}

static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility)
{
	callui_view_mc_split_h vd = (callui_view_mc_split_h)data;

	if (visibility) {
		elm_object_signal_emit(vd->base_view.contents, "hide_callers_info", "multi_call_split");
	} else {
		elm_object_signal_emit(vd->base_view.contents, "show_callers_info", "multi_call_split");
	}
}
