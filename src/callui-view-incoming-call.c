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
#include <Elementary.h>

#include "callui-view-incoming-call.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-view-layout.h"
#include "callui-view-elements.h"
#include "callui-view-manager.h"
#include "callui-view-circle.h"
#include "callui-common.h"
#include "callui-state-provider.h"

#define CALLUI_REJ_MSG_GENLIST_DATA "reject_msg_genlist_data"
#define CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY "list_open_status_key"

#define SCALE_SIZE(x, h) (((x) * (h)) / MAIN_SCREEN_H)

struct _callui_view_incoming_call {
	call_view_data_base_t base_view;

	Evas_Object *caller_info;

	Evas_Object *lock_accept;
	Evas_Object *lock_reject;

	Evas_Object *dimming_ly;

	Evas_Object *reject_msg_layout;
	Evas_Object *reject_msg_glayer;
	Evas_Object *reject_msg_genlist;

	int reject_msg_start_y;
	int reject_msg_cur_y;

	int reject_msg_height;

	Eina_Bool is_mouse_down_pressed;

	Elm_Genlist_Item_Class *reject_msg_itc;
	Evas_Coord momentum_y;
	char reject_msg[CALLUI_REJ_MSG_MAX_LENGTH];
};
typedef struct _callui_view_incoming_call _callui_view_incoming_call_t;

static callui_result_e __callui_view_incoming_call_oncreate(call_view_data_base_t *view_data, void *appdata);
static callui_result_e __callui_view_incoming_call_onupdate(call_view_data_base_t *view_data);
static callui_result_e __callui_view_incoming_call_ondestroy(call_view_data_base_t *view_data);

static callui_result_e __create_main_content(callui_view_incoming_call_h vd);
static callui_result_e __update_displayed_data(callui_view_incoming_call_h vd);

static char *__callui_view_incoming_call_reject_msg_gl_label_get_msg(void *data, Evas_Object *obj, const char *part);
static void __reject_msg_gl_sel_msg(void *data, Evas_Object *obj, void *event_info);

static void __reject_msg_genlist_create(callui_view_incoming_call_h vd);
static void __reject_msg_genlist_init_item_class(callui_view_incoming_call_h vd);
static void __reject_msg_genlist_deinit_item_class(callui_view_incoming_call_h vd);
static int __reject_msg_genlist_fill(callui_view_incoming_call_h vd);
static Elm_Object_Item *__reject_msg_genlist_append_item(Evas_Object *msg_glist, Elm_Genlist_Item_Class * itc_reject_msg, int index);

static void __reject_msg_list_param_reset(callui_view_incoming_call_h vd);
static void __reject_screen_transit_complete_cb(void *data, Elm_Transit *transit);
static Eina_Bool __reject_msg_show_sliding_effect(void *data);
static Evas_Event_Flags __reject_msg_flick_gesture_move_event_cb(void *data, void *event_info);
static void __reject_msg_create_gesture_layer(callui_view_incoming_call_h vd);

static void __reject_msg_bg_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void __reject_msg_bg_mouse_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void __reject_msg_bg_mouse_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void __reject_msg_list_height_update(callui_view_incoming_call_h vd);
static void __reject_msg_create_genlist(callui_view_incoming_call_h vd);
static Eina_Bool __reject_msg_check_tel_num(const char *call_num);
static Evas_Object *__create_reject_msg_layout(callui_view_incoming_call_h vd);

callui_view_incoming_call_h _callui_view_incoming_call_new()
{
	callui_view_incoming_call_h incoming_lock_view = calloc(1, sizeof(_callui_view_incoming_call_t));
	CALLUI_RETURN_NULL_IF_FAIL(incoming_lock_view);

	incoming_lock_view->base_view.onCreate = __callui_view_incoming_call_oncreate;
	incoming_lock_view->base_view.onUpdate = __callui_view_incoming_call_onupdate;
	incoming_lock_view->base_view.onDestroy = __callui_view_incoming_call_ondestroy;

	return incoming_lock_view;
}

static callui_result_e __callui_view_incoming_call_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_incoming_call_h vd = (callui_view_incoming_call_h)view_data;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;

	vd->base_view.ad = ad;

	if (_callui_lock_manager_is_started(ad->lock_handle)) {
		_callui_lock_manager_force_stop(ad->lock_handle);
	}

	evas_object_resize(ad->win, ad->root_w,  ad->root_h);
	_callui_common_win_set_noti_type(ad, EINA_TRUE);

	evas_object_pointer_mode_set(ad->win, EVAS_OBJECT_POINTER_MODE_NOGRAB);

	if (elm_win_keygrab_set(ad->win, CALLUI_KEY_SELECT, 0, 0, 0, ELM_WIN_KEYGRAB_TOPMOST)) {
		dbg("KEY_SELECT key grab failed");
	}

	callui_result_e res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return __update_displayed_data(vd);
}

static callui_result_e __callui_view_incoming_call_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	return __update_displayed_data((callui_view_incoming_call_h)view_data);
}

static callui_result_e __callui_view_incoming_call_ondestroy(call_view_data_base_t *view_data)
{
	callui_view_incoming_call_h vd = (callui_view_incoming_call_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	// TODO: need to replace from view
#ifdef _DBUS_DVC_LSD_TIMEOUT_
	/* Set LCD timeout for call state */
	/* LCD is alwasy on during incoming call screen */
	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->call_sdm);
	if (audio_state == CALLUI_AUDIO_STATE_SPEAKER) {
		_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_SET);
	}
#endif

	DELETE_EVAS_OBJECT(ad->second_call_popup);

	DELETE_EVAS_OBJECT(vd->base_view.contents);

	_callui_common_win_set_noti_type(ad, EINA_FALSE);

	elm_win_keygrab_unset(ad->win, CALLUI_KEY_SELECT, 0, 0);

	free(vd);

	return CALLUI_RESULT_OK;
}

static char *__callui_view_incoming_call_reject_msg_gl_label_get_msg(void *data, Evas_Object *obj, const char *part)
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

static void __reject_msg_gl_sel_msg(void *data, Evas_Object *obj, void *event_info)
{
	int index = CALLUI_SAFE_C_CAST(int, data);
	dbg("index: %d", index);

	callui_view_incoming_call_h vd = (callui_view_incoming_call_h)evas_object_data_get(obj, CALLUI_REJ_MSG_GENLIST_DATA);
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

static void __reject_msg_genlist_create(callui_view_incoming_call_h vd)
{
	DELETE_EVAS_OBJECT(vd->reject_msg_genlist);

	vd->reject_msg_genlist = elm_genlist_add(vd->reject_msg_layout);
	elm_genlist_homogeneous_set(vd->reject_msg_genlist, EINA_TRUE);
	elm_genlist_mode_set(vd->reject_msg_genlist, ELM_LIST_COMPRESS);
	elm_scroller_content_min_limit(vd->reject_msg_genlist, EINA_FALSE, EINA_TRUE);
	CALLUI_RETURN_IF_FAIL(vd->reject_msg_genlist);

	evas_object_data_set(vd->reject_msg_genlist, CALLUI_REJ_MSG_GENLIST_DATA, vd);
	evas_object_size_hint_weight_set(vd->reject_msg_genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(vd->reject_msg_genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_part_content_set(vd->reject_msg_layout, "swl_msglist", vd->reject_msg_genlist);
}

static void __reject_msg_genlist_init_item_class(callui_view_incoming_call_h vd)
{
	Elm_Genlist_Item_Class *item_class = elm_genlist_item_class_new();
	CALLUI_RETURN_IF_FAIL(item_class);

	item_class->item_style = "type1";
	item_class->func.text_get = __callui_view_incoming_call_reject_msg_gl_label_get_msg;
	item_class->func.content_get = NULL;
	item_class->func.state_get = NULL;
	item_class->func.del = NULL;
	vd->reject_msg_itc = item_class;
}

static void __reject_msg_genlist_deinit_item_class(callui_view_incoming_call_h vd)
{
	elm_genlist_item_class_free(vd->reject_msg_itc);
	vd->reject_msg_itc = NULL;
}

static Elm_Object_Item *__reject_msg_genlist_append_item(Evas_Object *msg_glist, Elm_Genlist_Item_Class * itc_reject_msg, int index)
{
	Elm_Object_Item *item = NULL;
	item = elm_genlist_item_append(msg_glist, itc_reject_msg,
			CALLUI_SAFE_C_CAST(void *, index), NULL, ELM_GENLIST_ITEM_NONE,
			__reject_msg_gl_sel_msg, CALLUI_SAFE_C_CAST(void *, index));
	return item;
}

static int __reject_msg_genlist_fill(callui_view_incoming_call_h vd)
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

Evas_Object *_callui_view_incoming_call_get_accept_layout(callui_view_incoming_call_h vd)
{
	CALLUI_RETURN_NULL_IF_FAIL(vd);
	return vd->lock_accept;
}

callui_result_e _callui_view_incoming_call_set_accept_layout(callui_view_incoming_call_h vd, Evas_Object *layout)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vd, CALLUI_RESULT_INVALID_PARAM);

	vd->lock_accept = layout;
	return CALLUI_RESULT_OK;
}

Evas_Object *_callui_view_incoming_call_get_reject_layout(callui_view_incoming_call_h vd)
{
	CALLUI_RETURN_NULL_IF_FAIL(vd);
	return vd->lock_reject;
}

callui_result_e _callui_view_incoming_call_set_reject_layout(callui_view_incoming_call_h vd, Evas_Object *layout)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vd, CALLUI_RESULT_INVALID_PARAM);

	vd->lock_reject = layout;
	return CALLUI_RESULT_OK;
}

static void __reject_msg_list_param_reset(callui_view_incoming_call_h vd)
{
	vd->momentum_y = 0;
	vd->reject_msg_start_y = 0;
	vd->reject_msg_cur_y = 0;
	vd->is_mouse_down_pressed = EINA_FALSE;
}

static void __reject_screen_transit_complete_cb(void *data, Elm_Transit *transit)
{
	callui_view_incoming_call_h vd = (callui_view_incoming_call_h)data;

	if (EINA_FALSE == evas_object_data_get(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY)) {
		elm_object_tree_focus_allow_set(vd->reject_msg_genlist, EINA_FALSE);
		elm_object_signal_emit(vd->reject_msg_layout, "show-up-arrow", "reject_msg");
		evas_object_hide(vd->dimming_ly);
	} else {
		elm_object_tree_focus_allow_set(vd->reject_msg_genlist, EINA_TRUE);
		elm_object_signal_emit(vd->reject_msg_layout, "show-down-arrow", "reject_msg");
		evas_object_show(vd->dimming_ly);
	}
	__reject_msg_list_param_reset(vd);
}

static Eina_Bool __reject_msg_show_sliding_effect(void *data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_incoming_call_h vd = (callui_view_incoming_call_h)data;
	Evas_Object *screen_ly;
	Elm_Transit *transit;
	int xpos = 0;
	int ypos = 0;
	int width = 0;
	int height = 0;
	int transit_y = 0;
	int max_height_limit = 0;

	screen_ly = vd->reject_msg_layout;
	transit = elm_transit_add();
	elm_transit_object_add(transit, screen_ly);

	evas_object_geometry_get(vd->reject_msg_layout, &xpos, &ypos, &width, &height);
	dbg("reject_w_msg dimensions ---> x[%d] y[%d] w[%d] h[%d]", xpos, ypos, width, height);
	dbg("vd->y_momentum: %d", vd->momentum_y);

	/*Max height possible*/
	max_height_limit = (vd->reject_msg_height);
	dbg("max_height_limit: %d", max_height_limit);

	if (EINA_FALSE == evas_object_data_get(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY)) {
		dbg("Close list... Check if opening is feasible");

		if (vd->momentum_y) {
			dbg("Momentum...");

			if (vd->momentum_y < -500) {
				dbg("Huge Momentum... Move the layout");

				/*effect to pull up the window.*/
				transit_y = -(max_height_limit + ypos);

				elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
				evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
			} else {
				dbg("Small Momentum..");

				if (ypos != 0) {	/*Reject msg layout is displaced from its original position*/
					if (-ypos < max_height_limit/2) {	/*Layout position is lesser than half of the height*/
						dbg("Movement L.T. HALF the height..");

						/*effect to pull down the window.*/
						transit_y = -ypos;

						elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
						evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
					} else if (-ypos >= max_height_limit/2 && -ypos <= max_height_limit) {	/*Layout position is greater than half of the height*/
						dbg("Movement G.T. HALF the height..");

						/*effect to pull up the window.*/
						transit_y = -(max_height_limit + ypos);

						elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
						evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
					}
				}
			}
		} else {
			dbg("NO Momentum... Dont open");

			if (ypos != 0) {	/*Reject msg layout is displaced from its original position*/
				if (-ypos < max_height_limit/2) {	/*Layout position is lesser than half of the height*/
					dbg("Movement L.T. HALF the height..");

					/*effect to pull down the window.*/
					transit_y = -ypos;

					elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
					evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
				} else if (-ypos >= max_height_limit/2 && -ypos <= max_height_limit) {	/*Layout position is greater than half of the height*/
					dbg("Movement G.T. HALF the height..");

					/*effect to pull up the window.*/
					transit_y = -(max_height_limit + ypos);

					elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
					evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
				}
			}
		}
	} else {
		dbg("Open list... Check if closing is feasible");

		if (vd->momentum_y) {
			dbg("Momentum...");

			if (vd->momentum_y > 500) {
				dbg("Huge Momentum... Move the layout");

				/*effect to pull down the window.*/
				transit_y = -ypos;

				elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
				evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
			} else {
				dbg("Small Momentum..");

				if (ypos != 0) {	/*Reject msg layout is displaced from its original position*/
					if (-ypos < (max_height_limit * 0.8)) {	/*Layout position is lesser than 80% of the height*/
						dbg("Movement L.T. 80 percent of the height..");

						/*effect to pull down the window.*/
						transit_y = -ypos;

						elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
						evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
					} else if (-ypos >= (max_height_limit * 0.8) && -ypos <= max_height_limit) {	/*Layout position is greater than half of the height*/
						dbg("Movement G.T. 80 percent of the height..");

						/*effect to pull up the window.*/
						transit_y = -(max_height_limit + ypos);

						elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
						evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
					}
				}
			}
		} else {
			dbg("NO Momentum... Dont close");

			if (ypos != 0) {	/*Reject msg layout is displaced from its original position*/
				if (-ypos < (max_height_limit * 0.8)) {	/*Layout position is lesser than 80% of the height*/
					dbg("Movement L.T. 80 percent of the height..");

					/*effect to pull down the window.*/
					transit_y = -ypos;

					elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
					evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
				} else if (-ypos >= (max_height_limit * 0.8) && -ypos <= max_height_limit) {	/*Layout position is greater than half of the height*/
					dbg("Movement G.T. 80 percent of the height..");

					/*effect to pull up the window.*/
					transit_y = -(max_height_limit + ypos);

					elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
					evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
				}
			}
		}
	}

	elm_transit_del_cb_set(transit, __reject_screen_transit_complete_cb, vd);

	if (vd->momentum_y < 0)
		vd->momentum_y = -vd->momentum_y;

	if (vd->momentum_y < 1500) {
		elm_transit_duration_set(transit, 0.5);
	} else if (vd->momentum_y >= 1500 && vd->momentum_y < 3000) {
		elm_transit_duration_set(transit, 0.4);
	} else if (vd->momentum_y >= 3000 && vd->momentum_y < 4500) {
		elm_transit_duration_set(transit, 0.3);
	} else if (vd->momentum_y >= 4500) {
		elm_transit_duration_set(transit, 0.2);
	}
	evas_object_show(screen_ly);	/*It must be called before elm_transit_go(). transit policy*/
	elm_transit_objects_final_state_keep_set(transit, EINA_TRUE);	/*Keep the window position as end of effect.*/
	elm_transit_go(transit);

	return ECORE_CALLBACK_CANCEL;
}

static void __reject_msg_bg_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);

	callui_view_incoming_call_h vd = (callui_view_incoming_call_h)data;

	if (vd->is_mouse_down_pressed) {
		ecore_idle_enterer_before_add(__reject_msg_show_sliding_effect, vd);
	} else {
		dbg("mouse down was NOT pressed - DONT handle up event");
	}
}

static Evas_Event_Flags __reject_msg_flick_gesture_move_event_cb(void *data, void *event_info)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, EVAS_EVENT_FLAG_NONE);
	CALLUI_RETURN_VALUE_IF_FAIL(event_info, EVAS_EVENT_FLAG_NONE);

	callui_view_incoming_call_h vd = (callui_view_incoming_call_h)data;
	Elm_Gesture_Line_Info *info = (Elm_Gesture_Line_Info *)event_info;

	dbg("*********************************************");
	dbg("info->angle = %lf", info->angle);
	dbg("info->momentum.mx = %d, info->momentum.my = %d", info->momentum.mx, info->momentum.my);
	dbg("info->momentum.n = %d", info->momentum.n);
	dbg("info->momentum.tx = %d, info->momentum.ty = %d", info->momentum.tx, info->momentum.ty);
	dbg("info->momentum.x1 = %d, info->momentum.x2 = %d", info->momentum.x1, info->momentum.x2);
	dbg("info->momentum.y1 = %d, info->momentum.y2 = %d", info->momentum.y1, info->momentum.y2);
	dbg("*********************************************");

	vd->momentum_y = info->momentum.my;

	return EVAS_EVENT_FLAG_NONE;
}

static void __reject_msg_create_gesture_layer(callui_view_incoming_call_h vd)
{
	Evas_Object *reject_msg_bg = _callui_edje_object_part_get(vd->reject_msg_layout, "reject_msg_bg");
	if (vd->reject_msg_glayer) {
		evas_object_del(vd->reject_msg_glayer);
		vd->reject_msg_glayer = NULL;
	}

	vd->reject_msg_glayer = elm_gesture_layer_add(vd->reject_msg_layout);
	if (!elm_gesture_layer_attach(vd->reject_msg_glayer, reject_msg_bg)) {
		dbg("elm_gesture_layer_attach failed !!");
		evas_object_del(vd->reject_msg_glayer);
		vd->reject_msg_glayer = NULL;
	} else {
		elm_gesture_layer_cb_set(vd->reject_msg_glayer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_MOVE, __reject_msg_flick_gesture_move_event_cb, vd);
	}
}

static void __reject_msg_bg_mouse_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	CALLUI_RETURN_IF_FAIL(event_info);

	callui_view_incoming_call_h vd = (callui_view_incoming_call_h)data;
	Evas_Event_Mouse_Down *ev = event_info;

	vd->reject_msg_start_y = ev->canvas.y;
	vd->is_mouse_down_pressed = EINA_TRUE;
}

static void __reject_msg_bg_mouse_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	CALLUI_RETURN_IF_FAIL(event_info);

	callui_view_incoming_call_h vd = (callui_view_incoming_call_h)data;
	Evas_Event_Mouse_Move *ev = event_info;

	int diff_y = 0;
	int max_height_limit = 0;

	/*Max height possible*/
	max_height_limit = vd->reject_msg_height;
	dbg("max_height_limit: %d", max_height_limit);

	if (!vd->is_mouse_down_pressed) {
		dbg("mouse down was NOT pressed - DONT handle move event");
		return;
	}
	dbg("mouse down was pressed - handle move event");
	vd->reject_msg_cur_y = ev->cur.canvas.y;

	diff_y = vd->reject_msg_cur_y - vd->reject_msg_start_y;
	dbg("diff_y [<<< %d >>>>]", diff_y);

	if (EINA_FALSE == evas_object_data_get(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY)) {
		elm_object_signal_emit(vd->reject_msg_layout, "show-down-arrow", "reject_msg");
		if ((diff_y > -max_height_limit) && (diff_y <= 0)) {
			/*Lies between 0 and msg-list layout height*/
			evas_object_move(vd->reject_msg_layout, 0, diff_y);
		} else if (diff_y <= -max_height_limit) {
			/*Special case - Move the max distance - msg-list height*/
			evas_object_move(vd->reject_msg_layout, 0, -max_height_limit);
			evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
			__reject_screen_transit_complete_cb(vd, NULL);
		}
	} else {
		if ((diff_y >= 0) && (diff_y < max_height_limit)) {
			/*Lies between 0 and msg-list layout height*/
			evas_object_move(vd->reject_msg_layout, 0, -(max_height_limit - diff_y));
		} else if (diff_y >= max_height_limit) {
			/*Special case - Move the max distance - msg-list height*/
			evas_object_move(vd->reject_msg_layout, 0, 0);
			evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
			__reject_screen_transit_complete_cb(vd, NULL);
		}
	}
}

static void __reject_msg_list_height_update(callui_view_incoming_call_h vd)
{
	int msg_cnt = 0;
	if (0 != vconf_get_int(VCONFKEY_CISSAPPL_REJECT_CALL_MSG_INT, &msg_cnt)) {
		warn("vconf_get_int failed.");
	}
	if (msg_cnt == 0)
		msg_cnt = 1;

	int win_h = 0;
	callui_app_data_t *ad = vd->base_view.ad;
	elm_win_screen_size_get(ad->win, NULL, NULL, NULL, &win_h);

	vd->reject_msg_height = SCALE_SIZE((REJ_MSG_LIST_CREATE_MSG_BTN_H + ((ITEM_SIZE_H) * msg_cnt)), win_h);/* bottom btn height + (genlist item height * msg count) */
	if (vd->reject_msg_height > (SCALE_SIZE((MTLOCK_REJECT_MSG_LIST_HEIGHT + REJ_MSG_LIST_CREATE_MSG_BTN_H), win_h))) {
		vd->reject_msg_height = SCALE_SIZE((MTLOCK_REJECT_MSG_LIST_HEIGHT + REJ_MSG_LIST_CREATE_MSG_BTN_H), win_h);
	}
}

static void __reject_msg_create_genlist(callui_view_incoming_call_h vd)
{
	int msg_cnt = 0;
	callui_app_data_t *ad = vd->base_view.ad;

	__reject_msg_genlist_create(vd);
	__reject_msg_genlist_init_item_class(vd);
	msg_cnt = __reject_msg_genlist_fill(vd);
	__reject_msg_genlist_deinit_item_class(vd);

	_callui_create_reject_msg_button(ad, vd->reject_msg_layout, "bottom_btn");

	__reject_msg_list_height_update(vd);

	if (msg_cnt < 2)
		elm_object_signal_emit(vd->reject_msg_layout, "set_1item_list", "");
	else if (msg_cnt <= CALLUI_REJ_MSG_MAX_COUNT) {
		char signal[16] = { 0 };
		snprintf(signal, 16, "set_%ditems_list", msg_cnt);
		elm_object_signal_emit(vd->reject_msg_layout, signal, "");
	}
	elm_object_tree_focus_allow_set(vd->reject_msg_genlist, EINA_FALSE);
}

static callui_result_e __update_displayed_data(callui_view_incoming_call_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	const callui_call_state_data_t *incom = _callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_TYPE_INCOMING);
	CALLUI_RETURN_VALUE_IF_FAIL(incom, CALLUI_RESULT_FAIL);

	const callui_call_state_data_t *active =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_TYPE_ACTIVE);
	const callui_call_state_data_t *held =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_TYPE_HELD);
	if (ad->second_call_popup && (!active && !held)) {
		DELETE_EVAS_OBJECT(ad->second_call_popup);
	}

	const char *file_path = incom->call_ct_info.caller_id_path;

	if (strcmp(file_path, "default") != 0) {
		_callui_show_caller_id(vd->caller_info, file_path);
	}

	const char *call_name = incom->call_ct_info.call_disp_name;
	const char *call_number = NULL;
	if (incom->call_disp_num[0] != '\0') {
		call_number = incom->call_disp_num;
	} else {
		call_number = incom->call_num;
	}
	if (strlen(call_name) == 0) {
		_callui_show_caller_info_name(ad, call_number);
		elm_object_signal_emit(vd->caller_info, "1line", "caller_name");
	} else {
		_callui_show_caller_info_name(ad, call_name);
		_callui_show_caller_info_number(ad, call_number);
		elm_object_signal_emit(vd->caller_info, "2line", "caller_name");
	}

	if (!__reject_msg_check_tel_num(call_number)) {
		DELETE_EVAS_OBJECT(vd->reject_msg_layout);
	} else {
		__reject_msg_create_genlist(vd);
	}

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static Eina_Bool __reject_msg_check_tel_num(const char *call_num)
{
	CALLUI_RETURN_VALUE_IF_FAIL(call_num, EINA_FALSE);

	if (strlen(call_num) <= 0) {
		info("Invalid number");
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static Evas_Object *__create_reject_msg_layout(callui_view_incoming_call_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->reject_msg_layout = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_LOCK_REJECT_WITH_MSG);

	evas_object_resize(vd->reject_msg_layout, ad->root_w, ad->root_h);

	__reject_msg_create_gesture_layer(vd);

	Evas_Object *reject_msg_bg = _callui_edje_object_part_get(vd->reject_msg_layout, "reject_msg_bg");
	evas_object_event_callback_add(reject_msg_bg, EVAS_CALLBACK_MOUSE_DOWN, __reject_msg_bg_mouse_down_cb, vd);
	evas_object_event_callback_add(reject_msg_bg, EVAS_CALLBACK_MOUSE_MOVE, __reject_msg_bg_mouse_move_cb, vd);
	evas_object_event_callback_add(reject_msg_bg, EVAS_CALLBACK_MOUSE_UP, __reject_msg_bg_mouse_up_cb, vd);

	elm_object_part_text_set(vd->reject_msg_layout, "reject_msg_text", _("IDS_VCALL_BUTTON2_REJECT_CALL_WITH_MESSAGE"));

	elm_object_signal_emit(vd->reject_msg_layout, "show-up-arrow", "reject_msg");
	evas_object_data_set(vd->reject_msg_layout, CALLUI_REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
	evas_object_hide(vd->dimming_ly);

	__reject_msg_list_param_reset(vd);

	evas_object_show(vd->reject_msg_layout);

	return vd->reject_msg_layout;
}

static callui_result_e __create_main_content(callui_view_incoming_call_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME, GRP_MAIN_LY);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content",  vd->base_view.contents);

	vd->caller_info = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_CALLER_INFO);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->caller_info, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(vd->base_view.contents, "caller_info", vd->caller_info);

	_callui_show_caller_info_status(ad, _("IDS_CALL_BODY_INCOMING_CALL"));

	callui_result_e res = _callui_view_circle_create_accept_layout(ad, vd, vd->base_view.contents);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);
	res = _callui_view_circle_create_reject_layout(ad, vd, vd->base_view.contents);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	vd->dimming_ly = _callui_load_edj(vd->base_view.contents, EDJ_NAME, GRP_DIMMING_LAYOUT);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->dimming_ly, CALLUI_RESULT_ALLOCATION_FAIL);
	evas_object_resize(vd->dimming_ly, ad->root_w, ad->root_h);
	evas_object_move(vd->dimming_ly, 0, 0);

	CALLUI_RETURN_VALUE_IF_FAIL(__create_reject_msg_layout(vd), CALLUI_RESULT_FAIL);

	return res;
}
