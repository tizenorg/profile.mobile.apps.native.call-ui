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

#include <linux/limits.h>
#include <efl_extension.h>

#include "callui.h"
#include "callui-debug.h"
#include "callui-keypad.h"
#include "callui-common.h"
#include "callui-view-elements.h"
#include "callui-view-layout.h"
#include "callui-sound-manager.h"

#define CALLUI_GROUP_KEYPAD						"keypad"
#define CALLUI_PART_SWALLOW_TEXT_AREA			"swallow.textarea"
#define CALLUI_PART_SWALLOW_KEYPAD				"swallow.keypad_region"
#define CALLUI_PART_SWALLOW_KEYPAD_LAYOUT_AREA	"swallow.keypad_layout_area"

#define CALLUI_KEYPAD_ENTRY_FONT	"<font='Samsung Sans Num47:style=Light'>%s</>"
#define CALLUI_KEYPAD_ENTRY_STYLE	"DEFAULT='align=center color=#ffffffff font_size=76'"

#define CALLUI_KEYPAD_AUTOSPACE_TIMEOUT_SEC	5.0

#define CALLUI_KEYPAD_GEST_Y_COORD_MIN		100
#define CALLUI_KEYPAD_GEST_MOMENTUM_MIN		500

static int	__callui_keypad_init(callui_keypad_h keypad, callui_app_data_t *appdata);
static void __callui_keypad_deinit(callui_keypad_h keypad);

static void __back_button_click_cb(void *data, Evas_Object *obj, void *event_info);
static Evas_Event_Flags __arrow_flick_gesture_event_cb(void *data, void *event_info);
static void __arrow_mouse_down_event_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void __arrow_mouse_up_event_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static int __create_gesture_layer(callui_keypad_h keypad);
static void __on_key_down_click_event(void *data, Evas_Object *obj, const char *emission, const char *source);
static void __on_key_up_click_event(void *data, Evas_Object *obj, const char *emission, const char *source);
static Evas_Object *__create_single_line_scrolled_entry(Evas_Object *content);
static int __create_entry(callui_keypad_h keypad);
static void __clear_entry(callui_keypad_h keypad);
static void __hide_keypad(callui_keypad_h keypad, Eina_Bool is_immediately);
static void __on_hide_completed(void *data, Evas_Object *obj, const char *emission, const char *source);

struct _callui_keypad {
	Evas_Object *main_layout;
	Evas_Object *btns_layout;
	Evas_Object *entry;
	Evas_Object *gesture_layer;

	Eina_Bool is_keypad_show;
	int gesture_start_y;
	int gesture_momentum_y;

	callui_app_data_t *ad;

	show_state_change_cd cb_func;
	void *cb_data;

	Ecore_Timer *auto_spacing_timer;
};

typedef struct _callui_keypad _callui_keypad_t;

static int __callui_keypad_init(callui_keypad_h keypad, callui_app_data_t *appdata)
{
	keypad->ad = appdata;

	keypad->main_layout = _callui_load_edj(_callui_vm_get_main_ly(appdata->view_manager), CALLUI_CALL_EDJ_PATH, "keypad_layout");
	CALLUI_RETURN_VALUE_IF_FAIL(keypad->main_layout, CALLUI_RESULT_ALLOCATION_FAIL);

	elm_object_signal_callback_add(keypad->main_layout, "hide_completed", "*", __on_hide_completed, keypad);

	keypad->btns_layout = _callui_load_edj(keypad->main_layout, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_KEYPAD);
	CALLUI_RETURN_VALUE_IF_FAIL(keypad->btns_layout, CALLUI_RESULT_ALLOCATION_FAIL);

	elm_object_part_content_set(keypad->main_layout, CALLUI_PART_SWALLOW_KEYPAD, keypad->btns_layout);

	int res = __create_gesture_layer(keypad);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = __create_entry(keypad);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	evas_object_hide(keypad->main_layout);

	return res;
}

static void __callui_keypad_deinit(callui_keypad_h keypad)
{
	DELETE_ECORE_TIMER(keypad->auto_spacing_timer);

	if (keypad->main_layout) {
		evas_object_del(keypad->btns_layout);
	}
}

callui_keypad_h _callui_keypad_create(callui_app_data_t *appdata)
{
	CALLUI_RETURN_NULL_IF_FAIL(appdata);

	callui_keypad_h keypad = calloc(1, sizeof(_callui_keypad_t));
	CALLUI_RETURN_NULL_IF_FAIL(keypad);

	int res = __callui_keypad_init(keypad, appdata);
	if (res != CALLUI_RESULT_OK) {
		err("Init keypad failed");
		_callui_keypad_destroy(keypad);
		keypad = NULL;
	}

	return keypad;
}

void _callui_keypad_destroy(callui_keypad_h keypad)
{
	CALLUI_RETURN_IF_FAIL(keypad);

	__callui_keypad_deinit(keypad);

	keypad->ad->keypad = NULL;

	free(keypad);
}

static void __clear_entry(callui_keypad_h keypad)
{
	elm_entry_entry_set(keypad->entry, "");
	elm_entry_cursor_end_set(keypad->entry);
}

void _callui_keypad_clear_input(callui_keypad_h keypad)
{
	CALLUI_RETURN_IF_FAIL(keypad);

	__clear_entry(keypad);
}

static void __back_button_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	__hide_keypad((callui_keypad_h)data, EINA_FALSE);
}

static Evas_Event_Flags __arrow_flick_gesture_event_cb(void *data, void *event_info)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, EVAS_EVENT_FLAG_NONE);
	CALLUI_RETURN_VALUE_IF_FAIL(event_info, EVAS_EVENT_FLAG_NONE);

	callui_keypad_h keypad_data = (callui_keypad_h)data;
	Elm_Gesture_Line_Info *info = (Elm_Gesture_Line_Info *)event_info;

	keypad_data->gesture_momentum_y = info->momentum.my;

	return EVAS_EVENT_FLAG_NONE;
}

static void __arrow_mouse_down_event_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	CALLUI_RETURN_IF_FAIL(event_info);

	callui_keypad_h keypad_data = (callui_keypad_h)data;
	Evas_Event_Mouse_Move *ev = event_info;

	keypad_data->gesture_start_y = ev->cur.canvas.y;
}

static void __arrow_mouse_up_event_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	CALLUI_RETURN_IF_FAIL(event_info);

	callui_keypad_h keypad_data = (callui_keypad_h)data;
	Evas_Event_Mouse_Move *ev = event_info;

	if (((ev->cur.canvas.y-keypad_data->gesture_start_y) > CALLUI_KEYPAD_GEST_Y_COORD_MIN)
			&& (keypad_data->gesture_momentum_y > CALLUI_KEYPAD_GEST_MOMENTUM_MIN)) {
		__hide_keypad( keypad_data, EINA_FALSE);
	}
}

static int __create_gesture_layer(callui_keypad_h keypad)
{
	DELETE_EVAS_OBJECT(keypad->gesture_layer);

	Evas_Object *sweep_area = _callui_edje_object_part_get(keypad->btns_layout, "sweep_area");

	keypad->gesture_layer = elm_gesture_layer_add(keypad->btns_layout);
	if (!elm_gesture_layer_attach(keypad->gesture_layer, sweep_area)) {
		err("elm_gesture_layer_attach failed !!");
		DELETE_EVAS_OBJECT(keypad->gesture_layer);
		return CALLUI_RESULT_ALLOCATION_FAIL;
	} else {
		evas_object_event_callback_add(sweep_area, EVAS_CALLBACK_MOUSE_DOWN,
				__arrow_mouse_down_event_cb, keypad);

		evas_object_event_callback_add(sweep_area, EVAS_CALLBACK_MOUSE_UP,
				__arrow_mouse_up_event_cb, keypad);

		elm_gesture_layer_cb_set(keypad->gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_MOVE,
				__arrow_flick_gesture_event_cb, keypad);
	}

	return CALLUI_RESULT_OK;
}


Eina_Bool _callui_keypad_get_show_status(callui_keypad_h keypad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(keypad, EINA_FALSE);

	return keypad->is_keypad_show;
}

static void __on_hide_completed(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_keypad_h keypad = (callui_keypad_h)data;
	callui_app_data_t *ad = keypad->ad;
	Evas_Object *main_ly = _callui_vm_get_main_ly(ad->view_manager);

	_callui_lock_manager_start(ad->lock_handle);

	eext_object_event_callback_del(main_ly, EEXT_CALLBACK_BACK, __back_button_click_cb);

	keypad->main_layout = elm_object_part_content_unset(main_ly,
			CALLUI_PART_SWALLOW_KEYPAD_LAYOUT_AREA);

	evas_object_hide(keypad->main_layout);

	if (keypad->cb_func) {
		keypad->cb_func(keypad->cb_data, keypad->is_keypad_show);
	}
}

static Eina_Bool __auto_spacing_timer_cb(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_keypad_h keypad = data;
	if (keypad->entry) {
		elm_entry_entry_append(keypad->entry, " ");
	}
	return ECORE_CALLBACK_DONE;
}

static void __on_key_down_click_event(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);
	CALLUI_RETURN_IF_FAIL(source);

	callui_keypad_h keypad = data;
	callui_app_data_t *ad = keypad->ad;

	char *entry_dest = NULL;
	char *keypad_source = NULL;
	char *disp_str = NULL;

	if (strcmp(source, "star") == 0) {
		keypad_source = "*";
	} else if (strcmp(source, "sharp") == 0) {
		keypad_source = "#";
	} else {
		keypad_source = (char *)source;
	}

	callui_result_e res = _callui_sdm_start_dtmf(ad->sound_manager, keypad_source[0]);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_sdm_start_dtmf() failed. res[%d]", res);
		return;
	}

	const char *text = elm_entry_entry_get(keypad->entry);
	disp_str = elm_entry_markup_to_utf8(text);

	if (disp_str == NULL) {
		err("disp_str is null");
		return;
	} else if (strlen(disp_str) == 0) {
		entry_dest = calloc(1, 2);
		if (entry_dest == NULL) {
			err("entry_dest allocation fail");
			free(disp_str);
			return;
		}
		snprintf(entry_dest, 2, "%c", keypad_source[0]);
	} else {
		int buf_size = strlen(disp_str) + 2;
		entry_dest = calloc(1, buf_size);
		if (entry_dest == NULL) {
			err("entry_dest allocation fail");
			free(disp_str);
			return;
		}
		snprintf(entry_dest, buf_size, "%s%c", disp_str, keypad_source[0]);
	}

	free(disp_str);

	char tmp[NAME_MAX] = { 0 };
	snprintf(tmp, NAME_MAX, CALLUI_KEYPAD_ENTRY_FONT, entry_dest);

	elm_object_text_set(keypad->entry, tmp);

	elm_entry_cursor_end_set(keypad->entry);

	if (entry_dest) {
		free(entry_dest);
		entry_dest = NULL;
	}
}

static void __on_key_up_click_event(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_keypad_h keypad = data;

	callui_result_e res = _callui_sdm_stop_dtmf(keypad->ad->sound_manager);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_sdm_stop_dtmf() failed. res[%d]", res);
	}

	DELETE_ECORE_TIMER(keypad->auto_spacing_timer);
	keypad->auto_spacing_timer = ecore_timer_add(CALLUI_KEYPAD_AUTOSPACE_TIMEOUT_SEC, __auto_spacing_timer_cb, keypad);
}

static Evas_Object *__create_single_line_scrolled_entry(Evas_Object *content)
{
	CALLUI_RETURN_NULL_IF_FAIL(content);

	Elm_Entry_Filter_Accept_Set digits_filter_data;

	Evas_Object *en = elm_entry_add(content);
	CALLUI_RETURN_NULL_IF_FAIL(en);
	elm_entry_editable_set(en, EINA_FALSE);
	elm_entry_scrollable_set(en, EINA_TRUE);

	elm_entry_select_all(en);
	elm_scroller_policy_set(en, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	elm_scroller_bounce_set(en, EINA_FALSE, EINA_FALSE);
	elm_entry_line_wrap_set(en, ELM_WRAP_WORD);
	elm_entry_input_panel_enabled_set(en, EINA_FALSE);
	elm_entry_single_line_set(en, EINA_TRUE);

	digits_filter_data.accepted = "0123456789+*# ";
	digits_filter_data.rejected = NULL;
	elm_entry_markup_filter_append(en, elm_entry_filter_accept_set, &digits_filter_data);

	elm_entry_context_menu_disabled_set(en, EINA_TRUE);
	elm_entry_cursor_end_set(en);
	evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_entry_text_style_user_push(en, CALLUI_KEYPAD_ENTRY_STYLE);

	evas_object_show(en);

	return en;
}

static int __create_entry(callui_keypad_h keypad)
{
	keypad->entry = __create_single_line_scrolled_entry(keypad->main_layout);
	if (!keypad->entry) {
		err("Create entry failed");
		return CALLUI_RESULT_ALLOCATION_FAIL;
	}

	elm_object_signal_callback_add(keypad->btns_layout, "pad_down", "*", __on_key_down_click_event, keypad);
	elm_object_signal_callback_add(keypad->btns_layout, "pad_up", "*", __on_key_up_click_event, keypad);

	__clear_entry(keypad);

	elm_object_part_content_set(keypad->main_layout, CALLUI_PART_SWALLOW_TEXT_AREA, keypad->entry);

	return CALLUI_RESULT_OK;
}

void _callui_keypad_show(callui_keypad_h keypad)
{
	CALLUI_RETURN_IF_FAIL(keypad);
	callui_app_data_t *ad = keypad->ad;
	Evas_Object *main_ly = _callui_vm_get_main_ly(ad->view_manager);

	__clear_entry(keypad);

	elm_object_part_content_set(main_ly, CALLUI_PART_SWALLOW_KEYPAD_LAYOUT_AREA, keypad->main_layout);
	evas_object_show(keypad->main_layout);

	elm_object_signal_emit(keypad->btns_layout, "show", "keypad");

	elm_object_signal_emit(keypad->main_layout, "show", "keypad_layout");

	keypad->is_keypad_show = EINA_TRUE;

	_callui_lock_manager_stop(ad->lock_handle);

	eext_object_event_callback_add(main_ly, EEXT_CALLBACK_BACK, __back_button_click_cb, keypad);

	if (keypad->cb_func) {
		keypad->cb_func(keypad->cb_data, keypad->is_keypad_show);
	}
}

static void __hide_keypad(callui_keypad_h keypad, Eina_Bool is_immediately)
{
	if (!keypad->is_keypad_show) {
		return;
	}

	DELETE_ECORE_TIMER(keypad->auto_spacing_timer);

	keypad->is_keypad_show = EINA_FALSE;

	if (is_immediately) {
		elm_object_signal_emit(keypad->main_layout, "quick_hide", "keypad_layout");
	} else {
		elm_object_signal_emit(keypad->main_layout, "hide", "keypad_layout");
	}
}

void _callui_keypad_hide(callui_keypad_h keypad)
{
	CALLUI_RETURN_IF_FAIL(keypad);

	__hide_keypad(keypad, EINA_FALSE);
}

void _callui_keypad_hide_immediately(callui_keypad_h keypad)
{
	CALLUI_RETURN_IF_FAIL(keypad);

	__hide_keypad(keypad, EINA_TRUE);
}

void _callui_keypad_show_status_change_callback_set(callui_keypad_h keypad, show_state_change_cd cb_func, void *cb_data)
{
	CALLUI_RETURN_IF_FAIL(keypad);

	keypad->cb_func = cb_func;
	keypad->cb_data = cb_data;
}
