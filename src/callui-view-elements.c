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
#include <network/bluetooth.h>
#include <app_control.h>
#include <notification.h>
#include <efl_extension.h>

#include "callui-view-elements.h"
#include "callui-debug.h"
#include "callui-view-manager.h"
#include "callui-view-dialing.h"
#include "callui-view-single-call.h"
#include "callui-view-callend.h"
#include "callui-view-multi-call-split.h"
#include "callui-view-layout.h"
#include "callui-common.h"
#include "callui-keypad.h"
#include "callui-view-multi-call-conf.h"
#include "callui-view-quickpanel.h"
#include "callui-proximity-lock-manager.h"
#include "callui-manager.h"
#include "callui-sound-manager.h"
#include "callui-state-provider.h"

#define	CALLUI_POPUP_LIST_W		300
#define	CALLUI_POPUP_LIST_ITEM_H 	120

#define CALLUI_CID_THUMBN_SIZE_DEFAULT	0
#define CALLUI_CID_THUMBN_SIZE_TINY		98
#define CALLUI_CID_THUMBN_SIZE_SMALL	138
#define CALLUI_CID_THUMBN_SIZE_MEDIUM	168
#define CALLUI_CID_THUMBN_SIZE_BIG		348

#define CALLUI_PART_SWALLOW_END_BTN "swallow.end_btn"

typedef struct {
	int index;
	char option_msg[512];
} callui_sec_call_popup_data_t;

static int _g_cid_size[CALLUI_CID_SIZE_COUNT] = {
		CALLUI_CID_THUMBN_SIZE_DEFAULT,
		CALLUI_CID_THUMBN_SIZE_TINY,
		CALLUI_CID_THUMBN_SIZE_SMALL,
		CALLUI_CID_THUMBN_SIZE_MEDIUM,
		CALLUI_CID_THUMBN_SIZE_BIG
};

static Evas_Object *__callui_get_caller_info_layout(void *data);
static void __callui_hold_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void __callui_unhold_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void __callui_move_more_option(callui_app_data_t *ad, Evas_Object *ctxpopup);
static void __callui_more_option_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static void __callui_more_option_delete_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void __callui_unload_more_option(callui_app_data_t *ad);
static void __callui_unload_second_call_popup(callui_app_data_t *ad);
static char *__callui_gl_second_call_option_label_get(void *data, Evas_Object *obj, const char *part);
static void __callui_gl_second_call_option_del_cb(void *data, Evas_Object *obj);
static void __callui_gl_second_call_option_sel(void *data, Evas_Object *obj, void *event_info);
static void __callui_second_call_cancel_btn_response_cb(void *data, Evas_Object *obj, void *event_info);
static void __callui_unload_bt_popup(callui_app_data_t *ad);
static void __callui_bt_popup_cancel_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void __callui_bt_popup_ok_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void __callui_create_new_msg_btn_click_cb(void *data, Evas_Object *obj, void *event_info);

Evas_Object *_callui_load_edj(Evas_Object *parent, const char *file, const char *group)
{
	Evas_Object *eo = elm_layout_add(parent);
	if (eo) {
		int ret = elm_layout_file_set(eo, file, group);
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

static Evas_Object *__callui_get_caller_info_layout(void *data)
{
	Evas_Object *caller_info = NULL;
	Evas_Object *layout = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_VALUE_IF_FAIL(ad, NULL);

	layout = elm_object_part_content_get(_callui_vm_get_main_ly(ad->view_manager), "elm.swallow.content");
	CALLUI_RETURN_VALUE_IF_FAIL(layout, NULL);
	caller_info = elm_object_part_content_get(layout, "swallow.caller_info");

	return caller_info;
}

Evas_Object *_callui_create_end_call_button(Evas_Object *parent, Evas_Smart_Cb cb_func, void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(parent != NULL, NULL);

	Evas_Object *btn = elm_object_part_content_get(parent, CALLUI_PART_SWALLOW_END_BTN);
	if (!btn) {
		btn = elm_button_add(parent);
		elm_object_style_set(btn, "call_icon_only");

		Evas_Object *icon = elm_image_add(btn);
		elm_image_file_set(icon, CALLUI_CALL_EDJ_PATH, "call_button_icon_01.png");
		elm_object_part_content_set(btn, "elm.swallow.content", icon);
		elm_object_part_content_set(parent, CALLUI_PART_SWALLOW_END_BTN, btn);

		if (cb_func) {
			evas_object_smart_callback_add(btn, "clicked", cb_func, data);
		}
	}
	evas_object_show(btn);

	return btn;
}

Evas_Object *_callui_create_cid_thumbnail(Evas_Object *parent, callui_cid_type_e type, const char *path)
{
	return _callui_create_cid_thumbnail_with_size(parent, type, CALLUI_CID_SIZE_DEFAULT, path);
}

Evas_Object *_callui_create_cid_thumbnail_with_size(Evas_Object *parent,
		callui_cid_type_e type,
		callui_cid_size_e size,
		const char *path)
{
	CALLUI_RETURN_NULL_IF_FAIL(parent);
	CALLUI_RETURN_NULL_IF_FAIL(type >= CALLUI_CID_TYPE_SINGLE && type < CALLUI_CID_TYPE_COUNT);
	CALLUI_RETURN_NULL_IF_FAIL(size >= CALLUI_CID_SIZE_DEFAULT && size < CALLUI_CID_SIZE_COUNT);

	Evas_Object *layout = elm_layout_add(parent);
	CALLUI_RETURN_NULL_IF_FAIL(layout);
	elm_layout_file_set(layout, CALLUI_CALL_EDJ_PATH, "caller_id");
	evas_object_show(layout);

	Evas_Object *thumbnail = layout;
	Evas_Object *resizable_obj = layout;

	switch (type) {
	case CALLUI_CID_TYPE_SINGLE:
	{
		if (!STRING_EMPTY(path) && (strcmp(path, "default") != 0)) {

			Evas_Object *image = elm_image_add(layout);
			if(!image) {
				err("image is NULL");
				DELETE_EVAS_OBJECT(layout);
				return NULL;
			}
			Eina_Bool res = elm_image_file_set(image, path, NULL);
			if (!res) {
				err("elm_image_file_set() with path '%s' failed. Use default icon.", path);
				elm_object_signal_emit(layout, "show_default_cid", "");
				break;
			}
			resizable_obj = image;
			elm_image_aspect_fixed_set(image, EINA_TRUE);
			elm_image_fill_outside_set(image, EINA_TRUE);
			elm_object_part_content_set(layout, "swallow.image", image);
			elm_object_signal_emit(layout, "hide_default_cid", "");
		} else {
			elm_object_signal_emit(layout, "show_default_cid", "");
		}
	}
	break;
	case CALLUI_CID_TYPE_CONFERENCE:
		elm_object_signal_emit(layout, "show_conference_cid", "");
		break;
	case CALLUI_CID_TYPE_EMERGENCY:
		elm_object_signal_emit(layout, "show_emergency_cid", "");
		break;
	case CALLUI_CID_TYPE_MESSAGE:
		elm_object_signal_emit(layout, "show_message_cid", "");
		break;
	default:
		DELETE_EVAS_OBJECT(layout);
		return NULL;
	}

	if (size != CALLUI_CID_SIZE_DEFAULT) {
		if (resizable_obj == layout) {
			thumbnail = elm_grid_add(parent);
			if(!thumbnail) {
				err("thumbnail is NULL");
				DELETE_EVAS_OBJECT(layout);
				return NULL;
			}
			elm_grid_pack(thumbnail, layout, 0, 0, 100, 100);
			evas_object_show(thumbnail);
			resizable_obj = thumbnail;
		}
		evas_object_size_hint_min_set(resizable_obj,
				ELM_SCALE_SIZE(_g_cid_size[size]),
				ELM_SCALE_SIZE(_g_cid_size[size]));
	}

	return thumbnail;
}

/* Caller info name or number*/
void _callui_show_caller_info_name(void *data, const char *name)
{
	Evas_Object *layout = __callui_get_caller_info_layout(data);
	elm_object_translatable_part_text_set(layout, "contact_name", name);
}

/* Caller info number */
void _callui_show_caller_info_number(void *data, const char *number)
{
	Evas_Object *layout = __callui_get_caller_info_layout(data);
	elm_object_part_text_set(layout, "phone_number", number);
}

/* Caller info message */
void _callui_show_caller_info_message(void *data, const char *txt)
{
	Evas_Object *layout = __callui_get_caller_info_layout(data);
	elm_object_part_text_set(layout, "message", txt);
}

/* Caller info status*/
Evas_Object *_callui_show_caller_info_status(void *data, const char *status)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;
	Evas_Object *layout = elm_object_part_content_get(_callui_vm_get_main_ly(ad->view_manager), "elm.swallow.content");
	elm_object_translatable_part_text_set(layout, "call_txt_status", status);

	return layout;
}

Evas_Object *_callui_show_caller_id(Evas_Object *contents, const callui_call_data_t *call_data)
{
	CALLUI_RETURN_NULL_IF_FAIL(contents);
	CALLUI_RETURN_NULL_IF_FAIL(call_data);

	callui_cid_type_e type = CALLUI_CID_TYPE_SINGLE;
	if (call_data->is_emergency) {
		type = CALLUI_CID_TYPE_EMERGENCY;
	} else if (call_data->conf_member_count > 1) {
		type = CALLUI_CID_TYPE_CONFERENCE;
	}

	Evas_Object *thumbnail = _callui_create_cid_thumbnail(contents, type, call_data->call_ct_info.caller_id_path);
	CALLUI_RETURN_NULL_IF_FAIL(thumbnail);
	elm_object_part_content_set(contents, "swallow.caller_id", thumbnail);

	return thumbnail;
}

static void __callui_hold_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;

	__callui_unload_more_option(ad);

	callui_result_e res = _callui_manager_hold_call(ad->call_manager);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_hold_call() failed. res[%d]", res);
	}
}

static void __callui_unhold_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *)data;

	__callui_unload_more_option(ad);

	callui_result_e res = _callui_manager_unhold_call(ad->call_manager);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_unhold_call() failed. res[%d]", res);
	}
}

static void __callui_move_more_option(callui_app_data_t *ad, Evas_Object *ctxpopup)
{
	Evas_Coord w = 0, h = 0;
	int pos = -1;

	_callui_window_get_screen_size(ad->window, NULL, NULL, &w, &h);
	pos = _callui_window_get_rotation(ad->window);
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
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;
	bool is_lcd_locked = false;

	if (_callui_proximity_lock_manager_is_supported()) {
		is_lcd_locked = _callui_lock_manager_is_lcd_off(ad->lock_handle);
	} else {
		is_lcd_locked = _callui_lock_manager_is_started(ad->lock_handle);
	}

	if (is_lcd_locked) {
		dbg("Lock screen active. Do not show popup.");
		return;
	}

	if (ad->ctxpopup == NULL) {
		Evas_Object *ctxpopup = elm_ctxpopup_add(_callui_vm_get_main_ly(ad->view_manager));
		elm_object_style_set(ctxpopup, "more/default");
		elm_ctxpopup_auto_hide_disabled_set(ctxpopup, EINA_TRUE);
		evas_object_smart_callback_add(ctxpopup, "dismissed", __callui_more_option_dismissed_cb, ad);
		evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, __callui_more_option_delete_cb, ad);
		eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK, eext_ctxpopup_back_cb, NULL);
		eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_MORE, eext_ctxpopup_back_cb, NULL);

		/* Hold/Resume */
		if (_callui_vm_get_cur_view_type(ad->view_manager) != CALLUI_VIEW_MULTICALL_SPLIT) {
			const callui_call_data_t *call_data =
					_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
			if (call_data) {
				elm_ctxpopup_item_append(ctxpopup, _("IDS_CALL_BUTTON_HOLD"), NULL, __callui_hold_btn_cb, ad);
			} else {
				elm_ctxpopup_item_append(ctxpopup, _("IDS_CALL_BUTTON_RESUME_ABB"), NULL, __callui_unhold_btn_cb, ad);
			}
		}
		elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_UP,
											ELM_CTXPOPUP_DIRECTION_UNKNOWN,
											ELM_CTXPOPUP_DIRECTION_UNKNOWN,
											ELM_CTXPOPUP_DIRECTION_UNKNOWN);

		__callui_move_more_option(ad, ctxpopup);

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
		callui_sec_call_popup_data_t *item_data = (callui_sec_call_popup_data_t *)data;
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
	callui_sec_call_popup_data_t *item_data = (callui_sec_call_popup_data_t *)data;
	free(item_data);
}

static void __callui_gl_second_call_option_sel(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	CALLUI_RETURN_IF_FAIL(event_info);

	Elm_Object_Item *item = (Elm_Object_Item *) event_info;
	callui_sec_call_popup_data_t *item_data = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)data;

	__callui_unload_second_call_popup(ad);

	if (item != NULL) {
		item_data = (callui_sec_call_popup_data_t *) elm_object_item_data_get(item);
		CALLUI_RETURN_IF_FAIL(item_data);
		dbg("index: %d", item_data->index);

		const callui_call_data_t *hold_call_data =
				_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
		const callui_call_data_t *unhold_call_data =
				_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);

		if ((unhold_call_data) && (hold_call_data == NULL)) {
			dbg("1 active call OR 1 active conference call");
			if (item_data->index == 0) {
				_callui_manager_answer_call(ad->call_manager,
						CALLUI_CALL_ANSWER_HOLD_ACTIVE_AND_ACCEPT);
			} else if (item_data->index == 1) {
				_callui_manager_answer_call(ad->call_manager,
						CALLUI_CALL_ANSWER_RELEASE_ACTIVE_AND_ACCEPT);
			} else {
				err("Wrong index.. Should never get here");
			}
		} else if (((unhold_call_data) && (hold_call_data))) {
			dbg("1 active call + 1 held call OR 1 active conf call + 1 held call OR 1 active call + 1 held conf call");
			if (item_data->index == 0) {
				_callui_manager_answer_call(ad->call_manager,
						CALLUI_CALL_ANSWER_RELEASE_ACTIVE_AND_ACCEPT);
			} else if (item_data->index == 1) {
				_callui_manager_answer_call(ad->call_manager,
						CALLUI_CALL_ANSWER_RELEASE_HOLD_AND_ACCEPT);
			} else if (item_data->index == 2) {
				_callui_manager_answer_call(ad->call_manager,
						CALLUI_CALL_ANSWER_RELEASE_ALL_AND_ACCEPT);
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
	_callui_vm_change_view(ad->view_manager, CALLUI_VIEW_INCOMING_CALL);

	return;
}

void _callui_load_second_call_popup(callui_app_data_t *ad)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(ad);
	Evas_Object *box = NULL;

	const char *hold_call_number = NULL;
	const char *hold_call_name = NULL;
	const char *unhold_call_number = NULL;
	const char *unhold_call_name = NULL;

	const char *temp_str = NULL;
	callui_sec_call_popup_data_t *item_data = NULL;
	Elm_Genlist_Item_Class *itc = NULL;
	Evas_Object *genlist = NULL;
	CALLUI_RETURN_IF_FAIL(ad);

	const callui_call_data_t *hold_call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
	const callui_call_data_t *unhold_call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	if (unhold_call_data == NULL) {
		err("active call data is null");
		return;
	}

	__callui_unload_second_call_popup(ad);

	ad->second_call_popup = elm_popup_add(_callui_window_get_eo(ad->window));
	CALLUI_RETURN_IF_FAIL(ad->second_call_popup);
	elm_popup_align_set(ad->second_call_popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	elm_object_translatable_part_text_set(ad->second_call_popup, "title,text", "IDS_CALL_HEADER_ANSWER_CALL_ABB");
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

	if ((unhold_call_data) && (unhold_call_data->conf_member_count == 1) && (hold_call_data == NULL)) {
		dbg("1 active call");
		/* First option */
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
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
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 1;
		memset(item_data->option_msg, 0x00, 512);
		temp_str = _("IDS_CALL_OPT_END_CALL_WITH_PS");
		snprintf(item_data->option_msg, 512, temp_str, unhold_call_name);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);
	} else if ((unhold_call_data)
			&& (unhold_call_data->conf_member_count > 1)
			&& (hold_call_data == NULL)) {
		dbg("1 active conference call");
		/* First option */
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 0;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_BODY_HOLD_ACTIVE_CALL_ABB"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Second option */
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 1;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_OPT_END_CONFERENCE_CALL"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);
	} else if ((unhold_call_data)
			&& (hold_call_data) && (unhold_call_data->conf_member_count == 1)
			&& (hold_call_data->conf_member_count == 1)) {
		dbg("1 active call + 1 held call");
		/* First option */
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
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
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
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
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 2;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_OPT_END_ALL_CURRENT_CALLS"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);
	} else if ((unhold_call_data) && (hold_call_data)
			&& (unhold_call_data->conf_member_count > 1)
			&& (hold_call_data->conf_member_count == 1)) {
		dbg("1 active conf call + 1 held call");
		/* First option */
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 0;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, "End conference call", 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Second option */
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
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
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 2;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_OPT_END_ALL_CURRENT_CALLS"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);
	} else if ((unhold_call_data) && (hold_call_data)
			&& (unhold_call_data->conf_member_count == 1)
			&& (hold_call_data->conf_member_count > 1)) {
		dbg("1 active call + 1 held conf call");
		/* First option */
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
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
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
		if (item_data == NULL) {
			elm_genlist_item_class_free(itc);
			return;
		}
		item_data->index = 1;
		memset(item_data->option_msg, 0x00, 512);
		strncpy(item_data->option_msg, _("IDS_CALL_OPT_END_CONFERENCE_CALL"), 512-1);
		elm_genlist_item_append(genlist, itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, __callui_gl_second_call_option_sel, ad);

		/* Third option */
		item_data = (callui_sec_call_popup_data_t *) calloc(1, sizeof(callui_sec_call_popup_data_t));
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

	evas_object_size_hint_min_set(box, ELM_SCALE_SIZE(CALLUI_POPUP_LIST_W), ELM_SCALE_SIZE(CALLUI_POPUP_LIST_ITEM_H * (item_data->index + 1)));
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
	_callui_common_launch_setting_bluetooth(ad);
	return;
}

void _callui_load_bluetooth_popup(callui_app_data_t *ad)
{
	Evas_Object *btn_ok = NULL;
	Evas_Object *btn_cancel = NULL;
	CALLUI_RETURN_IF_FAIL(ad);

	__callui_unload_bt_popup(ad);
	ad->bt_popup = elm_popup_add(_callui_window_get_eo(ad->window));
	elm_popup_align_set(ad->bt_popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(ad->bt_popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_object_translatable_part_text_set(ad->bt_popup, "title,text", "IDS_CALL_HEADER_TURN_ON_BLUETOOTH_ABB");
	elm_object_translatable_text_set(ad->bt_popup, "IDS_CALL_POP_TO_SWITCH_TO_USING_YOUR_BLUETOOTH_DEVICE_BLUETOOTH_WILL_BE_TURNED_ON");

	btn_cancel = elm_button_add(ad->bt_popup);
	elm_object_style_set(btn_cancel, "popup");
	elm_object_translatable_text_set(btn_cancel, "IDS_COM_SK_CANCEL");
	elm_object_part_content_set(ad->bt_popup, "button1", btn_cancel);
	evas_object_smart_callback_add(btn_cancel, "clicked", __callui_bt_popup_cancel_btn_cb, ad);

	btn_ok = elm_button_add(ad->bt_popup);
	elm_object_style_set(btn_ok, "popup");
	elm_object_translatable_text_set(btn_ok, "IDS_COM_SK_OK");
	elm_object_part_content_set(ad->bt_popup, "button2", btn_ok);
	evas_object_smart_callback_add(btn_ok, "clicked", __callui_bt_popup_ok_btn_cb, ad);

	eext_object_event_callback_add(ad->bt_popup, EEXT_CALLBACK_BACK, __callui_bt_popup_cancel_btn_cb, ad);

	evas_object_show(ad->bt_popup);
	return;
}

static void __callui_create_new_msg_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_app_data_t *ad = (callui_app_data_t *)data;


	const callui_call_data_t *incom = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);
	CALLUI_RETURN_IF_FAIL(incom);

	_callui_common_launch_msg_composer(ad, incom->call_num, false);

	callui_result_e res = _callui_manager_reject_call(ad->call_manager);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_reject_call() failed. res[%d]", res);
	}
}

int _callui_create_reject_msg_button(void *app_data, Evas_Object *parent, const char *part)
{
	Evas_Object *msg_button = elm_button_add(parent);
	CALLUI_RETURN_VALUE_IF_FAIL(msg_button, CALLUI_RESULT_ALLOCATION_FAIL);

	elm_object_style_set(msg_button, "default");
	elm_object_translatable_text_set(msg_button,  "IDS_CALL_BUTTON_COMPOSE_MESSAGE_TO_SEND_ABB");
	evas_object_smart_callback_add(msg_button, "clicked", __callui_create_new_msg_btn_click_cb, app_data);
	elm_object_part_content_set(parent, part, msg_button);
	evas_object_show(msg_button);

	return CALLUI_RESULT_OK;
}
