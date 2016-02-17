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
#include "callui-view-layout.h"

#define EARSET_KEY_LONG_PRESS_TIMEOUT			1.0

static bool _callui_app_create_layout(void *data);
static Eina_Bool __callui_app_win_hard_key_down_cb(void *data, int type, void *event);
static Eina_Bool __callui_app_win_hard_key_up_cb(void *data, int type, void *event);
static void __callui_app_terminate(void *data);
static void __callui_app_terminate_or_view_change(callui_app_data_t *ad);

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

static void __callui_process_incoming_call(callui_app_data_t *ad)
{
	dbg("..");
	cm_call_data_t *cm_incom = NULL;

	CALLUI_RETURN_IF_FAIL(ad);

	cm_get_all_calldata(ad->cm_handle, &cm_incom, NULL, NULL);
	CALLUI_RETURN_IF_FAIL(cm_incom);

	__callui_update_call_data(&(ad->incom), cm_incom);
	_callui_vm_change_view(ad->view_manager_handle, VIEW_TYPE_INCOMING_LOCK);

	cm_call_data_free(cm_incom);
	return;
}

static void __callui_process_outgoing_call(callui_app_data_t *ad, char *number)
{
	sec_dbg("number is (%s)", number);
	CALLUI_RETURN_IF_FAIL(ad);

	if (CM_ERROR_NONE != cm_dial_call(ad->cm_handle, number, CM_CALL_TYPE_VOICE, CM_SIM_SLOT_DEFAULT_E)) {
		err("cm_dial_call failed!!");
		__callui_app_terminate_or_view_change(ad);
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
		_callui_vm_auto_change_view(ad->view_manager_handle);

#ifdef _DBUS_DVC_LSD_TIMEOUT_
		if (ad->speaker_status == EINA_TRUE) {
			_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_SET);
		}
#endif
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
				__callui_app_terminate_or_view_change(ad);
			} else if (incom_call == NULL && active_call == NULL && held_call == NULL) {
				_callui_common_delete_duration_timer();
				_callui_vm_change_view(ad->view_manager_handle, VIEW_TYPE_ENDCALL);
				_callui_lock_manager_stop(ad->lock_handle);
				__callui_update_all_call_data(ad, call_state_data);
			} else {
				__callui_update_all_call_data(ad, call_state_data);
				__callui_app_terminate_or_view_change(ad);
			}
		}
		break;
	case CM_CALL_EVENT_INCOMING:
		{
			_callui_common_dvc_control_lcd_state(LCD_ON_LOCK);
			__callui_update_all_call_data(ad, call_state_data);
			_callui_vm_auto_change_view(ad->view_manager_handle);
		}
		break;
	case CM_CALL_EVENT_DIALING:
	case CM_CALL_EVENT_HELD:
	case CM_CALL_EVENT_RETRIEVED:
	case CM_CALL_EVENT_SWAPPED:
	case CM_CALL_EVENT_JOIN:
	case CM_CALL_EVENT_SPLIT:
		__callui_update_all_call_data(ad, call_state_data);
		_callui_vm_auto_change_view(ad->view_manager_handle);
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

static gboolean __callui_init_sys_api(callui_app_data_t *ad)
{
	dbg("..");

	if (CM_ERROR_NONE != cm_init(&ad->cm_handle)) {
		err("cm_init() err");
		return false;
	}
	cm_set_call_event_cb(ad->cm_handle, __callui_call_event_cb, ad);
	cm_set_audio_state_changed_cb(ad->cm_handle, __callui_audio_state_changed_cb, ad);

	__callui_bt_init();

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

	dbg("Create window");
	Evas_Object *eo = elm_win_add(NULL, PACKAGE, ELM_WIN_BASIC);
	elm_win_alpha_set(eo, EINA_TRUE);
	elm_win_fullscreen_set(eo, EINA_FALSE);

	if (eo) {
		elm_win_title_set(eo, PACKAGE);
		evas_object_smart_callback_add(eo, "delete,request", __callui_win_delete_request_cb, NULL);
		elm_win_screen_size_get(eo, NULL, NULL, &ad->root_w, &ad->root_h);

		dbg("root_w = %d, root_h = %d..", ad->root_w, ad->root_h);
		evas_object_resize(eo, ad->root_w, ELM_SCALE_SIZE(MTLOCK_ACTIVE_CALL_HEIGHT));

		elm_win_center(eo, EINA_FALSE, EINA_TRUE);
		evas_object_move(eo, 0, 0);
		elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_SHOW);
		elm_win_indicator_opacity_set(eo, ELM_WIN_INDICATOR_TRANSLUCENT);
		elm_win_conformant_set(eo, EINA_TRUE);

		ad->win_conformant = elm_conformant_add(eo);
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

static bool _callui_app_create(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, false);

	callui_app_data_t *ad = data;

	elm_app_base_scale_set(2.6);

	elm_config_preferred_engine_set("opengl_x11");

	_callui_common_dvc_control_lcd_state(LCD_OFF_SLEEP_LOCK);

	int ret = __callui_init_sys_api(ad);
	if (!ret) {
		err("__callui_init_sys_api failed");
		return FALSE;
	}

	if (!_callui_app_create_layout(ad)) {
		err("_callui_app_create_layout failed");
		return FALSE;
	}
	ad->view_manager_handle = _callui_vm_create(ad);

	ad->lock_handle = _callui_lock_manager_create();

	ad->qp_minicontrol =_callui_qp_mc_create(ad);

	elm_theme_extension_add(NULL, CALL_THEME);

	return true;
}

static bool _callui_app_create_layout(void *data)
{
	callui_app_data_t *ad = data;

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

	if (ad->view_manager_handle) {
		_callui_vm_destroy(ad->view_manager_handle);
		ad->view_manager_handle = NULL;
	}

	if (ad->qp_minicontrol) {
		_callui_qp_mc_destroy(ad->qp_minicontrol);
		ad->qp_minicontrol = NULL;
	}

	if (ad->lock_handle) {
		_callui_lock_manager_destroy(ad->lock_handle);
		ad->lock_handle = NULL;
	}

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
}

static void _callui_app_service(app_control_h app_control, void *data)
{
	dbg("..");
	callui_app_data_t *ad = data;
	char *tmp = NULL;
	int ret = 0;
	char *uri_bundle = NULL;
	char *operation = NULL;

	if (_callui_vm_get_cur_view_type(ad->view_manager_handle) == VIEW_TYPE_UNDEFINED
			&& !ad->waiting_dialing) {
		err("VIEW_TYPE_UNDEFINED. Clear data");
		__callui_call_list_init(ad);
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
			ret = app_control_get_extra_data(app_control, "sim_slot", &tmp);
			if (ret != APP_CONTROL_ERROR_NONE) {
				err("app_control_get_extra_data failed");
			}
			if (tmp) {
				dbg("sim_slot: [%s]", tmp);
				ad->sim_slot = atoi(tmp);
				free(tmp);
				tmp = NULL;
			}
			__callui_process_incoming_call(ad);
		} else {
			tmp = (char *)uri_bundle + 4;
			sec_dbg("number: [%s]", tmp);
			if (tmp) {
				sec_dbg("number: [%s]", tmp);
				evas_object_resize(ad->win, ad->root_w, ad->root_h);
				if (!ad->waiting_dialing) {
					__callui_process_outgoing_call(ad, tmp);
				}
			} else {
				err("number val is NULL");
			}
		}
	} else if (strcmp(operation, APP_CONTROL_OPERATION_DEFAULT) == 0) {
		/* */
		warn("Unsupported operation type");
	} else if (strcmp(operation, APP_CONTROL_OPERATION_DURING_CALL) == 0) {
		if (CM_ERROR_NONE != cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_NORMAL)) {
			err("cm_answer_call failed. ret[%d]", ret);
		}
	} else if (strcmp(operation, APP_CONTROL_OPERATION_MESSAGE_REJECT) == 0) {
		/* TODO Implement reject with message button functionality */
	} else if (strcmp(operation, APP_CONTROL_OPERATION_END_CALL) == 0) {
		if (CM_ERROR_NONE != cm_reject_call(ad->cm_handle)) {
			err("cm_reject_call failed. ret[%d]", ret);
		}
	}

	free(operation);
	free(uri_bundle);
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

	dbg("Top view(%d)", _callui_vm_get_cur_view_type(ad->view_manager_handle));

	/*power key case */
	if (!strcmp(ev->keyname, CALLUI_KEY_POWER)) {
		dbg("in keypower");
		bpowerkey_enabled = _callui_common_is_powerkey_mode_on();
		dbg("[KEY]KEY_POWER pressed, bpowerkey_enabled(%d)", bpowerkey_enabled);
		if (bpowerkey_enabled == EINA_TRUE && !_callui_lock_manager_is_lcd_off(ad->lock_handle)) {
			if (_callui_vm_get_cur_view_type(ad->view_manager_handle) == VIEW_TYPE_DIALLING) {
				if (ad->active)
					cm_end_call(ad->cm_handle, ad->active->call_id, CALL_RELEASE_TYPE_BY_CALL_HANDLE);
			} else if (_callui_vm_get_cur_view_type(ad->view_manager_handle) == VIEW_TYPE_INCOMING_LOCK) {
				if (ad->incom)
					cm_end_call(ad->cm_handle, ad->incom->call_id, CALL_RELEASE_TYPE_BY_CALL_HANDLE);
			} else if ((_callui_vm_get_cur_view_type(ad->view_manager_handle) == VIEW_TYPE_SINGLECALL)
						|| (_callui_vm_get_cur_view_type(ad->view_manager_handle) == VIEW_TYPE_MULTICALL_CONF)
						|| (_callui_vm_get_cur_view_type(ad->view_manager_handle) == VIEW_TYPE_MULTICALL_LIST)) {
				if (ad->active)
					cm_end_call(ad->cm_handle, ad->active->call_id, CALL_RELEASE_TYPE_ALL_CALLS);
				else if (ad->held)
					cm_end_call(ad->cm_handle, ad->held->call_id, CALL_RELEASE_TYPE_ALL_CALLS);
			} else if (_callui_vm_get_cur_view_type(ad->view_manager_handle) == VIEW_TYPE_MULTICALL_SPLIT) {
				if (ad->active)
					cm_end_call(ad->cm_handle, ad->active->call_id, CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS);
			} else {
				dbg("nothing...");
			}
		} else {
			if (ad->incom && !ad->active && !ad->held) {
				_callui_vm_change_view(ad->view_manager_handle, VIEW_TYPE_INCOMING_LOCK);
			}
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
		if (_callui_vm_get_cur_view_type(ad->view_manager_handle) == VIEW_TYPE_INCOMING_LOCK) {
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
			//Ecore_X_Window focus_win = ecore_x_window_focus_get();
			//if (ad->win != NULL && focus_win == elm_win_xwindow_get(ad->win)) {
				/* ToDo: Use lock-screen interface to raise the home screen */
				_callui_common_win_set_noti_type(ad, EINA_FALSE);
				_callui_lock_manager_stop(ad->lock_handle);
				ad->on_background = true;
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

	if (_callui_vm_get_cur_view_type(ad->view_manager_handle) == VIEW_TYPE_UNDEFINED) {
		dbg("ad->view_top is UNDEFINED");
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

static void __callui_app_terminate(void *data)
{
	_callui_common_exit_app();
}

static void __callui_app_terminate_or_view_change(callui_app_data_t *ad)
{
	CALLUI_RETURN_IF_FAIL(ad);

	if ((NULL == ad->active) && (NULL == ad->incom) && (NULL == ad->held)) {
		if (_callui_lock_manager_is_lcd_off(ad->lock_handle)) {
			_callui_lock_manager_set_callback_on_unlock(ad->lock_handle, __callui_app_terminate, NULL);
		} else {
			__callui_app_terminate(ad->lock_handle);
		}
	} else {
		_callui_vm_auto_change_view(ad->view_manager_handle);
	}
}
