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

#include <app.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <bluetooth.h>
#include <system_settings.h>

#include "callui.h"
#include "callui-debug.h"
#include "callui-view-elements.h"
#include "callui-common.h"
#include "callui-view-quickpanel.h"
#include "callui-view-layout.h"
#include "callui-sound-manager.h"
#include "callui-state-provider.h"

#define EARSET_KEY_LONG_PRESS_TIMEOUT	1.0

static bool __app_create(void *data);
static void __app_terminate(void *data);
static void __app_pause(void *data);
static void __app_resume(void *data);
static void __app_service(app_control_h app_control, void *data);
static void __app_lang_changed_cb(app_event_info_h event_info, void *user_data);

static bool __app_init(callui_app_data_t *ad);
static bool __app_deinit(callui_app_data_t *ad);

static bool __create_main_gui_elem(callui_app_data_t *ad);
static Evas_Object *__create_main_window(callui_app_data_t *ad);
static Evas_Object *__create_conformant(Evas_Object *win);
static Evas_Object *__create_main_layout(Evas_Object *conf);

static void __init_app_event_handlers(callui_app_data_t *ad);

static void __main_win_delete_request_cb(void *data, Evas_Object *obj, void *event_info);

static Eina_Bool __hard_key_down_cb(void *data, int type, void *event);
static Eina_Bool __hard_key_up_cb(void *data, int type, void *event);

static void __audio_state_changed_cb(void *user_data,
		callui_audio_state_type_e audio_state);
static void __call_state_change_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info);

static void __process_outgoing_call(callui_app_data_t *ad, const char *number);
static void __process_incoming_call(callui_app_data_t *ad);

static void __reset_state_params(callui_app_data_t *ad);
static void __add_ecore_event_key_handlers(callui_app_data_t *ad);
static void __remove_ecore_event_key_handlers(callui_app_data_t *ad);
static void __set_main_win_key_grab(callui_app_data_t *ad);
static void __unset_main_win_key_grab(callui_app_data_t *ad);
static void __set_text_classes_params();

static callui_app_data_t g_ad;

static void __set_main_win_key_grab(callui_app_data_t *ad)
{
	int result = elm_win_keygrab_set(ad->win, CALLUI_KEY_MEDIA, 0, 0, 0, ELM_WIN_KEYGRAB_EXCLUSIVE);
	if (!result) {
		dbg("KEY_MEDIA key grab failed");
	}

	if (_callui_common_is_powerkey_mode_on()) {
		result = elm_win_keygrab_set(ad->win, CALLUI_KEY_POWER, 0, 0, 0, ELM_WIN_KEYGRAB_EXCLUSIVE);
	} else {
		result = elm_win_keygrab_set(ad->win, CALLUI_KEY_POWER, 0, 0, 0, ELM_WIN_KEYGRAB_SHARED);
	}
	if (!result) {
		dbg("KEY_POWER key grab failed");
	}
}

static void __unset_main_win_key_grab(callui_app_data_t *ad)
{
	elm_win_keygrab_unset(ad->win, CALLUI_KEY_SELECT, 0, 0);
	elm_win_keygrab_unset(ad->win, CALLUI_KEY_POWER, 0, 0);
	elm_win_keygrab_unset(ad->win, CALLUI_KEY_MEDIA, 0, 0);
}

static void __add_ecore_event_key_handlers(callui_app_data_t *ad)
{
	if (!ad->downkey_handler) {
		ad->downkey_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, __hard_key_down_cb, ad);
	}
	if (!ad->upkey_handler) {
		ad->upkey_handler = ecore_event_handler_add(ECORE_EVENT_KEY_UP, __hard_key_up_cb, ad);
	}
}

static void __remove_ecore_event_key_handlers(callui_app_data_t *ad)
{
	if (ad->downkey_handler) {
		ecore_event_handler_del(ad->downkey_handler);
		ad->downkey_handler = NULL;
	}
	if (ad->upkey_handler) {
		ecore_event_handler_del(ad->upkey_handler);
		ad->upkey_handler = NULL;
	}
}

static void __process_incoming_call(callui_app_data_t *ad)
{
	CALLUI_RETURN_IF_FAIL(ad);

	const callui_call_data_t *incom = _callui_stp_get_call_data(ad->state_provider,
					CALLUI_CALL_DATA_INCOMING);
	const callui_call_data_t *active = _callui_stp_get_call_data(ad->state_provider,
					CALLUI_CALL_DATA_ACTIVE);
	const callui_call_data_t *held = _callui_stp_get_call_data(ad->state_provider,
					CALLUI_CALL_DATA_HELD);

	callui_view_type_e type = CALLUI_VIEW_INCOMING_CALL;
	callui_view_type_e cur_type = _callui_vm_get_cur_view_type(ad->view_manager);
	if (_callui_common_get_idle_lock_type() == LOCK_TYPE_UNLOCK &&
			active == NULL &&
			held == NULL &&
			incom != NULL &&
			(cur_type == CALLUI_VIEW_UNDEFINED || cur_type == CALLUI_VIEW_ENDCALL)) {
		type = CALLUI_VIEW_INCOMING_CALL_NOTI;
	}
	_callui_vm_change_view(ad->view_manager, type);
}

static void __process_outgoing_call(callui_app_data_t *ad, const char *number)
{
	CALLUI_RETURN_IF_FAIL(ad);
	sec_dbg("Number is (%s)", number);

	callui_result_e res = _callui_manager_dial_voice_call(ad->call_manager,
			number, CALLUI_SIM_SLOT_DEFAULT);

	if (CALLUI_RESULT_OK != res) {
		err("_callui_manager_dial_voice_call() failed. ret[%d]", res);
		if (!_callui_stp_is_any_calls_available(ad->state_provider)) {
			dbg("No more calls available. Exit application");
			_callui_common_exit_app();
		}
	} else {
		ad->waiting_dialing = true;
	}
}

static void __audio_state_changed_cb(void *user_data, callui_audio_state_type_e audio_state)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_app_data_t *ad = (callui_app_data_t *)user_data;

	switch (audio_state) {
		case CALLUI_AUDIO_STATE_SPEAKER:
		case CALLUI_AUDIO_STATE_EARJACK:
		case CALLUI_AUDIO_STATE_BT:
			_callui_lock_manager_force_stop(ad->lock_handle);
			break;
		case CALLUI_AUDIO_STATE_RECEIVER:
			_callui_lock_manager_start(ad->lock_handle);
			break;
		default:
			break;
	}
}

static void __call_state_change_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_app_data_t *ad = user_data;

	ad->waiting_dialing = false;

	switch (call_event_type) {
	case CALLUI_CALL_EVENT_ACTIVE:
		if (_callui_lock_manager_is_lcd_off(ad->lock_handle)) {
			_callui_common_dvc_control_lcd_state(LCD_UNLOCK);
		} else {
			_callui_common_dvc_control_lcd_state(LCD_ON_UNLOCK);
		}
#ifdef _DBUS_DVC_LSD_TIMEOUT_
		callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->sound_manager);
		if (audio_state == CALLUI_AUDIO_STATE_SPEAKER) {
			_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_SET);
		}
#endif
		break;
	case CALLUI_CALL_EVENT_END:
		if (_callui_lock_manager_is_lcd_off(ad->lock_handle)) {
			_callui_common_dvc_control_lcd_state(LCD_UNLOCK);
		} else {
			_callui_common_dvc_control_lcd_state(LCD_ON_UNLOCK);
		}
		break;
	default:
		break;
	}
}

static void __reset_state_params(callui_app_data_t *ad)
{
	CALLUI_RETURN_IF_FAIL(ad);

	ad->start_lock_manager_on_resume = false;
	ad->on_background = false;

	return;
}

static void __bt_init()
{
	int ret = bt_initialize();
	if (BT_ERROR_NONE != ret) {
		err("bt_init() failed [%d]", ret);
	}
}

static void __bt_deinit()
{
	int ret = bt_deinitialize();
	if (BT_ERROR_NONE != ret) {
		err("bt_deinit() failed [%d]", ret);
	}
}

static void __set_text_classes_params()
{
	edje_text_class_set("ATO001", "R", 40);
	edje_text_class_set("ATO002", "R", 64);
	edje_text_class_set("ATO003", "R", 40);
	edje_text_class_set("ATO004", "R", 24);
	edje_text_class_set("ATO005", "R", 27);
	edje_text_class_set("ATO006", "R", 24);
	edje_text_class_set("ATO007", "R", 24);
	edje_text_class_set("ATO008", "R", 24);
	edje_text_class_set("ATO010", "R", 30);
	edje_text_class_set("ATO011", "R", 30);
	edje_text_class_set("ATO012", "R", 30);
	edje_text_class_set("ATO013", "R", 30);
	edje_text_class_set("ATO014", "R", 30);
	edje_text_class_set("ATO016", "R", 40);
	edje_text_class_set("ATO017", "R", 40);
	edje_text_class_set("ATO027", "R", 26);
	edje_text_class_set("ATO028", "R", 24);
	edje_text_class_set("ATO030", "R", 42);
	edje_text_class_set("ATO017", "R", 30);
}

static void __init_app_event_handlers(callui_app_data_t *ad)
{
	app_event_type_e events[APP_HANDLERS_COUNT] = {
			APP_EVENT_LANGUAGE_CHANGED
	};

	app_event_cb cbs[APP_HANDLERS_COUNT] = {
			__app_lang_changed_cb
	};

	int i = 0;
	for (; i < APP_HANDLERS_COUNT; ++i) {
		int res = ui_app_add_event_handler(&ad->app_event_handlers[i], events[i], cbs[i], ad);
		if (res != APP_ERROR_NONE) {
			warn("ui_app_add_event_handler(%d) failed. res[%d]", events[i], res);
			ad->app_event_handlers[i] = NULL;
		}
	}
}

static void __dial_status_cb(void *user_data, callui_dial_status_e dial_status)
{
	debug_enter();

	CALLUI_RETURN_IF_FAIL(user_data);

	callui_app_data_t *ad = user_data;

	if (dial_status != CALLUI_DIAL_SUCCESS) {
		if (!_callui_stp_is_any_calls_available(ad->state_provider)) {
			dbg("No more calls available. Exit application");
			_callui_common_exit_app();
		}
		ad->waiting_dialing = false;
	}
}

static bool __app_init(callui_app_data_t *ad)
{
	_callui_common_dvc_control_lcd_state(LCD_OFF_SLEEP_LOCK);

	__init_app_event_handlers(ad);

	__bt_init();

	ad->call_manager = _callui_manager_create();
	CALLUI_RETURN_VALUE_IF_FAIL(ad->call_manager, __app_deinit(ad));

	ad->state_provider = _callui_manager_get_state_provider(ad->call_manager);
	ad->sound_manager = _callui_manager_get_sound_manager(ad->call_manager);

	_callui_manager_add_dial_status_cb(ad->call_manager, __dial_status_cb, ad);
	_callui_stp_add_call_state_event_cb(ad->state_provider, __call_state_change_cb, ad);
	_callui_sdm_add_audio_state_changed_cb(ad->sound_manager, __audio_state_changed_cb, ad);

	CALLUI_RETURN_VALUE_IF_FAIL(__create_main_gui_elem(ad), __app_deinit(ad));

	ad->view_manager = _callui_vm_create(ad);
	CALLUI_RETURN_VALUE_IF_FAIL(ad->view_manager, __app_deinit(ad));

	ad->action_bar = _callui_action_bar_create(ad);
	CALLUI_RETURN_VALUE_IF_FAIL(ad->action_bar, __app_deinit(ad));

	ad->keypad = _callui_keypad_create(ad);
	CALLUI_RETURN_VALUE_IF_FAIL(ad->keypad, __app_deinit(ad));

	ad->lock_handle = _callui_lock_manager_create();
	CALLUI_RETURN_VALUE_IF_FAIL(ad->lock_handle, __app_deinit(ad));

	ad->qp_minicontrol =_callui_qp_mc_create(ad);
	CALLUI_RETURN_VALUE_IF_FAIL(ad->qp_minicontrol, __app_deinit(ad));

	__set_main_win_key_grab(ad);

	__add_ecore_event_key_handlers(ad);

	__set_text_classes_params();

	elm_theme_extension_add(NULL, CALL_THEME);

	return true;
}

static bool __app_create(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, false);

	elm_app_base_scale_set(2.6);

	elm_config_accel_preference_set("3d");

	return __app_init(data);
}

static void __app_lang_changed_cb(app_event_info_h event_info, void *user_data)
{
	char *language;
	int r = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &language);
	if (r == SYSTEM_SETTINGS_ERROR_NONE) {
		dbg("language: %s", language);
		elm_language_set(language);
		free(language);
	}

	CALLUI_RETURN_IF_FAIL(user_data);
	callui_app_data_t *ad = user_data;

	_callui_vm_update_language(ad->view_manager);
}

static void __main_win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	elm_exit();
}

static Evas_Object *__create_main_window(callui_app_data_t *ad)
{
	Evas_Object *eo = elm_win_add(NULL, PACKAGE, ELM_WIN_NOTIFICATION);
	CALLUI_RETURN_NULL_IF_FAIL(eo);

	elm_win_aux_hint_add(eo, "wm.policy.win.user.geometry", "1");
	elm_win_fullscreen_set(eo, EINA_FALSE);
	elm_win_alpha_set(eo, EINA_TRUE);

	elm_win_title_set(eo, PACKAGE);
	evas_object_smart_callback_add(eo, "delete,request", __main_win_delete_request_cb, NULL);

	elm_win_screen_size_get(eo, NULL, NULL, &ad->root_w, &ad->root_h);
	evas_object_resize(eo, ad->root_w, ELM_SCALE_SIZE(MTLOCK_ACTIVE_NOTI_CALL_HEIGHT));

	elm_win_center(eo, EINA_FALSE, EINA_TRUE);
	evas_object_move(eo, 0, 0);
	elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(eo, ELM_WIN_INDICATOR_TRANSLUCENT);
	elm_win_conformant_set(eo, EINA_TRUE);

	return eo;
}

static Evas_Object *__create_conformant(Evas_Object *win)
{
	Evas_Object *win_conformant = elm_conformant_add(win);
	CALLUI_RETURN_NULL_IF_FAIL(win_conformant);

	elm_object_signal_emit(win_conformant, "elm,state,indicator,overlap", "elm");
	evas_object_size_hint_weight_set(win_conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, win_conformant);
	evas_object_show(win_conformant);

	return win_conformant;
}

static Evas_Object *__create_main_layout(Evas_Object *conf)
{
	Evas_Object *layout = _callui_load_edj(conf, EDJ_NAME,  "app_main_ly");
	elm_object_content_set(conf, layout);
	evas_object_show(layout);

	return layout;
}

static bool __create_main_gui_elem(callui_app_data_t *ad)
{
	ad->win = __create_main_window(ad);
	CALLUI_RETURN_VALUE_IF_FAIL(ad->win, false)

	Evas_Object *conf = __create_conformant(ad->win);
	CALLUI_RETURN_VALUE_IF_FAIL(conf, false);

	ad->main_ly = __create_main_layout(conf);
	CALLUI_RETURN_VALUE_IF_FAIL(ad->main_ly, false);

	return true;
}

static bool __app_deinit(callui_app_data_t *ad)
{
	debug_enter();

	_callui_stp_remove_call_state_event_cb(ad->state_provider, __call_state_change_cb, ad);
	_callui_sdm_remove_audio_state_changed_cb(ad->sound_manager, __audio_state_changed_cb, ad);

	__remove_ecore_event_key_handlers(ad);

	__unset_main_win_key_grab(ad);

	if (ad->view_manager) {
		_callui_vm_destroy(ad->view_manager);
		ad->view_manager = NULL;
	}

	if (ad->qp_minicontrol) {
		_callui_qp_mc_destroy(ad->qp_minicontrol);
		ad->qp_minicontrol = NULL;
	}

	if (ad->lock_handle) {
		_callui_lock_manager_destroy(ad->lock_handle);
		ad->lock_handle = NULL;
	}

	if (ad->action_bar) {
		_callui_action_bar_destroy(ad->action_bar);
		ad->action_bar = NULL;
	}

	if (ad->keypad) {
		_callui_keypad_destroy(ad->keypad);
		ad->keypad = NULL;
	}

	if (ad->main_ly) {
		evas_object_del(ad->main_ly);
		ad->main_ly = NULL;
	}

	free(ad->end_call_data);

	__bt_deinit();

	debug_leave();

	return false;
}

static void __app_terminate(void *data)
{
	debug_enter();

	__app_deinit(data);

	debug_leave();
}

static void __app_pause(void *data)
{
	dbg("..");

	_callui_common_unset_lock_state_changed_cb();

	callui_app_data_t *ad = data;

	_callui_vm_pause(ad->view_manager);
}

static void __app_resume(void *data)
{
	dbg("..");

	callui_app_data_t *ad = data;

	_callui_common_set_lock_state_changed_cb();

	if (ad->start_lock_manager_on_resume) {
		ad->start_lock_manager_on_resume = false;
		_callui_lock_manager_start(ad->lock_handle);
	}

	_callui_vm_resume(ad->view_manager);
}

static void __app_service(app_control_h app_control, void *data)
{
	dbg("..");
	callui_app_data_t *ad = data;
	char *tmp = NULL;
	int ret = 0;
	char *uri_bundle = NULL;
	char *operation = NULL;

	if (_callui_vm_get_cur_view_type(ad->view_manager) == CALLUI_VIEW_UNDEFINED
			&& !ad->waiting_dialing) {
		err("CALLUI_VIEW_UNDEFINED. Clear data");
		__reset_state_params(ad);
		_callui_common_dvc_control_lcd_state(LCD_OFF_SLEEP_LOCK);
	}

	ret = app_control_get_operation(app_control, &operation);
	CALLUI_RETURN_IF_FAIL(ret == APP_CONTROL_ERROR_NONE);
	CALLUI_RETURN_IF_FAIL(operation != NULL);

	ret = app_control_get_uri(app_control, &uri_bundle);
	CALLUI_RETURN_IF_FAIL(ret == APP_CONTROL_ERROR_NONE);

	sec_warn("operation: [%s]", operation);
	sec_warn("uri_bundle: [%s]", uri_bundle);

	if ((strcmp(operation, APP_CONTROL_OPERATION_CALL) == 0) &&
		(strncmp(uri_bundle, "tel:", 4) == 0)) {
		if ((strncmp(uri_bundle, "tel:MT", 6) == 0)) {
			_callui_common_dvc_control_lcd_state(LCD_ON_LOCK);
			ret = app_control_get_extra_data(app_control, "handle", &tmp);
			if (ret != APP_CONTROL_ERROR_NONE) {
				err("app_control_get_extra_data failed");
			}
			if (tmp) {
				dbg("handle: [%s]", tmp);
				free(tmp);
				tmp = NULL;
			} else {
				err("handle val is NULL");
				return;
			}
			__process_incoming_call(ad);
		} else {
			tmp = (char *)uri_bundle + 4;
			sec_dbg("number: [%s]", tmp);
			if (tmp) {
				sec_dbg("number: [%s]", tmp);
				evas_object_resize(ad->win, ad->root_w, ad->root_h);
				if (!ad->waiting_dialing) {
					__process_outgoing_call(ad, tmp);
				}
			} else {
				err("number val is NULL");
			}
		}
	} else if (strcmp(operation, APP_CONTROL_OPERATION_DEFAULT) == 0) {
		warn("Unsupported operation type");
	} else if (strcmp(operation, APP_CONTROL_OPERATION_DURING_CALL) == 0) {
		ret = _callui_manager_answer_call(ad->call_manager, CALLUI_CALL_ANSWER_NORMAL);
		if (CALLUI_RESULT_OK != ret) {
			err("_callui_manager_answer_call() failed. ret[%d]", ret);
		}
	} else if (strcmp(operation, APP_CONTROL_OPERATION_MESSAGE_REJECT) == 0) {

		/* TODO Implement reject with message button functionality */

	} else if (strcmp(operation, APP_CONTROL_OPERATION_END_CALL) == 0) {
		ret = _callui_manager_reject_call(ad->call_manager);
		if (CALLUI_RESULT_OK != ret) {
			err("_callui_manager_reject_call() failed. ret[%d]", ret);
		}
	}

	free(operation);
	free(uri_bundle);
}

static Eina_Bool __hard_key_up_cb(void *data, int type, void *event)
{
	dbg("..");

	callui_app_data_t *ad = (callui_app_data_t *)data;
	Ecore_Event_Key *ev = event;
	const callui_call_data_t *call_data = NULL;

	if (ev == NULL) {
		err("ERROR!!! ========= Event is NULL!!!");
		return 0;
	}

	callui_view_type_e view_type = _callui_vm_get_cur_view_type(ad->view_manager);

	dbg("Top view(%d)", view_type);

	const callui_call_data_t *incom = _callui_stp_get_call_data(ad->state_provider,
					CALLUI_CALL_DATA_INCOMING);
	const callui_call_data_t *active = _callui_stp_get_call_data(ad->state_provider,
					CALLUI_CALL_DATA_ACTIVE);
	const callui_call_data_t *held = _callui_stp_get_call_data(ad->state_provider,
					CALLUI_CALL_DATA_HELD);

	/*power key case */
	if (!strcmp(ev->keyname, CALLUI_KEY_POWER)) {
		dbg("in keypower");
		int is_powerkey_enabled = _callui_common_is_powerkey_mode_on();
		dbg("[KEY]KEY_POWER pressed, is_powerkey_enabled(%d)", is_powerkey_enabled);

		if (is_powerkey_enabled && !_callui_lock_manager_is_lcd_off(ad->lock_handle)) {

			if (view_type == CALLUI_VIEW_DIALLING) {
				if (active) {
					call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
					if (call_data) {
						_callui_manager_end_call(ad->call_manager, call_data->call_id,
								CALLUI_CALL_RELEASE_BY_CALL_HANDLE);
					}
				}
			} else if (view_type == CALLUI_VIEW_INCOMING_CALL ||
					view_type == CALLUI_VIEW_INCOMING_CALL_NOTI) {
				if (incom) {
					call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);
					if (call_data) {
						_callui_manager_end_call(ad->call_manager, call_data->call_id,
								CALLUI_CALL_RELEASE_BY_CALL_HANDLE);
					}
				}
			} else if (view_type == CALLUI_VIEW_SINGLECALL ||
					view_type == CALLUI_VIEW_MULTICALL_CONF ||
					view_type == CALLUI_VIEW_MULTICALL_LIST) {

				call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
				if (call_data) {
					 _callui_manager_end_call(ad->call_manager, call_data->call_id,
							CALLUI_CALL_RELEASE_ALL);
				} else {
					call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
					if (call_data) {
						_callui_manager_end_call(ad->call_manager, call_data->call_id,
								CALLUI_CALL_RELEASE_ALL);
					}
				}
			} else if (view_type == CALLUI_VIEW_MULTICALL_SPLIT) {
				call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
				if (call_data) {
					_callui_manager_end_call(ad->call_manager, call_data->call_id,
							CALLUI_CALL_RELEASE_ALL_ACTIVE);
				}
			}
		} else {
			if (incom && !active && !held) {
				callui_view_type_e type = CALLUI_VIEW_INCOMING_CALL;
				callui_view_type_e cur_type = _callui_vm_get_cur_view_type(ad->view_manager);
				if (_callui_common_get_idle_lock_type() == LOCK_TYPE_UNLOCK &&
						(cur_type == CALLUI_VIEW_UNDEFINED || cur_type == CALLUI_VIEW_ENDCALL)) {
					type = CALLUI_VIEW_INCOMING_CALL_NOTI;
				}
				_callui_vm_change_view(ad->view_manager, type);
			}
		}
	} else if (!strcmp(ev->keyname, CALLUI_KEY_MEDIA)) {
		/* todo*/
		dbg("in key-media");
	} else if (!strcmp(ev->keyname, CALLUI_KEY_VOLUMEUP) || !strcmp(ev->keyname, CALLUI_KEY_VOLUMEDOWN)) {
		dbg("Handle Volume Up or Down key");
		call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);
		if (call_data) {
			_callui_manager_stop_alert(ad->call_manager);
		}
	} else if (!strcmp(ev->keyname,  CALLUI_KEY_SELECT) || !strcmp(ev->keyname,  CALLUI_KEY_HOME)) {
		dbg("in KEY_SELECT");
		int result = 0;
		result = elm_win_keygrab_unset(ad->win, CALLUI_KEY_SELECT, 0, 0);
		if (!result) {
			dbg("KEY_SELECT key ungrab failed");
		} else {
			dbg("KEY_SELECT key ungrab success");
		}
		if (view_type == CALLUI_VIEW_INCOMING_CALL ||
				view_type == CALLUI_VIEW_INCOMING_CALL_NOTI) {

			if (_callui_common_is_answering_mode_on()) {
				dbg("Answering mode on and Home key pressed on MT screen");

				int unhold_call_count = 0;
				if (active) {
					unhold_call_count = active->conf_member_count;
				}

				if (unhold_call_count == 0) {
					dbg("No Call Or Held call - Accept");

					_callui_manager_answer_call(ad->call_manager, CALLUI_CALL_ANSWER_NORMAL);

					if (_callui_common_get_idle_lock_type() == LOCK_TYPE_SWIPE_LOCK) {
						_callui_common_unlock_swipe_lock();
					}
				} else if (ad->second_call_popup == NULL) {
					dbg("Show popup - 2nd MT call - test volume popup");
					_callui_load_second_call_popup(ad);
				}
			} else {
				if (!elm_win_keygrab_set(ad->win, CALLUI_KEY_SELECT, 0, 0, 0, ELM_WIN_KEYGRAB_TOPMOST)) {
					dbg("KEY_SELECT key ungrab failed");
				}
			}
		} else {
			// TODO Implement other way to verify focus window == current
			//Ecore_X_Window focus_win = ecore_x_window_focus_get();
			//if (ad->win != NULL && focus_win == elm_win_xwindow_get(ad->win)) {
				/* ToDo: Use lock-screen interface to raise the home screen */
				_callui_common_win_set_noti_type(ad, false);
				_callui_lock_manager_stop(ad->lock_handle);
				ad->on_background = true;
			//}
		}
	} else if (!strcmp(ev->keyname, CALLUI_KEY_BACK)) {
		/* todo*/
		dbg("KEY_BACK section");
	}

	DELETE_ECORE_TIMER(ad->earset_key_longpress_timer);

	return EINA_FALSE;
}

static Eina_Bool __earset_key_longpress_timer_cb(void *data)
{
	dbg("..");

	callui_app_data_t *ad = (callui_app_data_t *)data;

	ad->earset_key_longpress_timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __hard_key_down_cb(void *data, int type, void *event)
{
	dbg("..");

	callui_app_data_t *ad = (callui_app_data_t *)data;
	Ecore_Event_Key *ev = event;

	if (ev == NULL) {
		err("ERROR!!! ========= Event is NULL!!!");
		return EINA_FALSE;
	}

	if (_callui_vm_get_cur_view_type(ad->view_manager) == CALLUI_VIEW_UNDEFINED) {
		dbg("ad->view_top is UNDEFINED");
		return EINA_FALSE;
	}

	if (!strcmp(ev->keyname, CALLUI_KEY_MEDIA)) {
		ad->earset_key_longpress_timer = ecore_timer_add(EARSET_KEY_LONG_PRESS_TIMEOUT,
				__earset_key_longpress_timer_cb, ad);
	} else if (!strcmp(ev->keyname, CALLUI_KEY_SELECT)) {
		/*todo*/
	}
	dbg("End..");
	return EINA_FALSE;
}

callui_app_data_t *_callui_get_app_data()
{
	return &g_ad;
}

CALLUI_EXPORT_API int main(int argc, char *argv[])
{
	dbg("..");
	ui_app_lifecycle_callback_s event_callback = {0,};

	event_callback.create = __app_create;
	event_callback.terminate = __app_terminate;
	event_callback.pause = __app_pause;
	event_callback.resume = __app_resume;
	event_callback.app_control = __app_service;

	memset(&g_ad, 0x0, sizeof(callui_app_data_t));

	int res = ui_app_main(argc, argv, &event_callback, &g_ad);
	if (res != APP_ERROR_NONE) {
		err("ui_app_main() is failed. res[%d]", res);
	}
	return res;
}
