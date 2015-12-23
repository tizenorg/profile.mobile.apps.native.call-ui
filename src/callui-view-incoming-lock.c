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

#include <msg.h>
#include <msg_transport.h>
#include <dlfcn.h>
#include <vconf.h>
#include <notification.h>
#include <Elementary.h>
#include <tzplatform_config.h>

#include "callui.h"
#include "callui-view-layout-wvga.h"
#include "callui-view-elements.h"
#include "callui-view-active-incoming-call.h"
#include "callui-view-incoming-call.h"
#include "callui-view-incoming-lock.h"
#include "callui-common.h"
#define REJ_MSG_LIST_OPEN_STATUS_KEY "list_open_status_key"
#define CALLUI_CST_SO_PATH	tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.call-setting/lib/ug/libcall-setting.so")
#define CALLUI_CST_REJECT_MSG_GET	"cst_reject_msg_get"
#define REJ_MSG_GENLIST_DATA "reject_msg_genlist_data"

static int __callui_view_incoming_lock_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *appdata);
static int __callui_view_incoming_lock_onupdate(call_view_data_t *view_data, void *update_data1);
static int __callui_view_incoming_lock_onhide(call_view_data_t *view_data);
static int __callui_view_incoming_lock_onshow(call_view_data_t *view_data, void *appdata);
static int __callui_view_incoming_lock_ondestroy(call_view_data_t *view_data);
static int __callui_view_incoming_lock_onrotate(call_view_data_t *view_data);

call_view_data_t *_callui_view_incoming_lock_new(callui_app_data_t *ad)
{
	dbg("_callui_view_incoming_lock_new");
	static call_view_data_t incoming_lock_view = {
		.type = VIEW_INCOMING_LOCK_VIEW,
		.layout = NULL,
		.onCreate = __callui_view_incoming_lock_oncreate,
		.onUpdate = __callui_view_incoming_lock_onupdate,
		.onHide = __callui_view_incoming_lock_onhide,
		.onShow = __callui_view_incoming_lock_onshow,
		.onDestroy = __callui_view_incoming_lock_ondestroy,
		.onRotate = __callui_view_incoming_lock_onrotate,
		.priv = NULL,
	};
	incoming_lock_view.priv = calloc(1, sizeof(incoming_lock_view_priv_t));

	if (!incoming_lock_view.priv) {
		err("ERROR!!!!!!!!!!!");
	}

	return &incoming_lock_view;
}

void _callui_view_incoming_lock_reject_msg_create_call_setting_handle(void *data)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	char *error = NULL;

	priv->dl_handle = dlopen(CALLUI_CST_SO_PATH, RTLD_LAZY);
	if (priv->dl_handle) {
		priv->msg_func_ptr = dlsym(priv->dl_handle, CALLUI_CST_REJECT_MSG_GET);
		if ((error = dlerror()) != NULL) {
			dbg("dlsym failed!!! error = %s", error);
			priv->msg_func_ptr = NULL;
			dlclose(priv->dl_handle);
			priv->dl_handle = NULL;
		}
	} else {
		dbg("failed to open libcall-setting.so");
	}
}

static void __send_reject_msg_status_cb(msg_handle_t Handle, msg_struct_t pStatus, void *pUserParam)
{
	CALLUI_RETURN_IF_FAIL(pStatus != NULL);
	int status = MSG_NETWORK_SEND_FAIL;

	msg_get_int_value(pStatus, MSG_SENT_STATUS_NETWORK_STATUS_INT, &status);
	dbg("status:[%d]", status);
}

void _callui_view_incoming_lock_view_send_reject_msg(void *data, call_data_t *call_data)
{
	CALLUI_RETURN_IF_FAIL(data != NULL);
	CALLUI_RETURN_IF_FAIL(call_data != NULL);
	dbg("..");
	call_view_data_t *vd = data;
	callui_app_data_t *ad = _callui_get_app_data();
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	int slot_id;

	msg_error_t err = MSG_SUCCESS;
	msg_handle_t msgHandle = NULL;

	if (strlen(priv->reject_msg) == 0) {
		warn("Is not reject with message case");
		return;
	}

	err = msg_open_msg_handle(&msgHandle);
	if (err != MSG_SUCCESS) {
		dbg("msg_open_msg_handle()- failed [%d]", err);
		return;
	}

	err = msg_reg_sent_status_callback(msgHandle, &__send_reject_msg_status_cb, NULL);
	if (err != MSG_SUCCESS) {
		dbg("msg_reg_sent_status_callback()- failed [%d]", err);
		msg_close_msg_handle(&msgHandle);
		return;
	}
	msg_struct_t msgInfo = msg_create_struct(MSG_STRUCT_MESSAGE_INFO);
	msg_struct_t sendOpt = msg_create_struct(MSG_STRUCT_SENDOPT);
	msg_struct_t pReq = msg_create_struct(MSG_STRUCT_REQUEST_INFO);

	/* Set message type to SMS reject*/
	msg_set_int_value(msgInfo, MSG_MESSAGE_TYPE_INT, MSG_TYPE_SMS_REJECT);

	slot_id = ad->sim_slot;
	dbg("msg_sms_send_message() Sim slot [%d]", slot_id);
	if (slot_id != -1) {
		slot_id++;
		msg_set_int_value(msgInfo, MSG_MESSAGE_SIM_INDEX_INT, slot_id);
	}

	/* No setting send option */
	msg_set_bool_value(sendOpt, MSG_SEND_OPT_SETTING_BOOL, FALSE);

	/* Set message body */
	if (msg_set_str_value(msgInfo, MSG_MESSAGE_SMS_DATA_STR, priv->reject_msg, strlen(priv->reject_msg)) != MSG_SUCCESS) {
		err("msg_set_str_value() - failed");
	} else {
		/* Create address list*/
		msg_struct_list_s *addr_list;
		msg_get_list_handle(msgInfo, MSG_MESSAGE_ADDR_LIST_STRUCT, (void **)&addr_list);
		msg_struct_t addr_info = addr_list->msg_struct_info[0];
		char *call_number = call_data->call_num;

		/* Set message address */
		msg_set_int_value(addr_info, MSG_ADDRESS_INFO_RECIPIENT_TYPE_INT, MSG_RECIPIENTS_TYPE_TO);
		msg_set_str_value(addr_info, MSG_ADDRESS_INFO_ADDRESS_VALUE_STR, call_number, strlen(call_number));
		addr_list->nCount = 1;

		/* Set message struct to Request*/
		msg_set_struct_handle(pReq, MSG_REQUEST_MESSAGE_HND, msgInfo);
		msg_set_struct_handle(pReq, MSG_REQUEST_SENDOPT_HND, sendOpt);

		/* Send message */
		err = msg_sms_send_message(msgHandle, pReq);
		if (err != MSG_SUCCESS) {
			err("msg_sms_send_message() - failed [%d]", err);
		} else {
			dbg("Sending...");
		}
	}
	msg_close_msg_handle(&msgHandle);
	msg_release_struct(&pReq);
	msg_release_struct(&msgInfo);
	msg_release_struct(&sendOpt);

	return;
}

static int __callui_view_incoming_lock_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *appdata)
{
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	if (_callui_common_get_idle_lock_type() == LOCK_TYPE_UNLOCK && ad->held == NULL
			&& ad->active == NULL && ad->incom != NULL && ad->active_incoming == false) {

		// TODO commented the active notification, because cannot change window size
		//_callui_view_active_incoming_call_oncreate(view_data, appdata);

		_callui_view_incoming_call_oncreate(view_data, appdata);
	} else {
		_callui_view_incoming_call_oncreate(view_data, appdata);
	}
	__callui_view_incoming_lock_onshow(view_data, ad);
	_callui_common_win_set_noti_type(ad, EINA_TRUE);
	return 0;
}

static int __callui_view_incoming_lock_onupdate(call_view_data_t *view_data, void *update_data1)
{
	dbg("mt-lock view update!!");
	callui_app_data_t *ad = _callui_get_app_data();
	__callui_view_incoming_lock_onshow(view_data, ad);
	return 0;
}

static int __callui_view_incoming_lock_onhide(call_view_data_t *view_data)
{
	dbg("mt-lock view hide!!");
	callui_app_data_t *ad = _callui_get_app_data();

	evas_object_hide(ad->main_ly);
	return 0;
}

static int __callui_view_incoming_lock_onshow(call_view_data_t *view_data, void *appdata)
{
	dbg("mt-lock view show!!");
	callui_app_data_t *ad = (callui_app_data_t *) appdata;
	int result = 0;
	if (_callui_common_get_idle_lock_type() == LOCK_TYPE_UNLOCK && ad->active == NULL
			&& ad->held == NULL && ad->incom != NULL && ad->active_incoming == false) {

		// TODO commented the active notification, because cannot change window size
		/* create active incoming call */
		//_callui_view_active_incoming_call_draw_screen(ad, view_data);

		_callui_view_incoming_call_draw_screen(ad, view_data);
	} else {
		/* create incoming call */
		_callui_view_incoming_call_draw_screen(ad, view_data);
	}

	ad->active_incoming = true;
	evas_object_show(ad->main_ly);
	result = elm_win_keygrab_set(ad->win, CALLUI_KEY_SELECT, 0, 0, 0, ELM_WIN_KEYGRAB_TOPMOST);
	if (result) {
		dbg("KEY_SELECT key grab failed");
	}

	return 0;
}

static char *__reject_list_get_msg_by_index(void *data, int index, Eina_Bool b_parsing)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;

	char *ret_str = NULL;

	if (priv->msg_func_ptr) {
		ret_str = (char *) priv->msg_func_ptr(index, b_parsing);  /* i : reject msg index(0 ~ 4)*/
		sec_dbg("b_parsing(%d),ret_str(%s)", b_parsing, ret_str);
	}

	return ret_str;
}

static char *__callui_view_incoming_lock_reject_msg_gl_label_get_msg(void *data, Evas_Object *obj, const char *part)
{
	CALLUI_RETURN_NULL_IF_FAIL(part);
	call_view_data_t *vd = (call_view_data_t *)evas_object_data_get(obj, REJ_MSG_GENLIST_DATA);
	CALLUI_RETURN_VALUE_IF_FAIL(vd, NULL);

	int index = (int)data;
	char *msg_str = NULL;

	if (!strcmp(part, "elm.text")) {
		if (index != -1) {
			msg_str = __reject_list_get_msg_by_index(vd, index, EINA_TRUE);
			sec_dbg("msg_str(%s)", msg_str);
			return msg_str; /*Send markup text to draw the genlist */
		} else {
			warn("invalid index: %d", index);
			msg_str = _("IDS_CALL_BODY_NO_MESSAGES");
			return strdup(msg_str);
		}
	}
	return NULL;
}

static void __reject_msg_gl_sel_msg(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	int index = (int)data;
	int ret = -1;
	dbg("index: %d", index);
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);

	if (index != -1) {
		char *reject_msg = NULL;
		char *ret_str = NULL;
		call_view_data_t *vd = (call_view_data_t *)evas_object_data_get(obj, REJ_MSG_GENLIST_DATA);
		CALLUI_RETURN_IF_FAIL(vd);
		incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
		CALLUI_RETURN_IF_FAIL(priv);

		ret_str =  __reject_list_get_msg_by_index(vd, index, EINA_TRUE);
		if (ret_str) {
			reject_msg = elm_entry_markup_to_utf8(ret_str); /*send utf8 text to MSG */
			if (reject_msg != NULL) {
				snprintf(priv->reject_msg, sizeof(priv->reject_msg), "%s", reject_msg);
				sec_dbg("priv->reject_msg(%s)", priv->reject_msg);
				free(reject_msg);
			}
			free(ret_str);
		}

		ret = cm_reject_call(ad->cm_handle);
		if (ret != CM_ERROR_NONE) {
			err("cm_reject_call() is failed");
		} else {
			_callui_view_incoming_lock_view_send_reject_msg(vd, ad->incom);
		}
	}

	return;
}

Elm_Object_Item *_callui_view_incoming_lock_append_genlist_item(Evas_Object *msg_glist, Elm_Genlist_Item_Class * itc_reject_msg, int index)
{
	Elm_Object_Item *item = NULL;
	item = elm_genlist_item_append(msg_glist, itc_reject_msg, (void *)index, NULL, ELM_GENLIST_ITEM_NONE, __reject_msg_gl_sel_msg, (void *)index);
	return item;
}

Elm_Genlist_Item_Class *_callui_view_incoming_lock_create_item_class()
{
	Elm_Genlist_Item_Class *item_class = elm_genlist_item_class_new();
	CALLUI_RETURN_VALUE_IF_FAIL(item_class, NULL);

	item_class->item_style = "type1";
	item_class->func.text_get = __callui_view_incoming_lock_reject_msg_gl_label_get_msg;
	item_class->func.content_get = NULL;
	item_class->func.state_get = NULL;
	item_class->func.del = NULL;
	return item_class;
}

static void __callui_view_incoming_lock_create_new_msg_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data != NULL);
	callui_app_data_t *ad = _callui_get_app_data();
	char *tel_number = NULL;
	int ret = -1;
	call_data_t *call_data = NULL;

	call_data = ad->incom;
	if (call_data != NULL) {
		tel_number = call_data->call_num;
		_callui_common_launch_msg_composer(ad, tel_number);
		ret = cm_reject_call(ad->cm_handle);
		if (ret != CM_ERROR_NONE) {
			err("cm_reject_call() is failed");
		}
	}
}

void _callui_view_incoming_lock_create_reject_msg_button(Evas_Object *parent, char *part, void *data)
{
	Evas_Object *msg_button = elm_button_add(parent);
	CALLUI_RETURN_IF_FAIL(msg_button);
	elm_object_style_set(msg_button, "default");
	elm_object_text_set(msg_button,  _("IDS_CALL_BUTTON_COMPOSE_MESSAGE_TO_SEND_ABB"));
	evas_object_smart_callback_add(msg_button, "clicked",  __callui_view_incoming_lock_create_new_msg_cb, data);
	elm_object_part_content_set(parent, part,  msg_button);
	evas_object_show(msg_button);
}

static int __callui_view_incoming_lock_ondestroy(call_view_data_t *vd)
{
	dbg("mt-lock view destroy!!");
	callui_app_data_t *ad = _callui_get_app_data();
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;

	/* Set LCD timeout for call state */
	/* LCD is alwasy on during incoming call screen */
	if (ad->speaker_status == EINA_TRUE) {
		_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_SET);
	}
	_callui_common_win_set_noti_type(ad, EINA_FALSE);
	if (priv != NULL) {
		if (priv->lock_accept) {
			evas_object_del(priv->lock_accept);
			priv->lock_accept = NULL;
		}
		if (priv->lock_reject) {
			evas_object_del(priv->lock_reject);
			priv->lock_reject = NULL;
		}

		if (ad->second_call_popup) {
			evas_object_del(ad->second_call_popup);
			ad->second_call_popup = NULL;
		}

		if (priv->itc_reject_msg) {
			priv->itc_reject_msg->func.text_get = NULL;
			priv->itc_reject_msg->func.content_get = NULL;
			priv->itc_reject_msg->func.state_get = NULL;
			priv->itc_reject_msg->func.del = NULL;
			elm_genlist_item_class_free(priv->itc_reject_msg);
			priv->itc_reject_msg = NULL;
		}

		if (priv->msg_glist) {
			elm_genlist_clear(priv->msg_glist);
			evas_object_del(priv->msg_glist);
			priv->msg_glist = NULL;
		}

		if (priv->reject_msg_gl) {
			evas_object_del(priv->reject_msg_gl);
			priv->reject_msg_gl = NULL;
		}

		_callui_view_incoming_call_ondestroy(priv);

		elm_object_signal_emit(priv->contents, "mt_circle_bg_hide", "mt_view");
		_callui_common_reset_main_ly_text_fields(priv->contents);

		if (ad->main_ly) {
			priv->contents = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
			if (priv->contents) {
				evas_object_del(priv->contents);
				priv->contents = NULL;
			}
		}

		if (priv->dimming_ly) {
			evas_object_del(priv->dimming_ly);
			priv->dimming_ly = NULL;
		}

		if (ad->win) {
			evas_object_resize(ad->win, ad->root_w, ad->root_h);
		}

		free(priv);
		priv = NULL;
	}
	elm_win_keygrab_unset(ad->win, CALLUI_KEY_SELECT, 0, 0);

	return 0;
}

static int __callui_view_incoming_lock_onrotate(call_view_data_t *view_data)
{
	dbg("*** Incoming view Rotate ***");

	return 0;
}

Evas_Object *_callui_view_incoming_lock_get_accept_layout(call_view_data_t *vd)
{
	incoming_lock_view_priv_t *priv = NULL;

	CALLUI_RETURN_VALUE_IF_FAIL(vd, NULL);
	priv = (incoming_lock_view_priv_t *) vd->priv;

	CALLUI_RETURN_VALUE_IF_FAIL(priv, NULL);
	return priv->lock_accept;
}

void _callui_view_incoming_lock_set_accept_layout(call_view_data_t *vd, Evas_Object *layout)
{
	incoming_lock_view_priv_t *priv = NULL;

	CALLUI_RETURN_IF_FAIL(vd);
	priv = (incoming_lock_view_priv_t *) vd->priv;

	CALLUI_RETURN_IF_FAIL(priv);
	priv->lock_accept = layout;
}

Evas_Object *_callui_view_incoming_lock_get_reject_layout(call_view_data_t *vd)
{
	incoming_lock_view_priv_t *priv = NULL;

	CALLUI_RETURN_VALUE_IF_FAIL(vd, NULL);
	priv = (incoming_lock_view_priv_t *) vd->priv;

	CALLUI_RETURN_VALUE_IF_FAIL(priv, NULL);
	return priv->lock_reject;
}

void _callui_view_incoming_lock_set_reject_layout(call_view_data_t *vd, Evas_Object *layout)
{
	incoming_lock_view_priv_t *priv = NULL;

	CALLUI_RETURN_IF_FAIL(vd);
	priv = (incoming_lock_view_priv_t *) vd->priv;

	CALLUI_RETURN_IF_FAIL(priv);
	priv->lock_reject = layout;
}
