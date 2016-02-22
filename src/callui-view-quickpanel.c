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

#include <minicontrol-provider.h>
#include <minicontrol-internal.h>
#include <app_control.h>
#include <bundle.h>

#include "callui.h"
#include "callui-view-elements.h"
#include "callui-view-quickpanel.h"
#include "callui-view-layout.h"
#include "callui-common.h"

#define TXT_TIMER_BUF_LEN 26
#define CALL_NUMBER_ONE 1
#define QP_WIN_H 172

struct _callui_qp_mc {
	Evas_Object *win_quickpanel;
	Evas_Object *quickpanel_layout;
	Evas_Object *caller_id;
	bool is_activated;
	int rotate_angle;
	callui_app_data_t *ad;
	//Ecore_Event_Handler *client_msg_handler;
};
typedef struct _callui_qp_mc callui_qp_mc_t;

static int  __callui_qp_mc_activate(callui_qp_mc_h qp);
static void __callui_qp_mc_deactivate(callui_qp_mc_h qp);
static void __callui_qp_mc_refresh(callui_qp_mc_h qp);
static void __callui_qp_mc_hide(callui_qp_mc_h qp);
static void __callui_qp_mc_update_text(char *txt_status, int count, Evas_Object *eo);

static void __callui_qp_mc_caller_id_cb(void *data, Evas_Object *obj, void *event_info);
static void __callui_qp_mc_launch_top_view_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void __callui_qp_mc_resume_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void __callui_qp_mc_end_btn_cb(void *data, Evas_Object *obj, void *event_info);

static Evas_Object *__callui_qp_mc_create_resume_btn(callui_qp_mc_h qp);
static Evas_Object *__callui_qp_mc_create_call_btn(callui_qp_mc_h qp);
static Evas_Object *__callui_qp_mc_create_end_btn(callui_qp_mc_h qp);

static void __callui_qp_mc_update_caller(Evas_Object *eo, call_data_t *call_data);
static void __callui_qp_mc_update_comp_status(callui_qp_mc_h qp,
		Evas_Object *eo,
		bool mute_state,
		char *ls_part,
		call_data_t *call_data);

static void __callui_qp_mc_draw_screen(callui_qp_mc_h qp);
static void __callui_qp_mc_provider_cb(minicontrol_viewer_event_e event_type, bundle *event_arg);
static Evas_Object *__callui_qp_mc_create_window();
static Evas_Object *__callui_qp_mc_create_contents(callui_qp_mc_h qp, char *group);

// TODO ecore x atom actions are not supported. Need to move on event from mini controller.
//static Eina_Bool __callui_qp_mc_client_message_cb(void *data, int type, void *event);

callui_qp_mc_h _callui_qp_mc_create(callui_app_data_t *ad)
{
	CALLUI_RETURN_NULL_IF_FAIL(ad);

	callui_qp_mc_h qp = calloc(1, sizeof(callui_qp_mc_t));

	CALLUI_RETURN_NULL_IF_FAIL(qp);

	qp->ad = ad;

	return qp;
};

void _callui_qp_mc_destroy(callui_qp_mc_h qp)
{
	CALLUI_RETURN_IF_FAIL(qp);

	__callui_qp_mc_deactivate(qp);

	free(qp);
}

// TODO ecore x atom actions are not supported. Need to move on event from mini controller.
/*
static Eina_Bool __callui_qp_mc_client_message_cb(void *data, int type, void *event)
{
	dbg("__callui_qp_mc_client_message_cb");
	int new_angle = 0;
	Ecore_X_Event_Client_Message *ev = (Ecore_X_Event_Client_Message *) event;
	callui_qp_mc_h qp = (callui_qp_mc_h)data;

	if ((ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE)
		|| (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE)) {
		new_angle = ev->data.l[0];
		dbg("ROTATION: %d", new_angle);
		qp->rotate_angle = new_angle;
		__callui_qp_mc_refresh(qp);
	}

	return ECORE_CALLBACK_RENEW;
}
*/

static void __callui_qp_mc_caller_id_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;

	CALLUI_RETURN_IF_FAIL(qp->ad);

	callui_app_data_t *ad = qp->ad;

	__callui_qp_mc_hide(qp);

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

static void __callui_qp_mc_launch_top_view_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;

	CALLUI_RETURN_IF_FAIL(qp->ad);

	callui_app_data_t *ad = qp->ad;
	__callui_qp_mc_hide(qp);

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

static void __callui_qp_mc_resume_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_qp_mc_h qp = (callui_qp_mc_h)data;
	CALLUI_RETURN_IF_FAIL(qp->ad);
	callui_app_data_t *ad = qp->ad;

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

static Evas_Object *__callui_qp_mc_create_resume_btn(callui_qp_mc_h qp)
{
	CALLUI_RETURN_NULL_IF_FAIL(qp);
	CALLUI_RETURN_NULL_IF_FAIL(qp->quickpanel_layout);
	CALLUI_RETURN_NULL_IF_FAIL(qp->ad);

	Evas_Object *btn = NULL;
	Evas_Object *layout = qp->quickpanel_layout;
	callui_app_data_t *ad = qp->ad;

	btn = edje_object_part_swallow_get(_EDJ(layout), "swallow.resume_button");
	if (btn) {
		sec_dbg("Object Already Exists, so Update Only");
		evas_object_smart_callback_del(btn, "clicked", __callui_qp_mc_resume_btn_cb);
	} else {
		sec_dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.resume_button", btn);
	}

	if (ad->held != NULL) {
		elm_object_style_set(btn, "style_call_icon_only_qp_resume");
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_resume_on");
	}
	evas_object_smart_callback_add(btn, "clicked", __callui_qp_mc_resume_btn_cb, qp);
	evas_object_propagate_events_set(btn, EINA_FALSE);

	return btn;
}

void _callui_qp_mc_update_speaker_status(callui_qp_mc_h qp, Eina_Bool is_disable)
{
	CALLUI_RETURN_IF_FAIL(qp);
	CALLUI_RETURN_IF_FAIL(qp->quickpanel_layout);
	CALLUI_RETURN_IF_FAIL(qp->ad);

	Evas_Object *btn, *layout, *sw;
	layout = qp->quickpanel_layout;
	callui_app_data_t *ad = qp->ad;

	sw = edje_object_part_swallow_get(_EDJ(layout), "swallow.speaker_button");
	if (sw) {
		sec_dbg("Object Already Exists, so Update Only");
		btn = sw;
		evas_object_smart_callback_del(btn, "clicked", _callui_spk_btn_cb);
	} else {
		sec_dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.speaker_button", btn);
	}

	if (ad->speaker_status == EINA_FALSE) {
		elm_object_style_set(btn, "style_call_icon_only_qp_speaker");
		evas_object_smart_callback_add(btn, "clicked", _callui_spk_btn_cb, ad);
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_speaker_on");
		evas_object_smart_callback_add(btn, "clicked", _callui_spk_btn_cb, ad);
	}
	evas_object_propagate_events_set(btn, EINA_FALSE);

	elm_object_disabled_set(btn, is_disable);
}

static Evas_Object *__callui_qp_mc_create_call_btn(callui_qp_mc_h qp)
{
	CALLUI_RETURN_NULL_IF_FAIL(qp);

	Evas_Object *layout, *sw;
	Evas_Object *btn = NULL;
	layout = qp->quickpanel_layout;

	sw = edje_object_part_swallow_get(_EDJ(layout), "swallow.call_button");
	if (sw) {
		dbg("Object Already Exists, so Update Only");
		btn = sw;
		evas_object_smart_callback_del(btn, "clicked", __callui_qp_mc_caller_id_cb);
	} else {
		dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.call_button", btn);
	}

	elm_object_style_set(btn, "style_call_icon_only_qp_call");

	evas_object_smart_callback_add(btn, "clicked", __callui_qp_mc_caller_id_cb, qp);
	evas_object_propagate_events_set(btn, EINA_FALSE);

	return btn;
}

static void __callui_qp_mc_end_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;

	CALLUI_RETURN_IF_FAIL(qp->ad);

	callui_app_data_t *ad = qp->ad;

	int ret = -1;
	__callui_qp_mc_hide(qp);

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

static Evas_Object *__callui_qp_mc_create_end_btn(callui_qp_mc_h qp)
{
	CALLUI_RETURN_NULL_IF_FAIL(qp);
	CALLUI_RETURN_NULL_IF_FAIL(qp->quickpanel_layout);

	Evas_Object *btn, *layout, *sw;
	layout = qp->quickpanel_layout;

	sw = edje_object_part_swallow_get(_EDJ(layout), "swallow.end_button");
	if (sw) {
		dbg("Object Already Exists, so Update Only");
		btn = sw;
		evas_object_smart_callback_del(btn, "clicked", __callui_qp_mc_end_btn_cb);
	} else {
		dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.end_button", btn);
	}

	elm_object_style_set(btn, "style_call_icon_only_qp_end");

	evas_object_smart_callback_add(btn, "clicked", __callui_qp_mc_end_btn_cb, qp);
	evas_object_propagate_events_set(btn, EINA_FALSE);
	return btn;
}

void _callui_qp_mc_update_mute_status(callui_qp_mc_h qp, Eina_Bool is_disable)
{
	CALLUI_RETURN_IF_FAIL(qp);
	CALLUI_RETURN_IF_FAIL(qp->quickpanel_layout);
	CALLUI_RETURN_IF_FAIL(qp->ad);

	Evas_Object *btn, *layout, *sw;
	layout = qp->quickpanel_layout;
	callui_app_data_t *ad = qp->ad;

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
	elm_object_disabled_set(btn, is_disable);
}

static void __callui_qp_mc_hide(callui_qp_mc_h qp)
{
	minicontrol_send_event(qp->win_quickpanel, MINICONTROL_PROVIDER_EVENT_REQUEST_HIDE, NULL);
}

static void __callui_qp_mc_update_caller(Evas_Object *eo, call_data_t *call_data)
{
	CALLUI_RETURN_IF_FAIL(eo);
	CALLUI_RETURN_IF_FAIL(call_data);

	char *call_name = call_data->call_ct_info.call_disp_name;
	char *file_path = call_data->call_ct_info.caller_id_path;

	char *call_number = NULL;
	if (strlen(call_data->call_disp_num) > 0) {
		call_number = call_data->call_disp_num;
	} else {
		call_number = call_data->call_num;
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

static void __callui_qp_mc_update_comp_status(callui_qp_mc_h qp,
		Evas_Object *eo,
		bool mute_state,
		char *ls_part,
		call_data_t *call_data)
{
	CALLUI_RETURN_IF_FAIL(qp);

	_callui_qp_mc_update_mute_status(qp, mute_state);
	__callui_qp_mc_update_caller(eo, call_data);
	elm_object_signal_emit(eo, ls_part, "");
}

static void __callui_qp_mc_draw_screen(callui_qp_mc_h qp)
{
	CALLUI_RETURN_IF_FAIL(qp);
	CALLUI_RETURN_IF_FAIL(qp->quickpanel_layout);
	CALLUI_RETURN_IF_FAIL(qp->ad);

	call_data_t *call_data = NULL;
	callui_app_data_t *ad = qp->ad;
	Evas_Object *eo = qp->quickpanel_layout;

	Evas_Object *call_btn = __callui_qp_mc_create_call_btn(qp);
	elm_object_disabled_set(call_btn, EINA_TRUE);
	_callui_qp_mc_update_speaker_status(qp, EINA_FALSE);

	if (ad->incom) {
		/**incoming call**/

		call_data = ad->incom;
		__callui_qp_mc_update_comp_status(qp, eo, EINA_TRUE, "incoming_call", call_data);
		__callui_qp_mc_update_text(_("IDS_CALL_BODY_INCOMING_CALL"), 0, eo);
		_callui_qp_mc_update_speaker_status(qp, EINA_TRUE);
		elm_object_disabled_set(call_btn, EINA_FALSE);
	} else if (ad->active && (CM_CALL_STATE_DIALING == ad->active->call_state)) {
		/**dialling**/

		call_data = ad->active;
		__callui_qp_mc_update_text(_("IDS_CALL_POP_DIALLING"), 0, eo);
		__callui_qp_mc_update_comp_status(qp, eo, EINA_TRUE, "outgoing_call", call_data);
		elm_object_signal_emit(eo, "outgoing_call", "");
	} else if ((ad->active) && (ad->held)) {
		/**split call**/

		call_data = ad->active;
		__callui_qp_mc_update_comp_status(qp, eo, EINA_FALSE, "during_call", call_data);
		__callui_qp_mc_update_text(NULL, ad->active->member_count, eo);
	} else if ((ad->active)) {
		/**active call**/

		call_data = ad->active;
		__callui_qp_mc_update_comp_status(qp, eo, EINA_FALSE, "during_call", call_data);
		__callui_qp_mc_update_text(NULL, ad->active->member_count, eo);
		_callui_common_update_call_duration(call_data->start_time);
	} else if ((ad->held)) {
		/**held call**/

		call_data = ad->held;
		__callui_qp_mc_update_comp_status(qp, eo, EINA_TRUE, "resume_call", call_data);
		__callui_qp_mc_create_resume_btn(qp);
		__callui_qp_mc_update_text(_("IDS_CALL_BODY_ON_HOLD_ABB"), ad->held->member_count, eo);
	} else {
		dbg("incoming call. error!");
	}
	__callui_qp_mc_create_end_btn(qp);
}

static void __callui_qp_mc_provider_cb(minicontrol_viewer_event_e event_type, bundle *event_arg)
{
	dbg("__callui_view_qp_viewer_cb %i", event_type);
	char *angle = NULL;
	if (event_arg) {
		bundle_get_str(event_arg, "angle", &angle);
	}

	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);

	callui_qp_mc_h qp = ad->qp_minicontrol;
	if (angle && angle[0] != '\0') {
		qp->rotate_angle = atoi(angle);
	}
	__callui_qp_mc_refresh(qp);
}

static Evas_Object *__callui_qp_mc_create_window()
{
	dbg("..");
	Evas_Object *win = minicontrol_create_window("org.tizen.call-ui",
			MINICONTROL_TARGET_VIEWER_QUICK_PANEL,
			__callui_qp_mc_provider_cb);

	evas_object_resize(win, ELM_SCALE_SIZE(MAIN_SCREEN_W), ELM_SCALE_SIZE(QP_WIN_H));

	if (elm_win_wm_rotation_supported_get(win)) {
		int rotate_angles[3] = {0, 90, 270};
		/*Set the required angles wherein the rotation has to be supported*/
		elm_win_wm_rotation_available_rotations_set(win, rotate_angles, 3);
	}

	return win;
}

static Evas_Object *__callui_qp_mc_create_contents(callui_qp_mc_h qp, char *group)
{
	CALLUI_RETURN_NULL_IF_FAIL(qp);

	return _callui_load_edj(qp->win_quickpanel, EDJ_NAME, group);
}

static int __callui_qp_mc_activate(callui_qp_mc_h qp)
{
	CALLUI_RETURN_VALUE_IF_FAIL(qp, CALLUI_RESULT_INVALID_PARAM);

	if (!qp->win_quickpanel) {
		qp->win_quickpanel = __callui_qp_mc_create_window();
		if (qp->win_quickpanel == NULL) {
			err("__callui_qp_mc_create_window FAILED!");
			return CALLUI_RESULT_FAIL;
		}

		qp->rotate_angle = elm_win_rotation_get(qp->win_quickpanel);
		dbg("current rotate angle(%d)", qp->rotate_angle);

		qp->quickpanel_layout = __callui_qp_mc_create_contents(qp, GRP_QUICKPANEL);
		if (qp->quickpanel_layout == NULL) {
			err("__callui_qp_mc_create_contents FAILED!");
			return CALLUI_RESULT_FAIL;
		}

		elm_win_resize_object_add(qp->win_quickpanel, qp->quickpanel_layout);
		evas_object_event_callback_add(qp->quickpanel_layout, EVAS_CALLBACK_MOUSE_UP, __callui_qp_mc_launch_top_view_cb, qp);
	}

	//TODO Ecore x is not supported. Need to implement handling of rotation change events from qp
	/*if (qp->client_msg_handler == NULL)
		qp->client_msg_handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, __callui_qp_mc_client_message_cb, qp);
		*/

	__callui_qp_mc_refresh(qp);

	return CALLUI_RESULT_OK;
}

static void __callui_qp_mc_refresh(callui_qp_mc_h qp)
{
	CALLUI_RETURN_IF_FAIL(qp);

	if (qp->rotate_angle == 0 || qp->rotate_angle == 180) {
		dbg("portrait mode layout");
		evas_object_resize(qp->win_quickpanel, ELM_SCALE_SIZE(MAIN_SCREEN_W), ELM_SCALE_SIZE(QP_WIN_H));
	} else if (qp->rotate_angle == 90 || qp->rotate_angle == 270) {
		dbg("landscape mode layout");
		evas_object_resize(qp->win_quickpanel, ELM_SCALE_SIZE(MAIN_SCREEN_H), ELM_SCALE_SIZE(QP_WIN_H));
	}

	__callui_qp_mc_draw_screen(qp);

	evas_object_show(qp->win_quickpanel);
	evas_object_show(qp->quickpanel_layout);

	/* Prohibit remove of mini control */
	minicontrol_send_event(qp->win_quickpanel, MINICONTROL_EVENT_REQUEST_LOCK, NULL);
}

static void __callui_qp_mc_deactivate(callui_qp_mc_h qp)
{
	CALLUI_RETURN_IF_FAIL(qp);

	if (qp != NULL) {
		if (qp->caller_id) {
			evas_object_del(qp->caller_id);
		}
		//ecore_event_handler_del(qp->client_msg_handler);
	}

	if (qp->quickpanel_layout) {
		evas_object_event_callback_del_full(qp->quickpanel_layout, EVAS_CALLBACK_MOUSE_UP, __callui_qp_mc_launch_top_view_cb, qp);
		evas_object_del(qp->quickpanel_layout);
		qp->quickpanel_layout = NULL;
	}

	if (qp->win_quickpanel) {
		evas_object_del(qp->win_quickpanel);
		qp->win_quickpanel = NULL;
	}
}

static void __callui_qp_mc_update_text(char *txt_status, int count, Evas_Object *eo)
{
	CALLUI_RETURN_IF_FAIL(eo);

	if (txt_status != NULL) {
		edje_object_part_text_set(_EDJ(eo), "txt_timer", txt_status);
	}
	if (count > 1) {
		edje_object_part_text_set(_EDJ(eo), "txt_call_name", _("IDS_CALL_OPT_CONFERENCE_CALL"));
	}
}

void _callui_qp_mc_update_calltime_status(callui_qp_mc_h qp, char *call_timer)
{
	CALLUI_RETURN_IF_FAIL(qp);

	callui_app_data_t *ad  = qp->ad;
	Evas_Object *qp_layout = qp->quickpanel_layout;

	if (!ad) {
		dbg("ad is NULL!!!");
		return;
	}

	if (ad->active) {
		if (ad->held) {
			char buf[TXT_TIMER_BUF_LEN] = {0};
			char buf_tmp[TXT_TIMER_BUF_LEN] = {0};
			snprintf(buf_tmp, sizeof(buf_tmp), "%s / %s", call_timer, _("IDS_CALL_BODY_PD_ON_HOLD_M_STATUS_ABB"));
			snprintf(buf, sizeof(buf), buf_tmp, CALL_NUMBER_ONE);
			edje_object_part_text_set(_EDJ(qp_layout), "txt_timer", buf);
		} else {
			edje_object_part_text_set(_EDJ(qp_layout), "txt_timer", _(call_timer));
		}
	}
}

void _callui_qp_mc_update(callui_qp_mc_h qp)
{
	debug_enter();
	int res;

	if (!qp->is_activated) {
		res = __callui_qp_mc_activate(qp);
		CALLUI_RETURN_IF_FAIL(res == CALLUI_RESULT_OK);
		qp->is_activated = true;
	}

	__callui_qp_mc_refresh(qp);

	debug_leave();
}

