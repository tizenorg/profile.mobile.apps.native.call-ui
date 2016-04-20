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
#include <app_control.h>
#include <Elementary.h>
#include <efl_extension.h>

#include "callui-view-incoming-call-noti.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-view-manager.h"
#include "callui-view-layout.h"
#include "callui-view-elements.h"
#include "callui-common.h"

#include "callui-manager.h"
#include "callui-state-provider.h"
#include "callui-sound-manager.h"

#define CALLUI_DURING_ICON	"call_button_icon_03.png"
#define CALLUI_REJECT_ICON	"call_button_icon_04.png"
#define CALLUI_END_ICON		"call_button_icon_01.png"

#define CALLUI_REJ_MSG_GENLIST_DATA "VIEW_DATA"

struct _callui_view_incoming_call_noti {
	call_view_data_base_t base_view;

	Evas_Object *reject_msg_layout;
	Evas_Object *reject_msg_genlist;

	Elm_Genlist_Item_Class *reject_msg_itc;
	char reject_msg[CALLUI_REJ_MSG_MAX_LENGTH];
};

typedef struct _callui_view_incoming_call_noti _callui_view_incoming_call_noti_t;

static callui_result_e __callui_view_incoming_call_noti_oncreate(call_view_data_base_t *view_data, void *appdata);
static callui_result_e __callui_view_incoming_call_noti_onupdate(call_view_data_base_t *view_data);
static callui_result_e __callui_view_incoming_call_noti_ondestroy(call_view_data_base_t *view_data);

static callui_result_e __create_main_content(callui_view_incoming_call_noti_h vd);
static callui_result_e __update_displayed_data(callui_view_incoming_call_noti_h vd);

static void __reject_msg_genlist_item_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __reject_msg_layout_back_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __reject_msg_genlist_add(callui_view_incoming_call_noti_h vd);
static char *__reject_msg_genlist_item_txt_cb(void *data, Evas_Object *obj, const char *part);
static void __reject_msg_genlist_init_item_class(callui_view_incoming_call_noti_h vd);
static void __reject_msg_genlist_deinit_item_class(callui_view_incoming_call_noti_h vd);
static Elm_Object_Item *__reject_msg_genlist_append_item(Evas_Object *msg_glist, Elm_Genlist_Item_Class * itc_reject_msg, int index);
static int __reject_msg_genlist_fill(callui_view_incoming_call_noti_h vd);

static void __swipe_layout_mouse_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static Eina_Bool __reject_msg_close_effect_activated_cb(void *data);
static void __show_reject_msg_btn_click_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void __create_reject_msg_content(callui_view_incoming_call_noti_h vd);

static void __launch_btn_click_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static callui_result_e __create_main_content(callui_view_incoming_call_noti_h vd);
static callui_result_e __create_custom_button(char *icon_name, char *part, Evas_Object_Event_Cb func, void *data, void *data_bt_cb);

callui_view_incoming_call_noti_h _callui_view_incoming_call_noti_new()
{
	callui_view_incoming_call_noti_h incoming_call_noti = calloc(1, sizeof(_callui_view_incoming_call_noti_t));
	CALLUI_RETURN_NULL_IF_FAIL(incoming_call_noti);

	incoming_call_noti->base_view.create = __callui_view_incoming_call_noti_oncreate;
	incoming_call_noti->base_view.update = __callui_view_incoming_call_noti_onupdate;
	incoming_call_noti->base_view.destroy = __callui_view_incoming_call_noti_ondestroy;

	return incoming_call_noti;
}

static callui_result_e __callui_view_incoming_call_noti_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	// TODO: need to remove this logic from here
	evas_object_resize(ad->win, ad->root_w, ELM_SCALE_SIZE(MTLOCK_ACTIVE_NOTI_CALL_HEIGHT));
	_callui_common_win_set_noti_type(ad, true);

	if (!elm_win_keygrab_set(ad->win, CALLUI_KEY_SELECT, 0, 0, 0, ELM_WIN_KEYGRAB_TOPMOST)) {
		dbg("KEY_SELECT key grab failed");
	}

	callui_result_e res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return __update_displayed_data(vd);
}

static callui_result_e __callui_view_incoming_call_noti_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	return __update_displayed_data((callui_view_incoming_call_noti_h)view_data);
}

static callui_result_e __callui_view_incoming_call_noti_ondestroy(call_view_data_base_t *view_data)
{
	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	// TODO: need to replace from view
#ifdef _DBUS_DISPLAY_DEVICE_TIMEOUT_
	/* Set display timeout for call state */
	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->sound_manager);
	if (audio_state == CALLUI_AUDIO_STATE_SPEAKER) {
		_callui_display_set_timeout(ad->display, CALLUI_DISPLAY_TIMEOUT_SET);
	}
#endif

	DELETE_EVAS_OBJECT(vd->base_view.contents);

	_callui_common_win_set_noti_type(ad, false);

	evas_object_resize(ad->win, ad->root_w, ad->root_h);

	elm_win_keygrab_unset(ad->win, CALLUI_KEY_SELECT, 0, 0);

	free(vd);

	return CALLUI_RESULT_OK;
}

static void __reject_msg_genlist_item_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	int index = CALLUI_SAFE_C_CAST(int, data);
	dbg("index: %d", index);

	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)evas_object_data_get(obj, CALLUI_REJ_MSG_GENLIST_DATA);
	CALLUI_RETURN_IF_FAIL(vd);
	callui_app_data_t *ad = vd->base_view.ad;

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);

	if (index != -1) {
		char *ret_str = _callui_common_get_reject_msg_by_index(index);
		if (ret_str) {
			char *reject_msg = elm_entry_markup_to_utf8(ret_str); /*send utf8 text to MSG */
			if (reject_msg != NULL) {
				snprintf(vd->reject_msg, sizeof(vd->reject_msg), "%s", reject_msg);
				free(reject_msg);
			}
			free(ret_str);
		}

		callui_result_e res = _callui_manager_reject_call(ad->call_manager);
		if (res != CALLUI_RESULT_OK) {
			err("cm_reject_call() is failed. res[%d]", res);
		} else {
			_callui_common_send_reject_msg(ad, vd->reject_msg);
		}
	}
}

static void __swipe_layout_mouse_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	ecore_idle_enterer_before_add(__reject_msg_close_effect_activated_cb, vd);

	evas_object_resize(ad->win, 0, 0);
}

static Eina_Bool __reject_msg_close_effect_activated_cb(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)data;

	Evas_Object *screen_ly = NULL;
	Elm_Transit *transit = NULL;
	int xpos = 0;
	int ypos = 0;
	int width = 0;
	int height = 0;
	int transit_y = 0;

	screen_ly = vd->base_view.contents;
	transit = elm_transit_add();
	elm_transit_object_add(transit, screen_ly);
	evas_object_geometry_get(screen_ly, &xpos, &ypos, &width, &height);
	transit_y = -(height + ypos);
	elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
	elm_transit_duration_set(transit, 0.5);
	evas_object_show(screen_ly);
	elm_transit_objects_final_state_keep_set(transit, EINA_TRUE);
	elm_transit_go(transit);

	return ECORE_CALLBACK_CANCEL;
}

static void __show_reject_msg_btn_click_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	evas_object_resize(ad->win, ad->root_w, ad->root_h);

	__create_reject_msg_content(vd);

	elm_object_signal_emit(vd->base_view.contents, "big_main_ly", "main_active_noti_call");
}

static void __reject_msg_layout_back_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	evas_object_resize(ad->win, ad->root_w, ELM_SCALE_SIZE(MTLOCK_ACTIVE_NOTI_CALL_HEIGHT));

	elm_object_signal_emit(vd->base_view.contents, "small_main_ly", "main_active_noti_call");
	evas_object_del(vd->reject_msg_genlist);
}

static void __reject_msg_genlist_add(callui_view_incoming_call_noti_h vd)
{
	if (vd->reject_msg_genlist) {
		evas_object_del(vd->reject_msg_genlist);
		vd->reject_msg_genlist = NULL;
	}

	vd->reject_msg_genlist = elm_genlist_add(vd->base_view.contents);
	CALLUI_RETURN_IF_FAIL(vd->reject_msg_genlist);
	eext_object_event_callback_add(vd->reject_msg_layout, EEXT_CALLBACK_BACK, __reject_msg_layout_back_click_cb, vd);
	evas_object_data_set(vd->reject_msg_genlist, CALLUI_REJ_MSG_GENLIST_DATA, vd);

	evas_object_size_hint_weight_set(vd->reject_msg_genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(vd->reject_msg_genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_object_part_content_set(vd->reject_msg_layout, "swallow.content", vd->reject_msg_genlist);
}

static char *__reject_msg_genlist_item_txt_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = CALLUI_SAFE_C_CAST(int, data);
	char *msg_str = NULL;

	if (!strcmp(part, "elm.text")) {
		if (index != -1) {
			msg_str = _callui_common_get_reject_msg_by_index(index);
			sec_dbg("msg_str(%s)", msg_str);
			return msg_str; /* Send markup text to draw the genlist */
		} else {
			warn("invalid index: %d", index);
			msg_str = _("IDS_CALL_BODY_NO_MESSAGES");
			return strdup(msg_str);
		}
	}
	return NULL;
}

static void __reject_msg_genlist_init_item_class(callui_view_incoming_call_noti_h vd)
{
	Elm_Genlist_Item_Class *item_class = elm_genlist_item_class_new();
	CALLUI_RETURN_IF_FAIL(item_class);

	item_class->item_style = "type1";
	item_class->func.text_get = __reject_msg_genlist_item_txt_cb;
	item_class->func.content_get = NULL;
	item_class->func.state_get = NULL;
	item_class->func.del = NULL;
	vd->reject_msg_itc = item_class;
}

static void __reject_msg_genlist_deinit_item_class(callui_view_incoming_call_noti_h vd)
{
	elm_genlist_item_class_free(vd->reject_msg_itc);
	vd->reject_msg_itc = NULL;
}

static Elm_Object_Item *__reject_msg_genlist_append_item(Evas_Object *msg_glist, Elm_Genlist_Item_Class * itc_reject_msg, int index)
{
	Elm_Object_Item *item = NULL;
	item = elm_genlist_item_append(msg_glist, itc_reject_msg,
			CALLUI_SAFE_C_CAST(void *, index), NULL, ELM_GENLIST_ITEM_NONE,
			__reject_msg_genlist_item_click_cb, CALLUI_SAFE_C_CAST(void *, index));
	return item;
}

static int __reject_msg_genlist_fill(callui_view_incoming_call_noti_h vd)
{
	int msg_cnt = 0;
	if (0 != vconf_get_int(VCONFKEY_CISSAPPL_REJECT_CALL_MSG_INT, &msg_cnt)) {
		warn("vconf_get_int failed.");
	}
	dbg("msg_cnt: %d", msg_cnt);

	int index = 0;
	if (msg_cnt == 0) {
		index = -1;
		Elm_Object_Item * item = __reject_msg_genlist_append_item(vd->reject_msg_genlist, vd->reject_msg_itc, index);
		if (item) {
			elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		}
	} else {
		for (index = 0; index < msg_cnt; index++) {
			__reject_msg_genlist_append_item(vd->reject_msg_genlist, vd->reject_msg_itc, index);
		}
	}
	return msg_cnt;
}

static void __create_reject_msg_content(callui_view_incoming_call_noti_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->reject_msg_layout = _callui_load_edj(vd->base_view.contents, EDJ_NAME, "reject_msg_ly");
	elm_object_part_content_set(vd->base_view.contents, "swallow.reject_msg", vd->reject_msg_layout);

	// TODO: possibly can be replaced in common part. Need investigate
	__reject_msg_genlist_add(vd);
	__reject_msg_genlist_init_item_class(vd);
	int msg_cnt = __reject_msg_genlist_fill(vd);
	__reject_msg_genlist_deinit_item_class(vd);

	_callui_create_reject_msg_button(ad, vd->reject_msg_layout, "swallow.button");

	elm_object_tree_focus_allow_set(vd->reject_msg_genlist, EINA_FALSE);
	evas_object_size_hint_min_set(vd->reject_msg_genlist, ad->root_w, 0);
	evas_object_size_hint_max_set(vd->reject_msg_genlist, ad->root_w, ELM_SCALE_SIZE(MTLOCK_REJECT_MSG_LIST_1ITEM_NEW_HEIGHT * msg_cnt));
	evas_object_show(vd->reject_msg_layout);
	evas_object_show(vd->reject_msg_genlist);
}

static void __launch_btn_click_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	char *key = (char *)data;

	app_control_h app_control = NULL;
	int ret;
	if ((ret = app_control_create(&app_control)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_create() is failed. ret[%d]", ret);
	} else if (app_control_set_app_id(app_control, PACKAGE) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_app_id() is failed. ret[%d]", ret);
	} else if ((ret = app_control_set_operation(app_control, key)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_set_operation() is failed. ret[%d]", ret);
	} else if ((ret = app_control_send_launch_request(app_control, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
		err("app_control_send_launch_request() is failed. ret[%d]", ret);
	}
	if (app_control) {
		app_control_destroy(app_control);
	}
}

static callui_result_e __create_main_content(callui_view_incoming_call_noti_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME,  "main_active_noti_call");
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", vd->base_view.contents);

	callui_result_e res = __create_custom_button(CALLUI_DURING_ICON, "swallow.call_button",
			__launch_btn_click_cb, vd, (void *)APP_CONTROL_OPERATION_DURING_CALL);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = __create_custom_button(CALLUI_REJECT_ICON, "swallow.rj_msg_button",
			__show_reject_msg_btn_click_cb, vd, (void *)vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = __create_custom_button(CALLUI_END_ICON, "swallow.end_call_button",
			__launch_btn_click_cb, vd, (void *)APP_CONTROL_OPERATION_END_CALL);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	Evas_Object *swipe_layout = elm_layout_add(vd->base_view.contents);
	CALLUI_RETURN_VALUE_IF_FAIL(swipe_layout, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_layout_file_set(swipe_layout, EDJ_NAME, "swipe_call_button_ly");
	evas_object_event_callback_add(swipe_layout, EVAS_CALLBACK_MOUSE_MOVE, __swipe_layout_mouse_move_cb, vd);
	elm_object_part_content_set(vd->base_view.contents, "swallow.swipe_button", swipe_layout);

	return res;
}

static callui_result_e __create_custom_button(char *icon_name, char *part, Evas_Object_Event_Cb func, void * data, void * data_bt_cb)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_incoming_call_noti_h vd = (callui_view_incoming_call_noti_h)data;
	callui_app_data_t *ad = vd->base_view.ad;

	Evas_Object *button_call = elm_layout_add(vd->base_view.contents);
	CALLUI_RETURN_VALUE_IF_FAIL(button_call, CALLUI_RESULT_ALLOCATION_FAIL);

	elm_layout_file_set(button_call, EDJ_NAME, "main_button_ly");
	Evas_Object *icon = elm_image_add(button_call);
	CALLUI_RETURN_VALUE_IF_FAIL(icon, CALLUI_RESULT_ALLOCATION_FAIL);

	elm_image_file_set(icon, EDJ_NAME, icon_name);
	elm_object_part_content_set(button_call, "swallow.icon", icon);
	elm_object_part_content_set(vd->base_view.contents, part, button_call);
	evas_object_data_set(button_call, "app_data", ad);
	evas_object_event_callback_add(button_call, EVAS_CALLBACK_MOUSE_UP, func, data_bt_cb);

	return CALLUI_RESULT_OK;
}

static callui_result_e __update_displayed_data(callui_view_incoming_call_noti_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	const callui_call_data_t *call_data = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);
	CALLUI_RETURN_VALUE_IF_FAIL(call_data, CALLUI_RESULT_FAIL);

	elm_object_signal_emit(vd->base_view.contents, "small_main_ly", "main_active_noti_call");

	const char *call_name = call_data->call_ct_info.call_disp_name;
	const char *call_number = NULL;
	if (call_data->call_disp_num[0] != '\0') {
		call_number = call_data->call_disp_num;
	} else {
		call_number = call_data->call_num;
	}
	const char *file_path = call_data->call_ct_info.caller_id_path;

	if (!(call_name && call_name[0] != '\0') && !(call_number && call_number[0] != '\0')) {
		elm_object_signal_emit(vd->base_view.contents, "big_buttons", "main_active_noti_call");
		elm_object_translatable_part_text_set(vd->base_view.contents,
				"text.contact_name", "IDS_CALL_BODY_UNKNOWN");
	} else if (!(call_name && call_name[0] != '\0')) {
		elm_object_signal_emit(vd->base_view.contents, "small_buttons", "main_active_noti_call");
		elm_object_part_text_set(vd->base_view.contents, "text.contact_name", call_number);
	} else {
		elm_object_signal_emit(vd->base_view.contents, "small_buttons", "main_active_noti_call");
		elm_object_part_text_set(vd->base_view.contents, "text.contact_name", call_name);
		elm_object_part_text_set(vd->base_view.contents, "text.contact_number", call_number);
	}

	if (strcmp(file_path, "default") != 0) {
		Evas_Object *layout = _callui_create_thumbnail(vd->base_view.contents, file_path, THUMBNAIL_98);
		elm_object_part_content_set(vd->base_view.contents, "contact_icon", layout);
		elm_object_signal_emit(vd->base_view.contents, "hide_def_caller_id", "main_active_noti_call");
	} else {
		elm_object_signal_emit(vd->base_view.contents, "show_def_caller_id", "main_active_noti_call");
	}

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}
