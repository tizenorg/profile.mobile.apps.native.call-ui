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
#define OUTGOING_CALL_TIME_DURATION_STR "00:00"
#define ENDING_TIMER_INTERVAL 2.0
#define BLINKING_TIMER_INTERVAL 0.5
#define BLINKING_MAX_COUNT 5
#define REPLY_BTNS_DIVIDER_WIDTH 2

#define AO016 217, 217, 217, 255

struct _callui_view_callend {
	call_view_data_base_t base_view;

	Evas_Object *create_update_popup;

	Evas_Object *reply_btns_box;

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

static callui_result_e __set_single_call_info(callui_view_callend_h vd, const callui_call_data_t *call_data);

static callui_result_e __create_reply_btns_panel(callui_view_callend_h vd);
static Evas_Object *__create_call_back_btn(callui_view_callend_h vd);
static Evas_Object *__create_message_btn(callui_view_callend_h vd);
static Evas_Object *__create_reply_btns_divider(callui_view_callend_h vd);

static callui_result_e __create_single_contact_info(callui_view_callend_h vd, const callui_call_data_t *call_data);

static void __set_emergency_call_info(callui_view_callend_h vd, const callui_call_data_t *call_data);
static void __set_conference_call_info(callui_view_callend_h vd, const callui_call_data_t *call_data);

static void __bg_mouse_down_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void __minimize_anim_completed_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void __maximize_anim_completed_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

static void __run_minimize_animation(callui_view_callend_h vd);
static void __run_maximize_animation(callui_view_callend_h vd);

static void __launch_contact_app(const char *operation, const char *call_number);

callui_view_callend_h _callui_view_callend_new()
{
	callui_view_callend_h callend_view = calloc(1, sizeof(_callui_view_callend_t));
	CALLUI_RETURN_NULL_IF_FAIL(callend_view);

	callend_view->base_view.create = __callui_view_callend_oncreate;
	callend_view->base_view.destroy = __callui_view_callend_ondestroy;

	return callend_view;
}

static callui_result_e __callui_view_callend_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_callend_h vd = (callui_view_callend_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	_callui_common_win_set_noti_type(vd->base_view.ad, true);

	callui_result_e res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = __update_displayed_data(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	__run_minimize_animation(vd);

	return res;
}

static void __bg_mouse_down_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);

	_callui_common_exit_app();
}

static callui_result_e __callui_view_callend_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_callend_h vd = (callui_view_callend_h)view_data;

	__delete_ending_timer(vd);

	free(vd->time_string);

	edje_object_signal_callback_del_full(_EDJ(vd->base_view.contents), "mouse,down,*", "background", __bg_mouse_down_cb, vd);

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

	elm_object_signal_callback_add(vd->base_view.contents,
			"mouse,down,*", "bg", __bg_mouse_down_cb, vd);
	elm_object_signal_callback_add(vd->base_view.contents,
			"minimize_anim_completed", "main_end_call", __minimize_anim_completed_cb, vd);
	elm_object_signal_callback_add(vd->base_view.contents,
			"maximize_anim_completed", "main_end_call", __maximize_anim_completed_cb, vd);

	Evas_Object *end_call_btn = _callui_create_end_call_button(vd->base_view.contents, NULL, NULL);
	CALLUI_RETURN_VALUE_IF_FAIL(end_call_btn, CALLUI_RESULT_ALLOCATION_FAIL);
	evas_object_freeze_events_set(end_call_btn, EINA_TRUE);

	return CALLUI_RESULT_OK;
}

static Evas_Object *__create_call_back_btn(callui_view_callend_h vd)
{
	Evas_Object *button_call_back = _callui_load_edj(vd->reply_btns_box, EDJ_NAME, GRP_ENDCALL_CALL_BACK_BTN);
	CALLUI_RETURN_NULL_IF_FAIL(button_call_back);
	edje_object_signal_callback_add(_EDJ(button_call_back), "clicked", "edje", __call_back_btn_click_cb, vd);
	elm_object_translatable_part_text_set(button_call_back, "text", "IDS_CALL_BUTTON_CALL");

	evas_object_size_hint_weight_set(button_call_back, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(button_call_back, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(button_call_back);

	return button_call_back;
}

static Evas_Object *__create_message_btn(callui_view_callend_h vd)
{
	Evas_Object *button_message = _callui_load_edj(vd->reply_btns_box, EDJ_NAME, GRP_ENDCALL_MSG_BTN);
	CALLUI_RETURN_NULL_IF_FAIL(button_message);
	edje_object_signal_callback_add(_EDJ(button_message), "clicked", "edje", __msg_btn_click_cb, vd);
	elm_object_translatable_part_text_set(button_message, "text", "IDS_COM_BODY_MESSAGE");

	evas_object_size_hint_weight_set(button_message, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(button_message, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(button_message);

	return button_message;
}

static callui_result_e __create_single_contact_info(callui_view_callend_h vd, const callui_call_data_t *call_data)
{
	const char *call_name = call_data->call_ct_info.call_disp_name;

	if (!(call_name && call_name[0] != '\0')) {
		Evas_Object *add_contact_btn = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_ENDCALL_ADD_CONTACT_BTN);
		CALLUI_RETURN_VALUE_IF_FAIL(add_contact_btn,  CALLUI_RESULT_ALLOCATION_FAIL);
		evas_object_event_callback_add(add_contact_btn, EVAS_CALLBACK_MOUSE_UP, __add_contact_btn_click_cb, vd);
		elm_object_part_content_set(vd->base_view.contents, "add_contact_btn", add_contact_btn);

		elm_object_part_text_set(vd->base_view.contents, "contact_name", vd->call_number);

		elm_object_translatable_part_text_set(vd->base_view.contents, "contact_number", "IDS_COM_OPT_ADD_TO_CONTACTS");
		elm_object_signal_emit(vd->base_view.contents, "set_add_contact_btn_enabled", "main_end_call");

		/* For big contact info */
		elm_object_signal_emit(vd->base_view.contents, "1line", "main_end_call");
		elm_object_part_text_set(vd->base_view.contents, "contact_name_big", vd->call_number);
	} else {
		elm_object_part_text_set(vd->base_view.contents, "contact_name", call_name);
		elm_object_part_text_set(vd->base_view.contents, "contact_number", vd->call_number);

		/* For big contact info */
		elm_object_signal_emit(vd->base_view.contents, "2line", "main_end_call");
		elm_object_part_text_set(vd->base_view.contents, "contact_name_big", call_name);
		elm_object_part_text_set(vd->base_view.contents, "contact_number_big", vd->call_number);
	}

	return CALLUI_RESULT_OK;
}

static Evas_Object *__create_reply_btns_divider(callui_view_callend_h vd)
{
	Evas_Object *divider = evas_object_rectangle_add(vd->reply_btns_box);
	CALLUI_RETURN_NULL_IF_FAIL(divider);

	evas_object_size_hint_fill_set(divider, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(divider, AO016);
	evas_object_size_hint_min_set(divider, ELM_SCALE_SIZE(REPLY_BTNS_DIVIDER_WIDTH), 0);
	evas_object_show(divider);

	return divider;
}

static callui_result_e __create_reply_btns_panel(callui_view_callend_h vd)
{

	vd->reply_btns_box = elm_box_add(vd->base_view.contents);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->reply_btns_box, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_box_horizontal_set(vd->reply_btns_box, EINA_TRUE);

	Evas_Object *call_back_btn = __create_call_back_btn(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(call_back_btn, CALLUI_RESULT_ALLOCATION_FAIL);

	Evas_Object *message_btn = __create_message_btn(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(message_btn, CALLUI_RESULT_ALLOCATION_FAIL);

	Evas_Object *divider = __create_reply_btns_divider(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(divider, CALLUI_RESULT_ALLOCATION_FAIL);

	elm_box_pack_end(vd->reply_btns_box, call_back_btn);
	elm_box_pack_end(vd->reply_btns_box, divider);
	elm_box_pack_end(vd->reply_btns_box, message_btn);
	evas_object_show(vd->reply_btns_box);

	elm_object_part_content_set(vd->base_view.contents, "reply_btns", vd->reply_btns_box);

	return CALLUI_RESULT_OK;
}

static callui_result_e __set_single_call_info(callui_view_callend_h vd, const callui_call_data_t *call_data)
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
		elm_object_translatable_part_text_set(vd->base_view.contents,
				"contact_name", "IDS_CALL_BODY_UNKNOWN");
		return CALLUI_RESULT_OK;
	}
	strncpy(vd->call_number, call_number, sizeof(vd->call_number));


	callui_result_e res = __create_single_contact_info(vd, call_data);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = __create_reply_btns_panel(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	elm_object_signal_emit(vd->base_view.contents, "show_reply_btns", "main_end_call");

	return res;
}

static void __set_emergency_call_info(callui_view_callend_h vd, const callui_call_data_t *call_data)
{
	elm_object_signal_emit(vd->base_view.contents, "set_emergency_mode", "main_end_call");
	elm_object_translatable_part_text_set(vd->base_view.contents, "contact_name", "IDS_COM_BODY_EMERGENCY_NUMBER");

	/* For big contact info */
	elm_object_signal_emit(vd->base_view.contents, "1line", "main_end_call");
	elm_object_translatable_part_text_set(vd->base_view.contents, "contact_name_big", "IDS_COM_BODY_EMERGENCY_NUMBER");
}

static void __set_conference_call_info(callui_view_callend_h vd, const callui_call_data_t *call_data)
{
	elm_object_signal_emit(vd->base_view.contents, "set_conference_mode", "main_end_call");

	char *status = _("IDS_CALL_BODY_WITH_PD_PEOPLE_M_CONFERENCE_CALL_ABB");
	char buf[CALLUI_BUF_MEMBER_SIZE] = { 0 };
	snprintf(buf, CALLUI_BUF_MEMBER_SIZE, status, call_data->conf_member_count);
	elm_object_part_text_set(vd->base_view.contents, "contact_number", buf);
	elm_object_translatable_part_text_set(vd->base_view.contents, "contact_name", "IDS_CALL_BODY_CONFERENCE");

	/* For big contact info */
	elm_object_signal_emit(vd->base_view.contents, "2line", "main_end_call");
	elm_object_translatable_part_text_set(vd->base_view.contents, "contact_name_big", "IDS_CALL_BODY_CONFERENCE");
	elm_object_translatable_part_text_set(vd->base_view.contents, "contact_number_big", buf);
}

static callui_result_e __set_ended_call_duration_sting(callui_view_callend_h vd, callui_call_data_t *call_data)
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

static void __minimize_anim_completed_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	callui_view_callend_h vd = data;

	if (vd->reply_btns_box) {
		evas_object_freeze_events_set(vd->reply_btns_box, EINA_FALSE);
	}

	callui_result_e res = __create_ending_timer(vd);
	CALLUI_RETURN_IF_FAIL(res == CALLUI_RESULT_OK);
}

static void __maximize_anim_completed_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	callui_view_callend_h vd = data;

	callui_result_e res = _callui_manager_dial_voice_call(vd->base_view.ad->call_manager, vd->call_number, CALLUI_SIM_SLOT_DEFAULT);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_dial_voice_call() failed. res[%d]", res);
		_callui_common_exit_app();
	}
}

static void __run_minimize_animation(callui_view_callend_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	if (vd->reply_btns_box) {
		evas_object_freeze_events_set(vd->reply_btns_box, EINA_TRUE);
	}

	_callui_action_bar_show(ad->action_bar);
	_callui_action_bar_set_disabled_state(ad->action_bar, true);

	elm_object_signal_emit(ad->main_ly, "minimize", "app_main_ly");
	elm_object_signal_emit(vd->base_view.contents, "minimize", "main_end_call");
}

static void __run_maximize_animation(callui_view_callend_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	__delete_ending_timer(vd);
	elm_object_part_text_set(vd->base_view.contents, "call_txt_status", "");

	if (vd->reply_btns_box) {
		evas_object_freeze_events_set(vd->reply_btns_box, EINA_TRUE);
	}

	_callui_action_bar_show(ad->action_bar);
	_callui_action_bar_set_disabled_state(ad->action_bar, true);

	elm_object_signal_emit(ad->main_ly, "maximize", "app_main_ly");
	elm_object_signal_emit(vd->base_view.contents, "maximize", "main_end_call");
}

static callui_result_e __update_displayed_data(callui_view_callend_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	callui_call_data_t *call_data = ad->end_call_data;
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

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static void __call_back_btn_click_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CALLUI_RETURN_IF_FAIL(data);

	__run_maximize_animation(data);
}

static char *__vcui_endcall_get_item_text(void *data, Evas_Object *obj, const char *part)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, NULL);
	if (!strcmp(part, "elm.text")) {
		return strdup(data);
	}
	return NULL;
}

static void __launch_contact_app(const char *operation, const char *call_number)
{
	app_control_h app_control = NULL;
	int ret;
	if ((ret = app_control_create(&app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_create() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, operation)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_mime(app_control, APP_CONTROL_MIME_CONTACT)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_mime() is failed. ret[%d]", ret);
	} else if ((ret = app_control_add_extra_data(app_control, APP_CONTROL_DATA_PHONE, call_number)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_add_extra_data() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed. ret[%d]", ret);
	}
	if (app_control) {
		app_control_destroy(app_control);
	}
}

static void __create_contact_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_callend_h vd = data;

	_callui_common_exit_app();

	__launch_contact_app(APP_CONTROL_OPERATION_ADD, vd->call_number);
}

static void __update_contact_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_callend_h vd = data;

	_callui_common_exit_app();

	__launch_contact_app(APP_CONTROL_OPERATION_EDIT, vd->call_number);
}

static void __popup_back_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_callend_h vd = data;

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
	callui_view_callend_h vd = data;
	callui_app_data_t *ad = vd->base_view.ad;

	edje_object_signal_callback_del_full(_EDJ(obj), "clicked", "edje", __msg_btn_click_cb, vd);

	_callui_common_launch_msg_composer(ad, vd->call_number);

	_callui_common_exit_app();
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
		elm_object_translatable_part_text_set(vd->base_view.contents,
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
