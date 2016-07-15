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

#include <vconf.h>
#include <app_control.h>
#include <Elementary.h>
#include <efl_extension.h>

#include "callui-view-incoming-call-noti.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-view-manager.h"
#include "callui-view-layout.h"
#include "callui-view-elements.h"
#include "callui-common.h"

#include "callui-manager.h"
#include "callui-state-provider.h"
#include "callui-sound-manager.h"
#include "callui-color-box.h"

#define CALLUI_GROUP_REJ_MSG_ITEM			"reject_msg_item"
#define CALLUI_GROUP_REJ_MSG_SCROLLER_BG	"reject_msg_scroller_bg"
#define CALLUI_GROUP_MAIN_ACTIVE_NOTI_CALL	"main_active_noti_call"

#define CALLUI_DURING_ICON	"call_button_icon_03.png"
#define CALLUI_END_ICON		"call_button_icon_01.png"

#define CALLUI_MOVE_TRANSITION_TIME_SEC		0.3

#define CALLUI_REJ_MSG_GENLIST_DATA			"view_data"
#define CALLUI_REJ_MSG_ADD_BTN_SIZE			60
#define CALLUI_REJ_MSG_ADD_BTN_COLOR		255, 255, 255, 255

#define CALLUI_REJ_MSG_ANIM_PATH			10000.0
#define CALLUI_REJ_MSG_ANIM_TIME			4.0
#define CALLUI_REJ_MSG_ANIM_VELOCITY		(CALLUI_REJ_MSG_ANIM_PATH / CALLUI_REJ_MSG_ANIM_TIME)

#define CALLUI_AN_CONTACT_INFO_BG_H			ELM_SCALE_SIZE(ACTIVE_NOTI_CONTACT_INFO_BG_H)
#define CALLUI_AN_ACTION_BG_W_REJECT_MSG_H	ELM_SCALE_SIZE(ACTIVE_NOTI_ACTION_BG_W_REJ_MSG_H)
#define CALLUI_AN_STROKE_H					ELM_SCALE_SIZE(ACTIVE_NOTI_STROKE_H)

#define CALLUI_REJ_MSG_SCREEN_USED_H		(CALLUI_AN_CONTACT_INFO_BG_H + CALLUI_AN_ACTION_BG_W_REJECT_MSG_H + CALLUI_AN_STROKE_H)
#define CALLUI_REJ_MSG_FLICK_TIME_LIMIT_MS	500

#define CALLUI_REJ_MSG_DIMMER_ALPHA_START	0
#define CALLUI_REJ_MSG_DIMMER_ALPHA_END		77
#define CALLUI_REJ_MSG_DIMMER_COLOR			0, 0, 0

#define CALLUI_REJ_MSG_OPACITY				(0.3 - 0.04)

#define AO028		0, 0, 0, 26
#define AO024_WO_A	0, 0, 0

typedef enum {
	CALLUI_REJ_MSG_UPDATE_GL_HINT = 0x01,
	CALLUI_REJ_MSG_UPDATE_AVAILABLE_SIZE = 0x02,
	CALLUI_REJ_MSG_ANCHOR_MOVED = 0x04
} _callui_rm_update_params_e;

typedef enum {
	CALLUI_REJ_MSG_HIDDEN,
	CALLUI_REJ_MSG_HIDE_REQ,
	CALLUI_REJ_MSG_HIDE_IN_PROG,
	CALLUI_REJ_MSG_SHOWN,
	CALLUI_REJ_MSG_SHOW_REQ,
	CALLUI_REJ_MSG_SHOW_IN_PROG,
} _callui_rm_state;

struct _callui_view_incoming_call_noti {
	call_view_data_base_t base_view;

	Evas_Object *parent;

	Evas_Object *box;
	Evas_Object *layout;

	Evas_Object *rm_scroller;
	Evas_Object *rm_scroller_anchor;
	Evas_Object *rm_scroller_stroke;
	Evas_Object *rm_scroller_bg;

	Ecore_Idle_Enterer *transit_effect_idler;
	Elm_Transit *transit;
	bool appear_anim_init;

	bool rm_turned_on;
	int rm_scroller_max_h;
	int rm_scroller_anim_start_h;
	int rm_scroller_anim_start_w;
	int rm_scroller_x;
	int rm_scroller_y;
	double rm_anim_path;
	_callui_rm_state rm_state;
	int rm_scroller_available_h;
	int rm_actualize_state;

	Ecore_Animator *rm_animator;
	int rm_dimmer_alpha;
	int rm_dimmer_alpha_start;
};

typedef struct _callui_view_incoming_call_noti _callui_view_incoming_call_noti_t;

static callui_result_e __callui_view_incoming_call_noti_oncreate(call_view_data_base_t *view_data, Evas_Object *parent, void *appdata);
static callui_result_e __callui_view_incoming_call_noti_ondestroy(call_view_data_base_t *view_data);

static void __reset_params_after_transit(callui_view_incoming_call_noti_h vd);
static void __appear_transit_del_cb(void *data, Elm_Transit *transit);
static void __disappear_transit_del_cb(void *data, Elm_Transit *transit);
static Eina_Bool __appear_effect_activated_cb(void *data);
static Eina_Bool __disappear_effect_activated_cb(void *data);
static void __show_reject_msg_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __box_changed_hints_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void __call_action_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static Evas_Event_Flags __gesture_layer_flick_end_cb(void *data, void *event_info);
static Evas_Object *__create_gesture_layer(callui_view_incoming_call_noti_h vd);
static void __contact_info_click_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static callui_result_e __create_main_content(callui_view_incoming_call_noti_h vd, Evas_Object *parent);
static Evas_Object *__create_btn(callui_view_incoming_call_noti_h vd,
		const char *style, const char *icon_name, const char *txt, Evas_Smart_Cb click_cb, void *cb_data);
static callui_result_e __update_displayed_data(callui_view_incoming_call_noti_h vd);
static void __move_active_noti(callui_view_incoming_call_noti_h vd);
static void __parent_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void __rm_create_content(callui_view_incoming_call_noti_h vd);
static void __rm_destroy_content(callui_view_incoming_call_noti_h vd);

static void __rm_reset_status_flags(callui_view_incoming_call_noti_h vd);
static void __rm_get_scroller_cur_size(callui_view_incoming_call_noti_h vd, int *w, int *h);
static void __rm_close_cb(callui_view_incoming_call_noti_h vd);
static void __rm_box_changed_hints_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void __rm_scroller_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void __rm_reset_anim_params(callui_view_incoming_call_noti_h vd);
static void __reject_msg_animation_end_cb(callui_view_incoming_call_noti_h vd);
static Eina_Bool __rm_animation_cb(void *data, double pos);
static void __rm_scroller_anchor_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _rm_message_item_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _rm_compose_item_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static callui_result_e __rm_create_message_items(callui_view_incoming_call_noti_h vd, Evas_Object *box, int *insert_item_count);
static callui_result_e __rm_create_compose_item(callui_view_incoming_call_noti_h vd, Evas_Object *box);
static int __rm_fill_box(callui_view_incoming_call_noti_h vd, Evas_Object *box);

static void __rm_prepare_animator(callui_view_incoming_call_noti_h vd);
static void __rm_recalculate(callui_view_incoming_call_noti_h vd);
static void __rm_show_animated(callui_view_incoming_call_noti_h vd);
static void __rm_hide_animated(callui_view_incoming_call_noti_h vd);
static void __rm_hide_instantly(callui_view_incoming_call_noti_h vd);
static void __rm_show_instantly(callui_view_incoming_call_noti_h vd);
static void __rm_try_actualize(callui_view_incoming_call_noti_h vd);
static void __rm_calc_available_size(callui_view_incoming_call_noti_h vd);

static Evas_Object *__create_btn(callui_view_incoming_call_noti_h vd,
		const char *style,
		const char *icon_name,
		const char *txt,
		Evas_Smart_Cb click_cb,
		void *cb_data);

callui_view_incoming_call_noti_h _callui_view_incoming_call_noti_new()
{
	callui_view_incoming_call_noti_h incoming_call_noti = calloc(1, sizeof(_callui_view_incoming_call_noti_t));
	CALLUI_RETURN_NULL_IF_FAIL(incoming_call_noti);

	incoming_call_noti->base_view.create = __callui_view_incoming_call_noti_oncreate;
	incoming_call_noti->base_view.update = NULL;
	incoming_call_noti->base_view.destroy = __callui_view_incoming_call_noti_ondestroy;

	return incoming_call_noti;
}

static callui_result_e __callui_view_incoming_call_noti_oncreate(call_view_data_base_t *view_data, Evas_Object *parent, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(parent, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	_callui_window_set_size_type(ad->window, CALLUI_WIN_SIZE_ACTIVE_NOTI);
	_callui_window_set_rotation_locked(ad->window, false);
	_callui_window_set_top_level_priority(ad->window, true);
	_callui_window_set_indicator_visible(ad->window, false);

	if (_callui_window_set_keygrab_mode(ad->window, CALLUI_KEY_SELECT, CALLUI_WIN_KEYGRAB_TOPMOST) != CALLUI_RESULT_OK) {
		dbg("KEY_SELECT key grab failed");
	}

	callui_result_e res = __create_main_content(vd, parent);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return __update_displayed_data(vd);
}

static callui_result_e __callui_view_incoming_call_noti_ondestroy(call_view_data_base_t *view_data)
{
	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	// TODO: need to replace from view
#ifdef _DBUS_DISPLAY_DEVICE_TIMEOUT_
	/* Set display timeout for call state */
	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->sound_manager);
	if (audio_state == CALLUI_AUDIO_STATE_SPEAKER) {
		_callui_display_set_timeout(ad->display, CALLUI_DISPLAY_TIMEOUT_SET);
	}
#endif

	evas_object_event_callback_del_full(vd->parent, EVAS_CALLBACK_RESIZE, __parent_resize_cb, vd);

	DELETE_ECORE_IDLE_ENTERER(vd->transit_effect_idler);
	DELETE_ELM_TRANSIT_HARD(vd->transit);
	DELETE_EVAS_OBJECT(vd->box);
	DELETE_EVAS_OBJECT(vd->rm_scroller_stroke);

	_callui_window_set_size_type(ad->window, CALLUI_WIN_SIZE_FULLSCREEN);
	_callui_window_set_rotation_locked(ad->window, true);
	_callui_window_set_top_level_priority(ad->window, false);
	_callui_window_set_indicator_visible(ad->window, true);

	_callui_window_unset_keygrab_mode(ad->window, CALLUI_KEY_SELECT);

	free(vd);

	return CALLUI_RESULT_OK;
}

static void __reset_params_after_transit(callui_view_incoming_call_noti_h vd)
{
	vd->transit = NULL;
	evas_object_freeze_events_set(vd->box, EINA_FALSE);
}

static void __appear_transit_del_cb(void *data, Elm_Transit *transit)
{
	CALLUI_RETURN_IF_FAIL(data);
	__reset_params_after_transit(data);
}

static void __disappear_transit_del_cb(void *data, Elm_Transit *transit)
{
	CALLUI_RETURN_IF_FAIL(data);
	__reset_params_after_transit(data);

	callui_view_incoming_call_noti_h vd = data;
	_callui_window_set_top_level_priority(vd->base_view.ad->window, false);
	_callui_window_minimize(vd->base_view.ad->window);
}

static Eina_Bool __appear_effect_activated_cb(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_incoming_call_noti_h vd = data;

	int height = 0;
	evas_object_geometry_get(vd->box, NULL, NULL, NULL, &height);

	DELETE_ELM_TRANSIT_HARD(vd->transit);
	vd->transit = elm_transit_add();
	if (!vd->transit) {
		err("transit is NULL");
		vd->transit_effect_idler = NULL;
		return ECORE_CALLBACK_CANCEL;
	}
	elm_transit_del_cb_set(vd->transit, __appear_transit_del_cb, vd);
	elm_transit_object_add(vd->transit, vd->box);
	elm_transit_effect_translation_add(vd->transit, 0, -height, 0, 0);
	elm_transit_duration_set(vd->transit, CALLUI_MOVE_TRANSITION_TIME_SEC);
	elm_transit_objects_final_state_keep_set(vd->transit, EINA_TRUE);
	elm_transit_go(vd->transit);

	vd->transit_effect_idler = NULL;
	evas_object_freeze_events_set(vd->box, EINA_TRUE);

	return ECORE_CALLBACK_DONE;
}

static Eina_Bool __disappear_effect_activated_cb(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_incoming_call_noti_h vd = data;

	int xpos = 0;
	int ypos = 0;
	int width = 0;
	int height = 0;
	evas_object_geometry_get(vd->box, &xpos, &ypos, &width, &height);
	int transit_y = -(height + ypos);

	DELETE_ELM_TRANSIT_HARD(vd->transit);
	vd->transit = elm_transit_add();
	if (!vd->transit) {
		err("transit is NULL");
		vd->transit_effect_idler = NULL;
		return ECORE_CALLBACK_CANCEL;
	}
	elm_transit_del_cb_set(vd->transit, __disappear_transit_del_cb, vd);
	elm_transit_object_add(vd->transit, vd->box);
	elm_transit_effect_translation_add(vd->transit, xpos, ypos, xpos, transit_y);
	elm_transit_duration_set(vd->transit, CALLUI_MOVE_TRANSITION_TIME_SEC);
	elm_transit_objects_final_state_keep_set(vd->transit, EINA_TRUE);
	elm_transit_go(vd->transit);

	vd->transit_effect_idler = NULL;
	evas_object_freeze_events_set(vd->box, EINA_TRUE);

	return ECORE_CALLBACK_CANCEL;
}

static void __rm_reset_status_flags(callui_view_incoming_call_noti_h vd)
{
	vd->rm_actualize_state |= CALLUI_REJ_MSG_UPDATE_GL_HINT;
	vd->rm_actualize_state |= CALLUI_REJ_MSG_UPDATE_AVAILABLE_SIZE;
	vd->rm_actualize_state |= CALLUI_REJ_MSG_ANCHOR_MOVED;
}

static void __show_reject_msg_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_incoming_call_noti_h vd = data;
	callui_app_data_t *ad = vd->base_view.ad;

	if (vd->rm_turned_on) {
		dbg("Hide reject messages request");

		__rm_reset_anim_params(vd);
		if (vd->rm_state == CALLUI_REJ_MSG_SHOW_IN_PROG ||
				vd->rm_state == CALLUI_REJ_MSG_SHOW_REQ) {
			dbg("Show animation in progress...");
		} else {
			dbg("No show animation in progress...");
		}
		vd->rm_state = CALLUI_REJ_MSG_HIDE_REQ;
		__rm_try_actualize(vd);
	} else {
		dbg("Show reject messages request");

		__rm_reset_anim_params(vd);
		if (vd->rm_state == CALLUI_REJ_MSG_HIDE_IN_PROG ||
				vd->rm_state == CALLUI_REJ_MSG_HIDE_REQ) {
			dbg("Hide animation in progress...");
			vd->rm_actualize_state &= ~CALLUI_REJ_MSG_UPDATE_AVAILABLE_SIZE;
		} else {
			dbg("No hide animation in progress...");
			__rm_reset_status_flags(vd);
			_callui_window_set_size_type(ad->window, CALLUI_WIN_SIZE_FULLSCREEN);
			elm_object_signal_emit(vd->layout, "hide_swipe", "main_active_noti_call");
			__rm_create_content(vd);
			__move_active_noti(vd);
		}
		vd->rm_state = CALLUI_REJ_MSG_SHOW_REQ;
		__rm_try_actualize(vd);
	}
	vd->rm_turned_on = !vd->rm_turned_on;
}

static void __rm_get_scroller_cur_size(callui_view_incoming_call_noti_h vd, int *w, int *h)
{
	evas_object_geometry_get(vd->rm_scroller, NULL, NULL, w, h);
	dbg("RM scroller size - w[%d] h[%d]", *w, *h);
}

static void __box_changed_hints_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_view_incoming_call_noti_h vd = data;

	if (vd->appear_anim_init) {
		return;
	}

	vd->appear_anim_init = true;
	vd->transit_effect_idler = ecore_idle_enterer_before_add(__appear_effect_activated_cb, vd);
}

static void __rm_close_cb(callui_view_incoming_call_noti_h vd)
{
	__rm_destroy_content(vd);

	elm_object_signal_emit(vd->layout, "show_swipe", "main_active_noti_call");
	_callui_window_set_size_type(vd->base_view.ad->window, CALLUI_WIN_SIZE_ACTIVE_NOTI);
	__move_active_noti(vd);
}

static void __rm_box_changed_hints_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_incoming_call_noti_h vd = data;
	Evas_Coord w, h;
	w = h = 0;
	evas_object_size_hint_min_get(obj, &w, &h);
	dbg("RM box hint min - w[%d]h[%d]", w, h);

	vd->rm_scroller_max_h = h;
	vd->rm_actualize_state &= ~CALLUI_REJ_MSG_UPDATE_GL_HINT;

	__rm_try_actualize(vd);
}

static void __rm_scroller_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_incoming_call_noti_h vd = data;
	Evas_Coord w, h;
	w = h = 0;
	evas_object_geometry_get(obj, NULL, NULL, &w, &h);

	dbg("RM scroller resize - w[%d]h[%d]", w, h);

	if (h == 0 && vd->rm_state == CALLUI_REJ_MSG_HIDDEN) {
		dbg("RM scroller height is 0");
		__rm_close_cb(vd);
	}
}

static void __rm_reset_anim_params(callui_view_incoming_call_noti_h vd)
{
	if (vd->rm_animator) {
		ecore_animator_del(vd->rm_animator);
		vd->rm_animator = NULL;
	}
	evas_object_freeze_events_set(vd->rm_scroller, EINA_FALSE);
}

static void __reject_msg_animation_end_cb(callui_view_incoming_call_noti_h vd)
{
	vd->rm_animator = NULL;

	if (vd->rm_state == CALLUI_REJ_MSG_SHOW_IN_PROG) {
		evas_object_freeze_events_set(vd->rm_scroller, EINA_FALSE);
		vd->rm_state = CALLUI_REJ_MSG_SHOWN;
	} else if (vd->rm_state == CALLUI_REJ_MSG_HIDE_IN_PROG) {
		vd->rm_state = CALLUI_REJ_MSG_HIDDEN;
		__rm_close_cb(vd);
	}
}

static int dtoi(double value) {
	if (value > 0) {
		return (value + 0.5);
	} else {
		return (value - 0.5);
	}
}

static Eina_Bool __rm_animation_cb(void *data, double pos)
{
	debug_enter();
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_incoming_call_noti_h vd = data;

	Evas_Coord path_step = dtoi((vd->rm_anim_path) * pos);
	Evas_Coord new_scroller_h = 0;

	int dim_step = 0;
	if (CALLUI_REJ_MSG_DIMMER_ALPHA_END == vd->rm_dimmer_alpha_start) {
		dim_step = dtoi(CALLUI_REJ_MSG_DIMMER_ALPHA_END * pos);
	} else {
		dim_step = dtoi((CALLUI_REJ_MSG_DIMMER_ALPHA_END - vd->rm_dimmer_alpha_start) * pos);
	}

	if (vd->rm_state == CALLUI_REJ_MSG_SHOW_IN_PROG) {
		new_scroller_h = vd->rm_scroller_anim_start_h + path_step;
		vd->rm_dimmer_alpha = vd->rm_dimmer_alpha_start + dim_step;
	} else if (vd->rm_state == CALLUI_REJ_MSG_HIDE_IN_PROG) {
		new_scroller_h = vd->rm_scroller_anim_start_h - path_step;
		vd->rm_dimmer_alpha = vd->rm_dimmer_alpha_start - dim_step;
	} else {
		err("Invalid state for animation. rm_state[%d]", vd->rm_state);
		__rm_reset_anim_params(vd);
		return ECORE_CALLBACK_CANCEL;
	}
	dbg("anim pos[%f], RM scroller new size [%d][%d]", pos,  vd->rm_scroller_anim_start_w, new_scroller_h);

	evas_object_color_set(vd->base_view.contents, CALLUI_REJ_MSG_DIMMER_COLOR, vd->rm_dimmer_alpha);

	dbg("rm_scroller_x[%d] rm_scroller_y[%d]" ,vd->rm_scroller_x, vd->rm_scroller_y);

	evas_object_resize(vd->rm_scroller, vd->rm_scroller_anim_start_w, new_scroller_h);
	evas_object_geometry_set(vd->rm_scroller_bg, vd->rm_scroller_x, vd->rm_scroller_y, vd->rm_scroller_anim_start_w, new_scroller_h);
	evas_object_move(vd->rm_scroller_stroke, vd->rm_scroller_x, vd->rm_scroller_y + new_scroller_h);

	if (pos == 1.0) {
		__reject_msg_animation_end_cb(vd);
	}

	return ECORE_CALLBACK_RENEW;
}

static void __rm_scroller_anchor_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	dbg("!!! __rm_scroller_anchor_move_cb !!!");
	callui_view_incoming_call_noti_h vd = data;

	Evas_Coord x = 0;
	Evas_Coord y = 0;
	evas_object_geometry_get(obj, &x, &y, NULL, NULL);
	dbg("RM scroller anchor geometry -  x[%d] y[%d]", x, y);

	vd->rm_scroller_x = x;
	vd->rm_scroller_y = y;
	evas_object_move(vd->rm_scroller, x, y);

	vd->rm_actualize_state &= ~CALLUI_REJ_MSG_ANCHOR_MOVED;
	__rm_try_actualize(vd);
}

static void _rm_compose_item_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;

	const callui_call_data_t *incom = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);
	CALLUI_RETURN_IF_FAIL(incom);

	_callui_common_launch_msg_composer(ad, incom->call_num);

	callui_result_e res = _callui_manager_reject_call(ad->call_manager);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_reject_call() failed. res[%d]", res);
	}
}

static void _rm_message_item_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	int index = SAFE_C_CAST(int, data);

	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)evas_object_data_get(obj, CALLUI_REJ_MSG_GENLIST_DATA);
	CALLUI_RETURN_IF_FAIL(vd);
	callui_app_data_t *ad = vd->base_view.ad;

	if (index != -1) {
		char *rm_markup_str = NULL;
		char *rm_str = _callui_common_get_reject_msg_by_index(index);
		if (rm_str) {
			rm_markup_str = elm_entry_markup_to_utf8(rm_str); /*send utf8 text to MSG */
			free(rm_str);
		}

		callui_result_e res = _callui_manager_reject_call(ad->call_manager);
		if (res != CALLUI_RESULT_OK) {
			err("cm_reject_call() is failed. res[%d]", res);
		} else {
			_callui_common_send_reject_msg(ad, rm_markup_str);
		}
		free(rm_markup_str);
	}
}

static callui_result_e __rm_create_message_items(callui_view_incoming_call_noti_h vd, Evas_Object *box, int *insert_item_count)
{
	int item_count = 0;
	callui_result_e res = _callui_common_get_reject_msg_count(&item_count);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, -1);

	int i = 0;
	for (; i < item_count; i++) {
		Evas_Object *item_ly = _callui_load_edj(box, CALLUI_CALL_EDJ_PATH, "reject_msg_item");
		CALLUI_RETURN_VALUE_IF_FAIL(item_ly, CALLUI_RESULT_FAIL);

		evas_object_size_hint_weight_set(item_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(item_ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
		elm_layout_signal_callback_add(item_ly, "cu,action,clicked", "main_active_noti_call", _rm_message_item_clicked_cb, SAFE_C_CAST(void *, i));

		char *txt = _callui_common_get_reject_msg_by_index(i);
		elm_object_part_text_set(item_ly, "callui.text.main", txt);
		free(txt);

		evas_object_data_set(item_ly, CALLUI_REJ_MSG_GENLIST_DATA, vd);

		evas_object_show(item_ly);
		elm_box_pack_end(box, item_ly);
	}
	*insert_item_count = item_count;
	return res;
}

static callui_result_e __rm_create_compose_item(callui_view_incoming_call_noti_h vd, Evas_Object *box)
{
	Evas_Object *item_ly = _callui_load_edj(vd->box, CALLUI_CALL_EDJ_PATH, "reject_msg_item_compose");
	CALLUI_RETURN_VALUE_IF_FAIL(item_ly, CALLUI_RESULT_FAIL);

	evas_object_size_hint_weight_set(item_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(item_ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_layout_signal_callback_add(item_ly, "cu,action,clicked", "main_active_noti_call", _rm_compose_item_clicked_cb, vd->base_view.ad);

	elm_object_translatable_part_text_set(item_ly, "callui.text.main", "IDS_CALL_BUTTON_COMPOSE_MESSAGE_TO_SEND_ABB");

	evas_object_show(item_ly);
	elm_box_pack_end(box, item_ly);

	return CALLUI_RESULT_OK;
}

static int __rm_fill_box(callui_view_incoming_call_noti_h vd, Evas_Object *box)
{
	int item_count = 0;

	callui_result_e res = __rm_create_message_items(vd, box, &item_count);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, -1);

	res = __rm_create_compose_item(vd, box);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, -1);

	return ++item_count;
}

static void __rm_scroller_bg_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Coord x, y, w, h;
	x = y = w = h = 0;
	evas_object_geometry_get(obj, &x, &y, &w, &h);

	dbg("RM scroller BG [resize] - x[%d]y[%d] w[%d]h[%d]", x, y, w, h);
}

static void __rm_scroller_bg_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Coord x, y, w, h;
	x = y = w = h = 0;
	evas_object_geometry_get(obj, &x, &y, &w, &h);

	dbg("RM scroller BG [move] - x[%d]y[%d] w[%d]h[%d]", x, y, w, h);
}

static void __rm_strock_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Coord x, y, w, h;
	x = y = w = h = 0;
	evas_object_geometry_get(obj, &x, &y, &w, &h);

	dbg("RM strock [move] - x[%d]y[%d] w[%d]h[%d]", x, y, w, h);
}

static void __rm_create_content(callui_view_incoming_call_noti_h vd)
{
	/* Reject message list background gradient color layout */
	vd->rm_scroller_bg = _callui_load_edj(vd->box, CALLUI_CALL_EDJ_PATH, "reject_msg_scroller_bg");
	evas_object_event_callback_add(vd->rm_scroller_bg, EVAS_CALLBACK_RESIZE, __rm_scroller_bg_resize_cb, vd);
	evas_object_event_callback_add(vd->rm_scroller_bg, EVAS_CALLBACK_MOVE, __rm_scroller_bg_move_cb, vd);
	CALLUI_RETURN_IF_FAIL(vd->rm_scroller_bg);
	evas_object_show(vd->rm_scroller_bg);

	/* Reject message scroller */
	vd->rm_scroller = elm_scroller_add(vd->box);
	if (!vd->rm_scroller) {
		err("rm_scroller is NULL");
		return __rm_destroy_content(vd);
	}
	// TODO: ELM_SCALE_SIZE is use to set size according to error edje element calculation
	// need to delete it after fix in EDJE library
	evas_object_resize(vd->rm_scroller, ELM_SCALE_SIZE(vd->base_view.ad->root_w), 0);
	evas_object_event_callback_add(vd->rm_scroller, EVAS_CALLBACK_RESIZE, __rm_scroller_resize_cb, vd);
	evas_object_show(vd->rm_scroller);

	/* Reject message box */
	callui_rgb_color_t item_init_color = { AO024_WO_A };
	Evas_Object *box = _callui_color_box_add(vd->rm_scroller, &item_init_color, CALLUI_REJ_MSG_OPACITY);
	if (!box) {
		err("box is NULL");
		return __rm_destroy_content(vd);
	}
	evas_object_event_callback_add(box, EVAS_CALLBACK_CHANGED_SIZE_HINTS, __rm_box_changed_hints_cb, vd);
	evas_object_show(box);

	int rm_item_count = __rm_fill_box(vd, box);
	if (rm_item_count < 0) {
		err("rm_item_count is %d", rm_item_count);
		return __rm_destroy_content(vd);
	}

	elm_object_content_set(vd->rm_scroller, box);

	/* Reject message scroller bottom stroke */
	vd->rm_scroller_stroke = evas_object_rectangle_add(evas_object_evas_get(vd->rm_scroller));
	evas_object_event_callback_add(vd->rm_scroller_stroke, EVAS_CALLBACK_MOVE, __rm_strock_move_cb, vd);
	if (!vd->rm_scroller_stroke) {
		return __rm_destroy_content(vd);
	}
	evas_object_color_set(vd->rm_scroller_stroke, AO028);
	evas_object_resize(vd->rm_scroller_stroke, vd->base_view.ad->root_w, CALLUI_AN_STROKE_H);
	evas_object_show(vd->rm_scroller_stroke);

	/* Reject message scroller/list background EDJE anchor */
	vd->rm_scroller_anchor = evas_object_rectangle_add(evas_object_evas_get(vd->layout));
	if (!vd->rm_scroller_anchor) {
		return __rm_destroy_content(vd);
	}
	evas_object_event_callback_add(vd->rm_scroller_anchor, EVAS_CALLBACK_MOVE, __rm_scroller_anchor_move_cb, vd);
	evas_object_color_set(vd->rm_scroller_anchor, 0, 0, 0, 0);
	elm_object_part_content_set(vd->layout, "swallow.reject_msg_gl_anchor", vd->rm_scroller_anchor);

	vd->rm_scroller_max_h = ELM_SCALE_SIZE(ACTIVE_NOTI_RM_ITEM_HEIGHT) * rm_item_count;
	vd->rm_actualize_state &= ~CALLUI_REJ_MSG_UPDATE_GL_HINT;
}

static void __rm_destroy_content(callui_view_incoming_call_noti_h vd)
{
	DELETE_EVAS_OBJECT(vd->rm_scroller_bg);
	DELETE_EVAS_OBJECT(vd->rm_scroller);
	DELETE_EVAS_OBJECT(vd->rm_scroller_stroke);
	DELETE_EVAS_OBJECT(vd->rm_scroller_anchor);
	vd->rm_scroller_max_h = 0;
}

static void __call_action_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	char *key = (char *)data;

	app_control_h app_control = NULL;
	int ret;
	if ((ret = app_control_create(&app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_create() is failed. ret[%d]", ret);
	} else if (app_control_set_app_id(app_control, CALLUI_PACKAGE) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, key)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed. ret[%d]", ret);
	}
	if (app_control) {
		app_control_destroy(app_control);
	}
}

static Evas_Event_Flags __gesture_layer_flick_end_cb(void *data, void *event_info)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, EVAS_EVENT_FLAG_NONE);

	callui_view_incoming_call_noti_h vd = data;
	Elm_Gesture_Line_Info *line_info = (Elm_Gesture_Line_Info *)event_info;

	if (((line_info->angle >= 340. && line_info->angle <= 360.) ||
			(line_info->angle >= 0. && line_info->angle <= 20.)) && !vd->rm_turned_on) {
		DELETE_ECORE_IDLE_ENTERER(vd->transit_effect_idler);
		vd->transit_effect_idler = ecore_idle_enterer_before_add(__disappear_effect_activated_cb, vd);
	}
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Object *__create_gesture_layer(callui_view_incoming_call_noti_h vd)
{
	Evas_Object *gesture_layer = elm_gesture_layer_add(vd->layout);
	CALLUI_RETURN_NULL_IF_FAIL(gesture_layer);

	elm_gesture_layer_attach(gesture_layer, vd->layout);
	elm_gesture_layer_flick_time_limit_ms_set(gesture_layer, CALLUI_REJ_MSG_FLICK_TIME_LIMIT_MS);
	elm_gesture_layer_cb_set(gesture_layer, ELM_GESTURE_N_LINES , ELM_GESTURE_STATE_END, __gesture_layer_flick_end_cb, vd);
	evas_object_show(gesture_layer);

	return gesture_layer;
}

static void __contact_info_click_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_incoming_call_noti_h vd = data;

	_callui_vm_change_view(vd->base_view.ad->view_manager, CALLUI_VIEW_INCOMING_CALL);
}

static callui_result_e __create_main_content(callui_view_incoming_call_noti_h vd, Evas_Object *parent)
{
	vd->base_view.contents = evas_object_rectangle_add(evas_object_evas_get(parent));
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	evas_object_size_hint_weight_set(vd->base_view.contents, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(vd->base_view.contents, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(vd->base_view.contents, CALLUI_REJ_MSG_DIMMER_COLOR, CALLUI_REJ_MSG_DIMMER_ALPHA_START);
	elm_object_part_content_set(parent, "elm.swallow.content", vd->base_view.contents);
	evas_object_show(vd->base_view.contents);

	vd->box = elm_box_add(vd->base_view.contents);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->box, CALLUI_RESULT_ALLOCATION_FAIL);
	evas_object_event_callback_add(vd->box, EVAS_CALLBACK_CHANGED_SIZE_HINTS, __box_changed_hints_cb, vd);
	elm_box_horizontal_set(vd->box, EINA_FALSE);
	evas_object_show(vd->box);

	vd->layout = _callui_load_edj(vd->box, CALLUI_CALL_EDJ_PATH,  "main_active_noti_call");
	CALLUI_RETURN_VALUE_IF_FAIL(vd->layout, CALLUI_RESULT_ALLOCATION_FAIL);
	evas_object_size_hint_weight_set(vd->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(vd->layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_object_signal_callback_add(vd->layout, "contact_info.clicked",
			"main_active_noti_call", __contact_info_click_cb, vd);

	elm_box_pack_end(vd->box, vd->layout);

	Evas_Object *answer_btn = __create_btn(vd, "answer_call_noti", CALLUI_DURING_ICON,
			NULL, __call_action_btn_click_cb, CALLUI_APP_CONTROL_OPERATION_DURING_CALL);
	CALLUI_RETURN_VALUE_IF_FAIL(answer_btn, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->layout, "swallow.call_accept_btn", answer_btn);

	Evas_Object *reject_btn = __create_btn(vd, "reject_call_noti", CALLUI_END_ICON,
			NULL, __call_action_btn_click_cb, CALLUI_APP_CONTROL_OPERATION_END_CALL);
	CALLUI_RETURN_VALUE_IF_FAIL(reject_btn, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->layout, "swallow.call_reject_btn", reject_btn);

	CALLUI_RETURN_VALUE_IF_FAIL(__create_gesture_layer(vd), CALLUI_RESULT_ALLOCATION_FAIL);

	elm_object_signal_emit(vd->layout, "start_swipe_anim", "main_active_noti_call");

	vd->rm_state = CALLUI_REJ_MSG_HIDDEN;
	vd->parent = parent;
	evas_object_event_callback_add(vd->parent, EVAS_CALLBACK_RESIZE, __parent_resize_cb, vd);

	return CALLUI_RESULT_OK;
}

static Evas_Object *__create_btn(callui_view_incoming_call_noti_h vd,
		const char *style,
		const char *icon_name,
		const char *txt,
		Evas_Smart_Cb click_cb,
		void *cb_data)
{
	Evas_Object *button = elm_button_add(vd->layout);
	CALLUI_RETURN_NULL_IF_FAIL(button);
	elm_object_style_set(button, style);
	evas_object_smart_callback_add(button, "clicked", click_cb, cb_data);

	if (icon_name) {
		Evas_Object *icon = elm_image_add(button);
		CALLUI_RETURN_NULL_IF_FAIL(icon);
		elm_image_file_set(icon, CALLUI_CALL_EDJ_PATH, icon_name);
		elm_object_part_content_set(button, "elm.swallow.content", icon);
	}
	if (txt) {
		elm_object_translatable_part_text_set(button, "elm.text", txt);
	}
	return button;
}

static callui_result_e __update_displayed_data(callui_view_incoming_call_noti_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	const callui_call_data_t *call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);
	CALLUI_RETURN_VALUE_IF_FAIL(call_data, CALLUI_RESULT_FAIL);

	elm_object_signal_emit(vd->layout, "small_main_ly", "main_active_noti_call");

	const char *call_name = call_data->call_ct_info.call_disp_name;
	const char *call_number = NULL;
	if (call_data->call_disp_num[0] != '\0') {
		call_number = call_data->call_disp_num;
	} else {
		call_number = call_data->call_num;
	}

	elm_object_translatable_part_text_set(vd->layout, "text.status", "IDS_CALL_BODY_INCOMING_CALL");

	bool is_unknown = false;
	if (!(call_name && call_name[0] != '\0') && !(call_number && call_number[0] != '\0')) {
		elm_object_signal_emit(vd->layout, "show_two_lines", "main_active_noti_call");
		elm_object_signal_emit(vd->layout, "hide_reject_msg_btn", "main_active_noti_call");
		elm_object_translatable_part_text_set(vd->layout, "text.contact_name", "IDS_CALL_BODY_UNKNOWN");
		is_unknown = true;
	} else if (!(call_name && call_name[0] != '\0')) {
		elm_object_signal_emit(vd->layout, "show_two_lines", "main_active_noti_call");
		elm_object_part_text_set(vd->layout, "text.contact_name", call_number);
	} else {
		elm_object_signal_emit(vd->layout, "show_three_lines", "main_active_noti_call");
		elm_object_part_text_set(vd->layout, "text.contact_name", call_name);
		elm_object_part_text_set(vd->layout, "text.contact_number", call_number);
	}

	if (!is_unknown) {
		Evas_Object *reject_msg_btn = __create_btn(vd, "reject_msg_noti", NULL,
				"IDS_VCALL_BUTTON2_REJECT_CALL_WITH_MESSAGE", __show_reject_msg_btn_click_cb, vd);
		CALLUI_RETURN_VALUE_IF_FAIL(reject_msg_btn, CALLUI_RESULT_ALLOCATION_FAIL);
		elm_object_part_content_set(vd->layout, "swallow.reject_msg_btn", reject_msg_btn);
	}

	CALLUI_RETURN_VALUE_IF_FAIL(_callui_show_caller_id(vd->layout, call_data), CALLUI_RESULT_FAIL);

	evas_object_show(vd->layout);

	return CALLUI_RESULT_OK;
}

static void __rm_prepare_animator(callui_view_incoming_call_noti_h vd)
{
	double anim_time = vd->rm_anim_path / CALLUI_REJ_MSG_ANIM_VELOCITY;

	dbg("Animation start params: path[%f], time[%f], w[%d], h[%d]",
			vd->rm_anim_path, anim_time, vd->rm_scroller_anim_start_w, vd->rm_scroller_anim_start_h);

	vd->rm_dimmer_alpha_start = vd->rm_dimmer_alpha;
	vd->rm_animator = ecore_animator_timeline_add(anim_time, __rm_animation_cb, vd);
	if (!vd->rm_animator) {
		err("Failed to create animator.");
	}
}

static void __rm_show_animated(callui_view_incoming_call_noti_h vd)
{
	vd->rm_state = CALLUI_REJ_MSG_SHOW_IN_PROG;
	evas_object_freeze_events_set(vd->rm_scroller, EINA_TRUE);

	if (vd->rm_scroller_max_h > vd->rm_scroller_available_h) {
		vd->rm_anim_path = vd->rm_scroller_available_h;
	} else {
		vd->rm_anim_path = vd->rm_scroller_max_h;
	}

	__rm_get_scroller_cur_size(vd, &vd->rm_scroller_anim_start_w, &vd->rm_scroller_anim_start_h);
	vd->rm_anim_path = vd->rm_anim_path - vd->rm_scroller_anim_start_h;

	__rm_prepare_animator(vd);
}

static void __rm_hide_animated(callui_view_incoming_call_noti_h vd)
{
	vd->rm_state = CALLUI_REJ_MSG_HIDE_IN_PROG;
	evas_object_freeze_events_set(vd->rm_scroller, EINA_TRUE);

	__rm_get_scroller_cur_size(vd, &vd->rm_scroller_anim_start_w, &vd->rm_scroller_anim_start_h);
	vd->rm_anim_path = vd->rm_scroller_anim_start_h;

	__rm_prepare_animator(vd);
}

static void __rm_recalculate(callui_view_incoming_call_noti_h vd)
{
	debug_enter();

	int new_height = 0;
	if (vd->rm_scroller_max_h > vd->rm_scroller_available_h) {
		new_height = vd->rm_scroller_available_h;
	} else {
		new_height = vd->rm_scroller_max_h;
	}

	int width = 0;
	int height = 0;
	__rm_get_scroller_cur_size(vd, &width, &height);

	evas_object_resize(vd->rm_scroller, width, new_height);
	evas_object_geometry_set(vd->rm_scroller_bg, vd->rm_scroller_x, vd->rm_scroller_y, width, new_height);
	evas_object_move(vd->rm_scroller_stroke, vd->rm_scroller_x, vd->rm_scroller_y + new_height);
}

static void __rm_hide_instantly(callui_view_incoming_call_noti_h vd)
{
	__rm_reset_anim_params(vd);

	vd->rm_dimmer_alpha = CALLUI_REJ_MSG_DIMMER_ALPHA_START;
	evas_object_color_set(vd->base_view.contents, CALLUI_REJ_MSG_DIMMER_COLOR, vd->rm_dimmer_alpha);

	__rm_close_cb(vd);

	vd->rm_state = CALLUI_REJ_MSG_HIDDEN;
}

static void __rm_show_instantly(callui_view_incoming_call_noti_h vd)
{
	__rm_reset_anim_params(vd);

	vd->rm_dimmer_alpha = CALLUI_REJ_MSG_DIMMER_ALPHA_END;
	evas_object_color_set(vd->base_view.contents, CALLUI_REJ_MSG_DIMMER_COLOR, vd->rm_dimmer_alpha);

	__rm_recalculate(vd);

	vd->rm_state = CALLUI_REJ_MSG_SHOWN;
}

static void __rm_try_actualize(callui_view_incoming_call_noti_h vd)
{
	dbg("rm_actualize_state[%d] rm_state[%d]", vd->rm_actualize_state, vd->rm_state);

	if (!vd->rm_actualize_state) {

		vd->rm_actualize_state |= CALLUI_REJ_MSG_UPDATE_AVAILABLE_SIZE;

		switch (vd->rm_state) {
		case CALLUI_REJ_MSG_HIDDEN:
			return;
		case CALLUI_REJ_MSG_SHOWN:
			__rm_recalculate(vd);
			break;
		case CALLUI_REJ_MSG_SHOW_REQ:
			__rm_show_animated(vd);
			break;
		case CALLUI_REJ_MSG_HIDE_IN_PROG:
			__rm_hide_instantly(vd);
			break;
		case CALLUI_REJ_MSG_SHOW_IN_PROG:
			__rm_show_instantly(vd);
			break;
		default:
			break;
		}
	}

	if (vd->rm_state == CALLUI_REJ_MSG_HIDE_REQ) {
		__rm_hide_animated(vd);
	}
}

static void __rm_calc_available_size(callui_view_incoming_call_noti_h vd)
{
	vd->rm_scroller_available_h = 0;
	callui_app_data_t *ad = vd->base_view.ad;

	int rotation = _callui_window_get_rotation(ad->window);
	switch (rotation) {
	case 0:
	case 180:
		vd->rm_scroller_available_h = ad->root_h - CALLUI_REJ_MSG_SCREEN_USED_H;
		break;
	case 90:
	case 270:
		vd->rm_scroller_available_h = ad->root_w - CALLUI_REJ_MSG_SCREEN_USED_H;
		break;
	default:
		return;
		break;
	}
	dbg("Available RM scroller height - [%d]",  vd->rm_scroller_available_h);
}

static void __move_active_noti(callui_view_incoming_call_noti_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;
	int rotation = _callui_window_get_rotation(ad->window);
	int new_x = 0;
	if (vd->rm_scroller) {
		if (rotation == 90 || rotation == 270)
			new_x = (ad->root_h - ad->root_w - ACTIVE_NOTI_LANDSCAPE_L_PAD);
	}
	evas_object_move(vd->box, new_x, 0);
}

static void __parent_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_incoming_call_noti_h vd = data;

	__move_active_noti(vd);
	__rm_calc_available_size(vd);

	vd->rm_actualize_state &= ~CALLUI_REJ_MSG_UPDATE_AVAILABLE_SIZE;
	__rm_try_actualize(vd);
}
