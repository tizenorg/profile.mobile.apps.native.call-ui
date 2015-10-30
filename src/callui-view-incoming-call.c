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

#include "callui-view-manager.h"
#include "callui-view-circle.h"
#include "callui-view-incoming-call.h"
#include "callui-view-incoming-lock.h"
#include "callui-view-layout-wvga.h"
#include "callui-view-elements.h"
#include <vconf.h>
#include "callui-common.h"

#define REJ_MSG_GENLIST_DATA "reject_msg_genlist_data"
#define REJ_MSG_LIST_OPEN_STATUS_KEY "list_open_status_key"
#define VIEW_INCOMING_LOCK_LAYOUT_ID "INCOMINGLOCKVIEW"
#define MAX_MSG_COUNT 6

static Evas_Object *__callui_view_incoming_call_create_reject_msg_layout(void *data);
static Eina_Bool __callui_view_incoming_lock_reject_msg_available(callui_app_data_t *ad, char *call_num);

#define SCALE_SIZE(x, h) (((x) * (h)) / MAIN_SCREEN_BIG_H)

static Evas_Object *__callui_view_incoming_call_create_contents(void *data, char *grpname)
{
	if (data == NULL) {
		err("ERROR");
		return NULL;
	}
	callui_app_data_t *ad = (callui_app_data_t *)data;
	Evas_Object *eo;

	/* load edje */
	eo = _callui_load_edj(ad->main_ly, EDJ_NAME, grpname);
	if (eo == NULL)
		return NULL;

	return eo;
}

static void __reject_msg_list_param_reset(void *data)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;

	priv->y_momentum = 0;
	priv->reject_with_msg_start_y = 0;
	priv->reject_with_msg_cur_y = 0;
	priv->bmouse_down_pressed = EINA_FALSE;
}

static void __reject_screen_transit_complete_cb(void *data, Elm_Transit *transit)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;

	if (EINA_FALSE == evas_object_data_get(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY)) {
		elm_object_tree_focus_allow_set(priv->msg_glist, EINA_FALSE);
		elm_object_signal_emit(priv->lock_reject_with_msg, "show-up-arrow", "reject_msg");
		evas_object_hide(priv->dimming_ly);
	} else {
		elm_object_tree_focus_allow_set(priv->msg_glist, EINA_TRUE);
		elm_object_signal_emit(priv->lock_reject_with_msg, "show-down-arrow", "reject_msg");
		evas_object_show(priv->dimming_ly);
	}

	__reject_msg_list_param_reset(vd);

	return;
}

static Eina_Bool __rej_msg_show_sliding_effect(void *data)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	Evas_Object *screen_ly;
	Elm_Transit *transit;
	int xpos = 0;
	int ypos = 0;
	int width = 0;
	int height = 0;
	int transit_y = 0;
	int max_height_limit = 0;

	screen_ly = priv->lock_reject_with_msg;
	transit = elm_transit_add();
	elm_transit_object_add(transit, screen_ly);

	evas_object_geometry_get(priv->lock_reject_with_msg, &xpos, &ypos, &width, &height);
	dbg("reject_w_msg dimensions ---> x[%d] y[%d] w[%d] h[%d]", xpos, ypos, width, height);
	dbg("priv->y_momentum: %d", priv->y_momentum);

	/*Max height possible*/
	max_height_limit = (priv->msg_list_height);
	dbg("max_height_limit: %d", max_height_limit);

	if (EINA_FALSE == evas_object_data_get(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY)) {
		dbg("Close list... Check if opening is feasible");

		if (priv->y_momentum) {
			dbg("Momentum...");

			if (priv->y_momentum < -500) {
				dbg("Huge Momentum... Move the layout");

				/*effect to pull up the window.*/
				transit_y = -(max_height_limit + ypos);

				elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
				evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
			} else {
				dbg("Small Momentum..");

				if (ypos != 0) {	/*Reject msg layout is displaced from its original position*/
					if (-ypos < max_height_limit/2) {	/*Layout position is lesser than half of the height*/
						dbg("Movement L.T. HALF the height..");

						/*effect to pull down the window.*/
						transit_y = -ypos;

						elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
						evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
					} else if (-ypos >= max_height_limit/2 && -ypos <= max_height_limit) {	/*Layout position is greater than half of the height*/
						dbg("Movement G.T. HALF the height..");

						/*effect to pull up the window.*/
						transit_y = -(max_height_limit + ypos);

						elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
						evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
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
					evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
				} else if (-ypos >= max_height_limit/2 && -ypos <= max_height_limit) {	/*Layout position is greater than half of the height*/
					dbg("Movement G.T. HALF the height..");

					/*effect to pull up the window.*/
					transit_y = -(max_height_limit + ypos);

					elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
					evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
				}
			}
		}
	} else {
		dbg("Open list... Check if closing is feasible");

		if (priv->y_momentum) {
			dbg("Momentum...");

			if (priv->y_momentum > 500) {
				dbg("Huge Momentum... Move the layout");

				/*effect to pull down the window.*/
				transit_y = -ypos;

				elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
				evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
			} else {
				dbg("Small Momentum..");

				if (ypos != 0) {	/*Reject msg layout is displaced from its original position*/
					if (-ypos < (max_height_limit * 0.8)) {	/*Layout position is lesser than 80% of the height*/
						dbg("Movement L.T. 80 percent of the height..");

						/*effect to pull down the window.*/
						transit_y = -ypos;

						elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
						evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
					} else if (-ypos >= (max_height_limit * 0.8) && -ypos <= max_height_limit) {	/*Layout position is greater than half of the height*/
						dbg("Movement G.T. 80 percent of the height..");

						/*effect to pull up the window.*/
						transit_y = -(max_height_limit + ypos);

						elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
						evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
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
					evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
				} else if (-ypos >= (max_height_limit * 0.8) && -ypos <= max_height_limit) {	/*Layout position is greater than half of the height*/
					dbg("Movement G.T. 80 percent of the height..");

					/*effect to pull up the window.*/
					transit_y = -(max_height_limit + ypos);

					elm_transit_effect_translation_add(transit, 0, 0, 0, transit_y);
					evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
				}
			}
		}
	}

	elm_transit_del_cb_set(transit, __reject_screen_transit_complete_cb, vd);

	if (priv->y_momentum < 0)
		priv->y_momentum = -priv->y_momentum;

	if (priv->y_momentum < 1500) {
		elm_transit_duration_set(transit, 0.5);
	} else if (priv->y_momentum >= 1500 && priv->y_momentum < 3000) {
		elm_transit_duration_set(transit, 0.4);
	} else if (priv->y_momentum >= 3000 && priv->y_momentum < 4500) {
		elm_transit_duration_set(transit, 0.3);
	} else if (priv->y_momentum >= 4500) {
		elm_transit_duration_set(transit, 0.2);
	}
	evas_object_show(screen_ly);	/*It must be called before elm_transit_go(). transit policy*/
	elm_transit_objects_final_state_keep_set(transit, EINA_TRUE);	/*Keep the window position as end of effect.*/
	elm_transit_go(transit);

	return ECORE_CALLBACK_CANCEL;
}

static void __callui_view_incoming_call_reject_with_msg_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;

	if (priv->bmouse_down_pressed) {
		ecore_idle_enterer_before_add(__rej_msg_show_sliding_effect, vd);
	} else {
		dbg("mouse down was NOT pressed - DONT handle up event");
	}
}

static void __callui_view_incoming_call_handle_reject_with_msg_event(void *data)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(data != NULL);

	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	int max_height_limit = 0;

	/*Max height possible*/
	max_height_limit = priv->msg_list_height;
	dbg("max_height_limit: %d", max_height_limit);

	dbg("mouse down was pressed - handle move event");

	if (EINA_FALSE == evas_object_data_get(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY)) {
		evas_object_move(priv->lock_reject_with_msg, 0, -max_height_limit);
		evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
		__reject_screen_transit_complete_cb(vd, NULL);
	} else {
		/*Special case - Move the max distance - msg-list height*/
		evas_object_move(priv->lock_reject_with_msg, 0, 0);
		evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
		__reject_screen_transit_complete_cb(vd, NULL);
	}
}

static void __callui_view_incoming_call_reject_msg_focus_key_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Key_Down *ev = event_info;
	if (!ev) return;
	if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
		return;

	if ((!strcmp(ev->keyname, "Return")) ||
		(!strcmp(ev->keyname, "KP_Enter"))) {
		dbg("..");
		__callui_view_incoming_call_handle_reject_with_msg_event(data);
	}
}

static Evas_Event_Flags __reject_msg_flick_gesture_move_event_cb(void *data, void *event_info)
{
	dbg("Flick_Gesture Move");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	Elm_Gesture_Line_Info *info = (Elm_Gesture_Line_Info *)event_info;

	dbg("*********************************************");
	dbg("info->angle = %lf", info->angle);
	dbg("info->momentum.mx = %d, info->momentum.my = %d", info->momentum.mx, info->momentum.my);
	dbg("info->momentum.n = %d", info->momentum.n);
	dbg("info->momentum.tx = %d, info->momentum.ty = %d", info->momentum.tx, info->momentum.ty);
	dbg("info->momentum.x1 = %d, info->momentum.x2 = %d", info->momentum.x1, info->momentum.x2);
	dbg("info->momentum.y1 = %d, info->momentum.y2 = %d", info->momentum.y1, info->momentum.y2);
	dbg("*********************************************");

	priv->y_momentum = info->momentum.my;

	return EVAS_EVENT_FLAG_NONE;
}

static void __reject_msg_create_gesture_layer(void *data)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	Evas_Object *rej_msg_bg = NULL;

	rej_msg_bg = _callui_edje_object_part_get(priv->lock_reject_with_msg, "reject_msg_bg");
	if (priv->reject_msg_gl) {
		evas_object_del(priv->reject_msg_gl);
		priv->reject_msg_gl = NULL;
	}

	priv->reject_msg_gl = elm_gesture_layer_add(priv->lock_reject_with_msg);
	if (FALSE == elm_gesture_layer_attach(priv->reject_msg_gl, rej_msg_bg)) {
		dbg("elm_gesture_layer_attach failed !!");
		evas_object_del(priv->reject_msg_gl);
		priv->reject_msg_gl = NULL;
	} else {
		elm_gesture_layer_cb_set(priv->reject_msg_gl, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_MOVE, __reject_msg_flick_gesture_move_event_cb, vd);
	}
}

static void __callui_view_incoming_call_reject_with_msg_mouse_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	Evas_Event_Mouse_Move *ev = event_info;

	priv->reject_with_msg_start_y = ev->cur.canvas.y;
	priv->bmouse_down_pressed = EINA_TRUE;
}

static void __callui_view_incoming_call_reject_with_msg_mouse_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	Evas_Event_Mouse_Move *ev = event_info;
	int diff_y = 0;
	int max_height_limit = 0;

	/*Max height possible*/
	max_height_limit = priv->msg_list_height;
	dbg("max_height_limit: %d", max_height_limit);

	if (!priv->bmouse_down_pressed) {
		dbg("mouse down was NOT pressed - DONT handle move event");
		return;
	}
	dbg("mouse down was pressed - handle move event");
	priv->reject_with_msg_cur_y = ev->cur.canvas.y;

	diff_y = priv->reject_with_msg_cur_y - priv->reject_with_msg_start_y;
	dbg("diff_y [<<< %d >>>>]", diff_y);

	if (EINA_FALSE == evas_object_data_get(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY)) {
		elm_object_signal_emit(priv->lock_reject_with_msg, "show-down-arrow", "reject_msg");
		if ((diff_y > -max_height_limit) && (diff_y <= 0)) {
			/*Lies between 0 and msg-list layout height*/
			evas_object_move(priv->lock_reject_with_msg, 0, diff_y);
		} else if (diff_y <= -max_height_limit) {
			/*Special case - Move the max distance - msg-list height*/
			evas_object_move(priv->lock_reject_with_msg, 0, -max_height_limit);
			evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_TRUE);
			__reject_screen_transit_complete_cb(vd, NULL);
		}
	} else {
		if ((diff_y >= 0) && (diff_y < max_height_limit)) {
			/*Lies between 0 and msg-list layout height*/
			evas_object_move(priv->lock_reject_with_msg, 0, -(max_height_limit - diff_y));
		} else if (diff_y >= max_height_limit) {
			/*Special case - Move the max distance - msg-list height*/
			evas_object_move(priv->lock_reject_with_msg, 0, 0);
			evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, (const void *)EINA_FALSE);
			__reject_screen_transit_complete_cb(vd, NULL);
		}
	}
}

static void __reject_msg_list_height_update(void *data)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;

	int msg_cnt = 0;
	if (0 != vconf_get_int(VCONFKEY_CISSAPPL_REJECT_CALL_MSG_INT, &msg_cnt)) {
		warn("vconf_get_int failed.");
	}
	if (msg_cnt == 0)
		msg_cnt = 1;

	int win_h = 0;

	// TODO ecore_x_window_size_get is not supported. Need to move on system settings.
	//ecore_x_window_size_get(ecore_x_window_root_first_get(), NULL, &win_h);
	win_h = 1280;

	priv->msg_list_height = SCALE_SIZE((REJ_MSG_LIST_CREATE_MSG_BTN_H + ((ITEM_SIZE_H) * msg_cnt)), win_h);/* bottom btn height + (genlist item height * msg count) */
	if (priv->msg_list_height > (SCALE_SIZE((MTLOCK_REJECT_MSG_LIST_HEIGHT + REJ_MSG_LIST_CREATE_MSG_BTN_H), win_h))) {
		priv->msg_list_height = SCALE_SIZE((MTLOCK_REJECT_MSG_LIST_HEIGHT + REJ_MSG_LIST_CREATE_MSG_BTN_H), win_h);
	}
}

static void __reject_msg_create_glist(void *data)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	CALLUI_RETURN_IF_FAIL(vd);
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	CALLUI_RETURN_IF_FAIL(priv);
	Elm_Object_Item *item = NULL;
	int msg_cnt = 0;
	int index = 0;

	/* msg count from setting */
	if (0 != vconf_get_int(VCONFKEY_CISSAPPL_REJECT_CALL_MSG_INT, &msg_cnt)) {
		warn("vconf_get_int failed.");
	}
	dbg("msg_cnt: %d", msg_cnt);

	/* create gen list */
	if (priv->msg_glist) {
		evas_object_del(priv->msg_glist);
		priv->msg_glist = NULL;
	}

	priv->msg_glist = elm_genlist_add(priv->lock_reject_with_msg);
	elm_genlist_homogeneous_set(priv->msg_glist, EINA_TRUE);
	elm_genlist_mode_set(priv->msg_glist, ELM_LIST_COMPRESS);
	elm_scroller_content_min_limit(priv->msg_glist, EINA_FALSE, EINA_TRUE);
	CALLUI_RETURN_IF_FAIL(priv->msg_glist);
	//elm_genlist_realization_mode_set(priv->msg_glist, EINA_TRUE);

	evas_object_data_set(priv->msg_glist, REJ_MSG_GENLIST_DATA, (const void *)vd);
	evas_object_size_hint_weight_set(priv->msg_glist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(priv->msg_glist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_part_content_set(priv->lock_reject_with_msg, "swl_msglist", priv->msg_glist);

	priv->itc_reject_msg = _callui_view_incoming_lock_create_item_class();

	if (msg_cnt == 0) {
		index = -1;
		item = _callui_view_incoming_lock_append_genlist_item(priv->msg_glist, priv->itc_reject_msg, index);
		if (item) {
			elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		}
	} else {
		for (index = 0; index < msg_cnt; index++) {
			_callui_view_incoming_lock_append_genlist_item(priv->msg_glist, priv->itc_reject_msg, index);
		}
	}
	elm_genlist_item_class_free(priv->itc_reject_msg);
	priv->itc_reject_msg = NULL;

	_callui_view_incoming_lock_create_reject_msg_button(priv->lock_reject_with_msg, "bottom_btn", data);

	/*Adjust the list height*/
	__reject_msg_list_height_update(vd);
	if (msg_cnt < 2)
		elm_object_signal_emit(priv->lock_reject_with_msg, "set_1item_list", "");
	else if (msg_cnt <= MAX_MSG_COUNT) {
		char signal[16] = {0,};
		snprintf(signal, 16, "set_%ditems_list", msg_cnt);
		elm_object_signal_emit(priv->lock_reject_with_msg, signal, "");
	}
	elm_object_tree_focus_allow_set(priv->msg_glist, EINA_FALSE);
}

void _callui_view_incoming_call_draw_screen(callui_app_data_t *ad, call_view_data_t *vd)
{
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	Evas_Object *eo = priv->contents;
	char *file_path = NULL;
	call_data_t *call_data = NULL;

	call_data = ad->incom;
	if (call_data == NULL) {
		err("call data is null");
		return;
	}

	file_path = call_data->call_ct_info.caller_id_path;

	sec_dbg("file_path: %s", file_path);
	if (strcmp(file_path, "default") != 0) {
		_callui_show_caller_id(priv->caller_info, file_path);
	}

	char *call_name = call_data->call_ct_info.call_disp_name;
	char *call_number = NULL;
	if (call_data->call_disp_num[0] != '\0') {
		call_number = call_data->call_disp_num;
	} else {
		call_number = call_data->call_num;
	}
	if (strlen(call_name) == 0) {
		_callui_show_caller_info_name(ad, call_number);
		elm_object_signal_emit(priv->caller_info, "1line", "caller_name");
	} else {
		_callui_show_caller_info_name(ad, call_name);
		_callui_show_caller_info_number(ad, call_number);
		elm_object_signal_emit(priv->caller_info, "2line", "caller_name");
	}

	_callui_show_caller_info_status(ad, _("IDS_CALL_BODY_INCOMING_CALL"));

	_callui_view_circle_create_accept_layout(ad, vd);
	_callui_view_circle_create_reject_layout(ad, vd);

	priv->dimming_ly = __callui_view_incoming_call_create_contents(ad, "dimming_ly");
	evas_object_resize(priv->dimming_ly, ad->root_w, ad->root_h);
	evas_object_move(priv->dimming_ly, 0, 0);

	if (__callui_view_incoming_lock_reject_msg_available(ad, call_number)) {
		__callui_view_incoming_call_create_reject_msg_layout(vd);
	}

	evas_object_show(eo);
}

static Eina_Bool __callui_view_incoming_lock_reject_msg_available(callui_app_data_t *ad, char *call_num)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad, EINA_FALSE);
	CALLUI_RETURN_VALUE_IF_FAIL(call_num, EINA_FALSE);

	if (strlen(call_num) <= 0) {
		info("Invalid number");
		return EINA_FALSE;
	}
/*	else if (ad->b_msg_restricted) {
		CALL_UI_DEBUG(VC_LOG_WARN, "MDM");
		return EINA_FALSE;
	}
*/
	return EINA_TRUE;
}

static void __callui_view_incoming_lock_reject_msg_close(void *data)
{
	CALLUI_RETURN_IF_FAIL(data != NULL);
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;

	if (EINA_FALSE != evas_object_data_get(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY)) {
		evas_object_move(priv->lock_reject_with_msg, 0, 0);
		elm_object_signal_emit(priv->lock_reject_with_msg, "show-up-arrow-landscape", "reject_msg");
		evas_object_hide(priv->dimming_ly);
		evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, EINA_FALSE);
	}
}

static void __callui_view_incoming_call_hw_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ev = event_info;
	if (ev->button == 3) {
		/* Handle mouse right button event */
		__callui_view_incoming_lock_reject_msg_close(data);
	}
}

void _callui_view_incoming_call_ondestroy(void *data)
{
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)data;
	callui_app_data_t *ad = _callui_get_app_data();
	if (priv->lock_reject_with_msg) {
		evas_object_event_callback_del(priv->lock_reject_with_msg, EVAS_CALLBACK_MOUSE_UP,
				__callui_view_incoming_call_hw_mouse_up_cb);
		evas_object_event_callback_del(ad->win_conformant, EVAS_CALLBACK_MOUSE_UP,
				__callui_view_incoming_call_hw_mouse_up_cb);

		evas_object_del(priv->lock_reject_with_msg);
		priv->lock_reject_with_msg = NULL;
		evas_object_del(priv->dimming_ly);
		priv->dimming_ly = NULL;
	}
}

static Evas_Object *__callui_view_incoming_call_create_reject_msg_layout(void *data)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *)vd->priv;
	callui_app_data_t *ad = _callui_get_app_data();
	Evas_Object *rej_msg_bg = NULL;
	Evas_Object *focus = NULL;

	if (priv->lock_reject_with_msg != NULL) {
		evas_object_event_callback_del(priv->lock_reject_with_msg,
				EVAS_CALLBACK_MOUSE_UP, __callui_view_incoming_call_hw_mouse_up_cb);
		evas_object_event_callback_del(ad->win_conformant,
				EVAS_CALLBACK_MOUSE_UP, __callui_view_incoming_call_hw_mouse_up_cb);

		evas_object_del(priv->lock_reject_with_msg);
		priv->lock_reject_with_msg = NULL;
	}
	priv->lock_reject_with_msg = _callui_load_edj(ad->win, EDJ_NAME, GRP_LOCK_REJECT_WITH_MSG);

	focus = _callui_view_create_focus_layout(priv->lock_reject_with_msg);

	elm_object_part_content_set(priv->lock_reject_with_msg, "reject_msg_focus", focus);
	evas_object_event_callback_add(focus, EVAS_CALLBACK_KEY_DOWN,
			__callui_view_incoming_call_reject_msg_focus_key_down, vd);

	evas_object_resize(priv->lock_reject_with_msg, ad->root_w, ad->root_h);

	__reject_msg_create_gesture_layer(vd);

	rej_msg_bg = _callui_edje_object_part_get(priv->lock_reject_with_msg, "reject_msg_bg");
	evas_object_event_callback_add(rej_msg_bg, EVAS_CALLBACK_MOUSE_DOWN, __callui_view_incoming_call_reject_with_msg_mouse_down_cb, vd);
	evas_object_event_callback_add(rej_msg_bg, EVAS_CALLBACK_MOUSE_MOVE, __callui_view_incoming_call_reject_with_msg_mouse_move_cb, vd);
	evas_object_event_callback_add(rej_msg_bg, EVAS_CALLBACK_MOUSE_UP, __callui_view_incoming_call_reject_with_msg_mouse_up_cb, vd);

	elm_object_part_text_set(priv->lock_reject_with_msg, "reject_msg_text", _("IDS_VCALL_BUTTON2_REJECT_CALL_WITH_MESSAGE"));

	elm_object_signal_emit(priv->lock_reject_with_msg, "show-up-arrow", "reject_msg");
	evas_object_data_set(priv->lock_reject_with_msg, REJ_MSG_LIST_OPEN_STATUS_KEY, EINA_FALSE);
	evas_object_hide(priv->dimming_ly);

	__reject_msg_list_param_reset(vd);

	evas_object_event_callback_add(ad->win_conformant, EVAS_CALLBACK_MOUSE_UP, __callui_view_incoming_call_hw_mouse_up_cb, vd);
	evas_object_event_callback_add(priv->lock_reject_with_msg, EVAS_CALLBACK_MOUSE_UP, __callui_view_incoming_call_hw_mouse_up_cb, vd);

	evas_object_show(priv->lock_reject_with_msg);
	_callui_view_incoming_lock_reject_msg_create_call_setting_handle(vd);
	__reject_msg_create_glist(vd);
	return priv->lock_reject_with_msg;
}

int _callui_view_incoming_call_oncreate(call_view_data_t *view_data, void *appdata)
{
	dbg("mt-lock view create!!");

	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	dbg(" active %d", ad->active);
	dbg(" incoming %d", ad->incom);
	incoming_lock_view_priv_t *priv = (incoming_lock_view_priv_t *) view_data->priv;

	evas_object_resize(ad->win, ad->root_w, ad->root_h);
	evas_object_pointer_mode_set(ad->win, EVAS_OBJECT_POINTER_MODE_NOGRAB);
	if (ad->main_ly) {
		priv->contents = __callui_view_incoming_call_create_contents(ad, GRP_MAIN_LY);
		elm_object_part_content_set(ad->main_ly, "elm.swallow.content",  priv->contents);

		priv->caller_info = elm_object_part_content_get(priv->contents, "caller_info");
		if (!priv->caller_info) {
			priv->caller_info = __callui_view_incoming_call_create_contents(ad, GRP_CALLER_INFO);
			elm_object_part_content_set(priv->contents, "caller_info", priv->caller_info);
		}
		elm_object_signal_emit(priv->contents, "mt_circle_bg_show", "mt_view");
		_callui_destroy_end_call_button(priv->contents);
		evas_object_name_set(priv->caller_info, VIEW_INCOMING_LOCK_LAYOUT_ID);
	}
	if (_callui_lock_manager_is_started(ad->lock_handle) == TRUE) {
		_callui_lock_manager_force_stop(ad->lock_handle);
	}

	return 0;
}
