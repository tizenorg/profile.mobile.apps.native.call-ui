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
#include "callui-keypad.h"
#include "callui-common.h"
#include "callui-view-elements.h"
#include "callui-view-layout.h"
#include "callui-view-caller-info-defines.h"

#define VC_KEYPAD_ENTRY_FONT "<font='Samsung Sans Num47:style=Light'>%s</>"
#define VC_KEYAD_ENTRY_STYLE "DEFAULT='align=center color=#ffffffff font_size=76'"

int __callui_keypad_init(callui_keypad_h keypad, Evas_Object *parent, callui_app_data_t *appdata);
void __callui_keypad_deinit(callui_keypad_h keypad);

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
static Eina_Bool __down_arrow_animation_timeout_cb(void *data);
static void __hide_keypad(callui_keypad_h keypad, Eina_Bool is_immediately);
static void __on_hide_completed(void *data, Evas_Object *obj, const char *emission, const char *source);

struct _callui_keypad {

	Evas_Object *main_layout;

	Evas_Object *btns_layout;
	Evas_Object *entry;
	Evas_Object *parent;

	Eina_Bool is_keypad_show;
	Ecore_Timer *anim_timer;

	Evas_Object *gesture_layer;
	int gesture_start_y;
	int gesture_momentum_y;

	callui_app_data_t *ad;

	show_state_change_cd cb_func;
	void *cb_data;
};

typedef struct _callui_keypad _callui_keypad_t;

int __callui_keypad_init(callui_keypad_h keypad, Evas_Object *parent, callui_app_data_t *appdata)
{
	keypad->ad = appdata;
	keypad->parent = parent;

	keypad->main_layout = _callui_load_edj(parent, EDJ_NAME, "keypad_layout");
	CALLUI_RETURN_VALUE_IF_FAIL(keypad->main_layout, CALLUI_RESULT_ALLOCATION_FAIL);

	elm_object_signal_callback_add(keypad->main_layout, "hide_completed", "*", __on_hide_completed, keypad);

	keypad->btns_layout = _callui_load_edj(keypad->main_layout, EDJ_NAME, GRP_KEYPAD);
	CALLUI_RETURN_VALUE_IF_FAIL(keypad->btns_layout, CALLUI_RESULT_ALLOCATION_FAIL);

	elm_object_part_content_set(keypad->main_layout, PART_SWALLOW_KEYPAD, keypad->btns_layout);

	int res = __create_gesture_layer(keypad);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = __create_entry(keypad);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	elm_object_part_content_set(keypad->ad->main_ly,
			PART_SWALLOW_KEYPAD_LAYOUT_AREA,
			keypad->main_layout);

	return res;
}

void __callui_keypad_deinit(callui_keypad_h keypad)
{
	if (keypad->anim_timer) {
		ecore_timer_del(keypad->anim_timer);
	}

	if (keypad->main_layout) {
		evas_object_del(keypad->btns_layout);
	}
}

callui_keypad_h _callui_keypad_create(Evas_Object *parent, void *appdata)
{
	CALLUI_RETURN_NULL_IF_FAIL(parent);
	CALLUI_RETURN_NULL_IF_FAIL(appdata);

	callui_keypad_h keypad = calloc(1, sizeof(_callui_keypad_t));
	CALLUI_RETURN_NULL_IF_FAIL(keypad);

	int res = __callui_keypad_init(keypad, parent, appdata);
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

	dbg("*********************************************");
	dbg("info->angle = %lf", info->angle);
	dbg("info->momentum.mx = %d, info->momentum.my = %d", info->momentum.mx, info->momentum.my);
	dbg("info->momentum.n = %d", info->momentum.n);
	dbg("info->momentum.tx = %d, info->momentum.ty = %d", info->momentum.tx, info->momentum.ty);
	dbg("info->momentum.x1 = %d, info->momentum.x2 = %d", info->momentum.x1, info->momentum.x2);
	dbg("info->momentum.y1 = %d, info->momentum.y2 = %d", info->momentum.y1, info->momentum.y2);
	dbg("*********************************************");

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

	if (((ev->cur.canvas.y-keypad_data->gesture_start_y) > 100) && (keypad_data->gesture_momentum_y > 500)) {
		__hide_keypad( keypad_data, EINA_FALSE);
	}
}

static int __create_gesture_layer(callui_keypad_h keypad)
{
	DELETE_EVAS_OBJECT(keypad->gesture_layer);

	Evas_Object *sweep_area = _callui_edje_object_part_get(keypad->btns_layout, "sweep_area");

	keypad->gesture_layer = elm_gesture_layer_add(keypad->btns_layout);
	if (FALSE == elm_gesture_layer_attach(keypad->gesture_layer, sweep_area)) {
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

	_callui_lock_manager_start(ad->lock_handle);

#ifdef _DBUS_DVC_LSD_TIMEOUT_
	_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_SET);
#endif

	eext_object_event_callback_del(keypad->parent, EEXT_CALLBACK_BACK,	__back_button_click_cb);

	if (keypad->anim_timer) {
		ecore_timer_del(keypad->anim_timer);
		keypad->anim_timer = NULL;
	}

	if (keypad->cb_func) {
		keypad->cb_func(keypad->cb_data, keypad->is_keypad_show);
	}
}

static void __on_key_down_click_event(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);
	CALLUI_RETURN_IF_FAIL(source);

	callui_keypad_h keypad = (callui_keypad_h)data;
	callui_app_data_t *ad = keypad->ad;

	char *entry_dest = NULL;
	char *keypad_source = NULL;
	char *entry_str = NULL;
	char *disp_str = NULL;

	if (strcmp(source, "star") == 0) {
		keypad_source = "*";
	} else if (strcmp(source, "sharp") == 0) {
		keypad_source = "#";
	} else {
		keypad_source = (char *)source;
	}

	cm_start_dtmf(ad->cm_handle, keypad_source[0]);

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

	entry_str = g_strdup_printf(VC_KEYPAD_ENTRY_FONT, entry_dest);
	if (entry_str) {
		elm_object_text_set(keypad->entry, entry_str);
		g_free(entry_str);
	}
	elm_entry_cursor_end_set(keypad->entry);

	if (entry_dest) {
		free(entry_dest);
		entry_dest = NULL;
	}
}

static void __on_key_up_click_event(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;

	cm_stop_dtmf(ad->cm_handle);
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

	digits_filter_data.accepted = "0123456789+*#";
	digits_filter_data.rejected = NULL;
	elm_entry_markup_filter_append(en, elm_entry_filter_accept_set, &digits_filter_data);

	elm_entry_context_menu_disabled_set(en, EINA_TRUE);
	elm_entry_cursor_end_set(en);
	evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_entry_text_style_user_push(en, VC_KEYAD_ENTRY_STYLE);

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
	elm_object_signal_callback_add(keypad->btns_layout, "pad_up", "*", __on_key_up_click_event, keypad->ad);

	__clear_entry(keypad);

	elm_object_part_content_set(keypad->main_layout, PART_SWALLOW_TEXTBLOCK_AREA, keypad->entry);

	return CALLUI_RESULT_OK;
}

static Eina_Bool __down_arrow_animation_timeout_cb(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_keypad_h keypad_data = (callui_keypad_h)data;

	if (keypad_data->btns_layout) {
		elm_object_signal_emit(keypad_data->btns_layout, "start_animation", "down_arrow");
	}

	return ECORE_CALLBACK_RENEW;
}

void _callui_keypad_show(callui_keypad_h keypad)
{
	CALLUI_RETURN_IF_FAIL(keypad);
	callui_app_data_t *ad = keypad->ad;

	elm_object_signal_emit(keypad->btns_layout, "SHOW", "KEYPADBTN");

	elm_object_signal_emit(keypad->btns_layout, "init", "down_arrow");
	elm_object_signal_emit(keypad->btns_layout, "start_animation", "down_arrow");

	elm_object_signal_emit(keypad->main_layout, "SHOW", "KEYPAD_BTN");

	keypad->is_keypad_show = EINA_TRUE;

	/* change LCD timeout duration */
	_callui_lock_manager_stop(ad->lock_handle);

#ifdef _DBUS_DVC_LSD_TIMEOUT_
	_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_KEYPAD_SET);
#endif

	eext_object_event_callback_add(keypad->parent, EEXT_CALLBACK_BACK, __back_button_click_cb, keypad);

	ecore_timer_del(keypad->anim_timer);
	keypad->anim_timer = ecore_timer_add(2.0, __down_arrow_animation_timeout_cb, keypad);

	if (keypad->cb_func) {
		keypad->cb_func(keypad->cb_data, keypad->is_keypad_show);
	}
}

static void __hide_keypad(callui_keypad_h keypad, Eina_Bool is_immediately)
{
	if (!keypad->is_keypad_show) {
		return;
	}

	keypad->is_keypad_show = EINA_FALSE;

	if (is_immediately) {
		elm_object_signal_emit(keypad->main_layout, "QUICK_HIDE", "KEYPAD_BTN");
	} else {
		elm_object_signal_emit(keypad->main_layout, "HIDE", "KEYPAD_BTN");
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
	CALLUI_RETURN_IF_FAIL(cb_func);
	CALLUI_RETURN_IF_FAIL(cb_data);

	keypad->cb_func = cb_func;
	keypad->cb_data = cb_data;
}