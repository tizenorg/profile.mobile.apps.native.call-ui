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
#include "callui-view-elements.h"
#include "callui-view-quickpanel.h"
#include <minicontrol-provider.h>
#include <minicontrol-internal.h>
#include "callui-view-layout-wvga.h"
#include "callui-common.h"
#include <app_control.h>
#include <bundle.h>

#define VIEW_QUICKPANEL_LAYOUT_ID "QUICKPANELVIEW"
#define MSG_DOMAIN_CONTROL_ACCESS (int)ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL
#define TXT_TIMER_BUF_LEN 26
#define CALL_NUMBER_ONE 1

struct callui_view_qp_priv {
	Evas_Object *caller_id;
	int rotate_angle;
	//Ecore_Event_Handler *client_msg_handler;
};
static Evas_Object *__callui_view_qp_create_contents(void *data, char *group);
static int __callui_view_qp_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *param3);
static int __callui_view_qp_onupdate(call_view_data_t *view_data, void *update_data1);
static int __callui_view_qp_onhide(call_view_data_t *view_data);
static int __callui_view_qp_onshow(call_view_data_t *view_data, void *appdata);
static int __callui_view_qp_ondestroy(call_view_data_t *view_data);
static Eina_Bool __callui_qp_client_message_cb(void *data, int type, void *event);

call_view_data_t *_callui_view_qp_new(callui_app_data_t *ad)
{
	static call_view_data_t qp_view = {
	.type = VIEW_QUICKPANEL_VIEW,
	.layout = NULL,
	.onCreate = __callui_view_qp_oncreate,
	.onUpdate = __callui_view_qp_onupdate,
	.onHide = __callui_view_qp_onhide,
	.onShow = __callui_view_qp_onshow,
	.onDestroy = __callui_view_qp_ondestroy,
	.onRotate = NULL,
	.priv = NULL,
	};
	qp_view.priv = calloc(1, sizeof(callui_view_qp_priv_t));
	if (!qp_view.priv) {
		err("ERROR!!!!!!!!!!! ");
	}
	return &qp_view;
};

void _callui_view_quickpanel_change(void)
{
	dbg("..");

	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);

	call_view_data_t *view_data = _callvm_get_call_view_data(ad, VIEW_QUICKPANEL_VIEW);

	if (ad->win_quickpanel) {
		if (!(ad->held) && !(ad->active) && (!ad->incom)) {
			dbg("destroy quickpanel");
			_callui_view_qp_hide(ad);
			_callvm_reset_call_view_data(ad, VIEW_QUICKPANEL_VIEW);
			ad->win_quickpanel = NULL;
		} else if (view_data && view_data->layout != NULL) {
			dbg("update quickpanel");
			__callui_view_qp_onupdate(view_data, NULL);
		}
	} else {
		dbg("create quickpanel");
		if (view_data == NULL) {
			view_data = _callui_view_qp_new(ad);
			CALLUI_RETURN_IF_FAIL(view_data);
			_callvm_set_call_view_data(ad, VIEW_QUICKPANEL_VIEW, view_data);
		}
		if (view_data->layout == NULL) {
			__callui_view_qp_oncreate(view_data, 0, NULL, ad);
		}
	}
}

// TODO ecore x atom actions are not supported. Need to move on event from mini controller.
/*
static Eina_Bool __callui_qp_client_message_cb(void *data, int type, void *event)
{
	dbg("__callui_qp_client_message_cb");
	int new_angle = 0;
	Ecore_X_Event_Client_Message *ev = (Ecore_X_Event_Client_Message *) event;
	call_view_data_t *vd = (call_view_data_t *)data;
	callui_view_qp_priv_t *priv = (callui_view_qp_priv_t *)vd->priv;

	if ((ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE)
		|| (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE)) {
		new_angle = ev->data.l[0];
		dbg("ROTATION: %d", new_angle);
		priv->rotate_angle = new_angle;
		__callui_view_qp_onshow(vd, NULL);
	}

	return ECORE_CALLBACK_RENEW;
}
*/
static void __callui_view_qp_caller_id_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");

	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad != NULL);

	_callui_view_qp_hide(ad);
	if (ad->incom) {
		int ret = -1;
		ret = cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_NORMAL);
		if (ret != CM_ERROR_NONE) {
			err("cm_end_call() is failed");
		}
	}
	/***to do***after lcd changes**/
/*	if (ad->b_lcd_on == EINA_FALSE) {
		return;
	}*/

/*	if (_callui_common_get_idle_lock_type() == LOCK_TYPE_SWIPE_LOCK) {
		vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);
	}*//**to do**/
/*	if (_callui_get_idle_lock_type() == CALL_LOCK)
		vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);*/

	elm_win_activate(ad->win);
}

static void __callui_qp_launch_top_view_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad != NULL);
	_callui_view_qp_hide(data);

	app_control_h request;
	app_control_create(&request);
	app_control_set_app_id(request, PACKAGE);
	int err = app_control_send_launch_request(request, NULL, NULL);
	if (err != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() failed(0x%x)", err);
		return;
	}
	app_control_destroy(request);
	ad->on_background = false;
	_callui_lock_manager_start(ad->lock_handle);
}

static void __callui_qp_spk_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad != NULL);
	int ret = -1;

	if (ad->speaker_status == EINA_TRUE) {
		ret = cm_speaker_off(ad->cm_handle);
		if (ret != CM_ERROR_NONE) {
			err("cm_speaker_off() is failed");
			return;
		}
	} else {
		ret = cm_speaker_on(ad->cm_handle);
		if (ret != CM_ERROR_NONE) {
			err("cm_speaker_off() is failed");
			return;
		}
	}
	_callui_update_speaker_btn(ad, !ad->speaker_status);
}

static void __callui_qp_resume_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad != NULL);
	int ret = -1;

	if (ad->held != NULL) {
		ret = cm_unhold_call(ad->cm_handle);
		if (ret != CM_ERROR_NONE) {
			err("cm_unhold_call() is failed");
		}
	} else {
		ret = cm_hold_call(ad->cm_handle);
		if (ret != CM_ERROR_NONE) {
			err("cm_hold_call() is failed");
		}
	}
}

Evas_Object *_callui_create_quickpanel_resume_button(void *data, Eina_Bool bdisable)
{
	dbg("..");
	Evas_Object *btn = NULL;
	Evas_Object *layout = NULL;
	CALLUI_RETURN_NULL_IF_FAIL(data);
	callui_app_data_t *ad = (callui_app_data_t *)data;

	layout = ad->quickpanel_layout;
	CALLUI_RETURN_NULL_IF_FAIL(layout);

	btn = edje_object_part_swallow_get(_EDJ(layout), "swallow.resume_button");
	if (btn) {
		sec_dbg("Object Already Exists, so Update Only");
		evas_object_smart_callback_del(btn, "clicked", __callui_qp_resume_btn_cb);
	} else {
		sec_dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.resume_button", btn);
	}

	if (ad->held != NULL) {
		elm_object_style_set(btn, "style_call_icon_only_qp_resume");
		evas_object_smart_callback_add(btn, "clicked", __callui_qp_resume_btn_cb, ad);
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_resume_on");
		evas_object_smart_callback_add(btn, "clicked", __callui_qp_resume_btn_cb, ad);
	}
	evas_object_propagate_events_set(btn, EINA_FALSE);

	return btn;
}

Evas_Object *_callui_create_quickpanel_speaker_button(void *data, Eina_Bool bdisable)
{
	dbg("..");
	Evas_Object *btn, *layout, *sw;
	CALLUI_RETURN_NULL_IF_FAIL(data);
	callui_app_data_t *ad = (callui_app_data_t *)data;

	layout = ad->quickpanel_layout;
	CALLUI_RETURN_NULL_IF_FAIL(layout);

	sw = edje_object_part_swallow_get(_EDJ(layout), "swallow.speaker_button");
	if (sw) {
		sec_dbg("Object Already Exists, so Update Only");
		btn = sw;
		evas_object_smart_callback_del(btn, "clicked", __callui_qp_spk_btn_cb);
	} else {
		sec_dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.speaker_button", btn);
	}

	if (ad->speaker_status == EINA_FALSE) {
		elm_object_style_set(btn, "style_call_icon_only_qp_speaker");
		evas_object_smart_callback_add(btn, "clicked", __callui_qp_spk_btn_cb, ad);
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_speaker_on");
		evas_object_smart_callback_add(btn, "clicked", __callui_qp_spk_btn_cb, ad);
	}
	evas_object_propagate_events_set(btn, EINA_FALSE);

	elm_object_disabled_set(btn, bdisable);
	return btn;
}

static Evas_Object *__callui_create_quickpanel_call_button(void *data)
{
	Evas_Object *layout, *sw;
	Evas_Object *btn = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)data;

	layout = ad->quickpanel_layout;
	CALLUI_RETURN_NULL_IF_FAIL(layout);
	sw = edje_object_part_swallow_get(_EDJ(layout), "swallow.call_button");
	if (sw) {
		dbg("Object Already Exists, so Update Only");
		btn = sw;
		evas_object_smart_callback_del(btn, "clicked", __callui_view_qp_caller_id_cb);
	} else {
		dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.call_button", btn);
	}

	elm_object_style_set(btn, "style_call_icon_only_qp_call");

	evas_object_smart_callback_add(btn, "clicked", __callui_view_qp_caller_id_cb, ad);
	evas_object_propagate_events_set(btn, EINA_FALSE);

	return btn;
}

static void __qp_end_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");

	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad != NULL);

	int ret = -1;
	_callui_view_qp_hide(ad);
	if (ad->incom) {
		ret = cm_reject_call(ad->cm_handle);
	} else if (CM_CALL_STATE_DIALING == ad->active->call_state)/*(dialling_view)*/ {
		ret = cm_end_call(ad->cm_handle, ad->active->call_id, CALL_RELEASE_TYPE_BY_CALL_HANDLE);
	} else if ((ad->active) && (ad->held)) {
		ret = cm_end_call(ad->cm_handle, 0, CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS);
	} else if ((ad->active) || (ad->held)) {/*single call*/
		ret = cm_end_call(ad->cm_handle, 0, CALL_RELEASE_TYPE_ALL_CALLS);
	} else {
		err("invalid case!!!!");
	}
	if (ret != CM_ERROR_NONE) {
		err("cm_end_call() is failed");
		return;
	}
}

static Evas_Object *__callui_create_quickpanel_end_button(void *data)
{
	Evas_Object *btn, *layout, *sw;
	callui_app_data_t *ad = (callui_app_data_t *)data;

	layout = ad->quickpanel_layout;
	CALLUI_RETURN_NULL_IF_FAIL(layout);
	sw = edje_object_part_swallow_get(_EDJ(layout), "swallow.end_button");
	if (sw) {
		dbg("Object Already Exists, so Update Only");
		btn = sw;
		evas_object_smart_callback_del(btn, "clicked", __qp_end_btn_cb);
	} else {
		dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.end_button", btn);
	}

	elm_object_style_set(btn, "style_call_icon_only_qp_end");

	evas_object_smart_callback_add(btn, "clicked", __qp_end_btn_cb, ad);
	evas_object_propagate_events_set(btn, EINA_FALSE);
	return btn;
}

Evas_Object *_callui_create_quickpanel_mute_button(void *data, Eina_Bool bdisable)
{
	dbg("..");
	Evas_Object *btn, *layout, *sw;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_NULL_IF_FAIL(ad);

	layout = ad->quickpanel_layout;
	CALLUI_RETURN_NULL_IF_FAIL(layout);

	sw = edje_object_part_swallow_get(_EDJ(layout), "swallow.mute_button");
	if (sw) {
		dbg("Object Already Exists, so Update Only");
		btn = sw;
		evas_object_smart_callback_del(btn, "clicked", _callui_mute_btn_cb);
	} else {
		dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.mute_button", btn);
	}

	if (ad->mute_status == EINA_FALSE) {
		elm_object_style_set(btn, "style_call_icon_only_qp_mute");
		evas_object_smart_callback_add(btn, "clicked", _callui_mute_btn_cb, ad);
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_mute_on");
		evas_object_smart_callback_add(btn, "clicked", _callui_mute_btn_cb, ad);
	}

	evas_object_propagate_events_set(btn, EINA_FALSE);
	elm_object_disabled_set(btn, bdisable);

	return btn;
}

void _callui_view_qp_hide(callui_app_data_t *ad)
{
	dbg("..");
	minicontrol_send_event(ad->win_quickpanel, MINICONTROL_EVENT_REQUEST_HIDE, NULL);
}

static void __callui_view_qp_update_caller(Evas_Object *eo, call_data_t *pcall_data)
{
	dbg("..");
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);

	if (pcall_data == NULL) {
		return;
	}

	char *call_name = pcall_data->call_ct_info.call_disp_name;
	char *file_path = pcall_data->call_ct_info.caller_id_path;

	char *call_number = NULL;
	if (strlen(pcall_data->call_disp_num) > 0) {
		call_number = pcall_data->call_disp_num;
	} else {
		call_number = pcall_data->call_num;
	}

	if (strlen(call_name) == 0) {
		if (strlen(call_number) == 0) {
			edje_object_part_text_set(_EDJ(eo), "txt_call_name", _("IDS_CALL_BODY_UNKNOWN"));
		} else {
			edje_object_part_text_set(_EDJ(eo), "txt_call_name", call_number);
		}
	} else {
		char *convert_text = evas_textblock_text_utf8_to_markup(NULL, call_name);
		if (convert_text) {
			edje_object_part_text_set(_EDJ(eo), "txt_call_name", convert_text);
			free(convert_text);
			convert_text = NULL;
		} else {
			err("Convert text is NULL");
		}
	}

	if (strcmp(file_path, "default") != 0) {
		_callui_show_caller_id(eo, file_path);
	} else {
		elm_object_signal_emit(eo, "show_image", "");
	}
}

static void __callui_view_qp_update_screen(void *data, Evas_Object *eo, bool mute_state, char *ls_part, call_data_t *call_data)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);
	_callui_create_quickpanel_mute_button(ad, mute_state);
	__callui_view_qp_update_caller(eo, call_data);
	elm_object_signal_emit(eo, ls_part, "");
}

static void __callui_view_qp_draw_screen(Evas_Object *eo, void *data)
{
	dbg("..");
	call_data_t *call_data = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)data;

	Evas_Object *call_btn = __callui_create_quickpanel_call_button(ad);
	elm_object_disabled_set(call_btn, EINA_TRUE);
	_callui_create_quickpanel_speaker_button(ad, EINA_FALSE);

	if (ad->incom) {
		/**incoming call**/

		call_data = ad->incom;
		__callui_view_qp_update_screen(ad, eo, EINA_TRUE, "incoming_call", call_data);
		_callui_view_qp_update_text(_("IDS_CALL_BODY_INCOMING_CALL"), 0, eo);
		_callui_create_quickpanel_speaker_button(ad, EINA_TRUE);
		elm_object_disabled_set(call_btn, EINA_FALSE);
	} else if (ad->active && (CM_CALL_STATE_DIALING == ad->active->call_state)) {
		/**dialling**/

		call_data = ad->active;
		_callui_view_qp_update_text(_("IDS_CALL_POP_DIALLING"), 0, eo);
		__callui_view_qp_update_screen(ad, eo, EINA_TRUE, "outgoing_call", call_data);
		elm_object_signal_emit(eo, "outgoing_call", "");
	} else if ((ad->active) && (ad->held)) {
		/**split call**/

		call_data = ad->active;
		__callui_view_qp_update_screen(ad, eo, EINA_FALSE, "during_call", call_data);
		_callui_view_qp_update_text(NULL, ad->active->member_count, eo);
	} else if ((ad->active)) {
		/**active call**/

		call_data = ad->active;
		__callui_view_qp_update_screen(ad, eo, EINA_FALSE, "during_call", call_data);
		_callui_view_qp_update_text(NULL, ad->active->member_count, eo);
		_callui_common_update_call_duration(call_data->start_time);
	} else if ((ad->held)) {
		/**held call**/

		call_data = ad->held;
		__callui_view_qp_update_screen(ad, eo, EINA_TRUE, "resume_call", call_data);
		_callui_create_quickpanel_resume_button(ad, EINA_TRUE);
		_callui_view_qp_update_text(_("IDS_CALL_BODY_ON_HOLD_ABB"), ad->held->member_count, eo);
	} else {
		dbg("incoming call. error!");
	}
	__callui_create_quickpanel_end_button(ad);
}

static void __callui_view_qp_provider_cb(minicontrol_viewer_event_e event_type, bundle *event_arg)
{
	dbg("__callui_view_qp_viewer_cb %i", event_type);
	char *angle = NULL;
	if (event_arg) {
		bundle_get_str(event_arg, "angle", &angle);
	}

	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);
	callui_view_qp_priv_t *priv = (callui_view_qp_priv_t *)ad->view_data->priv;
	if (angle && angle[0] != '/0') {
		priv->rotate_angle = atoi(angle);
	}
	__callui_view_qp_onshow(ad->view_data, NULL);
}

static Evas_Object *__callui_view_qp_create_window(callui_app_data_t *priv)
{
	Evas_Object *win;
	win = minicontrol_create_window("org.tizen.call-ui", MINICONTROL_TARGET_VIEWER_QUICK_PANEL, __callui_view_qp_provider_cb);
	elm_win_alpha_set(win, EINA_TRUE);
	evas_object_resize(win, ELM_SCALE_SIZE(MAIN_SCREEN_BIG_W), ELM_SCALE_SIZE(QP_WIN_H));

	if (elm_win_wm_rotation_supported_get(win)) {
		int rotate_angles[3] = {0, 90, 270};
		/*Set the required angles wherein the rotation has to be supported*/
		elm_win_wm_rotation_available_rotations_set(win, rotate_angles, 3);
	}
	return win;
}

static Evas_Object *__callui_view_qp_create_layout_main(Evas_Object *parent)
{
	if (parent == NULL) {
		dbg("ERROR");
		return NULL;
	}

	Evas_Object *ly;
	ly = elm_layout_add(parent);

	if (ly == NULL) {
		err("Failed elm_layout_add.");
		return NULL;
	}

	elm_layout_theme_set(ly, "layout", "application", "default");
	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(parent, ly);

	edje_object_signal_emit(_EDJ(ly), "elm,state,show,content", "elm");

	return ly;
}

static Evas_Object *__callui_view_qp_create_contents(void *data, char *group)
{
	if (data == NULL) {
		dbg("ERROR");
		return NULL;
	}
	callui_app_data_t *ad = _callui_get_app_data();
	Evas_Object *eo = NULL;

	/* load edje */
	eo = _callui_load_edj(ad->win_quickpanel, EDJ_NAME, group);

	if (eo == NULL)
		return NULL;

	return eo;
}

static int __callui_view_qp_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *appdata)
{
	dbg("quickpanel view create!!");

	callui_app_data_t *ad = (callui_app_data_t *) appdata;
	callui_view_qp_priv_t *priv = (callui_view_qp_priv_t *)view_data->priv;
	ad->view_data = view_data;
	if (!view_data->layout) {
		ad->win_quickpanel = __callui_view_qp_create_window(priv);
		if (ad->win_quickpanel == NULL) {
			dbg("ERROR");
			return -1;
		}
		priv->rotate_angle = elm_win_rotation_get(ad->win_quickpanel);
		dbg("current rotate angle(%d)", priv->rotate_angle);
		view_data->layout = __callui_view_qp_create_layout_main(ad->win_quickpanel);
		if (view_data->layout == NULL) {
			dbg("ERROR");
			return -1;
		}
	}

	//TODO Ecore x is not supported. Need to implement handling of rotation change events from qp
	/*if (priv->client_msg_handler == NULL)
		priv->client_msg_handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, __callui_qp_client_message_cb, view_data);
		*/

	__callui_view_qp_onshow(view_data, NULL);
	return 0;
}

static int __callui_view_qp_onupdate(call_view_data_t *view_data, void *update_data1)
{
	dbg("quickpanel view update!!");

	__callui_view_qp_onshow(view_data, NULL);
	return 0;
}

static int __callui_view_qp_onhide(call_view_data_t *view_data)
{
	dbg("quickpanel view hide!!");

	evas_object_hide(view_data->layout);
	return 0;
}

static int __callui_view_qp_onshow(call_view_data_t *view_data, void *appdata)
{
	dbg("quickpanel view show!!");
	callui_app_data_t *ad = _callui_get_app_data();
	callui_view_qp_priv_t *priv = (callui_view_qp_priv_t *)view_data->priv;
	if (ad->quickpanel_layout) {
		evas_object_event_callback_del(ad->quickpanel_layout, EVAS_CALLBACK_MOUSE_UP, __callui_qp_launch_top_view_cb);
		evas_object_del(ad->quickpanel_layout);
		ad->quickpanel_layout = NULL;
	}

	if (priv->rotate_angle == 0 || priv->rotate_angle == 180) {
		dbg("portrait mode layout");
		ad->landscape = false;
		evas_object_resize(ad->win_quickpanel, ELM_SCALE_SIZE(MAIN_SCREEN_BIG_W), ELM_SCALE_SIZE(QP_WIN_H));
		ad->quickpanel_layout = __callui_view_qp_create_contents(view_data, GRP_QUICKPANEL);
	} else if (priv->rotate_angle == 90 || priv->rotate_angle == 270) {
		dbg("landscape mode layout");
		ad->landscape = true;
		evas_object_resize(ad->win_quickpanel, ELM_SCALE_SIZE(MAIN_SCREEN_BIG_H), ELM_SCALE_SIZE(QP_WIN_H));
		ad->quickpanel_layout = __callui_view_qp_create_contents(view_data, GRP_QUICKPANEL_LS);
	}

	elm_object_part_content_set(view_data->layout, "elm.swallow.content", ad->quickpanel_layout);
	evas_object_name_set(ad->quickpanel_layout, VIEW_QUICKPANEL_LAYOUT_ID);
	dbg("[========== QUICKPANEL:ad->quickpanel_layout Addr : [%p] ==========]", ad->quickpanel_layout);
	evas_object_event_callback_add(ad->quickpanel_layout, EVAS_CALLBACK_MOUSE_UP, __callui_qp_launch_top_view_cb, ad);

	__callui_view_qp_draw_screen(ad->quickpanel_layout, ad);
	evas_object_show(ad->win_quickpanel);
	evas_object_show(ad->quickpanel_layout);
	evas_object_show(view_data->layout);

	/* Prohibit remove of mini control */

	// TODO MINICONTROL_EVENT_REQUEST_LOCK is not supported
	//minicontrol_send_event(ad->win_quickpanel, MINICONTROL_EVENT_REQUEST_LOCK, NULL);

	return 0;
}

static int __callui_view_qp_ondestroy(call_view_data_t *view_data)
{
	dbg("quickpanel view destroy!!");

	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_VALUE_IF_FAIL(ad, -1);
	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_QUICKPANEL_VIEW);
	CALLUI_RETURN_VALUE_IF_FAIL(vd, -1);

	callui_view_qp_priv_t *priv = (callui_view_qp_priv_t *)vd->priv;

	if (priv != NULL) {
		if (priv->caller_id) {
			evas_object_del(priv->caller_id);
			priv->caller_id = NULL;
		}

		//ecore_event_handler_del(priv->client_msg_handler);

		free(priv);
		priv = NULL;
	}

	if (ad->quickpanel_layout) {
		evas_object_del(ad->quickpanel_layout);
		ad->quickpanel_layout = NULL;
	}

	if (vd->layout != NULL) {
		evas_object_hide(vd->layout);
		evas_object_del(vd->layout);
		vd->layout = NULL;
	}

	_callvm_reset_call_view_data(ad, VIEW_QUICKPANEL_VIEW);

	if (ad->win_quickpanel) {
		evas_object_del(ad->win_quickpanel);
		ad->win_quickpanel = NULL;
	}

	return 0;
}

void _callui_view_qp_update_text(char *txt_status, int count, Evas_Object *eo)
{
	CALLUI_RETURN_IF_FAIL(eo);
	if (txt_status != NULL) {
		edje_object_part_text_set(_EDJ(eo), "txt_timer", txt_status);
	}
	if (count > 1) {
		edje_object_part_text_set(_EDJ(eo), "txt_call_name", _("IDS_CALL_OPT_CONFERENCE_CALL"));
	}

}

void _callui_view_qp_set_call_timer(Evas_Object *qp_layout, char *pcall_timer)
{
	CALLUI_RETURN_IF_FAIL(qp_layout);
	callui_app_data_t *ad  = _callui_get_app_data();
	if (!ad) {
		dbg("ad is NULL!!!");
		return;
	}

	if (ad->active) {
		if (ad->held) {
			char buf[TXT_TIMER_BUF_LEN] = {0};
			char buf_tmp[TXT_TIMER_BUF_LEN] = {0};
			snprintf(buf_tmp, sizeof(buf_tmp), "%s / %s", pcall_timer, _("IDS_CALL_BODY_PD_ON_HOLD_M_STATUS_ABB"));
			snprintf(buf, sizeof(buf), buf_tmp, CALL_NUMBER_ONE);
			edje_object_part_text_set(_EDJ(qp_layout), "txt_timer", buf);
		} else {
			edje_object_part_text_set(_EDJ(qp_layout), "txt_timer", _(pcall_timer));
		}
	}
}
