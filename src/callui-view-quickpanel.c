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

#include "callui-view-quickpanel.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-view-elements.h"
#include "callui-view-layout.h"
#include "callui-common.h"
#include "callui-sound-manager.h"
#include "callui-state-provider.h"

#define CALLUI_GROUP_QUICKPANEL	"quickpanel"

#define CALLUI_BUF_TIMER_TXT_LEN	26
#define CALLUI_CALL_NUMBER_ONE		1

typedef enum {
	CALLUI_QP_BTN_CALL = 0,
	CALLUI_QP_BTN_RESUME,
	CALLUI_QP_BTN_MUTE,
	CALLUI_QP_BTN_SPEAKER,
	CALLUI_QP_BTN_END,
	CALLUI_QP_BTN_COUNT
} callui_qp_mc_btn_type_e;

struct _callui_qp_mc {
	Evas_Object *win_quickpanel;
	Evas_Object *quickpanel_layout;
	Evas_Object *caller_id;
	bool is_activated;
	int rotate_angle;
	callui_app_data_t *ad;

	Evas_Object *buttons[CALLUI_QP_BTN_COUNT];
	bool is_available[CALLUI_QP_BTN_COUNT];

	Ecore_Timer *call_duration_timer;
	struct tm *call_duration_tm;
};
typedef struct _callui_qp_mc callui_qp_mc_t;

struct __btn_style {
	char *normal;
	char *active;
};
typedef struct __btn_style __btn_style_t;

typedef void (*btn_update_func)(callui_qp_mc_h qp);

struct __btn_params {
	char *part;
	Evas_Smart_Cb click_cb_func;
	btn_update_func update_func;
	__btn_style_t style;
};
typedef struct __btn_params __btn_params_t;

static void __caller_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __resume_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __mute_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __speaker_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __end_btn_click_cb(void *data, Evas_Object *obj, void *event_info);

static void __update_call_btn(callui_qp_mc_h qp);
static void __update_resume_btn(callui_qp_mc_h qp);
static void __update_mute_btn(callui_qp_mc_h qp);
static void __update_speaker_btn(callui_qp_mc_h qp);

static __btn_params_t btn_params[CALLUI_QP_BTN_COUNT] = {
		{
				"swallow.call_button",__caller_btn_click_cb, __update_call_btn,
				{"callui_qp_btn_call", NULL}
		},
		{
				"swallow.resume_button", __resume_btn_click_cb, __update_resume_btn,
				{"callui_qp_btn_resume", "callui_qp_btn_resume_on"}
		},
		{
				"swallow.mute_button", __mute_btn_click_cb, __update_mute_btn,
				{"callui_qp_btn_mute", "callui_qp_btn_mute_on"}
		},
		{
				"swallow.speaker_button", __speaker_btn_click_cb, __update_speaker_btn,
				{"callui_qp_btn_speaker", "callui_qp_btn_speaker_on"}
		},
		{
				"swallow.end_button", __end_btn_click_cb, NULL,
				{"callui_qp_btn_end", NULL}
		}
};

static callui_result_e __callui_qp_mc_init(callui_qp_mc_h qp, callui_app_data_t *ad);
static void __callui_qp_mc_deinit(callui_qp_mc_h qp);

static callui_result_e __activate(callui_qp_mc_h qp);
static void __deactivate(callui_qp_mc_h qp);

static Evas_Object *__create_qp_btn(callui_qp_mc_h qp, callui_qp_mc_btn_type_e type);

static void __refresh_components(callui_qp_mc_h qp);
static void __hide_minicontrol(callui_qp_mc_h qp);
static void __update_text_components(char *txt_status, int count, Evas_Object *eo);
static void __main_layout_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void __update_caller_info(Evas_Object *eo, const callui_call_data_t *call_data);
static void __update_comp_status(callui_qp_mc_h qp,
		Evas_Object *eo,
		char *ls_part,
		const callui_call_data_t *call_data);

static void __update_layout_components(callui_qp_mc_h qp);
static void __minicontrol_provider_cb(minicontrol_viewer_event_e event_type, bundle *event_arg);
static Evas_Object *__create_window(callui_app_data_t *ad);

static void __set_split_call_duration_time(struct tm *time, Evas_Object *obj, const char *txt_part);

static void __init_split_call_duration_timer(callui_qp_mc_h qp);
static void __init_active_call_duration_timer(callui_qp_mc_h qp);
static void __deinit_call_duration_timer(callui_qp_mc_h qp);
static Eina_Bool __split_call_duration_timer_cb(void *data);
static Eina_Bool __active_call_duration_timer_cb(void* data);

static void __call_state_event_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info);

static void __audio_state_changed_cb(void *user_data, callui_audio_state_type_e state);
static void __mute_state_changed_cb(void *user_data, bool is_enable);

static void __update_all_btns_state(callui_qp_mc_h qp);
static void __update_all_btns(callui_qp_mc_h qp);

static void __call_state_event_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_qp_mc_h qp = (callui_qp_mc_h)user_data;

	if (!_callui_stp_is_any_calls_available(qp->ad->state_provider)) {
		__deactivate(qp);
		return;
	}

	if (!qp->is_activated) {
		callui_result_e res = __activate(qp);
		CALLUI_RETURN_IF_FAIL(res == CALLUI_RESULT_OK);
	} else {
		__refresh_components(qp);
	}
}

void __audio_state_changed_cb(void *user_data, callui_audio_state_type_e state)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_qp_mc_h qp = (callui_qp_mc_h)user_data;

	if (qp->is_activated) {
		__update_speaker_btn(qp);
	}
}

void __mute_state_changed_cb(void *user_data, bool is_enable)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_qp_mc_h qp = (callui_qp_mc_h)user_data;

	if (qp->is_activated) {
		__update_mute_btn(qp);
	}
}

static callui_result_e __callui_qp_mc_init(callui_qp_mc_h qp, callui_app_data_t *ad)
{
	qp->ad = ad;

	callui_result_e res = _callui_stp_add_call_state_event_cb(ad->state_provider, __call_state_event_cb, qp);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = _callui_sdm_add_mute_state_changed_cb(ad->sound_manager, __mute_state_changed_cb, qp);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = _callui_sdm_add_audio_state_changed_cb(ad->sound_manager, __audio_state_changed_cb, qp);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	if (_callui_stp_is_any_calls_available(ad->state_provider)) {
		res = __activate(qp);
	}
	return res;
}

static void __callui_qp_mc_deinit(callui_qp_mc_h qp)
{
	callui_app_data_t *ad = qp->ad;

	__deactivate(qp);

	_callui_stp_remove_call_state_event_cb(ad->state_provider, __call_state_event_cb, qp);
	_callui_sdm_remove_mute_state_changed_cb(ad->sound_manager, __mute_state_changed_cb, qp);
	_callui_sdm_remove_audio_state_changed_cb(ad->sound_manager, __audio_state_changed_cb, qp);
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

static void __caller_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;
	callui_app_data_t *ad = qp->ad;

	__hide_minicontrol(qp);

	const callui_call_data_t *call_data = _callui_stp_get_call_data(ad->state_provider,
					CALLUI_CALL_DATA_INCOMING);

	if (call_data) {
		callui_result_e ret = _callui_manager_answer_call(ad->call_manager, CALLUI_CALL_ANSWER_NORMAL);
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

	_callui_window_activate(ad->window);
}

static void __main_layout_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_qp_mc_h qp = data;

	app_control_h app_control = NULL;
	int ret;
	if ((ret = app_control_create(&app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_create() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_app_id(app_control, CALLUI_PACKAGE)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, CALLUI_APP_CONTROL_OPERATION_QP_RESUME)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed. ret[%d]", ret);
	} else {
		callui_app_data_t *ad = qp->ad;

		ad->on_background = false;
		_callui_lock_manager_start(ad->lock_handle);
		__hide_minicontrol(qp);
	}
	if (app_control) {
		app_control_destroy(app_control);
	}
}

static void __hide_minicontrol(callui_qp_mc_h qp)
{
	minicontrol_send_event(qp->win_quickpanel, MINICONTROL_PROVIDER_EVENT_REQUEST_HIDE, NULL);
}

static void __update_caller_info(Evas_Object *eo, const callui_call_data_t *call_data)
{
	CALLUI_RETURN_IF_FAIL(eo);
	CALLUI_RETURN_IF_FAIL(call_data);

	const char *call_name = call_data->call_ct_info.call_disp_name;
	const char *call_number = NULL;

	if (strlen(call_data->call_disp_num) > 0) {
		call_number = call_data->call_disp_num;
	} else {
		call_number = call_data->call_num;
	}

	if (strlen(call_name) == 0) {
		if (strlen(call_number) == 0) {
			elm_object_translatable_part_text_set(eo,
					"contact_name", "IDS_CALL_BODY_UNKNOWN");
		} else {
			elm_object_part_text_set(eo, "contact_name", call_number);
		}
	} else {
		char *convert_text = evas_textblock_text_utf8_to_markup(NULL, call_name);
		if (convert_text) {
			elm_object_part_text_set(eo, "contact_name", convert_text);
			free(convert_text);
			convert_text = NULL;
		} else {
			err("Convert text is NULL");
		}
	}

	CALLUI_RETURN_IF_FAIL(_callui_show_caller_id(eo, call_data));
}

static void __update_comp_status(callui_qp_mc_h qp,
		Evas_Object *eo,
		char *ls_part,
		const callui_call_data_t *call_data)
{
	CALLUI_RETURN_IF_FAIL(qp);

	__update_caller_info(eo, call_data);
	elm_object_signal_emit(eo, ls_part, "");
}

static void __set_split_call_duration_time(struct tm *time, Evas_Object *obj, const char *txt_part)
{
	char dur[CALLUI_BUFF_SIZE_TINY];
	if (time->tm_hour > 0) {
		snprintf(dur, CALLUI_BUFF_SIZE_TINY, "%02d:%02d:%02d", time->tm_hour, time->tm_min, time->tm_sec);
	} else {
		snprintf(dur, CALLUI_BUFF_SIZE_TINY, "%02d:%02d", time->tm_min, time->tm_sec);
	}

	char buf[CALLUI_BUF_TIMER_TXT_LEN] = {0};
	char buf_tmp[CALLUI_BUF_TIMER_TXT_LEN] = {0};
	snprintf(buf_tmp, sizeof(buf_tmp), "%s / %s", dur, _("IDS_CALL_BODY_PD_ON_HOLD_M_STATUS_ABB"));
	snprintf(buf, sizeof(buf), buf_tmp, CALLUI_CALL_NUMBER_ONE);

	elm_object_part_text_set(obj, txt_part, buf);
}

static Eina_Bool __split_call_duration_timer_cb(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;

	struct tm *new_tm = _callui_stp_get_call_duration(qp->ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	if (!new_tm) {
		qp->call_duration_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	_callui_common_try_update_call_duration_time(qp->call_duration_tm,
			new_tm,
			__set_split_call_duration_time,
			qp->quickpanel_layout,
			"txt_timer");

	free(new_tm);

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool __active_call_duration_timer_cb(void* data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;

	struct tm *new_tm = _callui_stp_get_call_duration(qp->ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	if (!new_tm) {
		qp->call_duration_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	_callui_common_try_update_call_duration_time(qp->call_duration_tm,
			new_tm,
			_callui_common_set_call_duration_time,
			qp->quickpanel_layout,
			"txt_timer");

	free(new_tm);

	return ECORE_CALLBACK_RENEW;
}

static void __init_split_call_duration_timer(callui_qp_mc_h qp)
{
	qp->call_duration_tm = _callui_stp_get_call_duration(qp->ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	CALLUI_RETURN_IF_FAIL(qp->call_duration_tm);
	__set_split_call_duration_time(qp->call_duration_tm, qp->quickpanel_layout, "txt_timer");

	qp->call_duration_timer = ecore_timer_add(0.1, __split_call_duration_timer_cb, qp);
	CALLUI_RETURN_IF_FAIL(qp->call_duration_timer);
}

static void __deinit_call_duration_timer(callui_qp_mc_h qp)
{
	DELETE_ECORE_TIMER(qp->call_duration_timer);
	FREE(qp->call_duration_tm);
}

static void __init_active_call_duration_timer(callui_qp_mc_h qp)
{
	qp->call_duration_tm = _callui_stp_get_call_duration(qp->ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	CALLUI_RETURN_IF_FAIL(qp->call_duration_tm);
	_callui_common_set_call_duration_time(qp->call_duration_tm, qp->quickpanel_layout, "txt_timer");

	qp->call_duration_timer = ecore_timer_add(0.1, __active_call_duration_timer_cb, qp);
	CALLUI_RETURN_IF_FAIL(qp->call_duration_timer);
}

static void __update_all_btns_state(callui_qp_mc_h qp)
{
	callui_app_data_t *ad = qp->ad;

	const callui_call_data_t *incom = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_INCOMING);
	const callui_call_data_t *active = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_ACTIVE);
	const callui_call_data_t *held = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_HELD);

	qp->is_available[CALLUI_QP_BTN_CALL] = false;
	qp->is_available[CALLUI_QP_BTN_SPEAKER] = true;
	qp->is_available[CALLUI_QP_BTN_RESUME] = false;
	qp->is_available[CALLUI_QP_BTN_MUTE] = false;

	if (incom) {
		qp->is_available[CALLUI_QP_BTN_CALL] = true;
		qp->is_available[CALLUI_QP_BTN_SPEAKER] = false;
	} else if (active && !active->is_dialing) {
		qp->is_available[CALLUI_QP_BTN_MUTE] = true;
	} else if (held) {
		qp->is_available[CALLUI_QP_BTN_RESUME] = true;
	}
}

static void __update_all_btns(callui_qp_mc_h qp)
{
	int i = 0;
	for (; i < CALLUI_QP_BTN_COUNT; i++) {
		if (btn_params[i].update_func) {
			btn_params[i].update_func(qp);
		}
	}
}

static void __update_layout_components(callui_qp_mc_h qp)
{
	callui_app_data_t *ad = qp->ad;
	Evas_Object *eo = qp->quickpanel_layout;

	__update_all_btns_state(qp);
	__update_all_btns(qp);

	__deinit_call_duration_timer(qp);

	const callui_call_data_t *incom = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_INCOMING);
	const callui_call_data_t *active = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_ACTIVE);
	const callui_call_data_t *held = _callui_stp_get_call_data(ad->state_provider,
			CALLUI_CALL_DATA_HELD);
	if (incom) {
		/* Incoming call */
		__update_comp_status(qp, eo, "incoming_call", incom);
		__update_text_components(_("IDS_CALL_BODY_INCOMING_CALL"), 0, eo);
	} else if (active && active->is_dialing) {
		/* Dialing call */
		__update_comp_status(qp, eo, "outgoing_call", active);
		__update_text_components(_("IDS_CALL_POP_DIALLING"), 0, eo);
	} else if (active && held) {
		/* Split call */
		__update_comp_status(qp, eo, "during_call", active);
		__update_text_components(NULL, active->conf_member_count, eo);
		__init_split_call_duration_timer(qp);
	} else if (active) {
		/* Active call */
		__update_comp_status(qp, eo, "during_call", active);
		__update_text_components(NULL, active->conf_member_count, eo);
		__init_active_call_duration_timer(qp);
	} else if (held) {
		/* Held call */
		__update_comp_status(qp, eo, "resume_call", held);
		__update_text_components(_("IDS_CALL_BODY_ON_HOLD_ABB"), held->conf_member_count, eo);
	} else {
		dbg("incoming call. error!");
	}
}

static void __minicontrol_provider_cb(minicontrol_viewer_event_e event_type, bundle *event_arg)
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
	__refresh_components(qp);
}

static Evas_Object *__create_window(callui_app_data_t *ad)
{
	dbg("..");
	Evas_Object *win = minicontrol_create_window("org.tizen.call-ui",
			MINICONTROL_TARGET_VIEWER_QUICK_PANEL,
			__minicontrol_provider_cb);

	evas_object_resize(win, ad->root_w, ELM_SCALE_SIZE(QP_WIN_H));

	if (elm_win_wm_rotation_supported_get(win)) {
		int rotate_angles[3] = {0, 90, 270};
		elm_win_wm_rotation_available_rotations_set(win, rotate_angles, 3);
	}

	return win;
}

static Evas_Object *__create_qp_btn(callui_qp_mc_h qp, callui_qp_mc_btn_type_e type)
{
	Evas_Object *btn = elm_button_add(qp->quickpanel_layout);
	CALLUI_RETURN_NULL_IF_FAIL(btn);
	elm_object_style_set(btn, btn_params[type].style.normal);
	elm_object_part_content_set(qp->quickpanel_layout,  btn_params[type].part, btn);
	evas_object_smart_callback_add(btn, "clicked", btn_params[type].click_cb_func, qp);
	evas_object_propagate_events_set(btn, EINA_FALSE);
	return btn;
}

static callui_result_e __activate(callui_qp_mc_h qp)
{
	CALLUI_RETURN_VALUE_IF_FAIL(qp, CALLUI_RESULT_INVALID_PARAM);

	if (!qp->win_quickpanel) {
		qp->win_quickpanel = __create_window(qp->ad);
		if (qp->win_quickpanel == NULL) {
			err("__create_window() FAILED!");
			return CALLUI_RESULT_FAIL;
		}

		qp->rotate_angle = elm_win_rotation_get(qp->win_quickpanel);
		dbg("current rotate angle(%d)", qp->rotate_angle);

		qp->quickpanel_layout = _callui_load_edj(qp->win_quickpanel, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_QUICKPANEL);
		if (qp->quickpanel_layout == NULL) {
			err("__callui_qp_mc_create_contents() FAILED!");
			return CALLUI_RESULT_FAIL;
		}

		elm_win_resize_object_add(qp->win_quickpanel, qp->quickpanel_layout);
		evas_object_event_callback_add(qp->quickpanel_layout, EVAS_CALLBACK_MOUSE_UP,
				__main_layout_mouse_up_cb, qp);


		int i = 0;
		for (; i < CALLUI_QP_BTN_COUNT; i++) {
			qp->buttons[i] = __create_qp_btn(qp, i);
			qp->is_available[i] = true;
		}
	}
	qp->is_activated = true;

	__refresh_components(qp);

	return CALLUI_RESULT_OK;
}

static void __refresh_components(callui_qp_mc_h qp)
{
	CALLUI_RETURN_IF_FAIL(qp);

	if (qp->rotate_angle == 0 || qp->rotate_angle == 180) {
		dbg("portrait mode layout");
		evas_object_resize(qp->win_quickpanel, qp->ad->root_w, ELM_SCALE_SIZE(QP_WIN_H));
	} else if (qp->rotate_angle == 90 || qp->rotate_angle == 270) {
		dbg("landscape mode layout");
		evas_object_resize(qp->win_quickpanel, qp->ad->root_h, ELM_SCALE_SIZE(QP_WIN_H));
	}

	__update_layout_components(qp);

	evas_object_show(qp->win_quickpanel);
	evas_object_show(qp->quickpanel_layout);

	/* Prohibit remove of mini control */
	minicontrol_send_event(qp->win_quickpanel, MINICONTROL_EVENT_REQUEST_LOCK, NULL);
}

static void __deactivate(callui_qp_mc_h qp)
{
	CALLUI_RETURN_IF_FAIL(qp);

	qp->is_activated = false;

	int i = 0;
	for (; i < CALLUI_QP_BTN_COUNT; i++) {
		evas_object_smart_callback_del_full(qp->buttons[i], "clicked", btn_params[i].click_cb_func, qp);
		qp->buttons[i] = NULL;
		qp->is_available[i] = true;
	}

	if (qp->caller_id) {
		evas_object_del(qp->caller_id);
	}

	if (qp->quickpanel_layout) {
		evas_object_event_callback_del_full(qp->quickpanel_layout, EVAS_CALLBACK_MOUSE_UP,
				__main_layout_mouse_up_cb, qp);
		evas_object_del(qp->quickpanel_layout);
		qp->quickpanel_layout = NULL;
	}

	if (qp->win_quickpanel) {
		evas_object_del(qp->win_quickpanel);
		qp->win_quickpanel = NULL;
	}
}

static void __update_text_components(char *txt_status, int count, Evas_Object *eo)
{
	CALLUI_RETURN_IF_FAIL(eo);

	if (txt_status != NULL) {
		elm_object_part_text_set(eo, "txt_timer", txt_status);
	}
	if (count > 1) {
		elm_object_translatable_part_text_set(eo,
				"contact_name", "IDS_CALL_OPT_CONFERENCE_CALL");
	}
}

static void __update_call_btn(callui_qp_mc_h qp)
{
	Evas_Object *btn = qp->buttons[CALLUI_QP_BTN_CALL];
	CALLUI_RETURN_IF_FAIL(btn);

	elm_object_disabled_set(btn, !qp->is_available[CALLUI_QP_BTN_CALL]);
}

static void __update_resume_btn(callui_qp_mc_h qp)
{
	Evas_Object *btn = qp->buttons[CALLUI_QP_BTN_RESUME];
	CALLUI_RETURN_IF_FAIL(btn);

	const callui_call_data_t *held =
			_callui_stp_get_call_data(qp->ad->state_provider, CALLUI_CALL_DATA_HELD);
	if (held) {
		elm_object_style_set(btn, btn_params[CALLUI_QP_BTN_RESUME].style.normal);
	} else {
		elm_object_style_set(btn, btn_params[CALLUI_QP_BTN_RESUME].style.active);
	}
	elm_object_disabled_set(btn, !qp->is_available[CALLUI_QP_BTN_RESUME]);
}

static void __update_speaker_btn(callui_qp_mc_h qp)
{
	Evas_Object *btn = qp->buttons[CALLUI_QP_BTN_SPEAKER];
	CALLUI_RETURN_IF_FAIL(btn);

	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(qp->ad->sound_manager);
	if (audio_state != CALLUI_AUDIO_STATE_SPEAKER) {
		elm_object_style_set(btn, btn_params[CALLUI_QP_BTN_SPEAKER].style.normal);
	} else {
		elm_object_style_set(btn, btn_params[CALLUI_QP_BTN_SPEAKER].style.active);
	}
	elm_object_disabled_set(btn, !qp->is_available[CALLUI_QP_BTN_SPEAKER]);
}

static void __update_mute_btn(callui_qp_mc_h qp)
{
	Evas_Object *btn = qp->buttons[CALLUI_QP_BTN_MUTE];
	CALLUI_RETURN_IF_FAIL(btn);

	if (_callui_sdm_get_mute_state(qp->ad->sound_manager)) {
		elm_object_style_set(btn, btn_params[CALLUI_QP_BTN_MUTE].style.active);
	} else {
		elm_object_style_set(btn, btn_params[CALLUI_QP_BTN_MUTE].style.normal);
	}
	elm_object_disabled_set(btn, !qp->is_available[CALLUI_QP_BTN_MUTE]);
}

static void __resume_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;
	callui_app_data_t *ad = qp->ad;

	const callui_call_data_t *call_data =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);

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

static void __speaker_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;
	callui_app_data_t *ad = qp->ad;

	callui_result_e res = CALLUI_RESULT_FAIL;
	callui_audio_state_type_e state = _callui_sdm_get_audio_state(ad->sound_manager);
	if (state != CALLUI_AUDIO_STATE_SPEAKER) {
		res = _callui_sdm_set_speaker_state(ad->sound_manager, true);
	} else {
		res = _callui_sdm_set_speaker_state(ad->sound_manager, false);
	}
	if (res == CALLUI_RESULT_OK) {
		err("_callui_sdm_set_speaker_state() failed. res[%d]", res);
	}
}

static void __mute_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;
	callui_app_data_t *ad = qp->ad;

	callui_result_e res =
			_callui_sdm_set_mute_state(ad->sound_manager, !_callui_sdm_get_mute_state(ad->sound_manager));
	if (res == CALLUI_RESULT_OK) {
		err("_callui_sdm_set_speaker_state() failed. res[%d]", res);
	}
}

static void __end_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_qp_mc_h qp = (callui_qp_mc_h)data;
	callui_app_data_t *ad = qp->ad;

	__hide_minicontrol(qp);

	const callui_call_data_t *active =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	const callui_call_data_t *held =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
	const callui_call_data_t *incom =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);

	callui_result_e res = CALLUI_RESULT_FAIL;

	if (incom) {
		res = _callui_manager_reject_call(ad->call_manager);
	} else if (active && active->is_dialing) {
		res = _callui_manager_end_call(ad->call_manager, active->call_id, CALLUI_CALL_RELEASE_BY_CALL_HANDLE);
	} else if (active && held) {
		res = _callui_manager_end_call(ad->call_manager, 0, CALLUI_CALL_RELEASE_ALL_ACTIVE);
	} else if (active || held) {
		res = _callui_manager_end_call(ad->call_manager, 0, CALLUI_CALL_RELEASE_ALL);
	} else {
		err("invalid case!!!!");
	}

	if (res != CALLUI_RESULT_OK) {
		err("__end_btn_click_cb() failed. res[%d]", res);
	}
}
