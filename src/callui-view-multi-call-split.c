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
#include "callui.h"
#include "callui-keypad.h"
#include "callui-common.h"
#include "callui-view-caller-info-defines.h"
#include "callui-view-elements.h"

#define BUF_SIZE 256

struct _callui_view_mc_split {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;

	Evas_Object *hold_layout;
	Evas_Object *active_layout;
};
typedef struct _callui_view_mc_split _callui_view_mc_split_t;

static int _callui_view_multi_call_split_oncreate(call_view_data_base_t *view_data, void *appdata);
static int _callui_view_multi_call_split_onupdate(call_view_data_base_t *view_data);
static int _callui_view_multi_call_split_ondestroy(call_view_data_base_t *view_data);

static int __create_main_content(callui_view_mc_split_h vd);
static int __update_displayed_data(callui_view_mc_split_h vd);

static Evas_Object *__create_merge_swap_btn(Evas_Object *parent, const char *name, const char *text);

static void __update_hold_active_layout(Evas_Object *layout, call_data_t *call_data);
static void __fill_one_contact_layout(Evas_Object *parent, call_data_t *call_data);
static void __fill_conference_layout(Evas_Object *parent, call_data_t *call_data);
static void __set_hold_info(Evas_Object *parent, Evas_Object *content);
static void __set_active_info(Evas_Object *parent, Evas_Object *content, callui_app_data_t *ad);

static int __create_merge_swap_btns(Evas_Object *parent, callui_app_data_t *ad);

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __mng_callers_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src);
static void __merge_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src);
static void __swap_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src);
static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility);

callui_view_mc_split_h _callui_view_multi_call_split_new()
{
	callui_view_mc_split_h mc_split_view = calloc(1, sizeof(_callui_view_mc_split_t));
	CALLUI_RETURN_NULL_IF_FAIL(mc_split_view);

	mc_split_view->base_view.onCreate = _callui_view_multi_call_split_oncreate;
	mc_split_view->base_view.onUpdate = _callui_view_multi_call_split_onupdate;
	mc_split_view->base_view.onDestroy = _callui_view_multi_call_split_ondestroy;

	return mc_split_view;
}

static int _callui_view_multi_call_split_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_split_h vd = (callui_view_mc_split_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	int res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	_callui_lock_manager_start(ad->lock_handle);

	return __update_displayed_data(vd);
}

static int _callui_view_multi_call_split_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_split_h vd = (callui_view_mc_split_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	_callui_lock_manager_start(ad->lock_handle);

	return 	__update_displayed_data(vd);
}

static int _callui_view_multi_call_split_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_split_h vd = (callui_view_mc_split_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	DELETE_EVAS_OBJECT(vd->base_view.contents);

	_callui_keypad_hide_immediately(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, NULL, NULL);

	free(vd);

	return CALLUI_RESULT_OK;
}


static int __create_main_content(callui_view_mc_split_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME, GROUP_SPLIT);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", vd->base_view.contents);

	vd->caller_info = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GROUP_ONE_HOLD_IN_CONFERENCE);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->caller_info, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, PART_SWALLOW_CALL_INFO, vd->caller_info);

	vd->hold_layout = _callui_load_edj(vd->caller_info, EDJ_NAME, GROUP_ACTIVE_HOLD_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->hold_layout, CALLUI_RESULT_ALLOCATION_FAIL);

	vd->active_layout = _callui_load_edj(vd->caller_info, EDJ_NAME, GROUP_ACTIVE_HOLD_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->active_layout, CALLUI_RESULT_ALLOCATION_FAIL);

	int res = __create_merge_swap_btns(vd->caller_info, ad);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	Evas_Object *btn_region = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_BUTTON_LAYOUT);
	CALLUI_RETURN_VALUE_IF_FAIL(btn_region, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, PART_SWALLOW_BTN_REGION, btn_region);

	_callui_keypad_clear_input(ad->keypad);
	_callui_keypad_show_status_change_callback_set(ad->keypad, __keypad_show_state_change_cd, vd);

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_create_end_call_button(vd->base_view.contents,
			__end_call_btn_click_cb, vd), CALLUI_RESULT_ALLOCATION_FAIL);

	return res;
}

static Evas_Object *__create_merge_swap_btn(Evas_Object *parent, const char *name, const char *text)
{
	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, EDJ_NAME, name);

	elm_object_part_text_set(layout, PART_TEXT_MERGE_SWAP_BTN, text);

	return layout;
}

static void __update_hold_active_layout(Evas_Object *layout, call_data_t *call_data)
{
	if (call_data->member_count == 1) {
		__fill_one_contact_layout(layout, call_data);
	} else {
		__fill_conference_layout(layout, call_data);
	}
}

static void __fill_one_contact_layout(Evas_Object *parent, call_data_t *call_data)
{
	const char *pic_path = call_data->call_ct_info.caller_id_path;
	const char *main_text = call_data->call_ct_info.call_disp_name;
	const char *sub_text = call_data->call_num;

	Evas_Object *thumbnail = _callui_create_thumbnail(parent, pic_path, THUMBNAIL_138);
	elm_object_part_content_set(parent, PART_SWALLOW_CALLER_ID, thumbnail);

	if (main_text && *main_text) {
		elm_object_part_text_set(parent, PART_TEXT_MAIN, main_text);
		elm_object_part_text_set(parent, PART_TEXT_SUB, sub_text);
	} else {
		elm_object_part_text_set(parent, PART_TEXT_MAIN, sub_text);
		elm_object_part_text_set(parent, PART_TEXT_SUB, "");
	}
}

static void __fill_conference_layout(Evas_Object *parent, call_data_t *call_data)
{
	Evas_Object *thumbnail = _callui_create_thumbnail(parent, NULL, CONFERENCE_THUMBNAIL_138);
	elm_object_part_content_set(parent, PART_SWALLOW_CALLER_ID, thumbnail);

	elm_object_part_text_set(parent, PART_TEXT_MAIN, _("IDS_CALL_BODY_CONFERENCE"));

	char buffer[BUF_SIZE] = { 0 };
	const char *fmt = _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB");
	snprintf(buffer, BUF_SIZE, fmt, call_data->member_count);
	elm_object_part_text_set(parent, PART_TEXT_SUB, buffer);
}

static void __set_hold_info(Evas_Object *parent, Evas_Object *content)
{
	elm_object_part_text_set(content, PART_TEXT_STATUS, _("IDS_CALL_BODY_ON_HOLD_ABB"));

	elm_object_part_content_set(parent, PART_SWALLOW_HOLD_INFO, content);
	elm_object_signal_emit(content, SIGNAL_SET_BLURRED_BACKGROUND, "");
}

static void __set_active_info(Evas_Object *parent, Evas_Object *content, callui_app_data_t *ad)
{
	elm_object_part_text_set(content, PART_TEXT_STATUS, _("IDS_CALL_BODY_CONNECTED_M_STATUS_ABB"));

	elm_object_part_content_set(parent, PART_SWALLOW_ACTIVE_INFO, content);
	elm_object_signal_emit(content, SIGNAL_SET_TRANSPARENT_BACKGROUND, "");
	if (ad->active->member_count > 1) {
		elm_object_signal_emit(content, SIGNAL_SHOW_ARROW, "");
		elm_object_signal_callback_add(content, "mouse,up,*", "arrow", __mng_callers_btn_click_cb, ad);
	}
}

static int __create_merge_swap_btns(Evas_Object *parent, callui_app_data_t *ad)
{
	Evas_Object *merge = __create_merge_swap_btn(parent, GROUP_MERGE_BTN, _("IDS_CALL_BODY_MERGE_T_CALL"));
	CALLUI_RETURN_VALUE_IF_FAIL(merge, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(parent, PART_SWALLOW_MERGE, merge);
	elm_object_signal_callback_add(merge, "mouse,clicked,*", "*", __merge_btn_click_cb, ad);

	Evas_Object *swap = __create_merge_swap_btn(parent, GROUP_SWAP_BTN, _("IDS_CALL_SK_MULTICALL_SWAP"));
	CALLUI_RETURN_VALUE_IF_FAIL(merge, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(parent, PART_SWALLOW_SWAP, swap);
	elm_object_signal_callback_add(swap, "mouse,clicked,*", "*", __swap_btn_click_cb, ad);

	return CALLUI_RESULT_OK;
}

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_mc_split_h vd = (callui_view_mc_split_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	int ret = cm_end_call(ad->cm_handle, 0, CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS);
	if (ret != CM_ERROR_NONE) {
		err("cm_end_call() is failed");
	}
}

static int __update_displayed_data(callui_view_mc_split_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	__update_hold_active_layout(vd->hold_layout, ad->held);
	__set_hold_info(vd->caller_info, vd->hold_layout);

	__update_hold_active_layout(vd->active_layout, ad->active);
	__set_active_info(vd->caller_info, vd->active_layout, ad);

	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_top_first_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_top_second_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_top_third_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_bottom_first_button_disabled(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_bottom_second_button(ad), CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(
			_callui_create_bottom_third_button(ad), CALLUI_RESULT_FAIL);

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static void __mng_callers_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;

	_callui_vm_change_view(ad->view_manager_handle, VIEW_TYPE_MULTICALL_LIST);
}

static void __merge_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;

	int ret = cm_join_call(ad->cm_handle);
	if (ret != CM_ERROR_NONE) {
		err("cm_join_call() is failed");
	}
}

static void __swap_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *src)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;

	int ret = cm_swap_call(ad->cm_handle);
	if (ret != CM_ERROR_NONE) {
		err("cm_swap_call() is failed");
	}
}

static void __keypad_show_state_change_cd(void *data, Eina_Bool visibility)
{
	callui_view_mc_split_h vd = (callui_view_mc_split_h)data;

	if (visibility) {
		elm_object_signal_emit(vd->base_view.contents, "SHOW", "KEYPAD_BTN");
	} else {
		elm_object_signal_emit(vd->base_view.contents, "HIDE", "KEYPAD_BTN");
	}
}