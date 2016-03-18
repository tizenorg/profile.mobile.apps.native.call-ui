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
#include "callui-sound-manager.h"
#include "callui-state-provider.h"

#define TXT_TIMER_BUF_LEN 26
#define CALL_NUMBER_ONE 1
#define QP_WIN_H 172
#define TIME_BUF_LEN 16

struct _callui_qp_mc {
	Evas_Object *win_quickpanel;
	Evas_Object *quickpanel_layout;
	Evas_Object *caller_id;
	bool is_activated;
	int rotate_angle;
	callui_app_data_t *ad;

	Ecore_Timer *call_duration_timer;
	struct tm *call_duration_tm;

	//Ecore_Event_Handler *client_msg_handler;
};
typedef struct _callui_qp_mc callui_qp_mc_t;

static callui_result_e __callui_qp_mc_init(callui_qp_mc_h qp, callui_app_data_t *ad);
static void __callui_qp_mc_deinit(callui_qp_mc_h qp);

static callui_result_e  __callui_qp_mc_activate(callui_qp_mc_h qp);
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

static void __callui_qp_mc_update_caller(Evas_Object *eo, const callui_call_state_data_t *call_data);
static void __callui_qp_mc_update_comp_status(callui_qp_mc_h qp,
		Evas_Object *eo,
		bool mute_state,
		char *ls_part,
		const callui_call_state_data_t *call_data);

static void __callui_qp_mc_draw_screen(callui_qp_mc_h qp);
static void __callui_qp_mc_provider_cb(minicontrol_viewer_event_e event_type, bundle *event_arg);
static Evas_Object *__callui_qp_mc_create_window();
static Evas_Object *__callui_qp_mc_create_contents(callui_qp_mc_h qp, char *group);

static void __callui_qp_set_split_call_duration_time(struct tm *time, Evas_Object *obj, const char *txt_part);

static Eina_Bool __split_call_duration_timer_cb(void *data);
static Eina_Bool __active_call_duration_timer_cb(void* data);

static void __call_state_event_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type);

// TODO ecore x atom actions are not supported. Need to move on event from mini controller.
//static Eina_Bool __callui_qp_mc_client_message_cb(void *data, int type, void *event);

static void __call_state_event_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type)
{
	CALLUI_RETURN_IF_FAIL(user_data);
	callui_qp_mc_h qp = (callui_qp_mc_h)user_data;

	if (!_callui_stp_is_any_calls_available(qp->ad->call_stp)) {
		__callui_qp_mc_deactivate(qp);
		return;
	}

	if (!qp->is_activated) {
		callui_result_e res = __callui_qp_mc_activate(qp);
		CALLUI_RETURN_IF_FAIL(res == CALLUI_RESULT_OK);
	} else {
		__callui_qp_mc_refresh(qp);
	}
}

static callui_result_e __callui_qp_mc_init(callui_qp_mc_h qp, callui_app_data_t *ad)
{
	qp->ad = ad;

	callui_result_e res = _callui_stp_add_call_state_event_cb(ad->call_stp, __call_state_event_cb, qp);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	const callui_call_state_data_t *active = _callui_stp_get_call_data(ad->call_stp, CALLUI_CALL_DATA_TYPE_ACTIVE);
	const callui_call_state_data_t *incom = _callui_stp_get_call_data(ad->call_stp, CALLUI_CALL_DATA_TYPE_INCOMING);
	const callui_call_state_data_t *held = _callui_stp_get_call_data(ad->call_stp, CALLUI_CALL_DATA_TYPE_HELD);

	if (active || incom || held) {
		res = __callui_qp_mc_activate(qp);
	}
	return res;
}

static void __callui_qp_mc_deinit(callui_qp_mc_h qp)
{
	callui_app_data_t *ad = qp->ad;

	__callui_qp_mc_deactivate(qp);

	_callui_stp_remove_call_state_event_cb(ad->call_stp, __call_state_event_cb, qp);
}

callui_qp_mc_h _callui_qp_mc_create(callui_app_data_t *ad)
{
	CALLUI_RETURN_NULL_IF_FAIL(ad);

	callui_qp_mc_h qp = calloc(1, sizeof(callui_qp_mc_t));
	CALLUI_RETURN_NULL_IF_FAIL(qp);

	callui_result_e res = __callui_qp_mc_init(qp, ad);
	if (res != CALLUI_RESULT_OK) {
		FREE(qp);
	}
	return qp;
};

void _callui_qp_mc_destroy(callui_qp_mc_h qp)
{
	CALLUI_RETURN_IF_FAIL(qp);

	__callui_qp_mc_deinit(qp);

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

	const callui_call_state_data_t *call_data = _callui_stp_get_call_data(ad->call_stp,
					CALLUI_CALL_DATA_TYPE_INCOMING);

	if (call_data) {
		callui_result_e ret = _callui_manager_answer_call(ad->call_manager, CALLUI_CALL_ANSWER_TYPE_NORMAL);
		if (ret != CALLUI_RESULT_OK) {
			err("_callui_manager_answer_call() is failed");
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

	const callui_call_state_data_t *call_data =
			_callui_stp_get_call_data(ad->call_stp, CALLUI_CALL_DATA_TYPE_HELD);

	callui_result_e res = CALLUI_RESULT_FAIL;
	if (call_data) {
		res = _callui_manager_unhold_call(ad->call_manager);
		if (res != CALLUI_RESULT_OK) {
			err("_callui_manager_unhold_call() failed. res[%d]", res);
		}
	} else {
		res = _callui_manager_hold_call(ad->call_manager);
		if (res != CALLUI_RESULT_OK) {
			err("_callui_manager_hold_call() failed. res[%d]", res);
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

	const callui_call_state_data_t *held = _callui_stp_get_call_data(ad->call_stp, CALLUI_CALL_DATA_TYPE_HELD);
	if (held) {
		elm_object_style_set(btn, "style_call_icon_only_qp_resume");
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_resume_on");
	}
	evas_object_smart_callback_add(btn, "clicked", __callui_qp_mc_resume_btn_cb, qp);
	evas_object_propagate_events_set(btn, EINA_FALSE);

	return btn;
}


static void __speaker_btn_audio_st_changed_cb(void *user_data, callui_audio_state_type_e state)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	Evas_Object *btn = (Evas_Object *)user_data;
	if (state == CALLUI_AUDIO_STATE_SPEAKER) {
		elm_object_style_set(btn, "style_call_icon_only_qp_speaker_on");
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_speaker");
	}
}

static void __speaker_btn_button_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;
	_callui_sdm_remove_audio_state_changed_cb(ad->call_sdm, __speaker_btn_audio_st_changed_cb, obj);
}

static void __callui_qp_mc_create_update_speaker_btn(callui_qp_mc_h qp, Eina_Bool is_disable)
{
	Evas_Object *btn = NULL;
	Evas_Object *layout = qp->quickpanel_layout;
	callui_app_data_t *ad = qp->ad;

	Evas_Object *sw = edje_object_part_swallow_get(_EDJ(layout), "swallow.speaker_button");
	if (sw) {
		sec_dbg("Object Already Exists, so Update Only");
		btn = sw;
	} else {
		sec_dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.speaker_button", btn);
	}

	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->call_sdm);
	if (audio_state != CALLUI_AUDIO_STATE_SPEAKER) {
		elm_object_style_set(btn, "style_call_icon_only_qp_speaker");
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_speaker_on");
	}

	evas_object_smart_callback_del_full(btn, "clicked", _callui_spk_btn_cb, ad);
	evas_object_smart_callback_add(btn, "clicked", _callui_spk_btn_cb, ad);

	evas_object_event_callback_del_full(btn, EVAS_CALLBACK_DEL, __speaker_btn_button_del_cb, ad);
	evas_object_event_callback_add(btn, EVAS_CALLBACK_DEL, __speaker_btn_button_del_cb, ad);

	_callui_sdm_remove_audio_state_changed_cb(ad->call_sdm, __speaker_btn_audio_st_changed_cb, btn);
	_callui_sdm_add_audio_state_changed_cb(ad->call_sdm, __speaker_btn_audio_st_changed_cb, btn);

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
	callui_app_data_t *ad = qp->ad;

	__callui_qp_mc_hide(qp);

	const callui_call_state_data_t *active =
			_callui_stp_get_call_data(ad->call_stp, CALLUI_CALL_DATA_TYPE_ACTIVE);
	const callui_call_state_data_t *held =
			_callui_stp_get_call_data(ad->call_stp, CALLUI_CALL_DATA_TYPE_HELD);
	const callui_call_state_data_t *incom =
			_callui_stp_get_call_data(ad->call_stp, CALLUI_CALL_DATA_TYPE_INCOMING);

	callui_result_e res = CALLUI_RESULT_FAIL;
	if (incom) {
		res = _callui_manager_reject_call(ad->call_manager);
	} else if (active && active->is_dialing) {
		res = _callui_manager_end_call(ad->call_manager, active->call_id, CALLUI_CALL_RELEASE_TYPE_BY_CALL_HANDLE);
	} else if (active && held) {
		res = _callui_manager_end_call(ad->call_manager, 0, CALLUI_CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS);
	} else if (active || held) {/*single call*/
		res = _callui_manager_end_call(ad->call_manager, 0, CALLUI_CALL_RELEASE_TYPE_ALL_CALLS);
	} else {
		err("invalid case!!!!");
	}

	if (res != CALLUI_RESULT_OK) {
		err("__callui_qp_mc_end_btn_cb() failed. res[%d]", res);
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

static void __mute_btn_mute_st_changed_cb(void *user_data, bool is_enable)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	Evas_Object *btn = (Evas_Object *)user_data;
	if (is_enable) {
		elm_object_style_set(btn, "style_call_icon_only_qp_mute_on");
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_mute");
	}
}

static void __mute_btn_button_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;
	_callui_sdm_remove_mute_state_changed_cb(ad->call_sdm, __mute_btn_mute_st_changed_cb, obj);
}

static void __callui_qp_mc_create_update_mute_btn(callui_qp_mc_h qp, Eina_Bool is_disable)
{
	Evas_Object *btn = NULL;
	Evas_Object *layout = qp->quickpanel_layout;
	callui_app_data_t *ad = qp->ad;

	Evas_Object *sw = edje_object_part_swallow_get(_EDJ(layout), "swallow.mute_button");
	if (sw) {
		dbg("Object Already Exists, so Update Only");
		btn = sw;
	} else {
		dbg("Object Doesn't Exists, so Re-Create");
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "swallow.mute_button", btn);
	}

	if (_callui_sdm_get_mute_state(ad->call_sdm)) {
		elm_object_style_set(btn, "style_call_icon_only_qp_mute_on");
	} else {
		elm_object_style_set(btn, "style_call_icon_only_qp_mute");
	}

	evas_object_smart_callback_del_full(btn, "clicked", _callui_mute_btn_cb, ad);
	evas_object_smart_callback_add(btn, "clicked", _callui_mute_btn_cb, ad);

	evas_object_event_callback_del_full(btn, EVAS_CALLBACK_DEL, __mute_btn_button_del_cb, ad);
	evas_object_event_callback_add(btn, EVAS_CALLBACK_DEL, __mute_btn_button_del_cb, ad);

	_callui_sdm_remove_mute_state_changed_cb(ad->call_sdm, __mute_btn_mute_st_changed_cb, btn);
	_callui_sdm_add_mute_state_changed_cb(ad->call_sdm, __mute_btn_mute_st_changed_cb, btn);

	evas_object_propagate_events_set(btn, EINA_FALSE);

	elm_object_disabled_set(btn, is_disable);
}

static void __callui_qp_mc_hide(callui_qp_mc_h qp)
{
	minicontrol_send_event(qp->win_quickpanel, MINICONTROL_PROVIDER_EVENT_REQUEST_HIDE, NULL);
}

static void __callui_qp_mc_update_caller(Evas_Object *eo, const callui_call_state_data_t *call_data)
{
	CALLUI_RETURN_IF_FAIL(eo);
	CALLUI_RETURN_IF_FAIL(call_data);

	const char *call_name = call_data->call_ct_info.call_disp_name;
	const char *file_path = call_data->call_ct_info.caller_id_path;

	const char *call_number = NULL;
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
		const callui_call_state_data_t *call_data)
{
	CALLUI_RETURN_IF_FAIL(qp);

	__callui_qp_mc_create_update_mute_btn(qp, mute_state);
	__callui_qp_mc_update_caller(eo, call_data);
	elm_object_signal_emit(eo, ls_part, "");
}

static void __callui_qp_set_split_call_duration_time(struct tm *time, Evas_Object *obj, const char *txt_part)
{
	char dur[TIME_BUF_LEN];
	if (time->tm_hour > 0) {
		snprintf(dur, TIME_BUF_LEN, "%02d:%02d:%02d", time->tm_hour, time->tm_min, time->tm_sec);
	} else {
		snprintf(dur, TIME_BUF_LEN, "%02d:%02d", time->tm_min, time->tm_sec);
	}

	char buf[TXT_TIMER_BUF_LEN] = {0};
	char buf_tmp[TXT_TIMER_BUF_LEN] = {0};
	snprintf(buf_tmp, sizeof(buf_tmp), "%s / %s", dur, _("IDS_CALL_BODY_PD_ON_HOLD_M_STATUS_ABB"));
	snprintf(buf, sizeof(buf), buf_tmp, CALL_NUMBER_ONE);

	elm_object_part_text_set(obj, txt_part, buf);
}

static Eina_Bool __split_call_duration_timer_cb(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;

	struct tm *new_tm = _callui_stp_get_call_duration(qp->ad->call_stp, CALLUI_CALL_DATA_TYPE_ACTIVE);
	if (!new_tm) {
		qp->call_duration_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	_callui_common_try_update_call_duration_time(qp->call_duration_tm,
			new_tm,
			__callui_qp_set_split_call_duration_time,
			qp->quickpanel_layout,
			"call_txt_status");

	free(new_tm);

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool __active_call_duration_timer_cb(void* data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;

	struct tm *new_tm = _callui_stp_get_call_duration(qp->ad->call_stp, CALLUI_CALL_DATA_TYPE_ACTIVE);
	if (!new_tm) {
		qp->call_duration_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	_callui_common_try_update_call_duration_time(qp->call_duration_tm,
			new_tm,
			_callui_common_set_call_duration_time,
			qp->quickpanel_layout,
			"call_txt_status");

	free(new_tm);

	return ECORE_CALLBACK_RENEW;
}

static void __callui_qp_mc_draw_screen(callui_qp_mc_h qp)
{
	CALLUI_RETURN_IF_FAIL(qp);
	CALLUI_RETURN_IF_FAIL(qp->quickpanel_layout);
	CALLUI_RETURN_IF_FAIL(qp->ad);

	callui_app_data_t *ad = qp->ad;
	Evas_Object *eo = qp->quickpanel_layout;

	Evas_Object *call_btn = __callui_qp_mc_create_call_btn(qp);
	elm_object_disabled_set(call_btn, EINA_TRUE);
	__callui_qp_mc_create_update_speaker_btn(qp, EINA_FALSE);

	const callui_call_state_data_t *incom = _callui_stp_get_call_data(ad->call_stp,
			CALLUI_CALL_DATA_TYPE_INCOMING);
	const callui_call_state_data_t *active = _callui_stp_get_call_data(ad->call_stp,
			CALLUI_CALL_DATA_TYPE_ACTIVE);
	const callui_call_state_data_t *held = _callui_stp_get_call_data(ad->call_stp,
			CALLUI_CALL_DATA_TYPE_HELD);

	DELETE_ECORE_TIMER(qp->call_duration_timer);
	FREE(qp->call_duration_tm);

	if (incom) {
		/* Incoming call */
		__callui_qp_mc_update_comp_status(qp, eo, EINA_TRUE, "incoming_call", incom);
		__callui_qp_mc_update_text(_("IDS_CALL_BODY_INCOMING_CALL"), 0, eo);
		__callui_qp_mc_create_update_speaker_btn(qp, EINA_TRUE);
		elm_object_disabled_set(call_btn, EINA_FALSE);
	} else if (active && active->is_dialing) {
		/* Dialing call */
		__callui_qp_mc_update_text(_("IDS_CALL_POP_DIALLING"), 0, eo);
		__callui_qp_mc_update_comp_status(qp, eo, EINA_TRUE, "outgoing_call", active);
		elm_object_signal_emit(eo, "outgoing_call", "");
	} else if (active && held) {
		/* Split call */
		__callui_qp_mc_update_comp_status(qp, eo, EINA_FALSE, "during_call", active);
		__callui_qp_mc_update_text(NULL, active->conf_member_count, eo);

		qp->call_duration_tm = _callui_stp_get_call_duration(ad->call_stp, CALLUI_CALL_DATA_TYPE_ACTIVE);
		CALLUI_RETURN_IF_FAIL(qp->call_duration_tm);

		__callui_qp_set_split_call_duration_time(qp->call_duration_tm, qp->quickpanel_layout, "txt_timer");

		qp->call_duration_timer = ecore_timer_add(0.1, __split_call_duration_timer_cb, qp);
		CALLUI_RETURN_IF_FAIL(qp->call_duration_timer);
	} else if (active) {
		/* Active call */
		__callui_qp_mc_update_comp_status(qp, eo, EINA_FALSE, "during_call", active);
		__callui_qp_mc_update_text(NULL, active->conf_member_count, eo);

		qp->call_duration_tm = _callui_stp_get_call_duration(ad->call_stp, CALLUI_CALL_DATA_TYPE_ACTIVE);
		CALLUI_RETURN_IF_FAIL(qp->call_duration_tm);

		_callui_common_set_call_duration_time(qp->call_duration_tm, qp->quickpanel_layout, "txt_timer");

		qp->call_duration_timer = ecore_timer_add(0.1, __active_call_duration_timer_cb, qp);
		CALLUI_RETURN_IF_FAIL(qp->call_duration_timer);
	} else if (held) {
		/* Held call */
		__callui_qp_mc_update_comp_status(qp, eo, EINA_TRUE, "resume_call", held);
		__callui_qp_mc_create_resume_btn(qp);
		__callui_qp_mc_update_text(_("IDS_CALL_BODY_ON_HOLD_ABB"), held->conf_member_count, eo);
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

static callui_result_e __callui_qp_mc_activate(callui_qp_mc_h qp)
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

	qp->is_activated = true;

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
		elm_object_part_text_set(eo, "txt_timer", txt_status);
	}
	if (count > 1) {
		elm_object_part_text_set(eo, "txt_call_name", _("IDS_CALL_OPT_CONFERENCE_CALL"));
	}
}
