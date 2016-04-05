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
#include <efl_extension.h>

#include "callui-view-callend.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-view-elements.h"
#include "callui-common.h"
#include "callui-manager.h"
#include "callui-state-provider.h"

#define APP_CONTROL_MIME_CONTACT "application/vnd.tizen.contact"
#define CONTACT_NUMBER_BUF_LEN 32
#define OUTGOING_CALL_TIME_DURATION_STR "00:00"
#define ENDING_TIMER_INTERVAL 2.0
#define BLINKING_TIMER_INTERVAL 0.5
#define BLINKING_MAX_COUNT 5

struct _callui_view_callend {
	call_view_data_base_t base_view;

	Evas_Object *create_update_popup;

	char call_number[CALLUI_PHONE_DISP_NUMBER_LENGTH_MAX];

	Ecore_Timer *blink_timer;
	Ecore_Timer *ending_timer;
	char *time_string;

	int blink_cnt;
};
typedef struct _callui_view_callend _callui_view_callend_t;

static callui_result_e __callui_view_callend_oncreate(call_view_data_base_t *view_data, void *appdata);
static callui_result_e __callui_view_callend_ondestroy(call_view_data_base_t *view_data);

static callui_result_e __create_main_content(callui_view_callend_h vd);
static callui_result_e __update_displayed_data(callui_view_callend_h vd);

static void __call_back_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void __msg_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

static char *__vcui_endcall_get_item_text(void *data, Evas_Object *obj, const char *part);

static void __create_contact_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __update_contact_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __popup_back_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __add_contact_btn_click_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static Eina_Bool __ending_timer_expired_cb(void *data);
static Eina_Bool __ending_timer_blink_cb(void *data);
static callui_result_e __create_ending_timer(callui_view_callend_h vd);
static void __delete_ending_timer(callui_view_callend_h vd);

static callui_result_e __set_single_call_info(callui_view_callend_h vd, const callui_call_state_data_t *call_data);

static callui_result_e __create_call_back_btn(callui_view_callend_h vd);
static callui_result_e __create_message_btn(callui_view_callend_h vd);
static callui_result_e __create_single_contact_info(callui_view_callend_h vd, const callui_call_state_data_t *call_data);

static void __set_emergency_call_info(callui_view_callend_h vd, const callui_call_state_data_t *call_data);
static void __set_conference_call_info(callui_view_callend_h vd, const callui_call_state_data_t *call_data);

callui_view_callend_h _callui_view_callend_new()
{
	callui_view_callend_h callend_view = calloc(1, sizeof(_callui_view_callend_t));
	CALLUI_RETURN_NULL_IF_FAIL(callend_view);

	callend_view->base_view.onCreate = __callui_view_callend_oncreate;
	callend_view->base_view.onUpdate = NULL;
	callend_view->base_view.onDestroy = __callui_view_callend_ondestroy;

	return callend_view;
}

static callui_result_e __callui_view_callend_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_callend_h vd = (callui_view_callend_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	_callui_common_win_set_noti_type(vd->base_view.ad, EINA_TRUE);

	callui_result_e res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return __update_displayed_data(vd);
}

static callui_result_e __callui_view_callend_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_callend_h vd = (callui_view_callend_h)view_data;

	__delete_ending_timer(vd);

	free(vd->time_string);

	DELETE_EVAS_OBJECT(vd->create_update_popup);
	DELETE_EVAS_OBJECT(vd->base_view.contents);

	free(vd);

	return CALLUI_RESULT_OK;
}

static callui_result_e __create_main_content(callui_view_callend_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME, GRP_ENDCALL_MAIN_LAYOUT);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", vd->base_view.contents);

	return CALLUI_RESULT_OK;
}

static callui_result_e __create_call_back_btn(callui_view_callend_h vd)
{
	Evas_Object *button_call_back = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_ENDCALL_CALL_BACK_BTN);
	CALLUI_RETURN_VALUE_IF_FAIL(button_call_back, CALLUI_RESULT_ALLOCATION_FAIL);
	edje_object_signal_callback_add(_EDJ(button_call_back), "clicked", "edje", __call_back_btn_click_cb, vd);
	_callui_common_eo_txt_part_set_translatable_text(button_call_back,
			"end_btn_text", "IDS_CALL_BUTTON_CALL");
	elm_object_part_content_set(vd->base_view.contents, "button_call_back", button_call_back);

	return CALLUI_RESULT_OK;
}

static callui_result_e __create_message_btn(callui_view_callend_h vd)
{
	Evas_Object *button_message = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_ENDCALL_MSG_BTN);
	CALLUI_RETURN_VALUE_IF_FAIL(button_message,  CALLUI_RESULT_ALLOCATION_FAIL);
	edje_object_signal_callback_add(_EDJ(button_message), "clicked", "edje", __msg_btn_click_cb, vd);
	_callui_common_eo_txt_part_set_translatable_text(button_message,
			"end_btn_text", "IDS_COM_BODY_MESSAGE");
	elm_object_part_content_set(vd->base_view.contents, "button_message_back", button_message);

	return CALLUI_RESULT_OK;
}

static callui_result_e __create_single_contact_info(callui_view_callend_h vd, const callui_call_state_data_t *call_data)
{
	const char *call_name = call_data->call_ct_info.call_disp_name;

	if (!(call_name && call_name[0] != '\0')) {
		Evas_Object *button_create = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_ENDCALL_CREATE_CONT_BTN);
		CALLUI_RETURN_VALUE_IF_FAIL(button_create,  CALLUI_RESULT_ALLOCATION_FAIL);
		evas_object_event_callback_add(button_create, EVAS_CALLBACK_MOUSE_UP, __add_contact_btn_click_cb, vd);
		elm_object_part_content_set(vd->base_view.contents, "swallow.create_contact", button_create);

		elm_object_part_text_set(vd->base_view.contents, "contact_name", vd->call_number);
		_callui_common_eo_txt_part_set_translatable_text(vd->base_view.contents, "contact_number", "IDS_COM_OPT_ADD_TO_CONTACTS");
	} else {
		elm_object_part_text_set(vd->base_view.contents, "contact_name", call_name);
		elm_object_part_text_set(vd->base_view.contents, "contact_number", vd->call_number);
	}

	return CALLUI_RESULT_OK;
}

static callui_result_e __set_single_call_info(callui_view_callend_h vd, const callui_call_state_data_t *call_data)
{
	const char *file_path = call_data->call_ct_info.caller_id_path;
	const char *call_name = call_data->call_ct_info.call_disp_name;
	const char *call_number = NULL;

	if (call_data->call_disp_num[0] != '\0') {
		call_number = call_data->call_disp_num;
	} else {
		call_number = call_data->call_num;
	}

	if (strcmp(file_path, "default") != 0) {
		Evas_Object *layout = _callui_create_thumbnail(vd->base_view.contents, file_path, THUMBNAIL_138);
		elm_object_part_content_set(vd->base_view.contents, "caller_id", layout);
		elm_object_signal_emit(vd->base_view.contents, "hide_def_caller_id", "main_end_call");
	} else {
		elm_object_signal_emit(vd->base_view.contents, "show_def_caller_id", "main_end_call");
	}

	if (!(call_name && call_name[0] != '\0') && !(call_number && call_number[0] != '\0')) {
		_callui_common_eo_txt_part_set_translatable_text(vd->base_view.contents,
				"contact_name", "IDS_CALL_BODY_UNKNOWN");
		return CALLUI_RESULT_OK;
	}

	strncpy(vd->call_number, call_number, sizeof(vd->call_number));

	callui_result_e res = __create_call_back_btn(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = __create_message_btn(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = __create_single_contact_info(vd, call_data);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return res;
}

static void __set_emergency_call_info(callui_view_callend_h vd, const callui_call_state_data_t *call_data)
{
	elm_object_signal_emit(vd->base_view.contents, "set_emergency_mode", "main_end_call");
	_callui_common_eo_txt_part_set_translatable_text(vd->base_view.contents, "contact_name", "IDS_COM_BODY_EMERGENCY_NUMBER");
}

static void __set_conference_call_info(callui_view_callend_h vd, const callui_call_state_data_t *call_data)
{
	elm_object_signal_emit(vd->base_view.contents, "set_conference_mode", "main_end_call");
	_callui_common_eo_txt_part_set_translatable_text(vd->base_view.contents,
			"contact_name", "IDS_CALL_BODY_CONFERENCE");

	char *status = _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB");
	char buf[CALLUI_BUF_MEMBER_SIZE] = { 0 };
	snprintf(buf, CALLUI_BUF_MEMBER_SIZE, status, call_data->conf_member_count);
	elm_object_part_text_set(vd->base_view.contents, "contact_number", buf);
}

static callui_result_e __set_ended_call_duration_sting(callui_view_callend_h vd, callui_call_state_data_t *call_data)
{
	if (call_data->is_dialing) {
		vd->time_string = strdup(OUTGOING_CALL_TIME_DURATION_STR);
	} else {
		struct tm *call_time = _callui_common_get_current_time_diff_in_tm(call_data->start_time);
		CALLUI_RETURN_VALUE_IF_FAIL(call_time, CALLUI_RESULT_ALLOCATION_FAIL);
		vd->time_string = _callui_common_get_time_string(call_time);
		free(call_time);
	}
	return CALLUI_RESULT_OK;
}

static callui_result_e __update_displayed_data(callui_view_callend_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	callui_call_state_data_t *call_data = ad->end_call_data;
	CALLUI_RETURN_VALUE_IF_FAIL(call_data, CALLUI_RESULT_FAIL);
	callui_result_e res = CALLUI_RESULT_FAIL;

	if (call_data->is_emergency) {
		__set_emergency_call_info(vd, call_data);
	} else if (call_data->conf_member_count > 1) {
		__set_conference_call_info(vd, call_data);
	} else {
		res = __set_single_call_info(vd, call_data);
		CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);
	}

	res = __set_ended_call_duration_sting(vd, call_data);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = __create_ending_timer(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static void __call_back_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_view_callend_h vd = (callui_view_callend_h)data;

	edje_object_signal_callback_del_full(_EDJ(obj), "clicked", "edje", __call_back_btn_click_cb, vd);

	__delete_ending_timer(vd);

	callui_result_e res = _callui_manager_dial_voice_call(vd->base_view.ad->call_manager, vd->call_number, CALLUI_SIM_SLOT_DEFAULT);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_dial_voice_call() failed. res[%d]", res);
	}
}

static char *__vcui_endcall_get_item_text(void *data, Evas_Object *obj, const char *part)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, NULL);
	if (!strcmp(part, "elm.text")) {
		return strdup(data);
	}
	return NULL;
}

static void __create_contact_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_view_callend_h vd = (callui_view_callend_h)data;

	_callui_common_exit_app();

	// TODO: Need to replace into launcher
	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_ADD);
	app_control_set_mime(request, APP_CONTROL_MIME_CONTACT);
	app_control_add_extra_data(request, APP_CONTROL_DATA_PHONE, vd->call_number);

	int err = app_control_send_launch_request(request, NULL, NULL);
	if (err != APP_CONTROL_ERROR_NONE) {
		dbg("app_control_send_launch_request() is failed");
	}
	app_control_destroy(request);
}

static void __update_contact_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_view_callend_h vd = (callui_view_callend_h)data;

	_callui_common_exit_app();

	// TODO: Need to replace into launcher
	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_EDIT);
	app_control_set_mime(request, APP_CONTROL_MIME_CONTACT);
	app_control_add_extra_data(request, APP_CONTROL_DATA_PHONE, vd->call_number);

	int result = app_control_send_launch_request(request, NULL, NULL);
	if (result != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() failed (%d)", result);
	}
	app_control_destroy(request);
}

static void __popup_back_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_callend_h vd = (callui_view_callend_h)data;

	evas_object_del(obj);

	if (vd->ending_timer) {
		ecore_timer_thaw(vd->ending_timer);
	} else if (vd->blink_timer) {
		ecore_timer_thaw(vd->blink_timer);
	}
}

static void __add_contact_btn_click_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_view_callend_h vd = (callui_view_callend_h)data;

	if (vd->ending_timer) {
		ecore_timer_freeze(vd->ending_timer);
	} else if (vd->blink_timer) {
		ecore_timer_freeze(vd->blink_timer);
	}

	Evas_Object *parent = elm_object_top_widget_get(obj);
	CALLUI_RETURN_IF_FAIL(parent);

	vd->create_update_popup = elm_popup_add(parent);
	eext_object_event_callback_add(vd->create_update_popup, EEXT_CALLBACK_BACK, __popup_back_click_cb, vd);

	elm_popup_align_set(vd->create_update_popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	elm_object_part_text_set(vd->create_update_popup, "title,text",  vd->call_number);

	Evas_Object *genlist = elm_genlist_add(vd->create_update_popup);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	CALLUI_RETURN_IF_FAIL(itc);
	itc->item_style = "type1";
	itc->func.text_get = __vcui_endcall_get_item_text;

	elm_genlist_item_append(genlist, itc, _("IDS_COM_OPT_CREATE_CONTACT"),
			NULL, ELM_GENLIST_ITEM_NONE, __create_contact_btn_click_cb, vd);
	elm_genlist_item_append(genlist, itc, "Update contact",
			NULL, ELM_GENLIST_ITEM_NONE, __update_contact_btn_click_cb, vd);

	elm_genlist_item_class_free(itc);
	elm_object_content_set(vd->create_update_popup, genlist);
	elm_popup_orient_set(vd->create_update_popup, ELM_POPUP_ORIENT_CENTER);
	evas_object_show(vd->create_update_popup);
}

static void __msg_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_view_callend_h vd = (callui_view_callend_h)data;

	edje_object_signal_callback_del_full(_EDJ(obj), "clicked", "edje", __msg_btn_click_cb, vd);

	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_COMPOSE);
	char str[CONTACT_NUMBER_BUF_LEN];
	snprintf(str, sizeof(str), "sms:%s", vd->call_number);
	app_control_set_uri(request, str);

	int result = app_control_send_launch_request(request, NULL, NULL);
	if (result != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() failed (%d)", result);
	}
	app_control_destroy(request);
}

static Eina_Bool __ending_timer_expired_cb(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_callend_h vd = (callui_view_callend_h)data;

	vd->ending_timer = NULL;
	_callui_common_exit_app();

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __ending_timer_blink_cb(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_callend_h vd = (callui_view_callend_h)data;

	if ((vd->blink_cnt % 2) == 0) {
		_callui_show_caller_info_status(vd->base_view.ad, vd->time_string);
	} else if ((vd->blink_cnt % 2) == 1) {
		_callui_show_caller_info_status(vd->base_view.ad, "");
	}

	vd->blink_cnt++;
	if (vd->blink_cnt == BLINKING_MAX_COUNT) {
		/* Run a timer of 2 sec for destroying the end selection menu */
		DELETE_ECORE_TIMER(vd->ending_timer);
		_callui_common_eo_txt_part_set_translatable_text(vd->base_view.contents,
				"call_txt_status", "IDS_CALL_BODY_CALL_ENDE_M_STATUS_ABB");
		vd->ending_timer = ecore_timer_add(ENDING_TIMER_INTERVAL, __ending_timer_expired_cb, vd);

		vd->blink_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}
	return ECORE_CALLBACK_RENEW;
}

static callui_result_e __create_ending_timer(callui_view_callend_h vd)
{
	vd->blink_cnt = 0;
	DELETE_ECORE_TIMER(vd->blink_timer);
	vd->blink_timer = ecore_timer_add(BLINKING_TIMER_INTERVAL, __ending_timer_blink_cb, vd);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->blink_timer, CALLUI_RESULT_ALLOCATION_FAIL);
	return CALLUI_RESULT_OK;
}

static void __delete_ending_timer(callui_view_callend_h vd)
{
	DELETE_ECORE_TIMER(vd->ending_timer);
	DELETE_ECORE_TIMER(vd->blink_timer);
}
