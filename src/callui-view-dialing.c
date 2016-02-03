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

struct callui_view_dialing_priv {
	Evas_Object *contents;
	Evas_Object *caller_info;
	Evas_Object *btn_ly;
	Evas_Object *ic;
	Eina_Bool bredial;
};

#define	 VIEW_DIALING_LAYOUT_ID "DIALVIEW"

static int __callui_view_dialing_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *appdata);
static int __callui_view_dialing_onupdate(call_view_data_t *view_data, void *update_data1);
static int __callui_view_dialing_onhide(call_view_data_t *view_data);
static int __callui_view_dialing_onshow(call_view_data_t *view_data, void *appdata);
static int __callui_view_dialing_ondestroy(call_view_data_t *view_data);
static int __callui_view_dialing_onrotate(call_view_data_t *view_data);

call_view_data_t *_callui_dialing_view_dialing_new(callui_app_data_t *ad)
{
	static call_view_data_t dialing_view = {
		.type = VIEW_DIALLING_VIEW,
		.layout = NULL,
		.onCreate = __callui_view_dialing_oncreate,
		.onUpdate = __callui_view_dialing_onupdate,
		.onHide = __callui_view_dialing_onhide,
		.onShow = __callui_view_dialing_onshow,
		.onDestroy = __callui_view_dialing_ondestroy,
		.onRotate = __callui_view_dialing_onrotate,
		.priv = NULL,
	};
	dialing_view.priv = calloc(1, sizeof(callui_view_dialing_priv_t));

	if (!dialing_view.priv) {
		err("ERROR!!!!!!!!!!! ");
	}

	return &dialing_view;
}

static Evas_Object *__callui_view_dialing_create_contents(void *data, char *grpname)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;
	Evas_Object *eo = NULL;

	/* load edje */
	eo = _callui_load_edj(ad->main_ly, EDJ_NAME, grpname);
	if (eo == NULL) {
		err("__callui_view_dialing_create_contents ERROR");
		return NULL;
	}
	return eo;
}

static void __callui_view_dialing_draw_screen(callui_app_data_t *ad, Evas_Object *eo, void *data)
{
	dbg("__callui_view_dialing_draw_screen");

	call_view_data_t *vd = (call_view_data_t *)data;
	callui_view_dialing_priv_t *priv = (callui_view_dialing_priv_t *)vd->priv;
	call_data_t *now_call_data = ad->active;
	CALLUI_RETURN_IF_FAIL(now_call_data);
	char *file_path = NULL;

	if (now_call_data == NULL) {
		err("Now Data is NULL");
		return;
	}

	file_path = now_call_data->call_ct_info.caller_id_path;
	sec_dbg("file_path: %s", file_path);

	char *call_name = now_call_data->call_ct_info.call_disp_name;
	char *disp_number = NULL;
	if (strlen(now_call_data->call_disp_num) > 0) {
		disp_number = now_call_data->call_disp_num;
	} else {
		disp_number = now_call_data->call_num;
	}

	if (now_call_data->is_emergency == EINA_TRUE) {
		call_name = _("IDS_COM_BODY_EMERGENCY_NUMBER");
		disp_number = "";
	}

	if (strlen(call_name) == 0) {
		_callui_show_caller_info_name(ad, disp_number);
		elm_object_signal_emit(priv->caller_info, "1line", "caller_name");
	} else if (now_call_data->is_emergency == EINA_TRUE) {
		_callui_show_caller_info_name(ad, call_name);
		elm_object_signal_emit(priv->caller_info, "1line", "caller_name");
	} else {
		_callui_show_caller_info_name(ad, call_name);
		_callui_show_caller_info_number(ad, disp_number);
		elm_object_signal_emit(priv->caller_info, "2line", "caller_name");
	}

	_callui_show_caller_info_status(ad, _("IDS_CALL_POP_DIALLING"));

	_callui_create_top_first_button(ad);
	_callui_create_top_second_button(ad);
	_callui_create_top_third_button(ad);
	_callui_create_bottom_first_button_disabled(ad);
	_callui_create_bottom_second_button_disabled(ad);
	_callui_create_bottom_third_button_disabled(ad);

	elm_object_signal_emit(priv->contents, "SHOW_EFFECT", "ALLBTN");

	/*_vcui_elements_check_keypad_n_hide(vd);*/
	_callui_create_end_call_button(priv->contents, vd);

	evas_object_show(eo);

	if (now_call_data->is_emergency == EINA_TRUE) {
		elm_object_signal_emit(priv->caller_info, "set_emergency_mode", "");
	} else {
		if (strcmp(file_path, "default") != 0) {
			_callui_show_caller_id(priv->caller_info, file_path);
		}
	}
	/*evas_event_callback_add(evas_object_evas_get(eo),
			EVAS_CALLBACK_RENDER_POST, __vcui_view_dialing_post_render_cb, vd);*/
}

static int __callui_view_dialing_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *appdata)
{
	dbg("dialling view create!!");
	callui_app_data_t *ad = (callui_app_data_t *) appdata;
	callui_view_dialing_priv_t *priv = (callui_view_dialing_priv_t *)view_data->priv;

	if (ad->main_ly) {
		priv->contents = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
		if (!priv->contents) {
			priv->contents = __callui_view_dialing_create_contents(ad, GRP_MAIN_LY);
			elm_object_part_content_set(ad->main_ly, "elm.swallow.content", priv->contents);
		}

		priv->btn_ly = elm_object_part_content_get(priv->contents, "btn_region");
		if (!priv->btn_ly) {
			priv->btn_ly = __callui_view_dialing_create_contents(ad, GRP_BUTTON_LAYOUT);
			elm_object_part_content_set(priv->contents, "btn_region", priv->btn_ly);
		}

		priv->caller_info = elm_object_part_content_get(priv->contents, "caller_info");
		if (!priv->caller_info) {
			priv->caller_info = __callui_view_dialing_create_contents(ad, GRP_CALLER_INFO);
			elm_object_part_content_set(priv->contents, "caller_info", priv->caller_info);
		}
		evas_object_name_set(priv->contents, VIEW_DIALING_LAYOUT_ID);
		dbg("[========== DIALVIEW: priv->contents Addr : [%p] ==========]", priv->contents);

		/*create keypad layout*/
		_callui_keypad_create_layout(ad);
	}
	__callui_view_dialing_onshow(view_data, ad);

	_callui_lock_manager_start(ad->lock_handle);
	return 0;
}

static int __callui_view_dialing_onupdate(call_view_data_t *view_data, void *update_data)
{
	return 0;
}

static int __callui_view_dialing_onhide(call_view_data_t *view_data)
{
	dbg("dialling view hide");

	callui_app_data_t *ad = _callui_get_app_data();

	evas_object_hide(ad->main_ly);

	return 0;
}

static int __callui_view_dialing_onshow(call_view_data_t *view_data, void *appdata)
{
	dbg("dialling view show");
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	callui_view_dialing_priv_t *priv = (callui_view_dialing_priv_t *)view_data->priv;

	__callui_view_dialing_draw_screen(ad, priv->contents, view_data);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return 0;
}

static int __callui_view_dialing_ondestroy(call_view_data_t *vd)
{
	dbg("dialling view destroy");

	callui_app_data_t *ad = _callui_get_app_data();
	callui_view_dialing_priv_t *priv = (callui_view_dialing_priv_t *)vd->priv;

	if (priv != NULL) {
		/*Delete keypad layout */
		_callui_keypad_delete_layout(ad);
		elm_object_signal_emit(priv->contents, "HIDE_BTN_LY", "ALLBTN");
		_callui_common_reset_main_ly_text_fields(priv->contents);
		free(priv);
		priv = NULL;
	}

	_callvm_reset_call_view_data(ad, VIEW_DIALLING_VIEW);

	return 0;
}

static int __callui_view_dialing_onrotate(call_view_data_t *view_data)
{
	dbg("*** Dialling view Rotate ***");
	return 0;
}
