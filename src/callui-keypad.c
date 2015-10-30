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
#include "callui-view-layout-wvga.h"
#include "callui-view-caller-info-defines.h"

#define VC_KEYPAD_ENTRY_FONT "<font='Samsung Sans Num47:style=Light'>%s</>"
#define VC_KEYAD_ENTRY_STYLE "DEFAULT='align=center color=#ffffffff font_size=76'"
#define KEYPAD_ENTRY_DISP_DATA_SIZE		1024

typedef struct _keypad_data_t {
	Evas_Object *keypad_ly;
	Evas_Object *entry;
	int data_len;
	char entry_disp_data[KEYPAD_ENTRY_DISP_DATA_SIZE+1];
	Eina_Bool bkeypad_show;
	Ecore_Timer *anim_timer;

	Evas_Object *gesture_ly;
	int gesture_start_y;
	int gesture_momentum_y;
} keypad_data_t;

static keypad_data_t *gkeypad_data;

static Evas_Object *__callui_keypad_create_contents(callui_app_data_t *ad, char *grp_name)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad, NULL);
	Evas_Object *eo = NULL;

	/* load edje */
	eo = _callui_load_edj(ad->win, EDJ_NAME, grp_name);
	if (eo == NULL)
		return NULL;

	return eo;
}

static keypad_data_t *__callui_keypad_memory_alloc()
{
	dbg("..");

	CALLUI_RETURN_VALUE_IF_FAIL(gkeypad_data == NULL, gkeypad_data);

	gkeypad_data = (keypad_data_t *) calloc(1, sizeof(keypad_data_t));
	if (gkeypad_data == NULL) {
		err("keydata structure not allocated");
		return NULL;
	}
	memset(gkeypad_data, 0x00, sizeof(keypad_data_t));
	gkeypad_data->bkeypad_show = EINA_FALSE;

	return gkeypad_data;
}

static void __callui_keypad_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);

	_callui_keypad_hide_layout(ad);
}

static Evas_Event_Flags __callui_keypad_arrow_flick_gesture_event_cb(void *data, void *event_info)
{
	dbg("Flick_Gesture Move");
	Elm_Gesture_Line_Info *info = (Elm_Gesture_Line_Info *)event_info;
	keypad_data_t *pkeypad_data = gkeypad_data;

	dbg("*********************************************");
	dbg("info->angle = %lf", info->angle);
	dbg("info->momentum.mx = %d, info->momentum.my = %d", info->momentum.mx, info->momentum.my);
	dbg("info->momentum.n = %d", info->momentum.n);
	dbg("info->momentum.tx = %d, info->momentum.ty = %d", info->momentum.tx, info->momentum.ty);
	dbg("info->momentum.x1 = %d, info->momentum.x2 = %d", info->momentum.x1, info->momentum.x2);
	dbg("info->momentum.y1 = %d, info->momentum.y2 = %d", info->momentum.y1, info->momentum.y2);
	dbg("*********************************************");

	pkeypad_data->gesture_momentum_y = info->momentum.my;
	return EVAS_EVENT_FLAG_NONE;
}

static void __callui_keypad_arrow_mouse_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("..");
	keypad_data_t *pkeypad_data = gkeypad_data;
	Evas_Event_Mouse_Move *ev = event_info;

	pkeypad_data->gesture_start_y = ev->cur.canvas.y;

	return;
}

static void __callui_keypad_arrow_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	keypad_data_t *pkeypad_data = gkeypad_data;
	Evas_Event_Mouse_Move *ev = event_info;

	if (((ev->cur.canvas.y-pkeypad_data->gesture_start_y) > 100) && (pkeypad_data->gesture_momentum_y > 500)) {
		info("Hide keypad!!");
		_callui_keypad_hide_layout(ad);
	}

	return;
}

static void __callui_keypad_create_gesture_layer(void *data)
{
	dbg("..");

	keypad_data_t *pkeypad_data = gkeypad_data;
	Evas_Object *sweep_area = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)data;

	if (pkeypad_data->gesture_ly) {
		evas_object_del(pkeypad_data->gesture_ly);
		pkeypad_data->gesture_ly = NULL;
	}

	sweep_area = _callui_edje_object_part_get(pkeypad_data->keypad_ly, "sweep_area");
	pkeypad_data->gesture_ly = elm_gesture_layer_add(pkeypad_data->keypad_ly);
	if (FALSE == elm_gesture_layer_attach(pkeypad_data->gesture_ly, sweep_area)) {
		err("elm_gesture_layer_attach failed !!");
		evas_object_del(pkeypad_data->gesture_ly);
	} else {
		evas_object_event_callback_add(sweep_area, EVAS_CALLBACK_MOUSE_DOWN, __callui_keypad_arrow_mouse_down_cb, ad);
		evas_object_event_callback_add(sweep_area, EVAS_CALLBACK_MOUSE_UP, __callui_keypad_arrow_mouse_up_cb, ad);
		elm_gesture_layer_cb_set(pkeypad_data->gesture_ly, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_MOVE, __callui_keypad_arrow_flick_gesture_event_cb, ad);
	}

	return;
}


Eina_Bool _callui_keypad_get_show_status(void)
{
	keypad_data_t *pkeypad_data = gkeypad_data;
	CALLUI_RETURN_VALUE_IF_FAIL(pkeypad_data != NULL, EINA_FALSE);

	return pkeypad_data->bkeypad_show;
}

static void __callui_keypad_set_show_status(Eina_Bool bkeypad_status)
{
	keypad_data_t *pkeypad_data = gkeypad_data;

	CALLUI_RETURN_IF_FAIL(pkeypad_data != NULL);

	dbg("Set show status(%d)", bkeypad_status);
	pkeypad_data->bkeypad_show = bkeypad_status;
}

static void __callui_keypad_on_key_down(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	dbg("__callui_keypad_on_key_down");
	char *entry_dest = NULL;
	char *keypad_source = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);
	keypad_data_t *pkeypad_data = gkeypad_data;
	CALLUI_RETURN_IF_FAIL(pkeypad_data != NULL);
	char *entry_str = NULL;
	char *disp_str = NULL;

	if (source == NULL || strlen(source) == 0) {
		err("Source value is not valid");
		return;
	}

	if (strcmp(source, "star") == 0) {
		keypad_source = "*";
	} else if (strcmp(source, "sharp") == 0) {
		keypad_source = "#";
	} else {
		keypad_source = (char *)source;
	}

	cm_start_dtmf(ad->cm_handle, keypad_source[0]);

	const char *text = elm_entry_entry_get(pkeypad_data->entry);
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
		elm_object_text_set(pkeypad_data->entry, entry_str);
		g_free(entry_str);
	}
	elm_entry_cursor_end_set(pkeypad_data->entry);

	if (entry_dest) {
		free(entry_dest);
		entry_dest = NULL;
	}
}

static void __callui_keypad_on_key_up(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);

	cm_stop_dtmf(ad->cm_handle);
}

static Evas_Object *__callui_keypad_create_single_line_scrolled_entry(void *content)
{
	Evas_Object *en;
	Elm_Entry_Filter_Accept_Set digits_filter_data;

	if (content == NULL) {
		err("content is NULL!");
		return NULL;
	}

	en = elm_entry_add(content);
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

static void __callui_keypad_create_entry(void *data)
{
	dbg("..");
	keypad_data_t *pkeypad_data = gkeypad_data;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad != NULL);
	Evas_Object *current_ly = _callvm_get_view_layout(ad);

	CALLUI_RETURN_IF_FAIL(pkeypad_data != NULL);

	if (!pkeypad_data->entry) {
		dbg("..");
		pkeypad_data->entry = __callui_keypad_create_single_line_scrolled_entry(ad->win_conformant);
		memset(pkeypad_data->entry_disp_data, 0x0, sizeof(pkeypad_data->entry_disp_data));
		pkeypad_data->data_len = 0;

		elm_object_signal_callback_add(pkeypad_data->keypad_ly, "pad_down", "*", __callui_keypad_on_key_down, ad);
		elm_object_signal_callback_add(pkeypad_data->keypad_ly, "pad_up", "*", __callui_keypad_on_key_up, ad);

		if (current_ly) {
			edje_object_part_swallow(_EDJ(current_ly), PART_SWALLOW_TEXTBLOCK_AREA, pkeypad_data->entry);
		}
	}
}

static Eina_Bool __down_arrow_animation_timerout_cb(void *data)
{
	keypad_data_t *pkeypad_data = (keypad_data_t *)data;

	if (pkeypad_data->keypad_ly) {
		elm_object_signal_emit(pkeypad_data->keypad_ly, "start_animation", "down_arrow");
	}

	return ECORE_CALLBACK_RENEW;
}

void _callui_keypad_show_layout(void *app_data)
{
	dbg("..");
	keypad_data_t *pkeypad_data = gkeypad_data;
	CALLUI_RETURN_IF_FAIL(pkeypad_data);
	callui_app_data_t *ad = (callui_app_data_t *)app_data;
	CALLUI_RETURN_IF_FAIL(ad);
	Evas_Object *view_ly = _callvm_get_view_layout(ad);
	CALLUI_RETURN_IF_FAIL(view_ly);

	elm_object_signal_emit(pkeypad_data->keypad_ly, "SHOW", "KEYPADBTN");
	elm_object_signal_emit(view_ly, "SHOW", "KEYPAD_BTN");

	/* Start animation */
	elm_object_signal_emit(pkeypad_data->keypad_ly, "init", "down_arrow");
	elm_object_signal_emit(pkeypad_data->keypad_ly, "start_animation", "down_arrow");

	__callui_keypad_set_show_status(EINA_TRUE);
	/* change LCD timeout duration */
	_callui_lock_manager_stop(ad->lock_handle);
	_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_KEYPAD_SET);

	eext_object_event_callback_add(view_ly, EEXT_CALLBACK_BACK, __callui_keypad_back_cb, ad);

	if (pkeypad_data->anim_timer) {
		ecore_timer_del(pkeypad_data->anim_timer);
		pkeypad_data->anim_timer = NULL;
	}
	pkeypad_data->anim_timer = ecore_timer_add(2.0, __down_arrow_animation_timerout_cb, pkeypad_data);
}

void _callui_keypad_hide_layout(void *app_data)
{
	dbg("..");
	keypad_data_t *pkeypad_data = gkeypad_data;
	CALLUI_RETURN_IF_FAIL(pkeypad_data);
	callui_app_data_t *ad = (callui_app_data_t *)app_data;
	CALLUI_RETURN_IF_FAIL(ad);
	Evas_Object *view_ly = _callvm_get_view_layout(ad);
	CALLUI_RETURN_IF_FAIL(view_ly);

	elm_object_signal_emit(pkeypad_data->keypad_ly, "HIDE", "KEYPADBTN");
	elm_object_signal_emit(view_ly, "HIDE", "KEYPAD_BTN");

	__callui_keypad_set_show_status(EINA_FALSE);
	_callui_lock_manager_start(ad->lock_handle);
	_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_SET);

	eext_object_event_callback_del(view_ly, EEXT_CALLBACK_BACK,	__callui_keypad_back_cb);

	if (pkeypad_data->anim_timer) {
		ecore_timer_del(pkeypad_data->anim_timer);
		pkeypad_data->anim_timer = NULL;
	}
}

void _callui_keypad_create_layout(void *appdata)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	CALLUI_RETURN_IF_FAIL(ad);
	Evas_Object *parent_ly = _callvm_get_view_layout(ad);
	CALLUI_RETURN_IF_FAIL(parent_ly);

	_callui_keypad_delete_layout(ad);

	keypad_data_t *pkeypad_data = __callui_keypad_memory_alloc();
	CALLUI_RETURN_IF_FAIL(pkeypad_data != NULL);

	if (!pkeypad_data->keypad_ly) {
		dbg("..");
		pkeypad_data->keypad_ly = __callui_keypad_create_contents(ad, GRP_KEYPAD);

		__callui_keypad_create_gesture_layer(ad);

		elm_object_part_content_set(parent_ly, PART_SWALLOW_KEYPAD, pkeypad_data->keypad_ly);
	}

	__callui_keypad_create_entry(ad);
	memset(pkeypad_data->entry_disp_data, '\0', KEYPAD_ENTRY_DISP_DATA_SIZE+1);
	pkeypad_data->data_len = 0;
	elm_entry_entry_set(pkeypad_data->entry, "");
	elm_entry_cursor_end_set(pkeypad_data->entry);
}

void _callui_keypad_delete_layout(void *appdata)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	CALLUI_RETURN_IF_FAIL(ad);
	keypad_data_t *pkeypad_data = gkeypad_data;
	CALLUI_RETURN_IF_FAIL(pkeypad_data);
	Evas_Object *parent_ly = _callvm_get_view_layout(ad);
	CALLUI_RETURN_IF_FAIL(parent_ly);

	elm_object_signal_emit(parent_ly, "HIDE", "KEYPAD_AREA");

	if (pkeypad_data->entry) {
		edje_object_part_unswallow(_EDJ(pkeypad_data->keypad_ly), pkeypad_data->entry);
		evas_object_del(pkeypad_data->entry);
		pkeypad_data->entry = NULL;
	}

	if (pkeypad_data->anim_timer) {
		ecore_timer_del(pkeypad_data->anim_timer);
		pkeypad_data->anim_timer = NULL;
	}

	if (pkeypad_data->keypad_ly) {
		edje_object_part_unswallow(_EDJ(parent_ly), pkeypad_data->keypad_ly);
		evas_object_del(pkeypad_data->keypad_ly);
		pkeypad_data->keypad_ly = NULL;
	}

	g_free(pkeypad_data);
	pkeypad_data = NULL;
	gkeypad_data = NULL;
}

