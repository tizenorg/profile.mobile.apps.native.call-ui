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

#include <app_control.h>
#include <app.h>
#include <glib-object.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <bluetooth.h>
#include "sys/socket.h"
#include "sys/un.h"
#include "callui.h"
#include "callui-view-elements.h"
#include "callui-common.h"
#include <device/display.h>
#include <device/callback.h>
#include "callui-view-quickpanel.h"
#include "callui-view-layout-wvga.h"

#define EARSET_KEY_LONG_PRESS_TIMEOUT			1.0

static bool _callui_app_create_layout(void *data);
static Eina_Bool __callui_app_win_hard_key_down_cb(void *data, int type, void *event);
static Eina_Bool __callui_app_win_hard_key_up_cb(void *data, int type, void *event);

static callui_app_data_t g_ad;

static call_data_t *__callui_call_data_new(char *number)
{
	dbg("__calldoc_call_data_new()");
	call_data_t *call_data = NULL;
	if (NULL == number) {
		warn("number is NULL");
	}

	call_data = (call_data_t *)calloc(1, sizeof(call_data_t));
	if (NULL == call_data) {
		warn("failed to alloc memory");
		return NULL;
	}

	call_data->call_id = NO_HANDLE;
	call_data->member_count = 0;
	if (number) {
		g_strlcpy(call_data->call_num, number, CALLUI_PHONE_NUMBER_LENGTH_MAX);
	}
	call_data->call_ct_info.person_id = -1;

	return call_data;
}

static void __callui_app_win_key_grab(callui_app_data_t *ad)
{
	int result = 0;
	result = elm_win_keygrab_set(ad->win, CALLUI_KEY_MEDIA, 0, 0, 0, ELM_WIN_KEYGRAB_EXCLUSIVE);
	if (result)
		dbg("KEY_MEDIA key grab failed");

	if (_callui_common_is_powerkey_mode_on()) {
		result = elm_win_keygrab_set(ad->win, CALLUI_KEY_POWER, 0, 0, 0, ELM_WIN_KEYGRAB_EXCLUSIVE);
	} else {
		result = elm_win_keygrab_set(ad->win, CALLUI_KEY_POWER, 0, 0, 0, ELM_WIN_KEYGRAB_SHARED);
	}
	if (result) {
		dbg("KEY_POWER key grab failed");
	}

	if (ad->downkey_handler == NULL)
		ad->downkey_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, __callui_app_win_hard_key_down_cb, ad);
	if (ad->upkey_handler == NULL)
		ad->upkey_handler = ecore_event_handler_add(ECORE_EVENT_KEY_UP, __callui_app_win_hard_key_up_cb, ad);
}

static void __callui_update_call_data(call_data_t **call_data, cm_call_data_t* cm_call_data)
{
	dbg("__callui_update_call_data()");
	call_data_t *tmp_call_data = NULL;
	char *call_number = NULL;
	int person_id = -1;
	if (NULL == cm_call_data) {
		warn("cm_call_data is NULL");
		g_free(*call_data);
		*call_data = NULL;
		return;
	}

	cm_call_data_get_call_number(cm_call_data, &call_number);
	tmp_call_data = __callui_call_data_new(call_number);
	CALLUI_RETURN_IF_FAIL(tmp_call_data);

	cm_call_data_get_call_id(cm_call_data, &tmp_call_data->call_id);
	cm_call_data_get_call_member_count(cm_call_data, &tmp_call_data->member_count);
	cm_call_data_get_call_direction(cm_call_data, &tmp_call_data->call_direction);
	cm_call_data_get_call_domain(cm_call_data, &tmp_call_data->call_domain);
	cm_call_data_get_call_state(cm_call_data, &tmp_call_data->call_state);
	cm_call_data_get_call_type(cm_call_data, &tmp_call_data->call_type);
	cm_call_data_is_emergency_call(cm_call_data, &tmp_call_data->is_emergency);
	cm_call_data_get_start_time(cm_call_data, &tmp_call_data->start_time);

	/* Check for Contact Index and accordingly update the contact information if the contact is saved */
	cm_call_data_get_person_id(cm_call_data, &person_id);
	if ((person_id != -1) && (tmp_call_data->call_ct_info.person_id == -1)) {
		dbg("contact exists with index:[%d]", person_id);
		_callui_common_get_contact_info(person_id, &tmp_call_data->call_ct_info);
	}

	if (strlen(tmp_call_data->call_ct_info.caller_id_path) <= 0) {
		g_strlcpy(tmp_call_data->call_ct_info.caller_id_path, "default", CALLUI_IMAGE_PATH_LENGTH_MAX);
	}

	g_free(*call_data);
	*call_data = tmp_call_data;

	return;
}

static void __callui_update_all_call_data(callui_app_data_t *ad, cm_call_event_data_t* call_state_data)
{
	cm_call_data_t *call_data = NULL;
	CALLUI_RETURN_IF_FAIL(ad);
	CALLUI_RETURN_IF_FAIL(call_state_data);
	dbg("doc_data active!");
	cm_call_event_data_get_active_call(call_state_data, &call_data);
	__callui_update_call_data(&(ad->active), call_data);
	dbg("doc_data incom!");
	cm_call_event_data_get_incom_call(call_state_data, &call_data);
	__callui_update_call_data(&(ad->incom), call_data);
	dbg("doc_data held!");
	cm_call_event_data_get_held_call(call_state_data, &call_data);
	__callui_update_call_data(&(ad->held), call_data);
}

static void __callui_process_incoming_call(callui_app_data_t *ad, unsigned int call_id)
{
	dbg("..");
	cm_call_data_t *cm_incom = NULL;

	CALLUI_RETURN_IF_FAIL(ad);

	cm_get_all_calldata(ad->cm_handle, &cm_incom, NULL, NULL);
	CALLUI_RETURN_IF_FAIL(cm_incom);

	__callui_update_call_data(&(ad->incom), cm_incom);
	_callvm_view_change(VIEW_INCOMING_LOCK_VIEW, 0, NULL, ad);

	cm_call_data_free(cm_incom);
	return;
}

static void __callui_process_outgoing_call(callui_app_data_t *ad, char *number)
{
	sec_dbg("number is (%s)", number);
	CALLUI_RETURN_IF_FAIL(ad);

	if (CM_ERROR_NONE != cm_dial_call(ad->cm_handle, number, CM_CALL_TYPE_VOICE, CM_SIM_SLOT_DEFAULT_E)) {
		err("cm_dial_call failed!!");
		_callvm_terminate_app_or_view_change(ad);
		return;
	}
	ad->waiting_dialing = true;
	ad->active = __callui_call_data_new(number);
	return;
}

static void __callui_call_event_cb(cm_call_event_e call_event, cm_call_event_data_t *call_state_data, void *user_data)
{
	CALLUI_RETURN_IF_FAIL(call_state_data);
	info("Call event changed!! %d", call_event);
	callui_app_data_t *ad = (callui_app_data_t *)user_data;
	CALLUI_RETURN_IF_FAIL(ad);

	cm_call_event_data_get_sim_slot(call_state_data, &ad->sim_slot);
	ad->waiting_dialing = false;
	switch (call_event) {
	case CM_CALL_EVENT_ACTIVE:
		if (_callui_lock_manager_is_lcd_off(ad->lock_handle)) {
			_callui_common_dvc_control_lcd_state(LCD_UNLOCK);
		} else {
			_callui_common_dvc_control_lcd_state(LCD_ON_UNLOCK);
		}
		__callui_update_all_call_data(ad, call_state_data);
		_callui_common_create_duration_timer();
		_callvm_view_auto_change(ad);
		if (ad->speaker_status == EINA_TRUE) {
			_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_SET);
		}
		break;
	case CM_CALL_EVENT_IDLE:
		{
			unsigned int call_id = 0;
			cm_call_data_t *incom_call = NULL;
			cm_call_data_t *active_call = NULL;
			cm_call_data_t *held_call = NULL;
			if (_callui_lock_manager_is_lcd_off(ad->lock_handle)) {
				_callui_common_dvc_control_lcd_state(LCD_UNLOCK);
			} else {
				_callui_common_dvc_control_lcd_state(LCD_ON_UNLOCK);
			}
			cm_call_event_data_get_call_id(call_state_data, &call_id);
			cm_call_event_data_get_active_call(call_state_data, &active_call);
			cm_call_event_data_get_incom_call(call_state_data, &incom_call);
			cm_call_event_data_get_held_call(call_state_data, &held_call);
			if ((ad->incom) && (call_id == ad->incom->call_id)) {
				__callui_update_all_call_data(ad, call_state_data);
				_callvm_terminate_app_or_view_change(ad);
			} else if (incom_call == NULL && active_call == NULL && held_call == NULL) {
				_callui_common_delete_duration_timer();
				_callvm_view_change(VIEW_ENDCALL_VIEW, call_id, NULL, ad);
				_callui_lock_manager_stop(ad->lock_handle);
				__callui_update_all_call_data(ad, call_state_data);
			} else {
				__callui_update_all_call_data(ad, call_state_data);
				_callvm_terminate_app_or_view_change(ad);
			}
		}
		break;
	case CM_CALL_EVENT_INCOMING:
		{
			_callui_common_dvc_control_lcd_state(LCD_ON_LOCK);
			__callui_update_all_call_data(ad, call_state_data);
			_callvm_view_auto_change(ad);
		}
		break;
	case CM_CALL_EVENT_DIALING:
	case CM_CALL_EVENT_HELD:
	case CM_CALL_EVENT_RETRIEVED:
	case CM_CALL_EVENT_SWAPPED:
	case CM_CALL_EVENT_JOIN:
	case CM_CALL_EVENT_SPLIT:
		__callui_update_all_call_data(ad, call_state_data);
		_callvm_view_auto_change(ad);
		break;
	default:
		break;
	}

	dbg("Call event changed cb done");
	return;
}

static void __callui_audio_state_changed_cb(cm_audio_state_type_e audio_state, void *user_data)
{
	callui_app_data_t *ad = (callui_app_data_t *)user_data;
	CALLUI_RETURN_IF_FAIL(ad);
	dbg("__callui_audio_state_changed_cb, audio_state[%d]", audio_state);
	switch (audio_state) {
		case CM_AUDIO_STATE_SPEAKER_E:
			_callui_update_speaker_btn(ad, EINA_TRUE);
			_callui_update_headset_btn(ad, EINA_FALSE);
			ad->earphone_status = EINA_FALSE;
			if ((_callui_common_is_extra_volume_available() == EINA_TRUE) && (ad->extra_volume_status_force_stop == EINA_TRUE)) {
				int retextravol = -1;
				retextravol = cm_set_extra_vol(ad->cm_handle, TRUE);
				if (retextravol == CM_ERROR_NONE) {
					ad->extra_volume_status_force_stop = EINA_FALSE;
					_callui_update_extra_vol_btn(ad, EINA_TRUE);
				}
			}
			_callui_lock_manager_force_stop(ad->lock_handle);
			break;
		case CM_AUDIO_STATE_RECEIVER_E:
			_callui_update_speaker_btn(ad, EINA_FALSE);
			_callui_update_headset_btn(ad, EINA_FALSE);
			ad->earphone_status = EINA_FALSE;
			if ((_callui_common_is_extra_volume_available() == EINA_TRUE) && (ad->extra_volume_status_force_stop == EINA_TRUE)) {
				int retextravol = -1;
				retextravol = cm_set_extra_vol(ad->cm_handle, TRUE);
				if (retextravol == CM_ERROR_NONE) {
					ad->extra_volume_status_force_stop = EINA_FALSE;
					_callui_update_extra_vol_btn(ad, EINA_TRUE);
				}
			}
			_callui_lock_manager_start(ad->lock_handle);
			break;
		case CM_AUDIO_STATE_EARJACK_E:
		{
			ad->earphone_status = EINA_TRUE;
			_callui_update_speaker_btn(ad, EINA_FALSE);
			_callui_update_headset_btn(ad, EINA_FALSE);
			if (ad->extra_volume_status == EINA_TRUE) {
				dbg("Disable extra volume when earphone connected and speaker is turned off");
				int ret = -1;
				ret = cm_set_extra_vol(ad->cm_handle, FALSE);
				if (ret == CM_ERROR_NONE) {
					ad->extra_volume_status_force_stop = EINA_TRUE;
					_callui_create_extravolume_notify_popup();
					_callui_update_extra_vol_btn(ad, EINA_FALSE);
				} else {
					err("cm_set_extra_vol() is failed");
				}
			}
			_callui_lock_manager_force_stop(ad->lock_handle);
			break;
		}
		case CM_AUDIO_STATE_BT_E:
			ad->earphone_status = EINA_FALSE;
			_callui_update_speaker_btn(ad, EINA_FALSE);
			_callui_update_headset_btn(ad, EINA_TRUE);
			_callui_lock_manager_force_stop(ad->lock_handle);
			break;
		case CM_AUDIO_STATE_NONE_E:
		default:
			err("unhandled state[%d]", audio_state);
			break;
	}

	return;
}

static void __callui_call_list_init(callui_app_data_t *ad)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(ad);

	ad->active = NULL;
	ad->incom = NULL;
	ad->held = NULL;
	ad->active_incoming = false;
	ad->multi_call_list_end_clicked = false;
	ad->start_lock_manager_on_resume = false;
	ad->on_background = false;

	return;
}


static void __callui_bt_init()
{
	int ret = BT_ERROR_NONE;
	ret = bt_initialize();
	if (BT_ERROR_NONE != ret) {
		err("bt_init() failed [%d]", ret);
		return;
	}
}

static gboolean __callui_init(callui_app_data_t *ad)
{
	dbg("..");

	if (CM_ERROR_NONE != cm_init(&ad->cm_handle)) {
		err("cm_init() err");
		return false;
	}
	cm_set_call_event_cb(ad->cm_handle, __callui_call_event_cb, ad);
	cm_set_audio_state_changed_cb(ad->cm_handle, __callui_audio_state_changed_cb, ad);

	__callui_bt_init();

	ad->view_manager_handle = _callvm_init();
	return true;
}

static void __callui_win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	/* To make your application go to background,
		Call the elm_win_lower() instead
		Evas_Object *win = (Evas_Object *) data;
		elm_win_lower(win); */
	elm_exit();
}

static Evas_Object *__callui_create_main_win(callui_app_data_t *ad)
{
	/*
	 * Widget Tree
	 * Window
	 *  - conform
	 *   - layout main
	 *    - naviframe */

	Evas_Object *eo = NULL;
	const char *str = "mobile";

	// TODO No app_get_preinitialized_window function.
	//eo =  (Evas_Object *)app_get_preinitialized_window(PACKAGE);

	eo = ad->win;
	if (eo == NULL) {
		dbg("Create window");
		eo = elm_win_add(NULL, PACKAGE, ELM_WIN_BASIC);
		elm_win_alpha_set(eo, EINA_TRUE);
		elm_win_fullscreen_set(eo, EINA_FALSE);
	} else {
		dbg("Preinitialized window");
	}

	if (eo) {
		// TODO Remove
		//elm_win_profiles_set(eo, &str, 1);	/* Desktop mode only */
		elm_win_title_set(eo, PACKAGE);
		evas_object_smart_callback_add(eo, "delete,request", __callui_win_delete_request_cb, NULL);
		elm_config_engine_set("software_x11");

		evas_object_geometry_get(ad->win, NULL, NULL, &ad->root_w, &ad->root_h);
		dbg("root_w = %d, root_h = %d..", ad->root_w, ad->root_h);
		evas_object_resize(eo, ad->root_w, ELM_SCALE_SIZE(MTLOCK_ACTIVE_CALL_HEIGHT));

		elm_win_center(eo, EINA_FALSE, EINA_TRUE);
		evas_object_move(eo, 0, 0);
		elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_SHOW);
		elm_win_indicator_opacity_set(eo, ELM_WIN_INDICATOR_TRANSLUCENT);
		elm_win_conformant_set(eo, EINA_TRUE);

		// TODO No app_get_preinitialized_background function
		//ad->bg = (Evas_Object *)app_get_preinitialized_background();
		if (!ad->bg) {
			ad->bg = elm_bg_add(eo);
		}
		evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(ad->bg);
		elm_object_part_content_set(eo, "elm.swallow.bg", ad->bg);

		//TODO No app_get_preinitialized_conformant function.
		//conform = (Evas_Object *)app_get_preinitialized_conformant();
		if (ad->win_conformant == NULL) {
			ad->win_conformant = elm_conformant_add(eo);
		}
		elm_object_signal_emit(ad->win_conformant, "elm,state,indicator,overlap", "elm");
		evas_object_size_hint_weight_set(ad->win_conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_win_resize_object_add(eo, ad->win_conformant);
		evas_object_show(ad->win_conformant);

	}

	return eo;
}

static Evas_Object *__callui_create_base_layout(callui_app_data_t *ad)
{
	dbg("..");
	CALLUI_RETURN_VALUE_IF_FAIL(ad, NULL);
	Evas_Object *ly = NULL;

	ly = elm_layout_add(ad->win_conformant);
	if (ly == NULL) {
		err("ly is NULL");
		return NULL;
	}

	elm_layout_theme_set(ly, "layout", "application", "default");
	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(ly);

	return ly;
}

static void __callui_app_text_classes_set()
{
	edje_text_class_set("ATO001", "R", 40);
	edje_text_class_set("ATO002", "R", 64);
	edje_text_class_set("ATO003", "R", 40);
	edje_text_class_set("ATO004", "R", 24);
	edje_text_class_set("ATO005", "R", 24);
	edje_text_class_set("ATO006", "R", 24);
	edje_text_class_set("ATO007", "R", 24);
	edje_text_class_set("ATO008", "R", 24);
	edje_text_class_set("ATO010", "R", 30);
	edje_text_class_set("ATO011", "R", 30);
	edje_text_class_set("ATO012", "R", 30);
	edje_text_class_set("ATO013", "R", 30);
	edje_text_class_set("ATO014", "R", 30);
	edje_text_class_set("ATO016", "R", 24);
	edje_text_class_set("ATO017", "R", 24);
	edje_text_class_set("ATO027", "R", 26);
	edje_text_class_set("ATO028", "R", 24);
	edje_text_class_set("ATO030", "R", 42);
	edje_text_class_set("ATO017", "R", 30);
}

static bool _callui_app_create(void *data)
{
	dbg("..");
	callui_app_data_t *ad = data;
	int ret = 0;

	_callui_common_dvc_control_lcd_state(LCD_OFF_SLEEP_LOCK);

	elm_config_preferred_engine_set("opengl_x11");
	ret = __callui_init(ad);
	if (!ret) {
		err("__callui_init failed");
		elm_exit();
		return FALSE;
	}

	_callui_app_create_layout(ad);
	ad->lock_handle = _callui_lock_manager_create();

	elm_theme_extension_add(NULL, CALL_THEME);
	elm_theme_extension_add(NULL, CALL_GENLIST_THEME);

	return true;
}

static bool _callui_app_create_layout(void *data)
{
	callui_app_data_t *ad = data;

	/* Set base scale */
	elm_app_base_scale_set(2.6);

	ad->win = __callui_create_main_win(ad);
	if (ad->win == NULL) {
		err("__callui_create_main_win failed");
		return FALSE;
	}
	ad->main_ly = __callui_create_base_layout(ad);
	if (ad->main_ly == NULL) {
		err("__callui_create_base_layout failed");
		return FALSE;
	}
	elm_object_content_set(ad->win_conformant, ad->main_ly);

	__callui_app_text_classes_set();

	ad->evas = evas_object_evas_get(ad->win);
	__callui_app_win_key_grab(ad);

	return true;
}


static void __callui_bt_deinit()
{
	int ret = BT_ERROR_NONE;
	ret = bt_deinitialize();
	if (BT_ERROR_NONE != ret) {
		err("bt_deinit() failed [%d]", ret);
		return;
	}
}


static void _callui_app_terminate(void *data)
{
	callui_app_data_t *ad = data;
	Evas_Object *contents = NULL;
	Evas_Object *caller_info = NULL;
	Evas_Object *btn_ly = NULL;

	contents = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
	if (contents) {
		caller_info = elm_object_part_content_get(contents, "caller_info");
		if (caller_info) {
			evas_object_del(caller_info);
			caller_info = NULL;
		}

		btn_ly = elm_object_part_content_get(contents, "btn_region");
		if (btn_ly) {
			evas_object_del(btn_ly);
			btn_ly = NULL;
		}

		evas_object_del(contents);
		contents = NULL;
	}

	if (ad->downkey_handler) {
		ecore_event_handler_del(ad->downkey_handler);
		ad->downkey_handler = NULL;
	}
	if (ad->upkey_handler) {
		ecore_event_handler_del(ad->upkey_handler);
		ad->upkey_handler = NULL;
	}

	elm_win_keygrab_unset(ad->win, CALLUI_KEY_SELECT, 0, 0);
	elm_win_keygrab_unset(ad->win, CALLUI_KEY_POWER, 0, 0);
	elm_win_keygrab_unset(ad->win, CALLUI_KEY_MEDIA, 0, 0);

	if (ad->main_ly) {
		evas_object_del(ad->main_ly);
		ad->main_ly = NULL;
	}

	__callui_bt_deinit();

	cm_unset_audio_state_changed_cb(ad->cm_handle);
	cm_unset_call_event_cb(ad->cm_handle);
}

static void _callui_app_pause(void *data)
{
	dbg("..");
	_callui_common_unset_lock_state_changed_cb();
}

static void _callui_app_resume(void *data)
{
	dbg("..");
	callui_app_data_t *ad = data;
	_callui_common_set_lock_state_changed_cb();
	if (ad->start_lock_manager_on_resume) {
		ad->start_lock_manager_on_resume = false;
		_callui_lock_manager_start(ad->lock_handle);
	}
	_callui_common_set_quickpanel_scrollable(EINA_TRUE);
}

static void _callui_app_service(app_control_h app_control, void *data)
{
	dbg("..");
	callui_app_data_t *ad = data;
	char *tmp = NULL;
	int ret = 0;
	unsigned int call_id = -1;
	char *uri_bundle = NULL;
	char *operation = NULL;

	if (_callvm_get_top_view_id(ad->view_manager_handle) == -1 && !ad->waiting_dialing) {
		err("view_manager_handle->viewtop -1 ");
		__callui_call_list_init(ad);
		_callui_common_dvc_control_lcd_state(LCD_OFF_SLEEP_LOCK);
	}

	evas_object_color_set(ad->bg, 255, 255, 255, 255);

	ret = app_control_get_operation(app_control, &operation);
	CALLUI_RETURN_IF_FAIL(ret == APP_CONTROL_ERROR_NONE);
	CALLUI_RETURN_IF_FAIL(operation != NULL);


	ret = app_control_get_uri(app_control, &uri_bundle);
	CALLUI_RETURN_IF_FAIL(ret == APP_CONTROL_ERROR_NONE);

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
				call_id = atoi(tmp);
				g_free(tmp);
				tmp = NULL;
			} else {
				err("handle val is NULL");
				return;
			}
			ret = app_control_get_extra_data(app_control, "sim_slot", &tmp);
			if (ret != APP_CONTROL_ERROR_NONE) {
				err("app_control_get_extra_data failed");
			}
			if (tmp) {
				dbg("sim_slot: [%s]", tmp);
				ad->sim_slot = atoi(tmp);
				g_free(tmp);
				tmp = NULL;
			}
			__callui_process_incoming_call(ad, call_id);
		} else {
			tmp = (char *)uri_bundle + 4;
			sec_dbg("number: [%s]", tmp);
			if (tmp) {
				sec_dbg("number: [%s]", tmp);
				if (!ad->main_ly) {
					_callui_app_create_layout(ad);
				}
				evas_object_resize(ad->win, ad->root_w, ad->root_h);
				if (!ad->waiting_dialing) {
					__callui_process_outgoing_call(ad, tmp);
				}
			} else {
				err("number val is NULL");
			}
		}
	} else if (strcmp(operation, APP_CONTROL_OPERATION_DEFAULT) == 0) {
		if (!ad->main_ly) {
			_callui_app_create_layout(ad);
		}
		__callui_process_incoming_call(ad, call_id);
	} else if (strcmp(operation, APP_CONTROL_OPERATION_DURING_CALL) == 0) {
		if (!ad->main_ly) {
			_callui_app_create_layout(ad);
		}
		ret = cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_NORMAL);
	} else if (strcmp(operation, APP_CONTROL_OPERATION_MESSAGE_REJECT) == 0) {
		/* TODO Implement reject with message button functionality */
	} else if (strcmp(operation, APP_CONTROL_OPERATION_END_CALL) == 0) {
		cm_reject_call(ad->cm_handle);
	}

	if (operation) {
		g_free(operation);
		operation = NULL;
	}

	if (uri_bundle) {
		g_free(uri_bundle);
		uri_bundle = NULL;
	}

}

callui_app_data_t *_callui_get_app_data()
{
	return &g_ad;
}


CALLUI_EXPORT_API int main(int argc, char *argv[])
{
	dbg("..");
	ui_app_lifecycle_callback_s event_callback = {0,};

	event_callback.create = _callui_app_create;
	event_callback.terminate = _callui_app_terminate;
	event_callback.pause = _callui_app_pause;
	event_callback.resume = _callui_app_resume;
	event_callback.app_control = _callui_app_service;

	memset(&g_ad, 0x0, sizeof(callui_app_data_t));

	int ret = APP_ERROR_NONE;

	ret = ui_app_main(argc, argv, &event_callback, &g_ad);
	if (ret != APP_ERROR_NONE) {
		err("ui_app_main() is failed. err = %d", ret);
	}

	 return ret;
}


static Eina_Bool __callui_app_win_hard_key_up_cb(void *data, int type, void *event)
{
	dbg("..");
	gboolean bpowerkey_enabled = EINA_FALSE;
	gboolean banswering_enabled = EINA_FALSE;

	callui_app_data_t *ad = (callui_app_data_t *)data;
	Ecore_Event_Key *ev = event;

	if (ev == NULL) {
		err("ERROR!!! ========= Event is NULL!!!");
		return 0;
	}

	if (elm_win_xwindow_get(ad->win) != ev->event_window) {
		err("Event window is not main window. Ignore event");
		return EINA_FALSE;
	}

	dbg("Top view(%d)", _callvm_get_top_view_id(ad->view_manager_handle));

	/*power key case */
	if (!strcmp(ev->keyname, CALLUI_KEY_POWER)) {
		dbg("in keypower");
		bpowerkey_enabled = _callui_common_is_powerkey_mode_on();
		dbg("[KEY]KEY_POWER pressed, bpowerkey_enabled(%d)", bpowerkey_enabled);
		if (bpowerkey_enabled == EINA_TRUE && !_callui_lock_manager_is_lcd_off(ad->lock_handle)) {
			if (_callvm_get_top_view_id(ad->view_manager_handle) == VIEW_DIALLING_VIEW) {
				if (ad->active)
				cm_end_call(ad->cm_handle, ad->active->call_id, CALL_RELEASE_TYPE_BY_CALL_HANDLE);
			} else if (_callvm_get_top_view_id(ad->view_manager_handle) == VIEW_INCOMING_LOCK_VIEW) {
				/* ToDo: Stop Incoming ringtone alert to be implemented*/
			} else if ((_callvm_get_top_view_id(ad->view_manager_handle) == VIEW_INCALL_ONECALL_VIEW)
						|| (_callvm_get_top_view_id(ad->view_manager_handle) == VIEW_INCALL_MULTICALL_CONF_VIEW)
						|| (_callvm_get_top_view_id(ad->view_manager_handle) == VIEW_INCALL_MULTICALL_LIST_VIEW)) {
				if (ad->active)
					cm_end_call(ad->cm_handle, ad->active->call_id, CALL_RELEASE_TYPE_ALL_CALLS);
				else if (ad->held)
					cm_end_call(ad->cm_handle, ad->held->call_id, CALL_RELEASE_TYPE_ALL_CALLS);
			} else if (_callvm_get_top_view_id(ad->view_manager_handle) == VIEW_INCALL_MULTICALL_SPLIT_VIEW) {
				if (ad->active)
					cm_end_call(ad->cm_handle, ad->active->call_id, CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS);
			} else {
				dbg("nothing...");
			}
		} else {
			if (ad->incom && !ad->active && !ad->held) {
				_callvm_view_change(VIEW_INCOMING_LOCK_VIEW, 0, NULL, ad);
			}
			/* ToDo: Stop Incoming ringtone alert to be implemented*/
			/*
			if (_callvm_get_top_view_id(ad->view_manager_handle) == VIEW_INCOMING_LOCK_VIEW) {
				vcall_engine_stop_alert();
				ad->bmute_ringtone = EINA_TRUE;
			}
			if ((TRUE == _vcui_is_security_lock()) || (_vcui_get_idle_lock_type() != CALL_UNLOCK)) {
				Ecore_X_Window focus_win = ecore_x_window_focus_get();
				if (ad->win_main!= NULL && focus_win == elm_win_xwindow_get(ad->win_main)) {
					_vcui_app_win_set_noti_type(EINA_TRUE);
					_vcui_view_common_set_quickpanel_scrollable(EINA_FALSE);
				}
			}*/
		}
	} else if (!strcmp(ev->keyname, CALLUI_KEY_MEDIA)) {
		/* todo*/
		dbg("in key-media");
	} else if (!strcmp(ev->keyname, CALLUI_KEY_VOLUMEUP) || !strcmp(ev->keyname, CALLUI_KEY_VOLUMEDOWN)) {
		dbg("Handle Volume Up or Down key");
		if (ad->incom) {
			cm_stop_alert(ad->cm_handle);
		}
	} else if (!strcmp(ev->keyname,  CALLUI_KEY_SELECT) || !strcmp(ev->keyname,  CALLUI_KEY_HOME)) {
		dbg("in KEY_SELECT");
		int result = 0;
		result = elm_win_keygrab_unset(ad->win, CALLUI_KEY_SELECT, 0, 0);
		if (result) {
			dbg("KEY_SELECT key ungrab failed");
		} else {
			dbg("KEY_SELECT key ungrab success");
		}
		if (_callvm_get_top_view_id(ad->view_manager_handle) == VIEW_INCOMING_LOCK_VIEW) {
			banswering_enabled = _callui_common_is_answering_mode_on();
			if (banswering_enabled == EINA_TRUE) {
				int unhold_call_count = 0;
				if (ad->active) {
					unhold_call_count = ad->active->member_count;
				}
				dbg("Answering mode on and Home key pressed on MT screen");

				if (unhold_call_count == 0) {
					dbg("No Call Or Held call - Accept");
					cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_NORMAL);
					if (_callui_common_get_idle_lock_type() == LOCK_TYPE_SWIPE_LOCK)
						_callui_common_unlock_swipe_lock();
				} else if (ad->second_call_popup == NULL) {
					dbg("Show popup - 2nd MT call - test volume popup");
					_callui_load_second_call_popup(ad);
				}
			} else {
				int result = 0;
				/* Grab home key event to keep incoming call view */
				result = elm_win_keygrab_set(ad->win, CALLUI_KEY_SELECT, 0, 0, 0, ELM_WIN_KEYGRAB_TOPMOST);
				if (result) {
					dbg("KEY_SELECT key ungrab failed");
				}
			}

		} else {
			// TODO Implement other way to verify focus window == current
			//if (ad->win != NULL && focus_win == elm_win_xwindow_get(ad->win)) {
				/* ToDo: Use lock-screen interface to raise the home screen */
				_callui_common_win_set_noti_type(ad, EINA_FALSE);
				_callui_lock_manager_stop(ad->lock_handle);
				ad->on_background = true;
		/* todo: once quickpanel is implemented we have to enable this
				_vcui_view_common_set_quickpanel_scrollable(EINA_TRUE);
		 */
			//}
		}
	} else if (!strcmp(ev->keyname, CALLUI_KEY_BACK)) {
		/* todo*/
		dbg("KEY_BACK section");
	}


	ad->b_earset_key_longpress = EINA_FALSE;

	if (ad->earset_key_longpress_timer) {
		ecore_timer_del(ad->earset_key_longpress_timer);
		ad->earset_key_longpress_timer = NULL;
	}

	return EINA_FALSE;
}

static Eina_Bool __callui_app_win_earset_key_longpress_timer_cb(void *data)
{
	dbg("..");

	callui_app_data_t *ad = (callui_app_data_t *)data;

	ad->b_earset_key_longpress = EINA_TRUE;
	ad->earset_key_longpress_timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __callui_app_win_hard_key_down_cb(void *data, int type, void *event)
{
	dbg("..");

	callui_app_data_t *ad = (callui_app_data_t *)data;
	Ecore_Event_Key *ev = event;

	if (ev == NULL) {
		err("ERROR!!! ========= Event is NULL!!!");
		return EINA_FALSE;
	}

	if (elm_win_xwindow_get(ad->win) != ev->event_window) {
		err("Event window is not main window. Ignore event");
		return EINA_FALSE;
	}

	if (_callvm_get_top_view_id(ad->view_manager_handle) == -1) {
		dbg("ad->view_top is -1.");
		return EINA_FALSE;
	}

	if (!strcmp(ev->keyname, CALLUI_KEY_MEDIA)) {
		ad->earset_key_longpress_timer = ecore_timer_add(EARSET_KEY_LONG_PRESS_TIMEOUT, __callui_app_win_earset_key_longpress_timer_cb, ad);
	} else if (!strcmp(ev->keyname, CALLUI_KEY_SELECT)) {
		/*todo*/
	}
	dbg("End..");
	return EINA_FALSE;
}
