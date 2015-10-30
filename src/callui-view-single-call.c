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

#define	 VIEW_SINGLE_CALL_LAYOUT_ID "ONEVIEW"

struct incall_one_view_priv {
	Evas_Object *contents;
	Evas_Object *caller_info;
	Evas_Object *held_call_ly;
	Evas_Object *extra_vol_ly;
	Evas_Object *btn_ly;
	Evas_Object *ic;
};

static int __callui_view_single_call_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *param3);
static int __callui_view_single_call_onupdate(call_view_data_t *view_data, void *update_data1);
static int __callui_view_single_call_onhide(call_view_data_t *view_data);
static int __callui_view_single_call_onshow(call_view_data_t *view_data, void *appdata);
static int __callui_view_single_call_ondestroy(call_view_data_t *view_data);
static Evas_Object *__callui_view_single_call_create_contents(void *data, char *grpname);
static int __callui_view_single_call_onRotate(call_view_data_t *view_data);

call_view_data_t *_callui_view_single_call_new(callui_app_data_t *ad)
{
	static call_view_data_t one_call_view = {
		.type = VIEW_INCALL_ONECALL_VIEW,
		.layout = NULL,
		.onCreate = __callui_view_single_call_oncreate,
		.onUpdate = __callui_view_single_call_onupdate,
		.onHide = __callui_view_single_call_onhide,
		.onShow = __callui_view_single_call_onshow,
		.onDestroy = __callui_view_single_call_ondestroy,
		.onRotate = __callui_view_single_call_onRotate,
		.priv = NULL,
	};
	one_call_view.priv = calloc(1, sizeof(incall_one_view_priv_t));

	if (!one_call_view.priv) {
		err("ERROR!!!!!!!!!!! ");
	}

	return &one_call_view;
}

static Evas_Object *__callui_view_single_call_create_contents(void *data, char *grpname)
{
	if (data == NULL) {
		err("ERROR");
		return NULL;
	}
	callui_app_data_t *ad = (callui_app_data_t *)data;
	Evas_Object *eo = NULL;

	/* load edje */
	eo = _callui_load_edj(ad->main_ly, EDJ_NAME, grpname);
	if (eo == NULL)
		return NULL;

	return eo;
}

void __callui_view_single_more_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	CALLUI_RETURN_IF_FAIL(vd != NULL);
	_callui_load_more_option(vd);
	return;
}

static void __callui_view_single_call_draw_screen(callui_app_data_t *ad, Evas_Object *eo, void *data)
{
	dbg("__callui_view_single_call_draw_screen");

	call_view_data_t *vd = (call_view_data_t *)data;
	incall_one_view_priv_t *priv = (incall_one_view_priv_t *)vd->priv;
	call_data_t *call_data = NULL;
	char *file_path = NULL;
	Eina_Bool is_held = EINA_FALSE;
	char *sim_name = NULL;
	char status_txt[128 + 1] = {0, };

	if (ad->active) {
		call_data = ad->active;
		is_held = EINA_FALSE;
	} else {
		call_data = ad->held;
		is_held = EINA_TRUE;
	}
	if (call_data == NULL) {
		dbg("call Data is NULL");
		return;
	}

	file_path = call_data->call_ct_info.caller_id_path;

	sec_dbg("file_path: %s", file_path);

	if (call_data->is_emergency == EINA_TRUE) {
		elm_object_signal_emit(priv->caller_info, "set_emergency_mode", "");
	} else {
		if (strcmp(file_path, "default") != 0) {
			_callui_show_caller_id(priv->caller_info, file_path);
		}
	}

	if (is_held) {
		dbg("=================HOLD======================");
		evas_object_show(priv->held_call_ly);
		elm_object_part_content_set(priv->contents, "resume_icon_swallow", priv->held_call_ly);
		snprintf(status_txt, sizeof(status_txt), _("IDS_CALL_BODY_ON_HOLD_ABB"));
		_callui_show_caller_info_status(ad, status_txt);
	} else {
		dbg("=================UNHOLD======================");
		evas_object_hide(priv->held_call_ly);
		elm_object_part_content_unset(priv->contents, "resume_icon_swallow");
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
		elm_object_signal_emit(priv->caller_info, "1line", "caller_name");
	} else if (call_data->is_emergency == EINA_TRUE) {
		_callui_show_caller_info_name(ad, call_name);
		elm_object_signal_emit(priv->caller_info, "1line", "caller_name");
	} else {
		_callui_show_caller_info_name(ad, call_name);
		_callui_show_caller_info_number(ad, disp_number);
		elm_object_signal_emit(priv->caller_info, "2line", "caller_name");
	}

	_callui_create_top_third_button(ad);

	if (is_held) {
		_callui_create_top_second_button_disabled(ad);
		_callui_create_bottom_second_button_disabled(ad);

	} else {
		_callui_create_top_second_button(ad);
		_callui_create_bottom_second_button(ad);
	}

	_callui_create_bottom_first_button(ad);
	_callui_create_top_first_button(ad);
	_callui_create_bottom_third_button(ad);

	elm_object_signal_emit(priv->contents, "SHOW_EFFECT", "ALLBTN");
	if (priv->contents) {
		eext_object_event_callback_del(priv->contents, EEXT_CALLBACK_MORE, __callui_view_single_more_btn_cb);
	}
	eext_object_event_callback_add(priv->contents, EEXT_CALLBACK_MORE, __callui_view_single_more_btn_cb, vd);
	_callui_create_end_call_button(priv->contents, vd);
	evas_object_show(eo);
}

static int __callui_view_single_call_oncreate(call_view_data_t *vd, unsigned int param1, void *param2, void *appdata)
{
	dbg("incall view create");

	incall_one_view_priv_t *priv = (incall_one_view_priv_t *)vd->priv;
	callui_app_data_t *ad = (callui_app_data_t *) appdata;
	call_data_t *call_data = NULL;

	if (ad->active) {
		call_data = ad->active;
	} else {
		call_data = ad->held;
	}
	if (call_data == NULL) {
		err("call data is null");
		return -1;
	}

	if (ad->main_ly) {
		priv->contents = __callui_view_single_call_create_contents(ad, GRP_MAIN_LY);
		elm_object_part_content_set(ad->main_ly, "elm.swallow.content", priv->contents);

		/* Extra Volume Layout */
		priv->extra_vol_ly = __callui_view_single_call_create_contents(ad, GRP_EXTRA_VOLUME);

		priv->btn_ly = elm_object_part_content_get(priv->contents, "btn_region");
		if (!priv->btn_ly) {
			priv->btn_ly = __callui_view_single_call_create_contents(ad, GRP_BUTTON_LAYOUT);
			elm_object_part_content_set(priv->contents, "btn_region", priv->btn_ly);
		}

		priv->caller_info = elm_object_part_content_get(priv->contents, "caller_info");
		if (!priv->caller_info) {
			priv->caller_info = __callui_view_single_call_create_contents(ad, GRP_CALLER_INFO);
			elm_object_part_content_set(priv->contents, "caller_info", priv->caller_info);
		}

		evas_object_name_set(priv->contents, VIEW_SINGLE_CALL_LAYOUT_ID);
		dbg("[========== ONEVIEW: priv->contents Addr : [%p] ==========]", priv->contents);

		/*create keypad layout*/
		_callui_keypad_create_layout(ad);
	} else {
		err("ERROR");
		return -1;
	}

	__callui_view_single_call_onshow(vd, ad);

	_callui_lock_manager_start(ad->lock_handle);

	return 0;
}

static int __callui_view_single_call_onupdate(call_view_data_t *view_data, void *update_data)
{
	dbg("incall view update");
	callui_app_data_t *ad = _callui_get_app_data();
	__callui_view_single_call_onshow(view_data, ad);
	return 0;
}

static int __callui_view_single_call_onhide(call_view_data_t *view_data)
{
	dbg("incall view hide");
	callui_app_data_t *ad = _callui_get_app_data();

	evas_object_hide(ad->main_ly);
	return 0;
}

static int __callui_view_single_call_onshow(call_view_data_t *view_data, void *appdata)
{
	dbg("incall view show");
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	incall_one_view_priv_t *priv = (incall_one_view_priv_t *)view_data->priv;

	__callui_view_single_call_draw_screen(ad, priv->contents, view_data);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return 0;
}

static int __callui_view_single_call_ondestroy(call_view_data_t *view_data)
{
	dbg("incall view destroy");
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_VALUE_IF_FAIL(ad, -1);


	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCALL_ONECALL_VIEW);
	CALLUI_RETURN_VALUE_IF_FAIL(vd, -1);
	incall_one_view_priv_t *priv = (incall_one_view_priv_t *)vd->priv;

	if (priv != NULL) {
		if (ad->ctxpopup) {
			elm_ctxpopup_dismiss(ad->ctxpopup);
			ad->ctxpopup = NULL;
		}

		if (priv->extra_vol_ly) {
			evas_object_del(priv->extra_vol_ly);
			priv->extra_vol_ly = NULL;
		}

		if (priv->held_call_ly) {
			evas_object_del(priv->held_call_ly);
			priv->held_call_ly = NULL;
		}

		/*Delete keypad layout*/
		_callui_keypad_delete_layout(ad);

		if (priv->contents) {
			eext_object_event_callback_del(priv->contents, EEXT_CALLBACK_MORE, __callui_view_single_more_btn_cb);
			_callui_common_reset_main_ly_text_fields(priv->contents);
		}
		elm_object_signal_emit(priv->contents, "HIDE_BTN_LY", "ALLBTN");

		free(priv);
		priv = NULL;
	}

	_callvm_reset_call_view_data(ad, VIEW_INCALL_ONECALL_VIEW);

	dbg("complete destroy one view");
	return 0;
}

static int __callui_view_single_call_onRotate(call_view_data_t *view_data)
{
	return 0;
}

