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

#include "callui-view-incoming-lock.h"
#include "callui-view-active-incoming-call.h"
#include "callui-view-manager.h"
#include "callui-view-layout.h"
#include "callui-view-elements.h"
#include "callui-common.h"
#include <vconf.h>
#include <app_control.h>

#define DURING_ICON		"call_button_icon_03.png"
#define REJECT_ICON		"call_button_icon_04.png"
#define END_ICON		"call_button_icon_01.png"
#define REJ_MSG_GENLIST_DATA "reject_msg_genlist_data"

static Eina_Bool __callui_view_active_incoming_call_effect_close_active(void *data);
static void __callui_view_active_incoming_call_reject_msg_create_glist(void *data);
typedef void (*button_cb)(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void __callui_view_active_incoming_call_mouse_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	if (data) {
		incoming_lock_view_priv_t *prev = (incoming_lock_view_priv_t *) data;
		ecore_idle_enterer_before_add(__callui_view_active_incoming_call_effect_close_active, prev);
		callui_app_data_t * ad = (callui_app_data_t *)evas_object_data_get(obj, "app_data");
		if (ad) {
			evas_object_resize(ad->win, 0, 0);
		}
	}
}

static Eina_Bool __callui_view_active_incoming_call_effect_close_active(void *data)
{
	if (data) {
		incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *) data;
		Evas_Object *screen_ly = NULL;
		Elm_Transit *transit = NULL;
		int xpos = 0;
		int ypos = 0;
		int width = 0;
		int height = 0;
		int transit_y = 0;

		screen_ly = priv->contents;
		transit = elm_transit_add();
		elm_transit_object_add(transit, screen_ly);
		evas_object_geometry_get(screen_ly, &xpos, &ypos, &width, &height);
		transit_y = -(height + ypos);
		elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
		elm_transit_duration_set(transit, 0.5);
		evas_object_show(screen_ly);
		elm_transit_objects_final_state_keep_set(transit, EINA_TRUE);
		elm_transit_go(transit);
	}
	return ECORE_CALLBACK_CANCEL;
}

static void __callui_view_active_incoming_call_reject_message_btn_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	if (data) {
		incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *) vd->priv;
		callui_app_data_t * ad = (callui_app_data_t *)evas_object_data_get(obj, "app_data");
		if (ad) {
			evas_object_resize(ad->win, ad->root_w, ad->root_h);
		}
		__callui_view_active_incoming_call_reject_msg_create_glist(vd);
		elm_object_signal_emit(priv->contents, "big_main_ly", "main_incoming_active_call");
	}
}

static void __callui_view_active_incoming_call_reject_ms_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (data) {
		incoming_lock_view_priv_t *prev = (incoming_lock_view_priv_t *) data;
		callui_app_data_t * ad = (callui_app_data_t *)evas_object_data_get(obj, "app_data");
		if (ad) {
			evas_object_resize(ad->win, ad->root_w, ELM_SCALE_SIZE(MTLOCK_ACTIVE_CALL_HEIGHT));
		}
		elm_object_signal_emit(prev->contents, "small_main_ly", "main_incoming_active_call");
		evas_object_del(prev->msg_glist);
	}
}

static void __callui_view_active_incoming_call_reject_msg_create_glist(void *data)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	CALLUI_RETURN_IF_FAIL(vd);
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	CALLUI_RETURN_IF_FAIL(priv);
	callui_app_data_t *ad = _callui_get_app_data();
	Elm_Object_Item *item = NULL;
	int msg_cnt = 0;
	int index = 0;

	if (0 != vconf_get_int(VCONFKEY_CISSAPPL_REJECT_CALL_MSG_INT, &msg_cnt)) {
		warn("vconf_get_int failed.");
	}

	priv->reject_msg_gl = _callui_load_edj(ad->win, EDJ_NAME, "reject_msg_ly");
	elm_object_part_content_set(priv->contents, "swallow.reject_msg", priv->reject_msg_gl);
	priv->msg_glist = elm_genlist_add(priv->contents);
	CALLUI_RETURN_IF_FAIL(priv->msg_glist);
	//elm_genlist_realization_mode_set(priv->msg_glist, EINA_TRUE);
	evas_object_data_set(priv->reject_msg_gl, "app_data", ad);
	eext_object_event_callback_add(priv->reject_msg_gl, EEXT_CALLBACK_BACK, __callui_view_active_incoming_call_reject_ms_back_cb, (void *)priv);

	evas_object_data_set(priv->msg_glist, REJ_MSG_GENLIST_DATA, (const void *)vd);
	evas_object_size_hint_weight_set(priv->msg_glist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(priv->msg_glist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_part_content_set(priv->reject_msg_gl, "swallow.content", priv->msg_glist);

	priv->itc_reject_msg = _callui_view_incoming_lock_create_item_class();

	if (msg_cnt == 0) {
		index = -1;
		item = _callui_view_incoming_lock_append_genlist_item(priv->msg_glist, priv->itc_reject_msg, index);
		if (item) {
			elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		}
	} else {
		for (index = 0; index < msg_cnt; index++) {
			_callui_view_incoming_lock_append_genlist_item(priv->msg_glist, priv->itc_reject_msg, index);
		}
	}
	elm_genlist_item_class_free(priv->itc_reject_msg);
	priv->itc_reject_msg = NULL;

	_callui_view_incoming_lock_create_reject_msg_button(priv->reject_msg_gl, "swallow.button", data);

	elm_object_tree_focus_allow_set(priv->msg_glist, EINA_FALSE);
	evas_object_size_hint_min_set(priv->msg_glist, ad->root_w, 0);
	evas_object_size_hint_max_set(priv->msg_glist, ad->root_w, ELM_SCALE_SIZE(MTLOCK_REJECT_MSG_LIST_1ITEM_NEW_HEIGHT * msg_cnt));
	evas_object_show(priv->reject_msg_gl);
	evas_object_show(priv->msg_glist);
}

static void __callui_view_active_incoming_call_app_control_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	char *key = (char *) data;
	if (key == NULL) {
		return;
	}
	app_control_h app_control = NULL;
	if (app_control_create(&app_control) != APP_CONTROL_ERROR_NONE) {
		dbg("app_control_create() is failed");
	} else if (app_control_set_app_id(app_control, "org.tizen.call-ui") != APP_CONTROL_ERROR_NONE) {
		dbg("app_control_set_app_id() is failed");
	} else if (app_control_set_operation(app_control, key) != APP_CONTROL_ERROR_NONE) {
		dbg("app_control_set_operation() is failed");
	} else if (app_control_send_launch_request(app_control, NULL, NULL)  != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed");
	}

	app_control_destroy(app_control);
}

int _callui_view_active_incoming_call_oncreate(call_view_data_t *view_data, void *appdata)
{
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *) view_data->priv;

	priv->contents = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
	if (priv->contents) {
		priv->caller_info = elm_object_part_content_get(priv->contents, "caller_info");
		if (priv->caller_info) {
			evas_object_del(priv->caller_info);
			priv->caller_info = NULL;
		}

		Evas_Object *btn_ly = elm_object_part_content_get(priv->contents, "btn_region");
		if (btn_ly) {
			evas_object_del(btn_ly);
			btn_ly = NULL;
		}
		evas_object_del(priv->contents);
		priv->contents = NULL;
	}
	if (ad->main_ly) {
		evas_object_resize(ad->win, ad->root_w, ELM_SCALE_SIZE(MTLOCK_ACTIVE_CALL_HEIGHT));
		priv->contents = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
		if (!priv->contents) {
			priv->contents = _callui_load_edj(ad->main_ly, EDJ_NAME,  "main_incoming_active_call");
			elm_object_part_content_set(ad->main_ly, "elm.swallow.content", priv->contents);
		}
		evas_object_show(priv->contents);
		evas_object_show(ad->main_ly);
	}
	return 0;
}

void _callui_view_active_incoming_call_create_button(callui_app_data_t *ad, char *icon_name, char *part, button_cb func, void * data, void * data_bt_cb)
{
	incoming_lock_view_priv_t * priv = (incoming_lock_view_priv_t *)data;
	if (priv) {
		Evas_Object *button_call = elm_layout_add(priv->contents);
		if (button_call) {
			elm_layout_file_set(button_call, EDJ_NAME, "main_button_ly");
			Evas_Object *icon = elm_image_add(button_call);
			elm_image_file_set(icon, EDJ_NAME, icon_name);
			elm_object_part_content_set(button_call, "swallow.icon", icon);
			elm_object_part_content_set(priv->contents, part, button_call);
			evas_object_data_set(button_call, "app_data", ad);
			evas_object_event_callback_add(button_call, EVAS_CALLBACK_MOUSE_UP, func, data_bt_cb);
		}
	}
}

void _callui_view_active_incoming_call_draw_screen(callui_app_data_t *ad, call_view_data_t *vd)
{
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	call_data_t *call_data = NULL;

	call_data = ad->incom;
	if (call_data == NULL) {
		err("call data is null");
		return;
	}
	elm_object_signal_emit(priv->contents, "small_main_ly", "main_incoming_active_call");

	_callui_view_active_incoming_call_create_button(ad, DURING_ICON, "swallow.call_button",
			__callui_view_active_incoming_call_app_control_cb, priv, (void *)APP_CONTROL_OPERATION_DURING_CALL);
	_callui_view_active_incoming_call_create_button(ad, REJECT_ICON, "swallow.rj_msg_button",
			__callui_view_active_incoming_call_reject_message_btn_cb, priv, (void *)vd);
	_callui_view_active_incoming_call_create_button(ad, END_ICON, "swallow.end_call_button",
			__callui_view_active_incoming_call_app_control_cb, priv, (void *)APP_CONTROL_OPERATION_END_CALL);

	Evas_Object *swipe_button = elm_layout_add(priv->contents);
	if (swipe_button) {
		elm_layout_file_set(swipe_button, EDJ_NAME, "swipe_call_button_ly");
		evas_object_event_callback_add(swipe_button, EVAS_CALLBACK_MOUSE_MOVE, __callui_view_active_incoming_call_mouse_move_cb, (void *)priv);
		evas_object_data_set(swipe_button, "app_data", ad);
	}
	elm_object_part_content_set(priv->contents, "swallow.swipe_button", swipe_button);

	char *call_name = call_data->call_ct_info.call_disp_name;
	char *call_number = NULL;
	if (call_data->call_disp_num[0] != '\0') {
		call_number = call_data->call_disp_num;
	} else {
		call_number = call_data->call_num;
	}
	char *file_path = call_data->call_ct_info.caller_id_path;

	if (!(call_name && call_name[0] != '\0') && !(call_number && call_number[0] != '\0')) {
		elm_object_signal_emit(priv->contents, "big_buttons", "main_incoming_active_call");
		elm_object_part_text_set(priv->contents, "text.contact_name", _("IDS_CALL_BODY_UNKNOWN"));
	} else if (!(call_name && call_name[0] != '\0')) {
		elm_object_signal_emit(priv->contents, "small_buttons", "main_incoming_active_call");
		elm_object_part_text_set(priv->contents, "text.contact_name", call_number);
	} else {
		elm_object_signal_emit(priv->contents, "small_buttons", "main_incoming_active_call");
		elm_object_part_text_set(priv->contents, "text.contact_name", call_name);
		elm_object_part_text_set(priv->contents, "text.contact_number", call_number);
	}

	if (strcmp(file_path, "default") != 0) {
		_callui_show_caller_id(priv->contents, file_path);
	} else {
		elm_object_signal_emit(priv->contents, "show_image", "main_incoming_active_call");
	}
}
