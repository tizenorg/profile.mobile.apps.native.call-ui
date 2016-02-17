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

#include <vconf.h>
#include <vconf-keys.h>
#include <bluetooth.h>

#include "callui-view-elements.h"
#include "callui-view-manager.h"
#include "callui-view-dialing.h"
#include "callui-view-single-call.h"
#include "callui-view-callend.h"
#include "callui-view-incoming-lock.h"
#include "callui-view-multi-call-split.h"
#include "callui-view-layout.h"
#include "callui-common.h"
#include "callui-keypad.h"
#include "callui-view-multi-call-conf.h"
#include "callui-view-incoming-lock.h"
#include "callui-view-quickpanel.h"
#include "callui-view-caller-info-defines.h"
#include "callui-proximity-lock-manager.h"
#include <app_control.h>
#include <notification.h>

#define	POPUP_LIST_W		300
#define	POPUP_LIST_ITEM_H 	120
#define APP_CONTROL_MIME_CONTACT "application/vnd.tizen.contact"
#define CONTACT_ID_BUF_LEN 16
#define CONTACT_NUMBER_BUF_LEN 32

typedef struct {
	int index;
	char option_msg[512];
} second_call_popup_data_t;

static void __vcui_msg_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void __vcui_view_contact_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void __vcui_update_existing_contact_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void __vcui_create_contact_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void __callui_unload_more_option(callui_app_data_t *ad);

const char *group_thumbnail[] = {
	GROUP_THUMBNAIL_98,
	GROUP_THUMBNAIL_138,
	"",
};

const char *group_default_thumbnail[] = {
	GROUP_DEFAULT_THUMBNAIL_98,
	GROUP_DEFAULT_THUMBNAIL_138,
	GROUP_DEFAULT_CONFERENCE_THUMBNAIL_138,
};

const int thumbnail_size[] = {
	98,
	138,
	138,
};

Evas_Object *_callui_load_edj(Evas_Object *parent, const char *file, const char *group)
{
	Evas_Object *eo;
	int ret;

	eo = elm_layout_add(parent);
	if (eo) {
		ret = elm_layout_file_set(eo, file, group);
		if (!ret) {
			evas_object_del(eo);
			err("ERROR!!");
			return NULL;
		}

		evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	}

	return eo;
}

Evas_Object *_callui_edje_object_part_get(Evas_Object *parent, const char *part)
{
	Evas_Object *obj = NULL;

	obj = (Evas_Object *)edje_object_part_object_get(elm_layout_edje_get(parent), part);
	if (obj == NULL) {
		err("(%s) part is not avialable", part);
	}

	return obj;
}

static Evas_Object *__callui_create_caller_info(void *data)
{
	Evas_Object *caller_info = NULL;
	Evas_Object *layout = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_VALUE_IF_FAIL(ad, NULL);

	layout = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
	CALLUI_RETURN_VALUE_IF_FAIL(layout, NULL);
	caller_info = elm_object_part_content_get(layout, "caller_info");

	return caller_info;
}

static Evas_Object *__callui_create_button_style(void *data, Evas_Object **p_button, char *part_name)
{
	Evas_Object *layout = NULL;
	Evas_Object *btn_ly = NULL;
	Evas_Object *sw = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_VALUE_IF_FAIL(ad, NULL);
	layout = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
	CALLUI_RETURN_VALUE_IF_FAIL(layout, NULL);
	btn_ly = elm_object_part_content_get(layout, "btn_region");
	CALLUI_RETURN_VALUE_IF_FAIL(btn_ly, NULL);

	sw = edje_object_part_swallow_get(_EDJ(btn_ly), part_name);
	if (sw) {
		dbg("Object Already Exists, so Update Only");
		*p_button = sw;
	} else {
		*p_button = elm_button_add(btn_ly);
		elm_object_part_content_set(btn_ly, part_name, *p_button);
	}

	return btn_ly;
}

void _callui_spk_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(data != NULL);
	callui_app_data_t *ad = (callui_app_data_t *)data;
	int ret = -1;

	if (ad->speaker_status == EINA_TRUE) {
		ret = cm_speaker_off(ad->cm_handle);
		if (ret != CM_ERROR_NONE) {
			err("cm_speaker_off() is failed. ret[%d]", ret);
			return;
		}
	} else {
		ret = cm_speaker_on(ad->cm_handle);
		if (ret != CM_ERROR_NONE) {
			err("cm_speaker_on() is failed. ret[%d]", ret);
			return;
		}
	}
	_callui_update_speaker_btn(ad, !ad->speaker_status);

	return;
}

static void __callui_contacts_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	__callui_unload_more_option(ad);
	_callui_common_launch_contacts(ad);
}

void _callui_update_speaker_btn(callui_app_data_t *ad, Eina_Bool is_on)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(ad);

	ad->speaker_status = is_on;

	/* Update Buttons */
	_callui_create_top_first_button(ad);
	_callui_qp_mc_update_speaker_status(ad->qp_minicontrol, EINA_FALSE);

	return;
}

void _callui_update_headset_btn(callui_app_data_t *ad, Eina_Bool is_on)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(ad);
	ad->headset_status = is_on;

	_callui_create_top_third_button(ad);

	return;
}

void _callui_update_mute_btn(callui_app_data_t *ad, Eina_Bool is_on)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(ad != NULL);

	ad->mute_status = is_on;
	_callui_create_bottom_second_button(ad);
	_callui_qp_mc_update_mute_status(ad->qp_minicontrol, EINA_FALSE);

	return;
}

void _callui_update_extra_vol_btn(callui_app_data_t *ad, Eina_Bool is_on)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(ad != NULL);

	ad->extra_volume_status = is_on;
	_callui_create_bottom_third_button(ad);

	return;
}

void _callui_mute_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad != NULL);
	int ret = -1;
	gboolean is_mute_on = FALSE;

	if (ad->mute_status == EINA_FALSE) {
		is_mute_on = TRUE;
	} else {
		is_mute_on = FALSE;
	}

	ret = cm_set_mute_state(ad->cm_handle, is_mute_on);
	if (ret != CM_ERROR_NONE) {
		err("cm_set_mute_state() get failed with err[%d]", ret);
		return;
	}

	_callui_update_mute_btn(ad, is_mute_on);
	return;
}

static void __callui_headset_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad != NULL);
	Eina_Bool bupdate_btn = EINA_FALSE;

	if (ad->headset_status == EINA_TRUE) {
		ad->headset_status = EINA_FALSE;
		ad->speaker_status = EINA_FALSE;
		cm_bluetooth_off(ad->cm_handle);
		bupdate_btn = EINA_TRUE;
	} else {
		/* Check If SCO path exists, open if exists*/
		if (EINA_TRUE == _callui_common_is_headset_conected()) {
			ad->headset_status = EINA_TRUE;
			ad->speaker_status = EINA_FALSE;
			cm_bluetooth_on(ad->cm_handle);
			bupdate_btn = EINA_TRUE;
		} else {
			bt_adapter_state_e bt_state = BT_ADAPTER_DISABLED;
			int ret_code = bt_adapter_get_state(&bt_state);
			if (ret_code == BT_ERROR_NONE) {
				info("BT status value: %d", bt_state);
				if (bt_state == BT_ADAPTER_DISABLED) {
					_callui_load_bluetooth_popup(ad);
				} else {
					_callui_common_launch_bt_app(ad);
				}
			} else {
				err("fail to get vconf key: %d", ret_code);
			}
			bupdate_btn = EINA_FALSE;
		}
	}

	if (bupdate_btn) {
		if (ad->extra_volume_status == EINA_TRUE) {
			int ret = -1;
			ret = cm_set_extra_vol(ad->cm_handle, EINA_FALSE);
			if (ret != CM_ERROR_NONE) {
				err("Fail cm_set_extra_vol");
			}
			ad->extra_volume_status = EINA_FALSE;
		}
		/* Update Headset/Speaker buttons */
		_callui_create_top_first_button(ad);
		_callui_create_top_third_button(ad);
		_callui_create_bottom_third_button(ad);
	}
}

static void __callui_addcall_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad != NULL);

	_callui_common_launch_dialer(ad);
}

static void __callui_keypad_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;

	if (_callui_keypad_get_show_status() == EINA_FALSE) {	/*show keypad region*/
		dbg("..");
		/*Actual show with animation*/
		_callui_keypad_show_layout(ad);
	} else {
		dbg("..");
		/*Hide animation on keypad*/
		_callui_keypad_hide_layout(ad);
	}
}

/* Speaker Button ENABLED */
Evas_Object *_callui_create_top_first_button(callui_app_data_t *ad)
{
	Evas_Object *btn = NULL;
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);
	__callui_create_button_style(ad, &btn, PART_TOP_FIRST_BTN);

	evas_object_smart_callback_del(btn, "clicked", _callui_spk_btn_cb);
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_SPEAKER"));
	evas_object_smart_callback_add(btn, "clicked", _callui_spk_btn_cb, ad);
	if (ad->speaker_status == EINA_FALSE) {
		elm_object_style_set(btn, "style_call_sixbtn_speaker");
	} else {
		elm_object_style_set(btn, "style_call_sixbtn_speaker_on");
	}

	elm_object_disabled_set(btn, EINA_FALSE);

	return btn;
}

/* Speaker Button DISABLED */
Evas_Object *_callui_create_top_first_button_disabled(callui_app_data_t *ad)
{
	Evas_Object *btn = NULL;
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);

	__callui_create_button_style(ad, &btn, PART_TOP_FIRST_BTN);
	elm_object_style_set(btn, "style_call_sixbtn_disabled_speaker");

	elm_object_text_set(btn, _("IDS_CALL_BUTTON_SPEAKER"));
	elm_object_disabled_set(btn, EINA_TRUE);

	return btn;
}

/* Keypad Button ENABLED */
Evas_Object *_callui_create_top_second_button(callui_app_data_t *ad)
{
	Evas_Object *btn = NULL;
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);

	__callui_create_button_style(ad, &btn, PART_TOP_SECOND_BTN);
	elm_object_style_set(btn, "style_call_sixbtn_keypad");

	evas_object_smart_callback_del(btn, "clicked", __callui_keypad_btn_cb);
	evas_object_smart_callback_add(btn, "clicked", __callui_keypad_btn_cb, ad);
	elm_object_text_set(btn, _("IDS_CALL_SK3_KEYPAD")); /* ToDo: Handle Keypad/Hide layout and text changes */
	elm_object_disabled_set(btn, EINA_FALSE);

	return btn;
}

/* Keypad Button DISABLED */
Evas_Object *_callui_create_top_second_button_disabled(callui_app_data_t *ad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);
	Evas_Object *btn = NULL;

	__callui_create_button_style(ad, &btn, PART_TOP_SECOND_BTN);
	elm_object_style_set(btn, "style_call_sixbtn_disabled_keypad");

	elm_object_text_set(btn, _("IDS_CALL_SK3_KEYPAD"));
	elm_object_disabled_set(btn, EINA_TRUE);

	return btn;
}

/* HeadSet Button ENABLED */
Evas_Object *_callui_create_top_third_button(callui_app_data_t *ad)
{
	Evas_Object *btn = NULL;
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);

	__callui_create_button_style(ad, &btn, PART_TOP_THIRD_BTN);

	if (ad->headset_status == EINA_FALSE) {
		elm_object_style_set(btn, "style_call_sixbtn_headset");
	} else {
		elm_object_style_set(btn, "style_call_sixbtn_headset_on");
	}
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_BLUETOOTH_ABB"));
	evas_object_smart_callback_del(btn, "clicked", __callui_headset_btn_cb);
	evas_object_smart_callback_add(btn, "clicked", __callui_headset_btn_cb, ad);
	elm_object_disabled_set(btn, EINA_FALSE);

	return btn;
}

/* HeadSet Button DISABLED */
Evas_Object *_callui_create_top_third_button_disabled(callui_app_data_t *ad)
{
	Evas_Object *btn = NULL;
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);

	__callui_create_button_style(ad, &btn, PART_TOP_THIRD_BTN);

	elm_object_style_set(btn, "style_call_sixbtn_disabled_headset");
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_BLUETOOTH_ABB"));

	elm_object_disabled_set(btn, EINA_TRUE);

	return btn;
}

/* Add-Call/Join button ENABLED*/
Evas_Object *_callui_create_bottom_first_button(callui_app_data_t *ad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);
	Evas_Object *btn = NULL;

	__callui_create_button_style(ad, &btn, PART_BOTTOM_FIRST_BTN);
	evas_object_smart_callback_del(btn, "clicked", __callui_addcall_btn_cb);
	elm_object_style_set(btn, "style_call_sixbtn_add");
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_ADD_CALL"));
	evas_object_smart_callback_add(btn, "clicked", __callui_addcall_btn_cb, ad);

	elm_object_disabled_set(btn, EINA_FALSE);

	return btn;
}

/* Add-Call/Join Button DISABLED */
Evas_Object *_callui_create_bottom_first_button_disabled(callui_app_data_t *ad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);
	Evas_Object *btn = NULL;

	__callui_create_button_style(ad, &btn, PART_BOTTOM_FIRST_BTN);

	elm_object_style_set(btn, "style_call_sixbtn_disabled_add");
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_ADD_CALL"));

	elm_object_disabled_set(btn, EINA_TRUE);

	return btn;
}

/* Mute Button ENABLED */
Evas_Object *_callui_create_bottom_second_button(callui_app_data_t *ad)
{
	Evas_Object *btn = NULL;
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);

	__callui_create_button_style(ad, &btn, PART_BOTTOM_SECOND_BTN);
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_MUTE_ABB"));
	if (ad->mute_status == EINA_FALSE) {
		elm_object_style_set(btn, "style_call_sixbtn_mute");
	} else {
		elm_object_style_set(btn, "style_call_sixbtn_mute_on");
	}
	elm_object_disabled_set(btn, EINA_FALSE);

	evas_object_smart_callback_del(btn, "clicked", _callui_mute_btn_cb);
	evas_object_smart_callback_add(btn, "clicked", _callui_mute_btn_cb, ad);

	return btn;
}

/* Mute Button DISABLED */
Evas_Object *_callui_create_bottom_second_button_disabled(callui_app_data_t *ad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);
	Evas_Object *btn = NULL;

	__callui_create_button_style(ad, &btn, PART_BOTTOM_SECOND_BTN);
	elm_object_style_set(btn, "style_call_sixbtn_disabled_mute");
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_MUTE_ABB"));

	elm_object_disabled_set(btn, EINA_TRUE);

	return btn;
}

/* Contacts Button ENABLED */
Evas_Object *_callui_create_bottom_third_button(callui_app_data_t *ad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);
	Evas_Object *btn = NULL;
	__callui_create_button_style(ad, &btn, PART_BOTTOM_THIRD_BTN);
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_CONTACTS"));
	evas_object_smart_callback_del(btn, "clicked", __callui_contacts_btn_cb);
	evas_object_smart_callback_add(btn, "clicked", __callui_contacts_btn_cb, ad);
	elm_object_disabled_set(btn, EINA_FALSE);
	elm_object_style_set(btn, "style_call_sixbtn_contacts");

	return btn;
}

/* Contacts Button DISABLED */
Evas_Object *_callui_create_bottom_third_button_disabled(callui_app_data_t *ad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad != NULL, NULL);
	Evas_Object *btn = NULL;
#if 1
	__callui_create_button_style(ad, &btn, PART_BOTTOM_THIRD_BTN);
	elm_object_style_set(btn, "style_call_sixbtn_disabled_contacts");

	elm_object_text_set(btn, _("IDS_CALL_BUTTON_CONTACTS"));
	elm_object_disabled_set(btn, EINA_TRUE);

#else
	__callui_create_button_style(data, &btn, PART_BOTTOM_THIRD_BTN);
	elm_object_style_set(btn, "style_call_sixbtn_disabled_contacts");

	elm_object_disabled_set(btn, EINA_TRUE);

#endif
	return btn;
}

static void __callui_end_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	CALLUI_RETURN_IF_FAIL(vd != NULL);
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad != NULL);
	int ret = -1;

	dbg("vd->type:[%d]", vd->type);

	switch (vd->type) {
	case VIEW_TYPE_DIALLING:
		{
			if (ad->active)
				ret = cm_end_call(ad->cm_handle, ad->active->call_id, CALL_RELEASE_TYPE_BY_CALL_HANDLE);
		}
		break;
	case VIEW_TYPE_SINGLECALL:
		{
			ret = cm_end_call(ad->cm_handle, 0, CALL_RELEASE_TYPE_ALL_CALLS);
		}
		break;
	case VIEW_TYPE_MULTICALL_SPLIT:
		{
			ret = cm_end_call(ad->cm_handle, 0, CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS);
		}
		break;
	case VIEW_TYPE_MULTICALL_CONF:
	case VIEW_TYPE_MULTICALL_LIST:
		{
			ret = cm_end_call(ad->cm_handle, 0, CALL_RELEASE_TYPE_ALL_CALLS);
		}
		break;
	default:
		err("ERROR - wrong vd type:[%d]", vd->type);
		break;
	}

	if (ret != CM_ERROR_NONE) {
		err("cm_end_call() is failed");
	}
	return;
}

Evas_Object *_callui_create_end_call_button(Evas_Object *parent, void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(parent != NULL, NULL);

	Evas_Object *btn = elm_object_part_content_get(parent, PART_END_BTN);
	if (!btn) {
		btn = elm_button_add(parent);
		elm_object_style_set(btn, "call_icon_only");

		Evas_Object *icon = elm_image_add(btn);
		elm_image_file_set(icon, EDJ_NAME, "call_button_icon_01.png");
		elm_object_part_content_set(btn, "elm.swallow.content", icon);
		elm_object_part_content_set(parent, PART_END_BTN, btn);
	}
	evas_object_smart_callback_del(btn, "clicked", __callui_end_btn_cb);
	evas_object_smart_callback_add(btn, "clicked", __callui_end_btn_cb, data);
	evas_object_show(btn);

	return btn;
}

void _callui_destroy_end_call_button(Evas_Object *parent)
{
	CALLUI_RETURN_IF_FAIL(parent != NULL);
	Evas_Object *btn = NULL;

	btn = elm_object_part_content_get(parent, PART_END_BTN);
	if (btn) {
		evas_object_del(btn);
		btn = NULL;
	}
	return;
}

Evas_Object *_callui_create_voicecall_button_disabled(void *data)
{
	Evas_Object *btn = NULL;
	Evas_Object *layout = NULL;

	call_view_data_t *vd = (call_view_data_t *)data;

	CALLUI_RETURN_VALUE_IF_FAIL(vd != NULL, NULL);
	Evas_Object *icon = NULL;

	layout = _callui_view_callend_get_layout(vd);

	btn = elm_button_add(layout);
	elm_object_style_set(btn, "call_icon_only");
	icon = elm_image_add(btn);
	elm_image_file_set(icon, EDJ_NAME, "call_button_icon_03.png");
	elm_object_part_content_set(btn, "elm.swallow.content", icon);

	elm_object_part_content_set(layout, "three_btn_voicecall", btn);
	elm_object_disabled_set(btn, EINA_TRUE);
	evas_object_show(btn);

	return btn;
}

static void __callui_voicecall_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("__vcui_voicecall_btn_cb..");
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);
	char *call_number = (char *)data;
	CALLUI_RETURN_IF_FAIL(call_number);

	evas_object_smart_callback_del(obj, "clicked", __callui_voicecall_btn_cb);

	_callui_common_delete_ending_timer();
	ad->speaker_status = EINA_FALSE;
	ad->mute_status = EINA_FALSE;
	ad->extra_volume_status = EINA_FALSE;
	cm_dial_call(ad->cm_handle, call_number, CM_CALL_TYPE_VOICE, ad->sim_slot);
	g_free(call_number);
	return;
}

Evas_Object *_callui_create_voicecall_button(void *data, char *number)
{
	Evas_Object *btn = NULL;
	Evas_Object *layout = NULL;

	call_view_data_t *vd = NULL;

	CALLUI_RETURN_VALUE_IF_FAIL((vd = (call_view_data_t *)data) != NULL, NULL);
	Evas_Object *icon = NULL;
	layout = _callui_view_callend_get_layout(vd);

	btn = elm_button_add(layout);
	elm_object_style_set(btn, "call_icon_only");
	icon = elm_image_add(btn);
	elm_image_file_set(icon, EDJ_NAME, "call_button_icon_03.png");
	elm_object_part_content_set(btn, "elm.swallow.content", icon);

	elm_object_part_content_set(layout, "three_btn_voicecall", btn);

	evas_object_smart_callback_del(btn, "clicked", __callui_voicecall_btn_cb);
	evas_object_smart_callback_add(btn, "clicked", __callui_voicecall_btn_cb, g_strdup(number));

	evas_object_show(btn);
	return btn;
}

Evas_Object *_callui_create_message_button_disabled(void *data)
{
	Evas_Object *btn = NULL;
	Evas_Object *layout = NULL;

	call_view_data_t *vd = (call_view_data_t *)data;

	CALLUI_RETURN_VALUE_IF_FAIL(vd != NULL, NULL);
	Evas_Object *icon = NULL;

	switch (vd->type) {
	case VIEW_TYPE_ENDCALL:
		{
			layout = _callui_view_callend_get_layout(vd);
		}
		break;
	default:
		err("ERROR - wrong vd type:[%d]", vd->type);
		return NULL;
	}

	btn = elm_button_add(layout);
	elm_object_style_set(btn, "call_icon_only");
	icon = elm_image_add(btn);
	elm_image_file_set(icon, EDJ_NAME, "call_button_icon_04.png");
	elm_object_part_content_set(btn, "elm.swallow.content", icon);

	elm_object_part_content_set(layout, "three_btn_message", btn);
	elm_object_disabled_set(btn, EINA_TRUE);
	evas_object_show(btn);

	return btn;
}

static void __vcui_msg_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	char *number = (char *)data;
	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_COMPOSE);
	char str[CONTACT_NUMBER_BUF_LEN];
	snprintf(str, sizeof(str), "%s%s", "sms:", number);
	app_control_set_uri(request, str);
	int result = app_control_send_launch_request(request, NULL, NULL);
	if (result != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() failed (%d)", result);
	}
	free(number);
	app_control_destroy(request);


}

static void __vcui_view_contact_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	char *contact_id = (char *)data;
	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_VIEW);
	app_control_set_mime(request, APP_CONTROL_MIME_CONTACT);
	app_control_add_extra_data(request, APP_CONTROL_DATA_ID, contact_id);

	int result = app_control_send_launch_request(request, NULL, NULL);
	if (result != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() failed (%d)", result);
	}
	free(contact_id);
	app_control_destroy(request);
}

Evas_Object *_callui_create_view_contact_button(void *data, int ct_id)
{
	Evas_Object *btn;
	Evas_Object *layout;
	Evas_Object *sw;

	call_view_data_t *vd = (call_view_data_t *)data;
	CALLUI_RETURN_VALUE_IF_FAIL(vd != NULL, NULL);

	switch (vd->type) {
	case VIEW_TYPE_ENDCALL:
		{
			layout = _callui_view_callend_get_layout(vd);
		}
		break;
	default:
		err("ERROR - wrong vd type:[%d]", vd->type);
		return NULL;
	}

	sw = edje_object_part_swallow_get(_EDJ(layout), "btn_view_contacts");
	if (sw) {
		btn = sw;
	} else {
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "btn_view_contacts", btn);
	}

	elm_object_style_set(btn, "style_call_end_view_contact_button");
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_VIEW_CONTACT_DETAILS_ABB"));
	char str[CONTACT_ID_BUF_LEN];
	sprintf(str, "%d", ct_id);
	evas_object_smart_callback_add(btn, "clicked", __vcui_view_contact_btn_cb, (void *)strdup(str));

	return btn;
}

Evas_Object *_callui_create_create_contacts_button(void *data, char *number)
{

	call_view_data_t *vd = (call_view_data_t *)data;
	CALLUI_RETURN_VALUE_IF_FAIL(vd != NULL, NULL);

	Evas_Object *btn = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *sw = NULL;

	switch (vd->type) {
	case VIEW_TYPE_ENDCALL:
		{
			layout = _callui_view_callend_get_layout(vd);
		}
		break;
	default:
		err("ERROR - wrong vd type:[%d]", vd->type);
		return NULL;
	}

	sw = edje_object_part_swallow_get(_EDJ(layout), "btn_create_contacts");
	if (sw) {
		btn = sw;
	} else {
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "btn_create_contacts", btn);
	}

	info("Going to set style");
	elm_object_style_set(btn, "style_call_end_create_contact_button");

	elm_object_text_set(btn, _("IDS_COM_OPT_CREATE_CONTACT"));
	evas_object_smart_callback_add(btn, "clicked", __vcui_create_contact_btn_cb, (void *)strdup(number));

	return btn;
}

static void __vcui_create_contact_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("_vcui_create_contact_btn_cb");
	char *number = (char *)data;
	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_ADD);
	app_control_set_mime(request, APP_CONTROL_MIME_CONTACT);

	if (number) {
		app_control_add_extra_data(request, APP_CONTROL_DATA_PHONE, number);
	}
	app_control_send_launch_request(request, NULL, NULL);
	free(number);
	app_control_destroy(request);
}

static void __vcui_update_existing_contact_btn_cb(void *data,
		Evas_Object *obj, void *event_info)
{
	dbg("__vcui_update_existing_contact_btn_cb");
	char * number = (char *)data;
	app_control_h request;
	app_control_create(&request);
	app_control_set_operation(request, APP_CONTROL_OPERATION_EDIT);
	app_control_set_mime(request, APP_CONTROL_MIME_CONTACT);

	if (number) {
		app_control_add_extra_data(request, APP_CONTROL_DATA_PHONE, number);
	}
	app_control_send_launch_request(request, NULL, NULL);
	free(number);
	app_control_destroy(request);
}

Evas_Object *_callui_create_update_existing_contact_button(void *data, char *number)
{
	Evas_Object *btn;
	Evas_Object *layout;
	Evas_Object *sw;

	call_view_data_t *vd = (call_view_data_t *)data;
	CALLUI_RETURN_VALUE_IF_FAIL(vd != NULL, NULL);

	switch (vd->type) {
	case VIEW_TYPE_ENDCALL:
		{
			layout = _callui_view_callend_get_layout(vd);
		}
		break;
	default:
		err("ERROR - wrong vd type:[%d]", vd->type);
		return NULL;
	}

	sw = edje_object_part_swallow_get(_EDJ(layout), "btn_update");
	if (sw) {
		btn = sw;
	} else {
		btn = elm_button_add(layout);
		elm_object_part_content_set(layout, "btn_update", btn);
	}

	elm_object_style_set(btn, "style_call_end_update_contact_button");
	elm_object_text_set(btn, _("IDS_CALL_BUTTON_UPDATE_EXISTING"));
	evas_object_smart_callback_add(btn, "clicked",
			__vcui_update_existing_contact_btn_cb, (void *)strdup(number));

	return btn;
}

Evas_Object *_callui_create_message_button(void *data, char *number)
{
	Evas_Object *btn = NULL;
	Evas_Object *layout = NULL;

	call_view_data_t *vd = NULL;
	CALLUI_RETURN_VALUE_IF_FAIL((vd = (call_view_data_t *)data) != NULL, NULL);

	Evas_Object *icon = NULL;
	switch (vd->type) {
	case VIEW_TYPE_ENDCALL:
		{
			layout = _callui_view_callend_get_layout(vd);
		}
		break;
	default:
		err("ERROR - wrong vd type:[%d]", vd->type);
		return NULL;
	}

	btn = elm_button_add(layout);
	elm_object_style_set(btn, "call_icon_only");
	icon = elm_image_add(btn);
	elm_image_file_set(icon, EDJ_NAME, "call_button_icon_04.png");
	elm_object_part_content_set(btn, "elm.swallow.content", icon);

	elm_object_part_content_set(layout, "three_btn_message", btn);
	evas_object_smart_callback_add(btn, "clicked", __vcui_msg_btn_cb, (void *)strdup(number));

	evas_object_show(btn);
	return btn;
}

/* Creates thumbnail */
Evas_Object *_callui_create_thumbnail(Evas_Object *parent, const char *path, thumbnail_type type)
{
	return _callui_create_thumbnail_with_size(parent, path, type, false);
}

/* Creates thumbnail with possibility to set size in code*/
Evas_Object *_callui_create_thumbnail_with_size(Evas_Object *parent, const char *path, thumbnail_type type, bool set_size)
{
	if (path && *path && (strcmp(path, "default") != 0)) {
		Evas_Object *layout = elm_layout_add(parent);
		elm_layout_file_set(layout, EDJ_NAME, group_thumbnail[type]);

		Evas_Object *image = elm_image_add(layout);
		elm_image_file_set(image, path, NULL);
		elm_image_aspect_fixed_set(image, EINA_TRUE);
		elm_image_fill_outside_set(image, EINA_TRUE);
		if (set_size) {
		evas_object_size_hint_min_set(image, ELM_SCALE_SIZE(thumbnail_size[type]), ELM_SCALE_SIZE(thumbnail_size[type]));
		}
		elm_object_part_content_set(layout, PART_SWALLOW_IMAGE, image);

		return layout;
	} else {
		Evas_Object *image = elm_image_add(parent);
		elm_image_file_set(image, EDJ_NAME, group_default_thumbnail[type]);
		if (set_size) {
			evas_object_size_hint_min_set(image, ELM_SCALE_SIZE(thumbnail_size[type]), ELM_SCALE_SIZE(thumbnail_size[type]));
		}
		return image;
	}
}

/* Caller info name or number*/
void _callui_show_caller_info_name(void *data, const char *name)
{
	Evas_Object *layout = NULL;

	layout = __callui_create_caller_info(data);
	edje_object_part_text_set(_EDJ(layout), "txt_call_name", name);
}

/* Caller info number */
void _callui_show_caller_info_number(void *data, const char *number)
{
	Evas_Object *layout = NULL;

	layout = __callui_create_caller_info(data);
	edje_object_part_text_set(_EDJ(layout), "txt_phone_num", number);
}

/* Caller info status*/
Evas_Object *_callui_show_caller_info_status(void *data, const char *status)
{
	Evas_Object *layout = NULL;

	callui_app_data_t *ad = (callui_app_data_t *)data;
	layout = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
	edje_object_part_text_set(_EDJ(layout), "call_txt_status", status);

	return layout;
}

Evas_Object *_callui_show_caller_id(Evas_Object *contents, char *path)
{
	dbg("..");
	Evas_Object *layout = _callui_create_thumbnail(contents, path, THUMBNAIL_138);

	elm_object_part_content_set(contents, "caller_id", layout);

	/*Hide default caller-ID*/
	elm_object_signal_emit(contents, "hide_default_cid", "");

	return layout;
}

static void __callui_hold_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;

	__callui_unload_more_option(ad);

/*	if (_vcui_keypad_get_show_status() == EINA_TRUE) {
			_vcui_keypad_hide(vd);
	}
	*/
	cm_hold_call(ad->cm_handle);
}

static void __callui_unhold_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;

	__callui_unload_more_option(ad);

	cm_unhold_call(ad->cm_handle);
}

static void __callui_move_more_option(Evas_Object *ctxpopup)
{
	callui_app_data_t *ad = _callui_get_app_data();
	Evas_Coord w = 0, h = 0;
	int pos = -1;

	elm_win_screen_size_get(ad->win, NULL, NULL, &w, &h);
	pos = elm_win_rotation_get(ad->win);
	dbg("w: %d, h: %d", w, h);

	switch (pos) {
		case 0:
		case 180:
			evas_object_move(ctxpopup, (w / 2), h);
			break;
		case 90:
			evas_object_move(ctxpopup,  (h / 2), w);
			break;
		case 270:
			evas_object_move(ctxpopup, (h / 2), w);
			break;
		default:
			evas_object_move(ctxpopup, (w / 2), h);
			break;
	}
}

static void __callui_more_option_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..**********..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	__callui_unload_more_option(ad);
}

static void __callui_more_option_delete_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	dbg("..**********..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	__callui_unload_more_option(ad);
}

void _callui_load_more_option(void *data)
{
	dbg("..");
	callui_app_data_t *ad = _callui_get_app_data();
	call_view_data_t *vd = (call_view_data_t *)data;

	bool is_lcd_locked = FALSE;
	if (_callui_proximity_lock_manager_is_supported()) {
		is_lcd_locked = _callui_lock_manager_is_lcd_off(ad->lock_handle);
	} else {
		is_lcd_locked = _callui_lock_manager_is_started(ad->lock_handle);
	}

	if (is_lcd_locked == TRUE) {
		dbg( "Lock screen active. Do not show popup.");
		return;
	}

	if (ad->ctxpopup == NULL) {
		Evas_Object *ctxpopup = NULL;

		ctxpopup = elm_ctxpopup_add(ad->main_ly);
		elm_object_style_set(ctxpopup, "more/default");
		elm_ctxpopup_auto_hide_disabled_set(ctxpopup, EINA_TRUE);
		evas_object_smart_callback_add(ctxpopup, "dismissed", __callui_more_option_dismissed_cb, ad);
		evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, __callui_more_option_delete_cb, ad);
		eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK, eext_ctxpopup_back_cb, NULL);
		eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_MORE, eext_ctxpopup_back_cb, NULL);

		/* Hold/Resume */
		if (vd->type != VIEW_TYPE_MULTICALL_SPLIT) {
			if (ad->active != NULL) {
				elm_ctxpopup_item_append(ctxpopup, _("IDS_CALL_BUTTON_HOLD"), NULL, __callui_hold_btn_cb, ad);
			} else {/* CALL_HOLD */
				elm_ctxpopup_item_append(ctxpopup, _("IDS_CALL_BUTTON_RESUME_ABB"), NULL, __callui_unhold_btn_cb, ad);
			}
		}

		/* SIM Service */
/*		if (__vcui_view_popup_is_sim_available() == EINA_TRUE) {
			ctx_it = elm_ctxpopup_item_append(ctxpopup, _("IDS_CALL_OPT_SIM_SERVICES"),
				NULL, __vcui_view_popup_sim_service_cb, vd);
		}
*/
		elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_UP,
											ELM_CTXPOPUP_DIRECTION_UNKNOWN,
											ELM_CTXPOPUP_DIRECTION_UNKNOWN,
											ELM_CTXPOPUP_DIRECTION_UNKNOWN);

		__callui_move_more_option(ctxpopup);

		evas_object_show(ctxpopup);
		ad->ctxpopup = ctxpopup;
	} else {
		elm_ctxpopup_dismiss(ad->ctxpopup);
	}
}

static void __callui_unload_more_option(callui_app_data_t *ad)
{
	CALLUI_RETURN_IF_FAIL(ad);
	if (ad->ctxpopup) {
		evas_object_smart_callback_del(ad->ctxpopup, "dismissed", __callui_more_option_dismissed_cb);
		evas_object_event_callback_del(ad->ctxpopup, EVAS_CALLBACK_DEL, __callui_more_option_delete_cb);

		evas_object_del(ad->ctxpopup);
		ad->ctxpopup = NULL;
	}
}

int _callui_set_object_data(Evas_Object *obj, char *key, void *value)
{
	CALLUI_RETURN_VALUE_IF_FAIL(obj, -1);
	CALLUI_RETURN_VALUE_IF_FAIL(key, -1);
	CALLUI_RETURN_VALUE_IF_FAIL(value, -1);

	evas_object_data_set(obj, key, value);
	return 0;
}

void *_callui_get_object_data(Evas_Object *obj, char *key)
{
	CALLUI_RETURN_VALUE_IF_FAIL(obj, NULL);
	CALLUI_RETURN_VALUE_IF_FAIL(key, NULL);

	return evas_object_data_get(obj, key);
}

static void __callui_unload_second_call_popup(callui_app_data_t *ad)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(ad);
	if (ad->second_call_popup) {
		evas_object_del(ad->second_call_popup);
		ad->second_call_popup = NULL;
	}
}

static char *__callui_gl_second_call_option_label_get(void *data, Evas_Object *obj, const char *part)
{
	CALLUI_RETURN_VALUE_IF_FAIL(part, NULL);
	CALLUI_RETURN_VALUE_IF_FAIL(data, NULL);
	if (strcmp(part, "elm.text") == 0) {
		char *markup_txt = NULL;
		second_call_popup_data_t *item_data = (second_call_popup_data_t *)data;
		if (strlen(item_data->option_msg) > 0) {
			markup_txt = evas_textblock_text_utf8_to_markup(NULL, item_data->option_msg);
		}
		return markup_txt;
	} else {
		return NULL;
	}
}

static void __callui_gl_second_call_option_del_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
	second_call_popup_data_t *item_data = (second_call_popup_data_t *)data;
	g_free(item_data);
}

static void __callui_gl_second_call_option_sel(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	Elm_Object_Item *item = (Elm_Object_Item *) event_info;
	call_data_t *hold_call_data = NULL;
	call_data_t *unhold_call_data = NULL;
	second_call_popup_data_t *item_data = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);

	__callui_unload_second_call_popup(ad);

	if (item != NULL) {
		item_data = (second_call_popup_data_t *) elm_object_item_data_get(item);
		CALLUI_RETURN_IF_FAIL(item_data);
		dbg("index: %d", item_data->index);

		hold_call_data = ad->held;
		unhold_call_data = ad->active;
		if ((unhold_call_data) && (hold_call_data == NULL)) {
			dbg("1 active call OR 1 active conference call");
			if (item_data->index == 0) {
				cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_HOLD_ACTIVE_AND_ACCEPT);
			} else if (item_data->index == 1) {
				cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_RELEASE_ACTIVE_AND_ACCEPT);
			} else {
				err("Wrong index.. Should never get here");
			}
		} else if (((unhold_call_data) && (hold_call_data))) {
			dbg("1 active call + 1 held call OR 1 active conf call + 1 held call OR 1 active call + 1 held conf call");
			if (item_data->index == 0) {
				cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_RELEASE_ACTIVE_AND_ACCEPT);
			} else if (item_data->index == 1) {
				cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_RELEASE_HOLD_AND_ACCEPT);
			} else if (item_data->index == 2) {
				cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_RELEASE_ALL_AND_ACCEPT);
			} else {
				err("Wrong index.. Should never get here");
			}
		}

	}
}

static void __callui_second_call_cancel_btn_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");

	callui_app_data_t *ad = (callui_app_data_t*) data;
	__callui_unload_second_call_popup(ad);
	_callui_vm_change_view(ad->view_manager_handle, VIEW_TYPE_INCOMING_LOCK);

	return;
}

void _callui_load_second_call_popup(callui_app_data_t *ad)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(ad);
	Evas_Object *box = NULL;
	call_data_t *hold_call_data = NULL;
	call_data_t *unhold_call_data = NULL;
	char *hold_call_number = NULL;
	char *hold_call_name = NULL;
	char *unhold_call_number = NULL;
	char *unhold_call_name = NULL;
	const char *temp_str = NULL;
	second_call_popup_data_t *item_data = NULL;
	Elm_Genlist_Item_Class *itc = NULL;
	Evas_Object *genlist = NULL;
	CALLUI_RETURN_IF_FAIL(ad);

	hold_call_data = ad->held;
	unhold_call_data = ad->active;
	if (unhold_call_data == NULL) {
		err("active call data is null");
		return;
	}

	__callui_unload_second_call_popup(ad);

	ad->second_call_popup = elm_popup_add(ad->win);
	CALLUI_RETURN_IF_FAIL(ad->second_call_popup);
	elm_popup_align_set(ad->second_call_popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	elm_object_domain_translatable_part_text_set(ad->second_call_popup, "title,text", CALLUI_TEXT_DOMAIN, "IDS_CALL_HEADER_ANSWER_CALL_ABB");
	evas_object_size_hint_weight_set(ad->second_call_popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	box = elm_box_add(ad->second_call_popup);
	CALLUI_RETURN_IF_FAIL(box);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	genlist = elm_genlist_add(box);
	CALLUI_RETURN_IF_FAIL(genlist);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

	itc = elm_genlist_item_class_new();
	CALLUI_RETURN_IF_FAIL(itc);

	itc->item_style = "type1";
	itc->func.text_get = __callui_gl_second_call_option_label_get;
	itc->func.content_get = NULL;
	itc->func.state_get = NULL;
	itc->func.del = __callui_gl_second_call_option_del_cb;

	if (hold_call_data != NULL) {
		hold_call_number = hold_call_data->call_num;
		hold_call_name = hold_call_data->call_ct_info.call_disp_name;
		if (strlen(hold_call_name) == 0)
			hold_call_name = hold_call_number;
	}

	if (unhold_call_data != NULL) {
		unhold_call_number = unhold_call_data->call_num;
		unhold_call_name = unhold_call_data->call_ct_info.call_disp_name;
		if (strlen(unhold_call_name) == 0)
			unhold_call_name = unhold_call_number;
	}

	if ((unhold_call_data) && (unhold_call_data->member_count == 1) && (hold_call_data == NULL)) {
		dbg("1 active call");
		/* First option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 0;
		memset(item_data->option_msg, 0x00, 512);
		temp_str = _("IDS_CALL_OPT_PUT_PS_ON_HOLD_ABB");
		snprintf(item_data->option_msg, 512, temp_str, unhold_call_name);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Second option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 1;
		memset(item_data->option_msg, 0x00, 512);
		temp_str = _("IDS_CALL_OPT_END_CALL_WITH_PS");
		snprintf(item_data->option_msg, 512, temp_str, unhold_call_name);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);
	} else if ((unhold_call_data) && (unhold_call_data->member_count > 1) && (hold_call_data == NULL)) {
		dbg("1 active conference call");
		/* First option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 0;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_BODY_HOLD_ACTIVE_CALL_ABB"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Second option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 1;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_OPT_END_CONFERENCE_CALL"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);
	} else if ((unhold_call_data) && (hold_call_data) && (unhold_call_data->member_count == 1) && (hold_call_data->member_count == 1)) {
		dbg("1 active call + 1 held call");
		/* First option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 0;
		memset(item_data->option_msg, 0x00, 512);
		temp_str = _("IDS_CALL_OPT_END_CALL_WITH_PS");
		snprintf(item_data->option_msg, 512, temp_str, unhold_call_name);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Second option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 1;
		memset(item_data->option_msg, 0x00, 512);
		temp_str = _("IDS_CALL_OPT_END_CALL_WITH_PS");
		snprintf(item_data->option_msg, 512, temp_str, hold_call_name);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Third option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 2;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_OPT_END_ALL_CURRENT_CALLS"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);
	} else if ((unhold_call_data) && (hold_call_data) && (unhold_call_data->member_count > 1) && (hold_call_data->member_count == 1)) {
		dbg("1 active conf call + 1 held call");
		/* First option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 0;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, "End conference call", 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Second option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 1;
		memset(item_data->option_msg, 0x00, 512);
		temp_str = _("IDS_CALL_OPT_END_CALL_WITH_PS");
		snprintf(item_data->option_msg, 512, temp_str, hold_call_name);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Third option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 2;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_OPT_END_ALL_CURRENT_CALLS"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);
	} else if ((unhold_call_data) && (hold_call_data) && (unhold_call_data->member_count == 1) && (hold_call_data->member_count > 1)) {
		dbg("1 active call + 1 held conf call");
		/* First option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 0;
		memset(item_data->option_msg, 0x00, 512);
		temp_str = _("IDS_CALL_OPT_END_CALL_WITH_PS");
		snprintf(item_data->option_msg, 512, temp_str, unhold_call_name);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Second option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 1;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_OPT_END_CONFERENCE_CALL"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Third option */
		item_data = (second_call_popup_data_t *) calloc(1, sizeof(second_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 2;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_OPT_END_ALL_CURRENT_CALLS"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);
	} else {
		err("invalid call data!!");
		elm_genlist_item_class_free(itc);
		return;
	}

	evas_object_smart_callback_add(ad->second_call_popup, "response",
				__callui_second_call_cancel_btn_response_cb, ad);
	eext_object_event_callback_add(ad->second_call_popup, EEXT_CALLBACK_BACK,
				__callui_second_call_cancel_btn_response_cb, ad);
	evas_object_smart_callback_add(ad->second_call_popup, "block,clicked",
				__callui_second_call_cancel_btn_response_cb, ad);

	elm_box_pack_end(box, genlist);
	evas_object_show(genlist);
	elm_genlist_item_class_free(itc);

	evas_object_size_hint_min_set(box, ELM_SCALE_SIZE(POPUP_LIST_W), ELM_SCALE_SIZE(POPUP_LIST_ITEM_H * (item_data->index + 1)));
	evas_object_show(box);
	elm_object_content_set(ad->second_call_popup, box);

	evas_object_show(ad->second_call_popup);
}

static void __callui_unload_bt_popup(callui_app_data_t *ad)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(ad);
	if (ad->bt_popup) {
		evas_object_del(ad->bt_popup);
		ad->bt_popup = NULL;
	}
	return;
}

static void __callui_bt_popup_cancel_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");

	__callui_unload_bt_popup(data);
	return;
}

static void __callui_bt_popup_ok_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);

	__callui_unload_bt_popup(ad);
	/* Launch the BT App Control */
	_callui_common_launch_bt_app(ad);
	return;
}

void _callui_load_bluetooth_popup(callui_app_data_t *ad)
{
	Evas_Object *btn_ok = NULL;
	Evas_Object *btn_cancel = NULL;
	CALLUI_RETURN_IF_FAIL(ad);

	__callui_unload_bt_popup(ad);
	ad->bt_popup = elm_popup_add(ad->win);
	elm_popup_align_set(ad->bt_popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(ad->bt_popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_object_domain_translatable_part_text_set(ad->bt_popup, "title,text", CALLUI_TEXT_DOMAIN, "IDS_CALL_HEADER_TURN_ON_BLUETOOTH_ABB");
	elm_object_text_set(ad->bt_popup, _("IDS_CALL_POP_TO_SWITCH_TO_USING_YOUR_BLUETOOTH_DEVICE_BLUETOOTH_WILL_BE_TURNED_ON"));

	btn_cancel = elm_button_add(ad->bt_popup);
	elm_object_style_set(btn_cancel, "popup");
	elm_object_text_set(btn_cancel, _("IDS_COM_SK_CANCEL"));
	elm_object_part_content_set(ad->bt_popup, "button1", btn_cancel);
	evas_object_smart_callback_add(btn_cancel, "clicked", __callui_bt_popup_cancel_btn_cb, ad);

	btn_ok = elm_button_add(ad->bt_popup);
	elm_object_style_set(btn_ok, "popup");
	elm_object_text_set(btn_ok, _("IDS_COM_SK_OK"));
	elm_object_part_content_set(ad->bt_popup, "button2", btn_ok);
	evas_object_smart_callback_add(btn_ok, "clicked", __callui_bt_popup_ok_btn_cb, ad);

	eext_object_event_callback_add(ad->bt_popup, EEXT_CALLBACK_BACK, __callui_bt_popup_cancel_btn_cb, ad);

	evas_object_show(ad->bt_popup);
	return;
}

void _callui_create_extravolume_notify_popup()
{
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad != NULL);
	char *text = NULL;
	cm_audio_state_type_e snd_path = CM_AUDIO_STATE_NONE_E;

	cm_get_audio_state(ad->cm_handle, &snd_path);
	dbg("sound path : %d", snd_path);
	switch (snd_path) {
	case CM_AUDIO_STATE_BT_E:
		{
			text = _("IDS_CALL_POP_EXTRA_VOLUME_CANNOT_BE_TURNED_ON_WHILE_BLUETOOTH_HEADSET_CONNECTED");
		}
		break;
	case CM_AUDIO_STATE_EARJACK_E:
		{
			text = _("IDS_CALL_POP_EXTRA_VOLUME_CANNOT_BE_TURNED_ON_WHILE_EARPHONES_CONNECTED");
		}
		break;
	default:
		text = _("IDS_CALL_POP_EXTRA_VOLUME_CANNOT_BE_TURNED_ON_WHILE_EARPHONES_OR_BLUETOOTH_HEADSET_CONNECTED");
		break;
	}
	_callui_create_toast_message(text);

}

void _callui_create_toast_message(char *string)
{
	dbg("$$$$$$ Noti-String : %s", string);

	if (string) {
		notification_status_message_post(string);
	}
	return;
}

bool _callui_is_on_handsfree_mode()
{
	callui_app_data_t *ad = _callui_get_app_data();
	return (ad->speaker_status || ad->headset_status || ad->earphone_status);
}

bool _callui_is_on_background()
{
	callui_app_data_t *ad = _callui_get_app_data();
	return ad->on_background;
}
