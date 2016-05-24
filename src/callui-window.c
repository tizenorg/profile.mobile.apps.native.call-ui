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

#include <efl_util.h>

#include "callui-window.h"
#include "callui.h"
#include "callui-common-def.h"
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
};

typedef struct __callui_window __callui_window_t;

static Evas_Object *__create_eo_window(callui_window_h win_handler);
static Evas_Object *__create_eo_conformant(Evas_Object *win);

static void __eo_win_rotation_changed_cb(void *data, Evas_Object *obj, void *event_info);

static void __eo_win_update_size(callui_window_h win_handler, callui_win_size_type_e size_type);
static callui_result_e __eo_win_set_rotation_locked(callui_window_h win_handler, bool is_locked);
static void __eo_win_move_and_resize(callui_window_h win_handler, bool force_resize);
static Elm_Win_Keygrab_Mode __convert_app_keygrab_mode(callui_win_keygrab_mode_e mode);

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

static void __eo_win_move_and_resize(callui_window_h win_handler, bool force_resize)
{
	callui_app_data_t *ad = win_handler->ad;

	int angle = elm_win_rotation_get(win_handler->win);
	dbg("Window orientation changed [%d]", angle);

	int win_h = win_handler->win_h;
	int win_w = win_handler->win_w;

	if (win_handler->size_type == CALLUI_WIN_SIZE_FULLSCREEN) {
		if (angle == 90 || angle == 270) {
			int temp = win_h;
			win_h = win_w;
			win_w = temp;
		}
	}
	if (force_resize) {
		evas_object_resize(win_handler->win, 0, 0);
	}
	evas_object_resize(win_handler->win, win_w, win_h);

	int move_x = 0;
	int move_y = 0;

	if (win_handler->size_type != CALLUI_WIN_SIZE_FULLSCREEN) {
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
	evas_object_move(win_handler->win, move_x, move_y);
}

static void __eo_win_rotation_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	__eo_win_move_and_resize(data, true);
}

static Evas_Object *__create_eo_window(callui_window_h win_handler)
{
	Evas_Object *eo = elm_win_add(NULL, PACKAGE, ELM_WIN_NOTIFICATION);
	CALLUI_RETURN_NULL_IF_FAIL(eo);

	elm_win_aux_hint_add(eo, "wm.policy.win.user.geometry", "1");
	elm_win_fullscreen_set(eo, EINA_FALSE);
	elm_win_alpha_set(eo, EINA_TRUE);

	evas_object_smart_callback_add(eo, "wm,rotation,changed", __eo_win_rotation_changed_cb, win_handler);

	elm_win_center(eo, EINA_FALSE, EINA_FALSE);
	elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(eo, ELM_WIN_INDICATOR_TRANSLUCENT);
	elm_win_conformant_set(eo, EINA_TRUE);

	return eo;
}

callui_result_e __callui_window_init(callui_window_h win_handler, callui_app_data_t *appdata)
{
	win_handler->ad = appdata;
	win_handler->rotation_locked = false;

	win_handler->win = __create_eo_window(win_handler);
	CALLUI_RETURN_VALUE_IF_FAIL(win_handler->win, CALLUI_RESULT_FAIL);

	win_handler->conformant = __create_eo_conformant(win_handler->win);
	CALLUI_RETURN_VALUE_IF_FAIL(win_handler->conformant, CALLUI_RESULT_FAIL);

	__eo_win_update_size(win_handler, CALLUI_WIN_SIZE_ACTIVE_NOTI);

	return __eo_win_set_rotation_locked(win_handler, true);
}

void __callui_window_deinit(callui_window_h win_handler)
{
	win_handler->conformant = NULL;
	evas_object_smart_callback_del_full(win_handler->win, "wm,rotation,changed", __eo_win_rotation_changed_cb, win_handler);
	DELETE_EVAS_OBJECT(win_handler->win);
}

callui_window_h _callui_window_create(callui_app_data_t *appdata)
{
	CALLUI_RETURN_NULL_IF_FAIL(appdata);

	callui_window_h win_handler = (callui_window_h)calloc(1, sizeof(__callui_window_t));
	CALLUI_RETURN_NULL_IF_FAIL(win_handler);

	if (__callui_window_init(win_handler, appdata) != CALLUI_RESULT_OK) {
		__callui_window_deinit(win_handler);
		FREE(win_handler);
	}
	return win_handler;
}

void _callui_window_destroy(callui_window_h win_handler)
{
	CALLUI_RETURN_IF_FAIL(win_handler);

	__callui_window_deinit(win_handler);

	free(win_handler);
}

Evas_Object *_callui_window_get_eo(callui_window_h win_handler)
{
	CALLUI_RETURN_NULL_IF_FAIL(win_handler);

	return win_handler->win;
}

Evas_Object *_callui_window_get_content_parent(callui_window_h win_handler)
{
	CALLUI_RETURN_NULL_IF_FAIL(win_handler);

	return win_handler->conformant;
}

void _callui_window_set_content(callui_window_h win_handler, Evas_Object *content)
{
	CALLUI_RETURN_IF_FAIL(win_handler);

	elm_object_content_set(win_handler->conformant, content);
}

static void __eo_win_update_size(callui_window_h win_handler, callui_win_size_type_e size_type)
{
	callui_app_data_t *ad = win_handler->ad;

	win_handler->size_type = size_type;
	win_handler->win_w = ad->root_w;

	switch (size_type) {
	case CALLUI_WIN_SIZE_FULLSCREEN:
		win_handler->win_h = ad->root_h;
		break;
	case CALLUI_WIN_SIZE_ACTIVE_NOTI:
		win_handler->win_h = CALLUI_WIN_ACT_NOTI_HEIGHT;
		break;
	default:
		break;
	}
	__eo_win_move_and_resize(win_handler, false);
}

void _callui_window_set_size_type(callui_window_h win_handler, callui_win_size_type_e size_type)
{
	CALLUI_RETURN_IF_FAIL(win_handler);
	CALLUI_RETURN_IF_FAIL((size_type >= CALLUI_WIN_SIZE_FULLSCREEN && size_type <= CALLUI_WIN_SIZE_ACTIVE_NOTI));

	__eo_win_update_size(win_handler, size_type);
}

static callui_result_e __eo_win_set_rotation_locked(callui_window_h win_handler, bool is_locked)
{
	CALLUI_RETURN_VALUE_IF_FAIL(win_handler->rotation_locked != is_locked,
			CALLUI_RESULT_ALREADY_REGISTERED);

	if (elm_win_wm_rotation_supported_get(win_handler->win)) {
		win_handler->rotation_locked = is_locked;

		if (win_handler->rotation_locked) {
			int rotate_angles[CALLUI_WIN_NORM_ROTATION_COUNT] = { 0 };
			elm_win_wm_rotation_available_rotations_set(win_handler->win,
					rotate_angles,
					CALLUI_WIN_NORM_ROTATION_COUNT);
		} else {
			int rotate_angles[CALLUI_WIN_ACT_NOTI_ROTATION_COUNT] = { 0, 90, 180, 270 };
			elm_win_wm_rotation_available_rotations_set(win_handler->win,
					rotate_angles,
					CALLUI_WIN_ACT_NOTI_ROTATION_COUNT);
		}
	} else {
		err("Window does not support rotation");
		return CALLUI_RESULT_FAIL;
	}

	return CALLUI_RESULT_OK;
}

callui_result_e _callui_window_set_rotation_locked(callui_window_h win_handler, bool is_locked)
{
	CALLUI_RETURN_VALUE_IF_FAIL(win_handler, CALLUI_RESULT_INVALID_PARAM);

	return __eo_win_set_rotation_locked(win_handler, is_locked);
}

int _callui_window_get_rotation(callui_window_h win_handler)
{
	CALLUI_RETURN_VALUE_IF_FAIL(win_handler, -1);

	return elm_win_rotation_get(win_handler->win);
}

callui_result_e _callui_window_set_top_level_priority(callui_window_h win_handler, bool is_toplevel)
{
	CALLUI_RETURN_VALUE_IF_FAIL(win_handler, CALLUI_RESULT_INVALID_PARAM);

	efl_util_notification_level_e efl_noti_level = EFL_UTIL_NOTIFICATION_LEVEL_NONE;
	if (is_toplevel) {
		dbg("Window level [TOP]");
		efl_noti_level = EFL_UTIL_NOTIFICATION_LEVEL_TOP;
	} else {
		dbg("Window level [NORMAL]");
	}
	int res = efl_util_set_notification_window_level(win_handler->win, efl_noti_level);
	if (res != EFL_UTIL_ERROR_NONE) {
		err("efl_util_set_notification_window_level() failed! res[%d]", res);
		return CALLUI_RESULT_FAIL;
	}

	return CALLUI_RESULT_OK;
}

callui_result_e _callui_window_set_screen_mode(callui_window_h win_handler, callui_win_screen_mode_e screen_mode)
{
	CALLUI_RETURN_VALUE_IF_FAIL(win_handler, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL((screen_mode >= CALLUI_WIN_SCREEN_MODE_DEFAULT &&
			screen_mode <= CALLUI_WIN_SCREEN_MODE_ALWAYS_ON), CALLUI_RESULT_INVALID_PARAM);

	efl_util_screen_mode_e efl_screen_mode = EFL_UTIL_SCREEN_MODE_DEFAULT;
	if (screen_mode == CALLUI_WIN_SCREEN_MODE_ALWAYS_ON) {
		dbg("Screen mode [Always on]");
		efl_screen_mode = EFL_UTIL_SCREEN_MODE_ALWAYS_ON;
	} else {
		dbg("Screen mode [Default]");
	}
	int res = efl_util_set_window_screen_mode(win_handler->win, efl_screen_mode);
	if (res != EFL_UTIL_ERROR_NONE) {
		err("efl_util_set_window_screen_mode() failed! res[%d]", res);
		return CALLUI_RESULT_FAIL;
	}

	return CALLUI_RESULT_OK;
}

void _callui_window_get_screen_size(callui_window_h win_handler, int *x, int *y, int *w, int *h)
{
	CALLUI_RETURN_IF_FAIL(win_handler);

	elm_win_screen_size_get(win_handler->win, x, y, w, h);
}

void _callui_window_activate(callui_window_h win_handler)
{
	CALLUI_RETURN_IF_FAIL(win_handler);

	elm_win_activate(win_handler->win);
	evas_object_show(win_handler->win);
}

void _callui_window_minimize(callui_window_h win_handler)
{
	CALLUI_RETURN_IF_FAIL(win_handler);

	elm_win_lower(win_handler->win);
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

callui_result_e _callui_window_set_keygrab_mode(callui_window_h win_handler, const char *key, callui_win_keygrab_mode_e mode)
{
	CALLUI_RETURN_VALUE_IF_FAIL(win_handler, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(key, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(mode >= CALLUI_WIN_KEYGRAB_UNKNOWN &&
			mode <= CALLUI_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE, CALLUI_RESULT_INVALID_PARAM);

	if (!elm_win_keygrab_set(win_handler->win, key, 0, 0, 0, __convert_app_keygrab_mode(mode))) {
		err("elm_win_keygrab_set() failed! key[%s] mode[%d]");
		return CALLUI_RESULT_FAIL;
	}
	return CALLUI_RESULT_OK;
}

callui_result_e _callui_window_unset_keygrab_mode(callui_window_h win_handler, const char *key)
{
	CALLUI_RETURN_VALUE_IF_FAIL(win_handler, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(key, CALLUI_RESULT_INVALID_PARAM);

	if (!elm_win_keygrab_unset(win_handler->win, key, 0, 0)) {
		err("elm_win_keygrab_unset() failed! key[%s]");
		return CALLUI_RESULT_FAIL;
	}
	return CALLUI_RESULT_OK;
}
