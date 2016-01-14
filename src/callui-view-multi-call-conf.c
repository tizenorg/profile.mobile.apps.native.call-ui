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
#include "callui-common.h"
#include "callui-view-elements.h"
#include "callui-view-multi-call-conf.h"
#include "callui-view-layout-wvga.h"
#include "callui-keypad.h"

#define	CALLUI_VIEW_MULTICALL_CONF_LAYOUT_ID "MULTIVIEWCONF"
typedef struct {
	Evas_Object *contents;
	Evas_Object *caller_info;
	Evas_Object *held_call_ly;
	Evas_Object *btn_ly;
	Evas_Object *ic;
	Evas_Object *manage_calls_ly;
	Eina_Bool is_held;
} callui_view_multi_call_conf_priv_t;

static int __callui_view_multi_call_conf_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *param3);
static int __callui_view_multi_call_conf_onupdate(call_view_data_t *view_data, void *update_data1);
static int __callui_view_multi_call_conf_onhide(call_view_data_t *view_data);
static int __callui_view_multi_call_conf_onshow(call_view_data_t *view_data,  void *appdata);
static int __callui_view_multi_call_conf_ondestroy(call_view_data_t *view_data);
static int __callui_view_multi_call_conf_onrotate(call_view_data_t *view_data);

static call_view_data_t s_view = {
	.type = VIEW_INCALL_MULTICALL_CONF_VIEW,
	.layout = NULL,
	.onCreate = __callui_view_multi_call_conf_oncreate,
	.onUpdate = __callui_view_multi_call_conf_onupdate,
	.onHide = __callui_view_multi_call_conf_onhide,
	.onShow = __callui_view_multi_call_conf_onshow,
	.onDestroy = __callui_view_multi_call_conf_ondestroy,
	.onRotate = __callui_view_multi_call_conf_onrotate,
	.priv = NULL,
};

call_view_data_t *_callui_view_multi_call_conf_new(callui_app_data_t *ad)
{
	s_view.priv = calloc(1, sizeof(callui_view_multi_call_conf_priv_t));

	if (!s_view.priv) {
		err("ERROR!!!!!!!!!!! ");
	}

	return &s_view;
}

static void __callui_view_manage_btn_clicked_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
	dbg("..");
	callui_app_data_t *ad = _callui_get_app_data();
	_callvm_view_change(VIEW_INCALL_MULTICALL_LIST_VIEW, 0, 0, ad);
	return;
}

static void __callui_view_multi_call_conf_draw_screen(Evas_Object *eo, void *data)
{
	dbg("..");

	call_view_data_t *vd = (call_view_data_t *)data;
	callui_view_multi_call_conf_priv_t *priv = (callui_view_multi_call_conf_priv_t *)vd->priv;
	callui_app_data_t *ad = _callui_get_app_data();
	char buf[512] = { 0, };
	call_data_t *call_data = NULL;
	char status_txt[128 + 1] = { '\0', };

	if (ad->active) {
		call_data = ad->active;
		priv->is_held = EINA_FALSE;
	} else if (ad->held) {
		call_data = ad->held;
		priv->is_held = EINA_TRUE;
	} else {
		err("call data is null");
		return;
	}

	if (priv->is_held == EINA_TRUE) {
		snprintf(status_txt, sizeof(status_txt), _("IDS_CALL_BODY_ON_HOLD_ABB"));
		_callui_show_caller_info_status(ad, status_txt);
		elm_object_signal_emit(priv->caller_info, "set-hold-state", "call-screen");

		elm_object_part_content_unset(priv->contents, "resume_icon_swallow");
		elm_object_part_content_set(priv->contents, "resume_icon_swallow", priv->held_call_ly);
		evas_object_show(priv->held_call_ly);

	} else {
		/*****deciding the call status according the sim name******/
		elm_object_signal_emit(priv->caller_info, "set-unhold-state", "call-screen");
		evas_object_hide(priv->held_call_ly);
		elm_object_part_content_unset(priv->contents, "resume_icon_swallow");
	}

	elm_object_part_content_unset(priv->caller_info, "manage_calls_icon_swallow");
	elm_object_part_content_set(priv->caller_info, "manage_calls_icon_swallow", priv->manage_calls_ly);
	evas_object_show(priv->manage_calls_ly);
	edje_object_signal_callback_del(_EDJ(priv->manage_calls_ly), "mouse,clicked,1", "btn", __callui_view_manage_btn_clicked_cb);
	edje_object_signal_callback_add(_EDJ(priv->manage_calls_ly), "mouse,clicked,1", "btn", __callui_view_manage_btn_clicked_cb, priv);

	elm_object_signal_emit(priv->caller_info, "set_conference_mode", "");
	_callui_show_caller_info_name(ad, _("IDS_CALL_BODY_CONFERENCE"));
	char * status = _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB");
	snprintf(buf, 512, status, call_data->member_count);
	_callui_show_caller_info_number(ad, buf);

	if (priv->is_held == EINA_TRUE) {
		_callui_create_top_second_button_disabled(ad);
		_callui_create_bottom_second_button_disabled(ad);
	} else {
		/*Delete keypad layout - before creating the keypad button again
		 * Since layout are created again in update_cb
		 * to handle rotation use-case*/
		if (_callui_keypad_get_show_status() == EINA_TRUE) {
			_callui_keypad_hide_layout(ad);
		}

		_callui_create_top_second_button(ad);
		_callui_create_bottom_second_button(ad);
	}
	_callui_create_top_third_button(ad);
	_callui_create_bottom_first_button(ad);

	_callui_create_top_first_button(ad);
	_callui_create_bottom_third_button(ad);
	elm_object_signal_emit(priv->contents, "SHOW_NO_EFFECT", "ALLBTN");
	_callui_create_end_call_button(priv->contents, vd);
	evas_object_show(eo);
}

static Evas_Object *__callui_view_multi_call_conf_create_contents(void *data, char *group)
{
	if (data == NULL) {
		err("ERROR");
		return NULL;
	}
	callui_app_data_t *ad = (callui_app_data_t *)data;
	Evas_Object *eo;

	/* load edje */
	eo = _callui_load_edj(ad->main_ly, EDJ_NAME, group);
	if (eo == NULL)
		return NULL;

	return eo;
}

static void __callui_view_multi_call_conf_more_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	CALLUI_RETURN_IF_FAIL(vd != NULL);
	_callui_load_more_option(vd);
	return;
}

static int __callui_view_multi_call_conf_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *param3)
{
	callui_view_multi_call_conf_priv_t *priv = (callui_view_multi_call_conf_priv_t *)view_data->priv;
	callui_app_data_t *ad = _callui_get_app_data();

	if (ad->main_ly) {
		/* Create Main Layout */
		priv->contents = __callui_view_multi_call_conf_create_contents(ad, GRP_MAIN_LY);
		elm_object_part_content_set(ad->main_ly, "elm.swallow.content", priv->contents);

		if (priv->contents) {
			eext_object_event_callback_del(priv->contents, EEXT_CALLBACK_MORE, __callui_view_multi_call_conf_more_btn_cb);
		}
		eext_object_event_callback_add(priv->contents, EEXT_CALLBACK_MORE, __callui_view_multi_call_conf_more_btn_cb, view_data);

		priv->caller_info = elm_object_part_content_get(priv->contents, "caller_info");
		if (!priv->caller_info) {
			priv->caller_info = __callui_view_multi_call_conf_create_contents(ad, GRP_CALLER_INFO);
			elm_object_part_content_set(priv->contents, "caller_info", priv->caller_info);
		}

		/* Manage button Layout */
		priv->manage_calls_ly = elm_object_part_content_get(priv->caller_info, "manage_calls_icon_swallow");
		if (!priv->manage_calls_ly) {
			priv->manage_calls_ly = __callui_view_multi_call_conf_create_contents(ad, GRP_MANAGE_CALLS);
		}

		priv->btn_ly = elm_object_part_content_get(priv->contents, "btn_region");
		if (!priv->btn_ly) {
			priv->btn_ly = __callui_view_multi_call_conf_create_contents(ad, GRP_BUTTON_LAYOUT);
			elm_object_part_content_set(priv->contents, "btn_region", priv->btn_ly);
		}

		evas_object_name_set(priv->contents, CALLUI_VIEW_MULTICALL_CONF_LAYOUT_ID);
		dbg("[========== MULTIVIEWCONF: priv->contents Addr : [%p] ==========]", priv->contents);

		/*create keypad layout*/
		_callui_keypad_create_layout(ad);
	} else {
		err("ERROR");
		return -1;
	}

	__callui_view_multi_call_conf_onshow(view_data, NULL);
	return 0;
}

static int __callui_view_multi_call_conf_onupdate(call_view_data_t *view_data, void *update_data1)
{
	dbg("multicall-conf view update");

	__callui_view_multi_call_conf_onshow(view_data, NULL);
	return 0;
}

static int __callui_view_multi_call_conf_onhide(call_view_data_t *view_data)
{
	dbg("multicall-conf view hide");
	callui_app_data_t *ad = _callui_get_app_data();
	evas_object_hide(ad->main_ly);
	return 0;
}

static int __callui_view_multi_call_conf_onshow(call_view_data_t *view_data,  void *appdata)
{
	dbg("multicall-conf view show");

	callui_view_multi_call_conf_priv_t *priv = (callui_view_multi_call_conf_priv_t *)view_data->priv;
	callui_app_data_t *ad = _callui_get_app_data();

	__callui_view_multi_call_conf_draw_screen(priv->contents, view_data);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return 0;
}

static int __callui_view_multi_call_conf_ondestroy(call_view_data_t *view_data)
{
	dbg("multicall-conf view destroy");
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_VALUE_IF_FAIL(ad, -1);

	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCALL_MULTICALL_CONF_VIEW);
	CALLUI_RETURN_VALUE_IF_FAIL(vd, -1);
	callui_view_multi_call_conf_priv_t *priv = (callui_view_multi_call_conf_priv_t *)vd->priv;

	if (priv != NULL) {
		if (ad->ctxpopup) {
			elm_ctxpopup_dismiss(ad->ctxpopup);
			ad->ctxpopup = NULL;
		}

		_callui_show_caller_info_number(ad, ""); /*reset the number text*/

		/*Delete keypad layout*/
		_callui_keypad_delete_layout(ad);

		if (priv->contents) {
			eext_object_event_callback_del(priv->contents, EEXT_CALLBACK_MORE, __callui_view_multi_call_conf_more_btn_cb);
			_callui_common_reset_main_ly_text_fields(priv->contents);
		}
		elm_object_signal_emit(priv->contents, "HIDE_BTN_LY", "ALLBTN");

		if (priv->manage_calls_ly) {
			elm_object_part_content_unset(priv->caller_info, "manage_calls_icon_swallow");
			edje_object_part_unswallow(_EDJ(priv->caller_info), priv->manage_calls_ly);
			evas_object_del(priv->manage_calls_ly);
			priv->manage_calls_ly = NULL;
		}

		if (priv->held_call_ly) {
			elm_object_part_content_unset(priv->contents, "resume_icon_swallow");
			edje_object_part_unswallow(_EDJ(priv->contents), priv->held_call_ly);
			evas_object_del(priv->held_call_ly);
			priv->held_call_ly = NULL;
		}

		free(priv);
		priv = NULL;
	}

	_callvm_reset_call_view_data(ad, VIEW_INCALL_MULTICALL_CONF_VIEW);

	dbg("complete destroy multi view conf");

	return 0;
}

static int __callui_view_multi_call_conf_onrotate(call_view_data_t *view_data)
{
	dbg("*** Multi Call conf-view Rotate ***");
	callui_view_multi_call_conf_priv_t *priv = view_data->priv;
	callui_app_data_t *ad = _callui_get_app_data();

	elm_object_signal_emit(priv->contents, "set_portrait", "multicall_conf_layout");
	elm_object_signal_emit(priv->contents, "SHOW_NO_EFFECT", "ALLBTN");
	elm_object_signal_emit(priv->caller_info, "set_portrait", "caller_info_layout");

	evas_object_show(ad->win);

	return 0;
}

