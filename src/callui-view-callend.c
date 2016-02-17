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
#include "callui-view-callend.h"
#include "callui-view-elements.h"
#include "callui-common.h"
#include <app_control.h>

#define	CALLUI_VIEW_CALLEND_LAYOUT_ID "ENDCALLVIEW"
struct callui_endcall_view_priv {
	Evas_Object *contents;
	Evas_Object *caller_info;
	Evas_Object *caller_id;
	Evas_Object *btn_ly;
	Evas_Object *ic;	/* small size call image */
	Eina_Bool bshowbutton;
	Evas_Object *create_update_popup;
	char caller_id_path[CALLUI_IMAGE_PATH_LENGTH_MAX];
	char call_number[CALLUI_PHONE_DISP_NUMBER_LENGTH_MAX];
	int contact_person_id;
	int call_end_type;
	bool is_emergency_call;
};

#define APP_CONTROL_MIME_CONTACT "application/vnd.tizen.contact"
#define CONTACT_NUMBER_BUF_LEN 32
#define BG_COLOR_ALPHA 120

static int __callui_view_callend_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *param3);
static int __callui_view_callend_onupdate(call_view_data_t *view_data, void *update_data1);
static int __callui_view_callend_onhide(call_view_data_t *view_data);
static int __callui_view_callend_onshow(call_view_data_t *view_data, void *appdata);
static int __callui_view_callend_ondestroy(call_view_data_t *view_data);
static int __callui_view_callend_onrotate(call_view_data_t *view_data);
static void __callui_view_callend_create_screen(callui_app_data_t *ad, Evas_Object *eo, void *data);
static Evas_Object *__callui_view_callend_create_contents(void *data, char *grp_name);

call_view_data_t *_callui_view_callend_new(callui_app_data_t *ad)
{
	static call_view_data_t callend_view = {
		.type = VIEW_ENDCALL_VIEW,
		.layout = NULL,
		.onCreate = __callui_view_callend_oncreate,
		.onUpdate = __callui_view_callend_onupdate,
		.onHide = __callui_view_callend_onhide,
		.onShow = __callui_view_callend_onshow,
		.onDestroy = __callui_view_callend_ondestroy,
		.onRotate = __callui_view_callend_onrotate,
		.priv = NULL,
	};

	callend_view.priv = calloc(1, sizeof(callui_endcall_view_priv_t));

	if (!callend_view.priv) {
		err("ERROR!!!!!!!!!!!");
	}

	return &callend_view;
}

static void __callui_endcall_voicecall_btn_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);

	evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP, __callui_endcall_voicecall_btn_cb);

	_callui_common_delete_ending_timer();
	ad->speaker_status = EINA_FALSE;
	ad->mute_status = EINA_FALSE;
	ad->extra_volume_status = EINA_FALSE;
	cm_dial_call(ad->cm_handle, data, CM_CALL_TYPE_VOICE, ad->sim_slot);
	return;
}

char *__vcui_endcall_get_item_text(void *data, Evas_Object *obj, const char *part)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, NULL);
	if (!strcmp(part, "elm.text")) {
		return strdup(data);
	}

	return NULL;
}

static void __vcui_endcall_create_contact_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);
	_callvm_terminate_app_or_view_change(ad);
	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_ADD);
	app_control_set_mime(request, APP_CONTROL_MIME_CONTACT);

	if (data) {
		app_control_add_extra_data(request, APP_CONTROL_DATA_PHONE, data);
	}
	int err = app_control_send_launch_request(request, NULL, NULL);
	if (err != APP_CONTROL_ERROR_NONE) {
		dbg("app_control_send_launch_request() is failed");
	}
	app_control_destroy(request);
}

static void __vcui_endcall_update_contact_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);
	_callvm_terminate_app_or_view_change(ad);
	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_EDIT);
	app_control_set_mime(request, APP_CONTROL_MIME_CONTACT);

	if (data) {
		app_control_add_extra_data(request, APP_CONTROL_DATA_PHONE, data);
	}
	int result = app_control_send_launch_request(request, NULL, NULL);
	if (result != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() failed (%d)", result);
	}
	app_control_destroy(request);
}

static void __vcui_endcall_popup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);
	evas_object_del(obj);

	if (ad->ending_timer) {
		ecore_timer_thaw(ad->ending_timer);
	} else if (ad->blink_timer) {
		ecore_timer_thaw(ad->blink_timer);
	}
}

static void __vcui_endcall_create_popup_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);

	if (ad->ending_timer) {
		ecore_timer_freeze(ad->ending_timer);
	} else if (ad->blink_timer) {
		ecore_timer_freeze(ad->blink_timer);
	}
	callui_endcall_view_priv_t *priv = (callui_endcall_view_priv_t *)data;
	CALLUI_RETURN_IF_FAIL(priv);
	Evas_Object *parent = elm_object_top_widget_get(obj);
	CALLUI_RETURN_IF_FAIL(parent);
	priv->create_update_popup = elm_popup_add(parent);
	eext_object_event_callback_add(priv->create_update_popup, EEXT_CALLBACK_BACK, __vcui_endcall_popup_back_cb, ad);

	elm_popup_align_set(priv->create_update_popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	elm_object_part_text_set(priv->create_update_popup, "title,text",  priv->call_number);

	Evas_Object *genlist = elm_genlist_add(priv->create_update_popup);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	CALLUI_RETURN_IF_FAIL(itc);
	itc->item_style = "type1";
	itc->func.text_get = __vcui_endcall_get_item_text;

	elm_genlist_item_append(genlist, itc, _("IDS_COM_OPT_CREATE_CONTACT"), NULL, ELM_GENLIST_ITEM_NONE, __vcui_endcall_create_contact_btn_cb, priv->call_number);
	elm_genlist_item_append(genlist, itc, "Update contact", NULL, ELM_GENLIST_ITEM_NONE, __vcui_endcall_update_contact_btn_cb,  priv->call_number);

	elm_genlist_item_class_free(itc);
	elm_object_content_set(priv->create_update_popup, genlist);
	elm_popup_orient_set(priv->create_update_popup, ELM_POPUP_ORIENT_CENTER);
	evas_object_show(priv->create_update_popup);
}

static void __vcui_endcall_msg_btn_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP, __vcui_endcall_msg_btn_cb);
	char *number = (char *)data;
	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_COMPOSE);
	char str[CONTACT_NUMBER_BUF_LEN];
	snprintf(str, sizeof(str), "sms:%s", number);
	app_control_set_uri(request, str);
	int result = app_control_send_launch_request(request, NULL, NULL);
	if (result != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() failed (%d)", result);
	}
	app_control_destroy(request);
}

static void __callui_view_callend_create_screen(callui_app_data_t *ad, Evas_Object *eo, void *data)
{
	dbg("..");

	call_view_data_t *vd = (call_view_data_t *)data;
	CALLUI_RETURN_IF_FAIL(vd);
	callui_endcall_view_priv_t *priv = (callui_endcall_view_priv_t *)vd->priv;
	CALLUI_RETURN_IF_FAIL(priv);
	call_data_t *call_data = NULL;

	if (ad->active) {
		call_data = ad->active;
	} else {
		call_data = ad->held;
	}
	CALLUI_RETURN_IF_FAIL(call_data);

	char *file_path = call_data->call_ct_info.caller_id_path;
	char *call_name = call_data->call_ct_info.call_disp_name;

	if (call_data->call_disp_num[0] != '\0') {
		strncpy(priv->call_number, call_data->call_disp_num, sizeof(priv->call_number));
	} else {
		strncpy(priv->call_number, call_data->call_num, sizeof(priv->call_number));
	}
	priv->contact_person_id = call_data->call_ct_info.person_id;

	Evas_Object *button_call_back = elm_layout_add(priv->contents);
	if (button_call_back) {
		elm_layout_file_set(button_call_back, EDJ_NAME, "call_back");
		evas_object_event_callback_add(button_call_back, EVAS_CALLBACK_MOUSE_UP,  __callui_endcall_voicecall_btn_cb, (void *)priv->call_number);
		elm_object_part_text_set(button_call_back, "end_btn_text", _("IDS_CALL_BUTTON_CALL"));
	}
	elm_object_part_content_set(priv->contents, "button_call_back", button_call_back);

	Evas_Object *button_message = elm_layout_add(priv->contents);
	if (button_message) {
		elm_layout_file_set(button_message, EDJ_NAME, "message_button");
		evas_object_event_callback_add(button_message, EVAS_CALLBACK_MOUSE_UP, __vcui_endcall_msg_btn_cb, (void *)priv->call_number);
		elm_object_part_text_set(button_message, "end_btn_text", _("IDS_COM_BODY_MESSAGE"));
	}
	elm_object_part_content_set(priv->contents, "button_message_back", button_message);
	elm_object_part_text_set(priv->contents, "main_title_status", _("IDS_CALL_BODY_CALL_ENDE_M_STATUS_ABB"));

	if (!(call_name && call_name[0] != '\0')) {
		Evas_Object *button_create = elm_layout_add(priv->contents);
		if (button_create) {
			elm_layout_file_set(button_create, EDJ_NAME, "create_contact_button");
			evas_object_event_callback_add(button_create, EVAS_CALLBACK_MOUSE_UP, __vcui_endcall_create_popup_cb, (void *)priv);
			elm_object_part_content_set(priv->contents, "swallow.create_contact", button_create);
		}
		elm_object_part_text_set(priv->contents, "contact_name", priv->call_number);
		elm_object_part_text_set(priv->contents, "contact_number", _("IDS_COM_OPT_ADD_TO_CONTACTS"));
	} else {
		elm_object_part_text_set(priv->contents, "contact_name", call_name);
		elm_object_part_text_set(priv->contents, "contact_number", priv->call_number);
	}

	if (strcmp(file_path, "default") != 0) {
		_callui_show_caller_id(priv->contents, file_path);
	} else {
		elm_object_signal_emit(priv->contents, "show_image", "main_end_call");
	}
	_callui_common_create_ending_timer(vd);
}

static Evas_Object *__callui_view_callend_create_contents(void *data, char *grp_name)
{
	if (data == NULL) {
		err("ERROR");
		return NULL;
	}
	callui_app_data_t *ad = (callui_app_data_t *)data;
	Evas_Object *eo = NULL;

	eo = _callui_load_edj(ad->main_ly, EDJ_NAME, grp_name);

	if (eo == NULL)
		return NULL;

	return eo;
}

static int __callui_view_callend_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *data)
{
	dbg("endcall view create");
	call_view_data_t *vd = view_data;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	callui_endcall_view_priv_t *priv = (callui_endcall_view_priv_t *)vd->priv;
	call_data_t *call_data = NULL;

	priv->bshowbutton = FALSE;	/* Init */

	if (ad->active) {
		call_data = ad->active;
	} else {
		call_data = ad->held;
	}
	CALLUI_RETURN_VALUE_IF_FAIL(call_data, -1);

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

	g_strlcpy(priv->caller_id_path, call_data->call_ct_info.caller_id_path, sizeof(priv->caller_id_path));
	if (ad->main_ly) {
		priv->contents = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
		if (!priv->contents) {
			priv->contents = __callui_view_callend_create_contents(ad, "main_end_call");
			_callui_set_object_data(priv->contents, CALLUI_END_TYPE_KEY, CALLUI_END_TYPE_SINGLE_CALL_END);
			elm_object_part_content_set(ad->main_ly, "elm.swallow.content", priv->contents);
			evas_object_name_set(priv->contents, CALLUI_VIEW_CALLEND_LAYOUT_ID);
		}

		__callui_view_callend_onshow(view_data, ad);
	}
	return 0;
}

static int __callui_view_callend_onupdate(call_view_data_t *view_data, void *update_data)
{
	dbg("end call view update");
	return 0;
}

static int __callui_view_callend_onhide(call_view_data_t *view_data)
{
	dbg("end call view hide");
	callui_app_data_t *ad = _callui_get_app_data();

	evas_object_hide(ad->main_ly);
	return 0;
}

static int __callui_view_callend_onshow(call_view_data_t *view_data, void *appdata)
{
	dbg("end call view show");
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	callui_endcall_view_priv_t *priv = (callui_endcall_view_priv_t *)view_data->priv;

	__callui_view_callend_create_screen(ad, priv->contents, view_data);

	_callui_common_win_set_noti_type(ad, EINA_TRUE);
	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);
	return 0;
}

static int __callui_view_callend_ondestroy(call_view_data_t *view_data)
{
	dbg("endcall view destroy");
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_VALUE_IF_FAIL(ad, -1);
	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_ENDCALL_VIEW);
	CALLUI_RETURN_VALUE_IF_FAIL(vd, -1);
	callui_endcall_view_priv_t *priv = (callui_endcall_view_priv_t *)vd->priv;

	_callui_common_delete_ending_timer();

	if (priv != NULL) {
		if (priv->contents) {
			evas_object_del(priv->contents);
			priv->contents = NULL;
		}

		if (priv->caller_info) {
			evas_object_del(priv->caller_info);
			priv->caller_info = NULL;
		}

		if (priv->caller_id) {
			evas_object_del(priv->caller_id);
			priv->caller_id = NULL;
		}

		if (priv->create_update_popup) {
			dbg("create_update_popup");
			evas_object_del(priv->create_update_popup);
		}
		free(priv);
		priv = NULL;
	}

	_callvm_reset_call_view_data(ad, VIEW_ENDCALL_VIEW);

	dbg("complete destroy one view");
	return 0;
}

static int __callui_view_callend_onrotate(call_view_data_t *view_data)
{
	dbg("*** Call End view Rotate ***");
	return 0;
}

Evas_Object *_callui_view_callend_get_layout(call_view_data_t *vd)
{
	dbg("..");
	callui_endcall_view_priv_t *priv = NULL;

	CALLUI_RETURN_VALUE_IF_FAIL(vd, NULL);
	priv = (callui_endcall_view_priv_t *) vd->priv;

	CALLUI_RETURN_VALUE_IF_FAIL(priv, NULL);
	return priv->contents;
}

