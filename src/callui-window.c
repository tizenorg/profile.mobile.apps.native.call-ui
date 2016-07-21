/*
 * Copyright (c) 2009-2016 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <efl_util.h>
#include <tzsh/tzsh.h>
#include <tzsh/tzsh_quickpanel.h>

#include "callui-window.h"
#include "callui.h"
#include "callui-common-defines.h"
#include "callui-debug.h"
#include "callui-view-layout.h"

#define CALLUI_WIN_ACT_NOTI_HEIGHT			ELM_SCALE_SIZE(413)
#define CALLUI_WIN_ACT_NOTI_ROTATION_COUNT	4
#define CALLUI_WIN_NORM_ROTATION_COUNT		1

struct __callui_window {

	Evas_Object *win;
	Evas_Object *conformant;

	callui_app_data_t *ad;

	int win_w;
	int win_h;
	bool rotation_locked;
	callui_win_size_type_e size_type;

	tzsh_h tzsh;
	tzsh_quickpanel_h tzsh_qp;
};

typedef struct __callui_window __callui_window_t;

static Evas_Object *__create_eo_window(callui_window_h window_h);
static Evas_Object *__create_eo_conformant(Evas_Object *win);

static void __eo_win_rotation_changed_cb(void *data, Evas_Object *obj, void *event_info);

static void __eo_win_update_size(callui_window_h window_h, callui_win_size_type_e size_type);
static callui_result_e __eo_win_set_rotation_locked(callui_window_h window_h, bool is_locked);
static void __eo_win_move_and_resize(callui_window_h window_h, bool force_resize);
static Elm_Win_Keygrab_Mode __convert_app_keygrab_mode(callui_win_keygrab_mode_e mode);
static callui_result_e __create_tzsh_qp(callui_window_h window_h);

static callui_result_e __create_tzsh_qp(callui_window_h window_h)
{
	window_h->tzsh = tzsh_create(TZSH_TOOLKIT_TYPE_EFL);
	CALLUI_RETURN_VALUE_IF_FAIL(window_h->tzsh, CALLUI_RESULT_FAIL);

	tzsh_window tz_win = elm_win_window_id_get(window_h->win);
	CALLUI_RETURN_VALUE_IF_FAIL(tz_win, CALLUI_RESULT_FAIL);

	window_h->tzsh_qp = tzsh_quickpanel_create(window_h->tzsh, tz_win);
	CALLUI_RETURN_VALUE_IF_FAIL(window_h->tzsh_qp, CALLUI_RESULT_FAIL);

	return CALLUI_RESULT_OK;
}

static Evas_Object *__create_eo_conformant(Evas_Object *win)
{
	Evas_Object *conf = elm_conformant_add(win);
	CALLUI_RETURN_NULL_IF_FAIL(conf);

	elm_object_signal_emit(conf, "elm,state,indicator,overlap", "elm");
	evas_object_size_hint_weight_set(conf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, conf);
	evas_object_show(conf);

	return conf;
}

static void __eo_win_move_and_resize(callui_window_h window_h, bool force_resize)
{
	callui_app_data_t *ad = window_h->ad;

	int angle = elm_win_rotation_get(window_h->win);
	dbg("Window orientation changed [%d]", angle);

	int win_h = window_h->win_h;
	int win_w = window_h->win_w;

	if (window_h->size_type == CALLUI_WIN_SIZE_FULLSCREEN) {
		if (angle == 90 || angle == 270) {
			int temp = win_h;
			win_h = win_w;
			win_w = temp;
		}
	}
	if (force_resize) {
		evas_object_resize(window_h->win, 0, 0);
	}
	evas_object_resize(window_h->win, win_w, win_h);

	int move_x = 0;
	int move_y = 0;

	if (window_h->size_type != CALLUI_WIN_SIZE_FULLSCREEN) {
		switch (angle) {
		case 90:
			move_y = ACTIVE_NOTI_LANDSCAPE_L_PAD;
			break;
		case 180:
			move_y = ad->root_h - win_h;
			break;
		case 270:
			move_x = ad->root_w - win_h;
			move_y = ad->root_h - win_w - ACTIVE_NOTI_LANDSCAPE_L_PAD;
			break;
		default:
			break;
		}
	}
	evas_object_move(window_h->win, move_x, move_y);
}

static void __eo_win_rotation_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	__eo_win_move_and_resize(data, true);
}

static Evas_Object *__create_eo_window(callui_window_h window_h)
{
	Evas_Object *eo = elm_win_add(NULL, CALLUI_PACKAGE, ELM_WIN_NOTIFICATION);
	CALLUI_RETURN_NULL_IF_FAIL(eo);

	elm_win_aux_hint_add(eo, "wm.policy.win.user.geometry", "1");
	elm_win_fullscreen_set(eo, EINA_FALSE);
	elm_win_alpha_set(eo, EINA_TRUE);

	evas_object_smart_callback_add(eo, "wm,rotation,changed", __eo_win_rotation_changed_cb, window_h);

	elm_win_center(eo, EINA_FALSE, EINA_FALSE);
	elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(eo, ELM_WIN_INDICATOR_TRANSLUCENT);
	elm_win_conformant_set(eo, EINA_TRUE);

	return eo;
}

callui_result_e __callui_window_init(callui_window_h window_h, callui_app_data_t *appdata)
{
	window_h->ad = appdata;
	window_h->rotation_locked = false;

	window_h->win = __create_eo_window(window_h);
	CALLUI_RETURN_VALUE_IF_FAIL(window_h->win, CALLUI_RESULT_FAIL);

	window_h->conformant = __create_eo_conformant(window_h->win);
	CALLUI_RETURN_VALUE_IF_FAIL(window_h->conformant, CALLUI_RESULT_FAIL);

	callui_result_e res = __create_tzsh_qp(window_h);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, CALLUI_RESULT_FAIL);

	__eo_win_update_size(window_h, CALLUI_WIN_SIZE_ACTIVE_NOTI);

	return __eo_win_set_rotation_locked(window_h, true);
}

void __callui_window_deinit(callui_window_h window_h)
{
	window_h->conformant = NULL;
	evas_object_smart_callback_del_full(window_h->win, "wm,rotation,changed", __eo_win_rotation_changed_cb, window_h);
	DELETE_EVAS_OBJECT(window_h->win);

	if (window_h->tzsh) {
		if (window_h->tzsh_qp) {
			tzsh_quickpanel_destroy(window_h->tzsh_qp);
		}
		tzsh_destroy(window_h->tzsh);
	}
}

callui_window_h _callui_window_create(callui_app_data_t *appdata)
{
	CALLUI_RETURN_NULL_IF_FAIL(appdata);

	callui_window_h window_h = (callui_window_h)calloc(1, sizeof(__callui_window_t));
	CALLUI_RETURN_NULL_IF_FAIL(window_h);

	if (__callui_window_init(window_h, appdata) != CALLUI_RESULT_OK) {
		__callui_window_deinit(window_h);
		FREE(window_h);
	}
	return window_h;
}

void _callui_window_destroy(callui_window_h window_h)
{
	CALLUI_RETURN_IF_FAIL(window_h);

	__callui_window_deinit(window_h);

	free(window_h);
}

Evas_Object *_callui_window_get_eo(callui_window_h window_h)
{
	CALLUI_RETURN_NULL_IF_FAIL(window_h);

	return window_h->win;
}

Evas_Object *_callui_window_get_content_parent(callui_window_h window_h)
{
	CALLUI_RETURN_NULL_IF_FAIL(window_h);

	return window_h->conformant;
}

void _callui_window_set_content(callui_window_h window_h, Evas_Object *content)
{
	CALLUI_RETURN_IF_FAIL(window_h);

	elm_object_content_set(window_h->conformant, content);
}

static void __eo_win_update_size(callui_window_h window_h, callui_win_size_type_e size_type)
{
	callui_app_data_t *ad = window_h->ad;

	window_h->size_type = size_type;
	window_h->win_w = ad->root_w;

	switch (size_type) {
	case CALLUI_WIN_SIZE_FULLSCREEN:
		window_h->win_h = ad->root_h;
		break;
	case CALLUI_WIN_SIZE_ACTIVE_NOTI:
		window_h->win_h = CALLUI_WIN_ACT_NOTI_HEIGHT;
		break;
	default:
		break;
	}
	__eo_win_move_and_resize(window_h, false);
}

void _callui_window_set_size_type(callui_window_h window_h, callui_win_size_type_e size_type)
{
	CALLUI_RETURN_IF_FAIL(window_h);
	CALLUI_RETURN_IF_FAIL((size_type >= CALLUI_WIN_SIZE_FULLSCREEN && size_type <= CALLUI_WIN_SIZE_ACTIVE_NOTI));

	__eo_win_update_size(window_h, size_type);
}

static callui_result_e __eo_win_set_rotation_locked(callui_window_h window_h, bool is_locked)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h->rotation_locked != is_locked,
			CALLUI_RESULT_ALREADY_REGISTERED);

	if (elm_win_wm_rotation_supported_get(window_h->win)) {
		window_h->rotation_locked = is_locked;

		if (window_h->rotation_locked) {
			int rotate_angles[CALLUI_WIN_NORM_ROTATION_COUNT] = { 0 };
			elm_win_wm_rotation_available_rotations_set(window_h->win,
					rotate_angles,
					CALLUI_WIN_NORM_ROTATION_COUNT);
		} else {
			int rotate_angles[CALLUI_WIN_ACT_NOTI_ROTATION_COUNT] = { 0, 90, 180, 270 };
			elm_win_wm_rotation_available_rotations_set(window_h->win,
					rotate_angles,
					CALLUI_WIN_ACT_NOTI_ROTATION_COUNT);
		}
	} else {
		err("Window does not support rotation");
		return CALLUI_RESULT_FAIL;
	}

	return CALLUI_RESULT_OK;
}

callui_result_e _callui_window_set_rotation_locked(callui_window_h window_h, bool is_locked)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h, CALLUI_RESULT_INVALID_PARAM);

	return __eo_win_set_rotation_locked(window_h, is_locked);
}

int _callui_window_get_rotation(callui_window_h window_h)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h, -1);

	return elm_win_rotation_get(window_h->win);
}

callui_result_e _callui_window_set_top_level_priority(callui_window_h window_h, bool is_toplevel)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h, CALLUI_RESULT_INVALID_PARAM);

	efl_util_notification_level_e efl_noti_level = EFL_UTIL_NOTIFICATION_LEVEL_NONE;
	if (is_toplevel) {
		dbg("Window level [TOP]");
		efl_noti_level = EFL_UTIL_NOTIFICATION_LEVEL_TOP;
	} else {
		dbg("Window level [NORMAL]");
	}
	int res = efl_util_set_notification_window_level(window_h->win, efl_noti_level);
	if (res != EFL_UTIL_ERROR_NONE) {
		err("efl_util_set_notification_window_level() failed! res[%d]", res);
		return CALLUI_RESULT_FAIL;
	}

	return CALLUI_RESULT_OK;
}

callui_result_e _callui_window_set_screen_mode(callui_window_h window_h, callui_win_screen_mode_e screen_mode)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL((screen_mode >= CALLUI_WIN_SCREEN_MODE_DEFAULT &&
			screen_mode <= CALLUI_WIN_SCREEN_MODE_ALWAYS_ON), CALLUI_RESULT_INVALID_PARAM);

	efl_util_screen_mode_e efl_screen_mode = EFL_UTIL_SCREEN_MODE_DEFAULT;
	if (screen_mode == CALLUI_WIN_SCREEN_MODE_ALWAYS_ON) {
		dbg("Screen mode [Always on]");
		efl_screen_mode = EFL_UTIL_SCREEN_MODE_ALWAYS_ON;
	} else {
		dbg("Screen mode [Default]");
	}
	int res = efl_util_set_window_screen_mode(window_h->win, efl_screen_mode);
	if (res != EFL_UTIL_ERROR_NONE) {
		err("efl_util_set_window_screen_mode() failed! res[%d]", res);
		return CALLUI_RESULT_FAIL;
	}

	return CALLUI_RESULT_OK;
}

void _callui_window_get_screen_size(callui_window_h window_h, int *x, int *y, int *w, int *h)
{
	CALLUI_RETURN_IF_FAIL(window_h);

	elm_win_screen_size_get(window_h->win, x, y, w, h);
}

void _callui_window_activate(callui_window_h window_h)
{
	CALLUI_RETURN_IF_FAIL(window_h);

	elm_win_activate(window_h->win);
	evas_object_show(window_h->win);
}

void _callui_window_minimize(callui_window_h window_h)
{
	CALLUI_RETURN_IF_FAIL(window_h);

	elm_win_lower(window_h->win);
}

static Elm_Win_Keygrab_Mode __convert_app_keygrab_mode(callui_win_keygrab_mode_e mode)
{
	switch (mode) {
	case CALLUI_WIN_KEYGRAB_UNKNOWN:
		return ELM_WIN_KEYGRAB_UNKNOWN;
	case CALLUI_WIN_KEYGRAB_SHARED:
		return ELM_WIN_KEYGRAB_SHARED;
	case CALLUI_WIN_KEYGRAB_TOPMOST:
		return ELM_WIN_KEYGRAB_TOPMOST;
	case CALLUI_WIN_KEYGRAB_EXCLUSIVE:
		return ELM_WIN_KEYGRAB_EXCLUSIVE;
	case CALLUI_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE:
		return ELM_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE;
	default:
		return ELM_WIN_KEYGRAB_UNKNOWN;
	}
}

callui_result_e _callui_window_set_keygrab_mode(callui_window_h window_h, const char *key, callui_win_keygrab_mode_e mode)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(key, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(mode >= CALLUI_WIN_KEYGRAB_UNKNOWN &&
			mode <= CALLUI_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE, CALLUI_RESULT_INVALID_PARAM);

	if (!elm_win_keygrab_set(window_h->win, key, 0, 0, 0, __convert_app_keygrab_mode(mode))) {
		err("elm_win_keygrab_set() failed! key[%s] mode[%d]");
		return CALLUI_RESULT_FAIL;
	}
	return CALLUI_RESULT_OK;
}

callui_result_e _callui_window_unset_keygrab_mode(callui_window_h window_h, const char *key)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(key, CALLUI_RESULT_INVALID_PARAM);

	if (!elm_win_keygrab_unset(window_h->win, key, 0, 0)) {
		err("elm_win_keygrab_unset() failed! key[%s]");
		return CALLUI_RESULT_FAIL;
	}
	return CALLUI_RESULT_OK;
}

callui_result_e _callui_window_set_indicator_visible(callui_window_h window_h, bool is_visible)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h, CALLUI_RESULT_INVALID_PARAM);

	Elm_Win_Indicator_Mode indicator_mode = elm_win_indicator_mode_get(window_h->win);

	if (is_visible && indicator_mode == ELM_WIN_INDICATOR_HIDE) {
		elm_win_indicator_mode_set(window_h->win, ELM_WIN_INDICATOR_SHOW);
	} else if (!is_visible && indicator_mode == ELM_WIN_INDICATOR_SHOW) {
		elm_win_indicator_mode_set(window_h->win, ELM_WIN_INDICATOR_HIDE);
	} else {
		dbg("Indicator is already %s", is_visible ? "shown": "hidden");
	}
	return CALLUI_RESULT_OK;
}

callui_result_e _callui_window_set_above_lockscreen_mode(callui_window_h window_h, bool above_lockscreen)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h, CALLUI_RESULT_INVALID_PARAM);

	dbg("Above lock screen [%s]", above_lockscreen ? "YES" : "NO");

	int id = elm_win_aux_hint_id_get(window_h->win, "wm.policy.win.above.lock");
	if (above_lockscreen) {
		if (id == -1) {
			id = elm_win_aux_hint_add(window_h->win, "wm.policy.win.above.lock", "1");
			CALLUI_RETURN_VALUE_IF_FAIL(id >= 0, CALLUI_RESULT_FAIL);
		} else {
			CALLUI_RETURN_VALUE_IF_FAIL(elm_win_aux_hint_val_set(window_h->win, id, "1"), CALLUI_RESULT_FAIL);
		}
	} else {
		if (id != -1) {
		    CALLUI_RETURN_VALUE_IF_FAIL(elm_win_aux_hint_val_set(window_h->win, id, "0"), CALLUI_RESULT_FAIL);
		}
	}

	return CALLUI_RESULT_OK;
}

callui_result_e _callui_window_set_quickpanel_disable(callui_window_h window_h, bool is_disable)
{
	CALLUI_RETURN_VALUE_IF_FAIL(window_h, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(window_h->tzsh_qp, CALLUI_RESULT_INVALID_PARAM);

	int res = TZSH_ERROR_NONE;
	if (is_disable) {
		res = tzsh_quickpanel_scrollable_unset(window_h->tzsh_qp);
	} else {
		res = tzsh_quickpanel_scrollable_set(window_h->tzsh_qp);
	}
	CALLUI_RETURN_VALUE_IF_FAIL(res == TZSH_ERROR_NONE, CALLUI_RESULT_FAIL);

	return CALLUI_RESULT_OK;
}
