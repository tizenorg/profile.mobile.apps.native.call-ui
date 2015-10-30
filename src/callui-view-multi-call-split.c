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
#include "callui-view-multi-call-split.h"
#include "callui-keypad.h"
#include "callui-common.h"
#include "callui-view-caller-info-defines.h"
#include "callui-view-elements.h"

#define BUF_SIZE 256

typedef struct {
	Evas_Object *contents;
} incall_multi_view_split_priv_t;

static Evas_Object *_create_merge_swap_btn(Evas_Object *parent, const char *name, const char *text);

static Evas_Object *_create_hold_active_layout(Evas_Object *parent, call_data_t *call_data);
static void _fill_one_contact_layout(Evas_Object *parent, call_data_t *call_data);
static void _fill_conference_layout(Evas_Object *parent, call_data_t *call_data);

static void _set_hold_info(Evas_Object *parent, Evas_Object *content);
static void _set_active_info(Evas_Object *parent, Evas_Object *content, callui_app_data_t *ad);
static void _set_merge_swap(Evas_Object *parent, callui_app_data_t *ad);

static void _create_btn_region(Evas_Object *parent);
static void _create_action_panel(Evas_Object *parent);

static Evas_Object *_create_call_info_layout(Evas_Object *parent, callui_app_data_t *ad);
static Evas_Object *_create_split_layout(Evas_Object *parent, callui_app_data_t *ad);

static Evas_Object *_create_split_view(Evas_Object *parent, callui_app_data_t *ad);

static void _callui_view_multi_call_split_draw_screen(Evas_Object *eo, void *data);

static void _manage_callers_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _merge_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _swap_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

static int _callui_view_multi_call_split_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *param3);
static int _callui_view_multi_call_split_onupdate(call_view_data_t *view_data, void *update_data1);
static int _callui_view_multi_call_split_onshow(call_view_data_t *view_data, void *appdata);
static int _callui_view_multi_call_split_ondestroy(call_view_data_t *view_data);

static call_view_data_t s_view = {
	.type = VIEW_INCALL_MULTICALL_SPLIT_VIEW,
	.layout = NULL,
	.onCreate = _callui_view_multi_call_split_oncreate,
	.onUpdate = _callui_view_multi_call_split_onupdate,
	.onHide = NULL,
	.onShow = _callui_view_multi_call_split_onshow,
	.onDestroy = _callui_view_multi_call_split_ondestroy,
	.onRotate = NULL,
	.priv = NULL,
};

static Evas_Object *_create_merge_swap_btn(Evas_Object *parent, const char *name, const char *text)
{
	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, EDJ_NAME, name);

	elm_object_part_text_set(layout, PART_TEXT_MERGE_SWAP_BTN, text);

	return layout;
}

static Evas_Object *_create_hold_active_layout(Evas_Object *parent, call_data_t *call_data)
{
	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, EDJ_NAME, GROUP_ACTIVE_HOLD_INFO);

	if (call_data->member_count == 1) {
		_fill_one_contact_layout(layout, call_data);
	} else {
		_fill_conference_layout(layout, call_data);
	}

	return layout;
}

static void _fill_one_contact_layout(Evas_Object *parent, call_data_t *call_data)
{
	const char *pic_path = call_data->call_ct_info.caller_id_path;
	const char *main_text = call_data->call_ct_info.call_disp_name;
	const char *sub_text = call_data->call_num;
	Evas_Object *thumbnail = NULL;

	thumbnail = _callui_create_thumbnail(parent, pic_path, THUMBNAIL_186);
	elm_object_part_content_set(parent, PART_SWALLOW_CALLER_ID, thumbnail);

	if (main_text && *main_text) {
		elm_object_part_text_set(parent, PART_TEXT_MAIN, main_text);
		elm_object_part_text_set(parent, PART_TEXT_SUB, sub_text);
	} else {
		elm_object_part_text_set(parent, PART_TEXT_MAIN, sub_text);
	}
}

static void _fill_conference_layout(Evas_Object *parent, call_data_t *call_data)
{
	CALLUI_RETURN_IF_FAIL(call_data);

	Evas_Object *thumbnail = _callui_create_thumbnail(parent, NULL, CONFERENCE_THUMBNAIL_186);
	elm_object_part_content_set(parent, PART_SWALLOW_CALLER_ID, thumbnail);

	elm_object_part_text_set(parent, PART_TEXT_MAIN, _("IDS_CALL_BODY_CONFERENCE"));

	char buffer[BUF_SIZE] = {0};
	snprintf(buffer, BUF_SIZE, _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB"), call_data->member_count);
	elm_object_part_text_set(parent, PART_TEXT_SUB, buffer);
}

static void _set_hold_info(Evas_Object *parent, Evas_Object *content)
{
	elm_object_part_text_set(content, PART_TEXT_STATUS, _("IDS_CALL_BODY_ON_HOLD_ABB"));

	elm_object_part_content_set(parent, PART_SWALLOW_HOLD_INFO, content);
	elm_object_signal_emit(content, SIGNAL_SET_BLURRED_BACKGROUND, "");
}

static void _set_active_info(Evas_Object *parent, Evas_Object *content, callui_app_data_t *ad)
{
	elm_object_part_text_set(content, PART_TEXT_STATUS, _("IDS_CALL_BODY_CONNECTED_M_STATUS_ABB"));

	elm_object_part_content_set(parent, PART_SWALLOW_ACTIVE_INFO, content);
	elm_object_signal_emit(content, SIGNAL_SET_TRANSPARENT_BACKGROUND, "");
	if (ad->active->member_count > 1) {
		elm_object_signal_emit(content, SIGNAL_SHOW_ARROW, "");
		elm_object_signal_callback_add(content, "mouse,up,*", "arrow", _manage_callers_cb, ad);
	}
}

static void _set_merge_swap(Evas_Object *parent, callui_app_data_t *ad)
{
	Evas_Object *merge = _create_merge_swap_btn(parent, GROUP_MERGE_BTN, _("IDS_CALL_BODY_MERGE_T_CALL"));
	elm_object_part_content_set(parent, PART_SWALLOW_MERGE, merge);
	elm_object_signal_callback_add(merge, "mouse,clicked,*", "*", _merge_cb, ad);

	Evas_Object *swap = _create_merge_swap_btn(parent, GROUP_SWAP_BTN, _("IDS_CALL_SK_MULTICALL_SWAP"));
	elm_object_part_content_set(parent, PART_SWALLOW_SWAP, swap);
	elm_object_signal_callback_add(swap, "mouse,clicked,*", "*", _swap_cb, ad);
}

static void _create_btn_region(Evas_Object *parent)
{
	Evas_Object *btn_region = elm_layout_add(parent);
	elm_layout_file_set(btn_region, EDJ_NAME, GROUP_BTN_REGION);
	elm_object_part_content_set(parent, PART_SWALLOW_BTN_REGION, btn_region);
}

static void _create_action_panel(Evas_Object *parent)
{
	Evas_Object *action_panel = elm_layout_add(parent);
	elm_layout_file_set(action_panel, EDJ_NAME, GROUP_ACTION_PANEL);
	elm_object_part_content_set(parent, PART_SWALLOW_ACTIONS_PANEL, action_panel);
}

static Evas_Object *_create_call_info_layout(Evas_Object *parent, callui_app_data_t *ad)
{
	Evas_Object *one_hold_layout = elm_layout_add(parent);
	elm_layout_file_set(one_hold_layout, EDJ_NAME, GROUP_ONE_HOLD_IN_CONFERENCE);

	Evas_Object *hold_layout = _create_hold_active_layout(one_hold_layout, ad->held);
	Evas_Object *active_layout = _create_hold_active_layout(one_hold_layout, ad->active);

	_set_hold_info(one_hold_layout, hold_layout);
	_set_active_info(one_hold_layout, active_layout, ad);

	_set_merge_swap(one_hold_layout, ad);

	return one_hold_layout;
}

static Evas_Object *_create_split_layout(Evas_Object *parent, callui_app_data_t *ad)
{
	Evas_Object *split_layout = elm_layout_add(parent);
	elm_layout_file_set(split_layout, EDJ_NAME, GROUP_SPLIT);

	Evas_Object *call_info_layout = _create_call_info_layout(parent, ad);
	elm_object_part_content_set(split_layout, PART_SWALLOW_CALL_INFO, call_info_layout);

	_create_btn_region(split_layout);
	_create_action_panel(split_layout);

	return split_layout;
}

static Evas_Object *_create_split_view(Evas_Object *parent, callui_app_data_t *ad)
{
	Evas_Object *split_layout = _create_split_layout(parent, ad);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", split_layout);

	_callui_keypad_create_layout(ad);

	return split_layout;
}

static void _callui_view_multi_call_split_draw_screen(Evas_Object *eo, void *data)
{
	dbg("..");
	call_view_data_t *vd = data;
	incall_multi_view_split_priv_t *priv = vd->priv;
	callui_app_data_t *ad = _callui_get_app_data();

	priv->contents = _create_split_view(ad->main_ly, ad);

	_callui_create_top_first_button(ad);
	_callui_create_top_second_button(ad);
	_callui_create_top_third_button(ad);
	_callui_create_bottom_first_button_disabled(ad);
	_callui_create_bottom_second_button(ad);
	_callui_create_bottom_third_button(ad);

	_callui_create_end_call_button(priv->contents, vd);
}

static void _manage_callers_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	callui_app_data_t *ad = data;
	_callvm_view_change(VIEW_INCALL_MULTICALL_LIST_VIEW, 0, NULL, ad);
}

static void _merge_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	callui_app_data_t *ad = data;
	int ret = cm_join_call(ad->cm_handle);
	if (ret != CM_ERROR_NONE) {
		err("cm_join_call() is failed");
	}
}

static void _swap_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	callui_app_data_t *ad = data;
	int ret = cm_swap_call(ad->cm_handle);
	if (ret != CM_ERROR_NONE) {
		err("cm_swap_call() is failed");
	}
}

static int _callui_view_multi_call_split_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *param3)
{
	dbg("multi-split view create");

	callui_app_data_t *ad = _callui_get_app_data();

	_callui_view_multi_call_split_onshow(view_data, ad);
	_callui_lock_manager_start(ad->lock_handle);

	return 0;
}

static int _callui_view_multi_call_split_onupdate(call_view_data_t *view_data, void *update_data1)
{
	dbg("multi-split view update");
	callui_app_data_t *ad = _callui_get_app_data();
	_callui_view_multi_call_split_onshow(view_data, ad);
	_callui_lock_manager_start(ad->lock_handle);
	return 0;
}

static int _callui_view_multi_call_split_onshow(call_view_data_t *view_data, void *appdata)
{
	dbg("multi-split view show");
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	incall_multi_view_split_priv_t *priv = (incall_multi_view_split_priv_t *)view_data->priv;
	_callui_view_multi_call_split_draw_screen(priv->contents, view_data);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return 0;
}

static int _callui_view_multi_call_split_ondestroy(call_view_data_t *view_data)
{
	dbg("multi-split view destroy");

	callui_app_data_t *ad = _callui_get_app_data();

	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCALL_MULTICALL_SPLIT_VIEW);
	CALLUI_RETURN_VALUE_IF_FAIL(vd, -1);

	free(vd->priv);
	vd->priv = NULL;

	_callvm_reset_call_view_data(ad, VIEW_INCALL_MULTICALL_SPLIT_VIEW);
	return 0;
}

call_view_data_t *_callui_view_multi_call_split_new(callui_app_data_t*ad)
{
	s_view.priv = calloc(1, sizeof(incall_multi_view_split_priv_t));

	if (!s_view.priv) {
		err("ERROR!");
	}

	return &s_view;
}
