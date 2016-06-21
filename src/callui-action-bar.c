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

#include <stdbool.h>
#include <network/bluetooth_type.h>
#include <network/bluetooth.h>

#include "callui-action-bar.h"
#include "callui-common-types.h"
#include "callui-debug.h"
#include "callui-keypad.h"
#include "callui-view-elements.h"
#include "callui-common.h"
#include "callui-sound-manager.h"
#include "callui-state-provider.h"

#define CALLUI_GROUP_BUTTON_LAYOUT			"action_bar"

#define CALLUI_PART_SWALLOW_TOP_FIRST		"swallow.top_first_btn"
#define CALLUI_PART_SWALLOW_TOP_SECOND		"swallow.top_second_btn"
#define CALLUI_PART_SWALLOW_TOP_THIRD		"swallow.top_third_btn"
#define CALLUI_PART_SWALLOW_BOTTOM_FIRST	"swallow.bottom_first_btn"
#define CALLUI_PART_SWALLOW_BOTTOM_SECOND	"swallow.bottom_second_btn"
#define CALLUI_PART_SWALLOW_BOTTOM_THIRD	"swallow.bottom_third_btn"
#define CALLUI_PART_SWALLOW_ACTION_BAR		"swallow.action_bar"

typedef enum {
	CALLUI_ACTION_BTN_SPEAKER = 0,
	CALLUI_ACTION_BTN_KEYPAD,
	CALLUI_ACTION_BTN_BT,
	CALLUI_ACTION_BTN_ADD_CALL,
	CALLUI_ACTION_BTN_MUTE,
	CALLUI_ACTION_BTN_CONTACT,
	CALLUI_ACTION_BTN_COUNT
} callui_btn_type_e;

struct _callui_action_bar {
	Evas_Object *main_layout;
	Evas_Object *buttons[CALLUI_ACTION_BTN_COUNT];
	bool is_available[CALLUI_ACTION_BTN_COUNT];
	callui_app_data_t *ad;
	bool is_disabled;
};
typedef struct _callui_action_bar _callui_action_bar_t;

struct __action_btn_style {
	char *normal;
	char *active;
	char *disable;
};
typedef struct __action_btn_style _action_btn_style_t;

typedef callui_result_e (*action_btn_update_func)(callui_action_bar_h action_bar);

struct __action_btn_params {
	char *txt;
	char *part;
	action_btn_update_func update_func;
	Evas_Smart_Cb click_cb_func;
	_action_btn_style_t style;
};
typedef struct __action_btn_params __action_btn_params_t;

static void __speaker_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __keypad_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __bluetooth_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __add_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __mute_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __contacts_btn_click_cb(void *data, Evas_Object *obj, void *event_info);

static callui_result_e __update_speaker_btn(callui_action_bar_h action_bar);
static callui_result_e __update_keypad_btn(callui_action_bar_h action_bar);
static callui_result_e __update_bluetooth_btn(callui_action_bar_h action_bar);
static callui_result_e __update_add_call_btn(callui_action_bar_h action_bar);
static callui_result_e __update_mute_btn(callui_action_bar_h action_bar);
static callui_result_e __update_contacts_btn(callui_action_bar_h action_bar);

static __action_btn_params_t btn_params[CALLUI_ACTION_BTN_COUNT] = {
		{
				"IDS_CALL_BUTTON_SPEAKER", CALLUI_PART_SWALLOW_TOP_FIRST,
				__update_speaker_btn, __speaker_btn_click_cb,
				{
						"callui_action_btn_speaker",
						"callui_action_btn_speaker_on",
						"callui_action_btn_speaker_disabled"
				}
		},
		{
				"IDS_CALL_SK3_KEYPAD", CALLUI_PART_SWALLOW_TOP_SECOND,
				__update_keypad_btn, __keypad_btn_click_cb,
				{
						"callui_action_btn_keypad",
						NULL,
						"callui_action_btn_keypad_disabled"
				}
		},
		{
				"IDS_CALL_BUTTON_BLUETOOTH_ABB", CALLUI_PART_SWALLOW_TOP_THIRD,
				__update_bluetooth_btn, __bluetooth_btn_click_cb,
				{
						"callui_action_btn_headset",
						"callui_action_btn_headset_on",
						"callui_action_btn_headset_disabled"
				}
		},
		{
				"IDS_CALL_BUTTON_ADD_CALL", CALLUI_PART_SWALLOW_BOTTOM_FIRST,
				__update_add_call_btn, __add_call_btn_click_cb,
				{
						"callui_action_btn_add",
						NULL,
						"callui_action_btn_add_disabled"
				}
		},
		{
				"IDS_CALL_BUTTON_MUTE_ABB", CALLUI_PART_SWALLOW_BOTTOM_SECOND,
				__update_mute_btn, __mute_btn_click_cb,
				{
						"callui_action_btn_mute",
						"callui_action_btn_mute_on",
						"callui_action_btn_mute_disabled"
				}
		},
		{
				"IDS_CALL_BUTTON_CONTACTS", CALLUI_PART_SWALLOW_BOTTOM_THIRD,
				__update_contacts_btn, __contacts_btn_click_cb,
				{
						"callui_action_btn_contacts",
						NULL,
						"callui_action_btn_contacts_disabled"
				}
		}
};

static callui_result_e __callui_action_bar_init(callui_action_bar_h action_bar, callui_app_data_t *ad);
static void __callui_action_bar_deinit(callui_action_bar_h action_bar);

static void __main_layout_del_cb(void *data, Evas *evas, Evas_Object *obj,void *event_info);
static Evas_Object *__create_main_layout(callui_action_bar_h action_bar, Evas_Object *parent);
static Evas_Object *__create_action_button( callui_action_bar_h action_bar, callui_btn_type_e type);

static void __disable_action_button(Evas_Object *action_btn, callui_btn_type_e type);

static void __update_btns_state(callui_action_bar_h action_bar);
static void __update_all_btns(callui_action_bar_h action_bar);

static void __call_state_event_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info);

static void __audio_state_changed_cb(void *user_data, callui_audio_state_type_e state);
static void __mute_state_changed_cb(void *user_data, bool is_enable);

static void __disable_action_button(Evas_Object *action_btn, callui_btn_type_e type)
{
	elm_object_style_set(action_btn, btn_params[type].style.disable);
	elm_object_disabled_set(action_btn, EINA_TRUE);
}

static void __speaker_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;
	callui_result_e res = CALLUI_RESULT_FAIL;

	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->sound_manager);
	bool speaker_status = false;
	if (audio_state == CALLUI_AUDIO_STATE_SPEAKER) {
		speaker_status = true;
	}

	res = _callui_sdm_set_speaker_state(ad->sound_manager, !speaker_status);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_sdm_set_speaker_state() failed. res[%d]", res);
	}
}

static void __keypad_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;

	if (_callui_keypad_get_show_status(ad->keypad)) {
		_callui_keypad_hide(ad->keypad);
	} else {
		_callui_keypad_show(ad->keypad);
	}
}

static void __bluetooth_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;

	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->sound_manager);
	switch (audio_state) {
	case CALLUI_AUDIO_STATE_BT:
		_callui_sdm_set_bluetooth_state(ad->sound_manager, false);
		break;
	case CALLUI_AUDIO_STATE_NONE:
		err("Invalid audio state");
		return;
	default:
		if (_callui_common_is_headset_conected(ad)) {
			_callui_sdm_set_bluetooth_state(ad->sound_manager, true);
		} else {
			bt_adapter_state_e bt_state = BT_ADAPTER_DISABLED;
			int ret_code = bt_adapter_get_state(&bt_state);
			if (ret_code == BT_ERROR_NONE) {
				info("BT status value: %d", bt_state);
				if (bt_state == BT_ADAPTER_DISABLED) {
					_callui_load_bluetooth_popup(ad);
				} else {
					_callui_common_launch_setting_bluetooth(ad);
				}
			} else {
				err("Fail to get vconf key: %d", ret_code);
			}
		}
	}
}

static void __add_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	_callui_common_launch_dialer(data);
}

static void __mute_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;

	callui_result_e res = _callui_sdm_set_mute_state(ad->sound_manager,
			!_callui_sdm_get_mute_state(ad->sound_manager));
	if (res != CALLUI_RESULT_OK) {
		err("_callui_sdm_set_mute_state() failed. res[%d]", res);
	}
}

static void __contacts_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	_callui_common_launch_contacts(data);
}

static callui_result_e __update_speaker_btn(callui_action_bar_h action_bar)
{
	callui_app_data_t *ad = action_bar->ad;
	Evas_Object *btn = action_bar->buttons[CALLUI_ACTION_BTN_SPEAKER];
	CALLUI_RETURN_VALUE_IF_FAIL(btn, CALLUI_RESULT_FAIL);

	if (!action_bar->is_available[CALLUI_ACTION_BTN_SPEAKER]) {
		__disable_action_button(btn, CALLUI_ACTION_BTN_SPEAKER);
		return CALLUI_RESULT_OK;
	}

	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->sound_manager);
	if (audio_state == CALLUI_AUDIO_STATE_SPEAKER) {
		elm_object_style_set(btn, btn_params[CALLUI_ACTION_BTN_SPEAKER].style.active);
	} else {
		elm_object_style_set(btn, btn_params[CALLUI_ACTION_BTN_SPEAKER].style.normal);
	}
	elm_object_disabled_set(btn, EINA_FALSE);

	return CALLUI_RESULT_OK;
}

static callui_result_e __update_keypad_btn(callui_action_bar_h action_bar)
{
	Evas_Object *btn = action_bar->buttons[CALLUI_ACTION_BTN_KEYPAD];
	CALLUI_RETURN_VALUE_IF_FAIL(btn, CALLUI_RESULT_FAIL);

	if (!action_bar->is_available[CALLUI_ACTION_BTN_KEYPAD]) {
		__disable_action_button(btn, CALLUI_ACTION_BTN_KEYPAD);
		return CALLUI_RESULT_OK;
	}

	elm_object_style_set(btn, btn_params[CALLUI_ACTION_BTN_KEYPAD].style.normal);
	elm_object_disabled_set(btn, EINA_FALSE);

	return CALLUI_RESULT_OK;
}

static callui_result_e __update_bluetooth_btn(callui_action_bar_h action_bar)
{
	callui_app_data_t *ad = action_bar->ad;
	Evas_Object *btn = action_bar->buttons[CALLUI_ACTION_BTN_BT];
	CALLUI_RETURN_VALUE_IF_FAIL(btn, CALLUI_RESULT_FAIL);

	if (!action_bar->is_available[CALLUI_ACTION_BTN_BT]) {
		__disable_action_button(btn, CALLUI_ACTION_BTN_BT);
		return CALLUI_RESULT_OK;
	}

	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->sound_manager);
	if (audio_state == CALLUI_AUDIO_STATE_BT) {
		elm_object_style_set(btn, btn_params[CALLUI_ACTION_BTN_BT].style.active);
	} else {
		elm_object_style_set(btn, btn_params[CALLUI_ACTION_BTN_BT].style.normal);
	}
	elm_object_disabled_set(btn, EINA_FALSE);

	return CALLUI_RESULT_OK;
}

static callui_result_e __update_add_call_btn(callui_action_bar_h action_bar)
{
	Evas_Object *btn = action_bar->buttons[CALLUI_ACTION_BTN_ADD_CALL];
	CALLUI_RETURN_VALUE_IF_FAIL(btn, CALLUI_RESULT_FAIL);

	if (!action_bar->is_available[CALLUI_ACTION_BTN_ADD_CALL]) {
		__disable_action_button(btn, CALLUI_ACTION_BTN_ADD_CALL);
		return CALLUI_RESULT_OK;
	}

	elm_object_style_set(btn, btn_params[CALLUI_ACTION_BTN_ADD_CALL].style.normal);
	elm_object_disabled_set(btn, EINA_FALSE);

	return CALLUI_RESULT_OK;
}

static callui_result_e __update_mute_btn(callui_action_bar_h action_bar)
{
	callui_app_data_t *ad = action_bar->ad;
	Evas_Object *btn = action_bar->buttons[CALLUI_ACTION_BTN_MUTE];
	CALLUI_RETURN_VALUE_IF_FAIL(btn, CALLUI_RESULT_FAIL);

	if (!action_bar->is_available[CALLUI_ACTION_BTN_MUTE]) {
		__disable_action_button(btn, CALLUI_ACTION_BTN_MUTE);
		return CALLUI_RESULT_OK;
	}

	if (_callui_sdm_get_mute_state(ad->sound_manager)) {
		elm_object_style_set(btn, btn_params[CALLUI_ACTION_BTN_MUTE].style.active);
	} else {
		elm_object_style_set(btn, btn_params[CALLUI_ACTION_BTN_MUTE].style.normal);
	}
	elm_object_disabled_set(btn, EINA_FALSE);

	return CALLUI_RESULT_OK;
}

static callui_result_e __update_contacts_btn(callui_action_bar_h action_bar)
{
	Evas_Object *btn = action_bar->buttons[CALLUI_ACTION_BTN_CONTACT];
	CALLUI_RETURN_VALUE_IF_FAIL(btn, CALLUI_RESULT_FAIL);

	if (!action_bar->is_available[CALLUI_ACTION_BTN_CONTACT]) {
		__disable_action_button(btn, CALLUI_ACTION_BTN_CONTACT);
		return CALLUI_RESULT_OK;
	}

	elm_object_style_set(btn, btn_params[CALLUI_ACTION_BTN_CONTACT].style.normal);
	elm_object_disabled_set(btn, EINA_FALSE);

	return CALLUI_RESULT_OK;
}

static void __main_layout_del_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_action_bar_h action_bar = data;

	action_bar->main_layout = NULL;
	_callui_action_bar_destroy(action_bar);
}

static Evas_Object *__create_main_layout(callui_action_bar_h action_bar, Evas_Object *parent)
{
	Evas_Object *layout = _callui_load_edj(parent, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_BUTTON_LAYOUT);
	CALLUI_RETURN_NULL_IF_FAIL(layout);
	evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, __main_layout_del_cb, action_bar);

	return layout;
}

static Evas_Object *__create_action_button(
		callui_action_bar_h action_bar,
		callui_btn_type_e type)
{
	Evas_Object *parent = action_bar->main_layout;

	Evas_Object *btn = elm_button_add(parent);
	CALLUI_RETURN_NULL_IF_FAIL(btn);
	action_bar->buttons[type] = btn;

	if (btn_params[type].update_func) {
		int res = btn_params[type].update_func(action_bar);
		CALLUI_RETURN_NULL_IF_FAIL(res == CALLUI_RESULT_OK);
	}

	elm_object_translatable_text_set(btn, btn_params[type].txt);
	evas_object_smart_callback_add(btn, "clicked", btn_params[type].click_cb_func, action_bar->ad);
	elm_object_part_content_set(parent, btn_params[type].part, btn);
	evas_object_show(btn);

	return btn;
}

static void __audio_state_changed_cb(void *user_data, callui_audio_state_type_e state)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_action_bar_h action_bar = user_data;

	if (action_bar->is_disabled) {
		dbg("Ignored. Action bar is in disabled state.");
		return;
	}

	__update_speaker_btn(action_bar);
	__update_bluetooth_btn(action_bar);
}

static void __mute_state_changed_cb(void *user_data, bool is_enable)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_action_bar_h action_bar = user_data;

	if (action_bar->is_disabled) {
		dbg("Ignored. Action bar is in disabled state.");
		return;
	}

	__update_mute_btn(action_bar);
}

static void __update_btns_state(callui_action_bar_h action_bar)
{
	int i = 0;
	for (; i < CALLUI_ACTION_BTN_COUNT; i++) {
		action_bar->is_available[i] = true;
	}

	const callui_call_data_t *active =
			_callui_stp_get_call_data(action_bar->ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	const callui_call_data_t *held =
			_callui_stp_get_call_data(action_bar->ad->state_provider, CALLUI_CALL_DATA_HELD);

	if (active && active->is_dialing) {
		action_bar->is_available[CALLUI_ACTION_BTN_ADD_CALL] = false;
		action_bar->is_available[CALLUI_ACTION_BTN_MUTE] = false;
		action_bar->is_available[CALLUI_ACTION_BTN_CONTACT] = false;
	} else if (active && held) {
		action_bar->is_available[CALLUI_ACTION_BTN_ADD_CALL] = false;
	} else if (held) {
		action_bar->is_available[CALLUI_ACTION_BTN_KEYPAD] = false;
		action_bar->is_available[CALLUI_ACTION_BTN_MUTE] = false;
	}
}

static void __update_all_btns(callui_action_bar_h action_bar)
{
	int i = 0;
	for (; i < CALLUI_ACTION_BTN_COUNT; i++) {
		if (btn_params[i].update_func) {
			btn_params[i].update_func(action_bar);
		}
	}
}

static void __call_state_event_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info)
{
	debug_enter();
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_action_bar_h action_bar = user_data;

	if (action_bar->is_disabled) {
		dbg("Ignored. Action bar is in disabled state.");
		return;
	}

	__update_btns_state(action_bar);

	__update_all_btns(action_bar);
}

static callui_result_e __callui_action_bar_init(callui_action_bar_h action_bar,	callui_app_data_t *ad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad->sound_manager, CALLUI_RESULT_FAIL);

	action_bar->ad = ad;
	action_bar->is_disabled = false;

	_callui_sdm_add_audio_state_changed_cb(ad->sound_manager, __audio_state_changed_cb, action_bar);
	_callui_sdm_add_mute_state_changed_cb(ad->sound_manager, __mute_state_changed_cb, action_bar);
	_callui_stp_add_call_state_event_cb(ad->state_provider, __call_state_event_cb, action_bar);

	__update_btns_state(action_bar);

	action_bar->main_layout = __create_main_layout(action_bar, _callui_vm_get_main_ly(ad->view_manager));
	CALLUI_RETURN_VALUE_IF_FAIL(action_bar->main_layout, CALLUI_RESULT_ALLOCATION_FAIL);

	Evas_Object *btn;
	int i = 0;
	for (; i < CALLUI_ACTION_BTN_COUNT; i++) {
		btn = __create_action_button(action_bar, i);
		CALLUI_RETURN_VALUE_IF_FAIL(btn, CALLUI_RESULT_ALLOCATION_FAIL);
	}

	return CALLUI_RESULT_OK;
}

callui_action_bar_h _callui_action_bar_create(callui_app_data_t *appdata)
{
	CALLUI_RETURN_NULL_IF_FAIL(appdata);

	callui_action_bar_h action_bar = calloc(1, sizeof(_callui_action_bar_t));
	CALLUI_RETURN_NULL_IF_FAIL(action_bar);

	int res = __callui_action_bar_init(action_bar, appdata);
	if (res != CALLUI_RESULT_OK) {
		err("Init action bar failed. res[%d]", res);
		_callui_action_bar_destroy(action_bar);
		action_bar = NULL;
	}
	return action_bar;
}

static void __callui_action_bar_deinit(callui_action_bar_h action_bar)
{
	callui_app_data_t *ad = action_bar->ad;

	evas_object_event_callback_del_full(action_bar->main_layout,
			EVAS_CALLBACK_DEL, __main_layout_del_cb, action_bar);

	_callui_sdm_remove_audio_state_changed_cb(ad->sound_manager, __audio_state_changed_cb, action_bar);
	_callui_sdm_remove_mute_state_changed_cb(ad->sound_manager, __mute_state_changed_cb, action_bar);
	_callui_stp_remove_call_state_event_cb(ad->state_provider, __call_state_event_cb, action_bar);

	int i = 0;
	for (; i < CALLUI_ACTION_BTN_COUNT; i++) {
		evas_object_smart_callback_del_full(action_bar->buttons[i], "clicked",
				btn_params[i].click_cb_func, ad);
	}

	evas_object_del(action_bar->main_layout);
}

void _callui_action_bar_destroy(callui_action_bar_h action_bar)
{
	CALLUI_RETURN_IF_FAIL(action_bar);

	__callui_action_bar_deinit(action_bar);

	free(action_bar);
}

static void __update_btns_txt(callui_action_bar_h action_bar)
{
	int i = 0;
	for (; i < CALLUI_ACTION_BTN_COUNT; i++) {
		elm_object_translatable_text_set(action_bar->buttons[i], btn_params[i].txt);
	}
}

void _callui_action_bar_show(callui_action_bar_h action_bar)
{
	CALLUI_RETURN_IF_FAIL(action_bar);

	__update_btns_txt(action_bar);
	elm_object_part_content_set(_callui_vm_get_main_ly(action_bar->ad->view_manager), CALLUI_PART_SWALLOW_ACTION_BAR, action_bar->main_layout);
	evas_object_show(action_bar->main_layout);
}

void _callui_action_bar_hide(callui_action_bar_h action_bar)
{
	CALLUI_RETURN_IF_FAIL(action_bar);

	elm_object_part_content_unset(_callui_vm_get_main_ly(action_bar->ad->view_manager), CALLUI_PART_SWALLOW_ACTION_BAR);
	evas_object_hide(action_bar->main_layout);
}

void _callui_action_bar_set_disabled_state(callui_action_bar_h action_bar, bool is_disabled)
{
	CALLUI_RETURN_IF_FAIL(action_bar);

	if (action_bar->is_disabled == is_disabled) {
		return;
	}
	action_bar->is_disabled = is_disabled;

	if (is_disabled) {
		int i = 0;
		for (; i < CALLUI_ACTION_BTN_COUNT; i++) {
			action_bar->is_available[i] = !is_disabled;
		}
	} else {
		__update_btns_state(action_bar);
	}
	__update_all_btns(action_bar);
}

bool _callui_action_bar_get_disabled_state(callui_action_bar_h action_bar)
{
	CALLUI_RETURN_VALUE_IF_FAIL(action_bar, false);

	return action_bar->is_disabled;
}
