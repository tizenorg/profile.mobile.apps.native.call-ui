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
#include "callui-view-incoming-lock.h"
#include "callui-view-elements.h"
#include "callui-view-layout-wvga.h"
#include "callui-common.h"

static void __callui_view_circle_mouse_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void __callui_view_circle_multi_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void __callui_view_circle_mouse_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void __callui_view_circle_multi_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void __callui_view_circle_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void __callui_view_circle_multi_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void __callui_view_circle_accept_down(call_view_data_t *vd, callui_app_data_t *ad, int x, int y);
static void __callui_view_circle_accept_move(call_view_data_t *vd, callui_app_data_t *ad, int x, int y);
static void __callui_view_circle_accept_up(call_view_data_t *vd, callui_app_data_t *ad, int x, int y);
static void __callui_view_circle_reject_down(call_view_data_t *vd, callui_app_data_t *ad, int x, int y);
static void __callui_view_circle_reject_move(call_view_data_t *vd, callui_app_data_t *ad, int x, int y);
static void __callui_view_circle_reject_up(call_view_data_t *vd, callui_app_data_t *ad, int x, int y);

static int accept_touch_num = -1;
static int reject_touch_num = -1;

static int accept_button_center_x = -1;
static int reject_button_center_x = -1;
static int button_center_y = -1;
static int button_radius = -1;
static int accept_outer_circle_x = -1;
static int reject_outer_circle_x = -1;
static int outer_circle_y = -1;
static int outer_circle_radius = -1;
static int outer_circle_width = -1;
static int outer_circle_height = -1;

static Eina_Bool baccept_clicked;
static Eina_Bool breject_clicked;

static void __callui_view_circle_handle_accept(call_view_data_t *vd, callui_app_data_t *ad)
{
	dbg("..");

	int ret = -1;
	if (_callui_common_get_idle_lock_type() == LOCK_TYPE_SWIPE_LOCK) {
		_callui_common_unlock_swipe_lock();
	}

	if (vd->type == VIEW_INCOMING_LOCK_VIEW) {
		if (ad->active == NULL) {
			dbg("No Call Or Held call - Accept");

			ret = cm_answer_call(ad->cm_handle, CALL_ANSWER_TYPE_NORMAL);
			if (ret != CM_ERROR_NONE) {
				err("cm_answer_call() is failed");
				return;
			}
		} else {
			dbg("Show popup - 2nd MT call - test volume popup");
			_callui_load_second_call_popup(ad);
		}
	}
}

static void __callui_view_circle_handle_reject(call_view_data_t *vd, callui_app_data_t *ad)
{
	dbg("..");
	int ret = -1;

	if (vd->type == VIEW_INCOMING_LOCK_VIEW) {
		ret = cm_reject_call(ad->cm_handle);
		if (ret != CM_ERROR_NONE) {
			err("cm_reject_call() is failed");
		}
	}
	return;
}

static Evas_Object *__callui_view_circle_get_accept_layout(void *data)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	Evas_Object *layout = NULL;
	switch (vd->type) {
	case VIEW_INCOMING_LOCK_VIEW:
		{
			layout = _callui_view_incoming_lock_get_accept_layout(vd);
		}
		break;
	default:
		err("vd type is error");
		return NULL;
	}
	return layout;
}

static void __callui_view_circle_set_accept_layout(void *data, Evas_Object *layout)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	switch (vd->type) {
	case VIEW_INCOMING_LOCK_VIEW:
		{
			_callui_view_incoming_lock_set_accept_layout(vd, layout);
		}
		break;
	default:
		err("vd type is error");
	}
}

static Evas_Object *__callui_view_circle_get_reject_layout(void *data)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	Evas_Object *layout = NULL;
	switch (vd->type) {
	case VIEW_INCOMING_LOCK_VIEW:
		{
			layout = _callui_view_incoming_lock_get_reject_layout(vd);
		}
		break;
	default:
		err("code should never reach here");
		return NULL;
	}
	return layout;
}

static void __callui_view_circle_set_reject_layout(void *data, Evas_Object *layout)
{
	call_view_data_t *vd = (call_view_data_t *)data;
	switch (vd->type) {
	case VIEW_INCOMING_LOCK_VIEW:
		{
			_callui_view_incoming_lock_set_reject_layout(vd, layout);
		}
		break;
	default:
		err("vd type is error");
	}
}

static Evas_Coord __callui_view_circle_get_distance(callui_app_data_t *ad, int centre_x, int point_x, int point_y)
{
	int centre_y = 0;

	centre_y = button_center_y;
	return (Evas_Coord)sqrt((point_x-centre_x)*(point_x-centre_x) + (point_y-centre_y)*(point_y-centre_y));
}

static void __callui_view_circle_accept_down(call_view_data_t *vd, callui_app_data_t *ad, int x, int y)
{
	dbg("..");
	Evas_Object *lock_accept = __callui_view_circle_get_accept_layout(vd);

	int point_distance = 0;

	point_distance = __callui_view_circle_get_distance(ad, accept_button_center_x, x, y);
	dbg("point_distance(%d)", point_distance);

	if (point_distance < button_radius) {
		dbg("Point lies inside the region");
		baccept_clicked = EINA_TRUE;
		elm_object_signal_emit(lock_accept, "outer_circle,show", "outer-circle");
	} else {
		warn("Point lies outside the region");
		baccept_clicked = EINA_FALSE;
	}
}

static void __callui_view_circle_mouse_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("");
	Evas_Event_Mouse_Down *ev = event_info;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCOMING_LOCK_VIEW);

	CALLUI_RETURN_IF_FAIL(vd);

	if (-1 == accept_touch_num && -1 == reject_touch_num) {
		__callui_view_circle_accept_down(vd, ad, ev->canvas.x, ev->canvas.y);
		if (TRUE == baccept_clicked) {
			accept_touch_num = 0;
		} else {
			__callui_view_circle_reject_down(vd, ad, ev->canvas.x, ev->canvas.y);
			if (TRUE == breject_clicked)
				reject_touch_num = 0;
		}
	}
}

static void __callui_view_circle_multi_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("..");
	Evas_Event_Multi_Down *ev = event_info;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCOMING_LOCK_VIEW);

	CALLUI_RETURN_IF_FAIL(vd);

	dbg("ev->device = %d, accept_touch_num = %d", ev->device, accept_touch_num);
	if (-1 == accept_touch_num && -1 == reject_touch_num) {
		__callui_view_circle_accept_down(vd, ad, ev->canvas.x, ev->canvas.y);
		if (TRUE == baccept_clicked) {
			accept_touch_num = ev->device;
		} else {
			__callui_view_circle_reject_down(vd, ad, ev->canvas.x, ev->canvas.y);
			if (TRUE == breject_clicked)
				reject_touch_num = ev->device;
		}
	}

}

static void __callui_view_circle_accept_move(call_view_data_t *vd, callui_app_data_t *ad, int x, int y)
{
	Evas_Object *lock_accept = __callui_view_circle_get_accept_layout(vd);

	Evas_Coord point_distance = 0;

	if (baccept_clicked) {
		Evas_Object *circle_bg = _callui_edje_object_part_get(lock_accept, "accept_inner_circle_bg");
		point_distance = __callui_view_circle_get_distance(ad, accept_button_center_x, x, y);

		if (point_distance < outer_circle_radius) {
			evas_object_move(circle_bg, accept_button_center_x-point_distance, button_center_y-point_distance);
			evas_object_resize(circle_bg, point_distance*2, point_distance*2);
			evas_object_image_fill_set(circle_bg, 0, 0, point_distance*2, point_distance*2);
		} else {
			evas_object_move(circle_bg, accept_outer_circle_x, outer_circle_y);
			evas_object_resize(circle_bg, outer_circle_width, outer_circle_height);
			evas_object_image_fill_set(circle_bg, 0, 0, outer_circle_width, outer_circle_height);
		}
	} else {
		warn("Initial click is not in correct region");
	}
}

static void __callui_view_circle_mouse_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Move *ev = event_info;
	CALLUI_RETURN_IF_FAIL(ev);
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);
	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCOMING_LOCK_VIEW);
	CALLUI_RETURN_IF_FAIL(vd);
	if (accept_touch_num == 0)
		__callui_view_circle_accept_move(vd, ad, ev->cur.canvas.x, ev->cur.canvas.y);
	else if (reject_touch_num == 0)
		__callui_view_circle_reject_move(vd, ad, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void __callui_view_circle_multi_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Multi_Move *ev = event_info;
	callui_app_data_t *ad = (callui_app_data_t *)data;
	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCOMING_LOCK_VIEW);
	dbg("ev->device = %d, accept_touch_num = %d", ev->device, accept_touch_num);
	if (vd) {
		if (accept_touch_num == ev->device)
			__callui_view_circle_accept_move(vd, ad, ev->cur.canvas.x, ev->cur.canvas.y);
		else if (reject_touch_num == ev->device)
			__callui_view_circle_reject_move(vd, ad, ev->cur.canvas.x, ev->cur.canvas.y);
	}

}

static void __callui_view_circle_accept_up(call_view_data_t *vd, callui_app_data_t *ad, int x, int y)
{
	dbg("..");
	Evas_Object *lock_accept = __callui_view_circle_get_accept_layout(vd);
	Evas_Coord point_distance = 0;

	if (baccept_clicked) {
		dbg("Initial click is in right region");
		Evas_Object *circle_bg = _callui_edje_object_part_get(lock_accept, "accept_inner_circle_bg");
		point_distance = __callui_view_circle_get_distance(ad, accept_button_center_x, x, y);
		if (point_distance >= outer_circle_radius) {
			evas_object_move(circle_bg, accept_outer_circle_x, outer_circle_y);
			evas_object_resize(circle_bg, outer_circle_width, outer_circle_height);
			evas_object_image_fill_set(circle_bg, 0, 0, outer_circle_width, outer_circle_height);

			dbg("__callui_view_circle_handle_accept");
			__callui_view_circle_handle_accept(vd, ad);
		} else {
			elm_object_signal_emit(lock_accept, "outer_circle,hide", "outer-circle");
		}
	} else {
		warn("Initial click is not in correct region");
	}
}

static void __callui_view_circle_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("..");
	Evas_Event_Mouse_Up *ev = event_info;
	CALLUI_RETURN_IF_FAIL(ev);
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);
	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCOMING_LOCK_VIEW);
	CALLUI_RETURN_IF_FAIL(vd);

	if (accept_touch_num == 0) {
		__callui_view_circle_accept_up(vd, ad, ev->canvas.x, ev->canvas.y);
		accept_touch_num = -1;
	} else if (reject_touch_num == 0) {
		__callui_view_circle_reject_up(vd, ad, ev->canvas.x, ev->canvas.y);
		reject_touch_num = -1;
	}
}

static void __callui_view_circle_multi_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("..");
	Evas_Event_Multi_Up *ev = event_info;
	CALLUI_RETURN_IF_FAIL(ev);
	callui_app_data_t *ad = (callui_app_data_t *)data;
	CALLUI_RETURN_IF_FAIL(ad);
	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCOMING_LOCK_VIEW);
	CALLUI_RETURN_IF_FAIL(vd);
	dbg("ev->device = %d", ev->device);

	if (accept_touch_num == ev->device) {
		__callui_view_circle_accept_up(vd, ad, ev->canvas.x, ev->canvas.y);
		accept_touch_num = -1;
	} else if (reject_touch_num == ev->device) {
		__callui_view_circle_reject_up(vd, ad, ev->canvas.x, ev->canvas.y);
		reject_touch_num = -1;
	}
}

static void __callui_view_circle_reject_down(call_view_data_t *vd, callui_app_data_t *ad, int x, int y)
{
	dbg("..");
	Evas_Object *lock_reject = __callui_view_circle_get_reject_layout(vd);
	int	point_distance = 0;

	point_distance = __callui_view_circle_get_distance(ad, reject_button_center_x, x, y);

	if (point_distance < button_radius) {
		dbg("Point lies inside the region");
		breject_clicked = EINA_TRUE;
		elm_object_signal_emit(lock_reject, "outer_circle,show", "outer-circle");
	} else {
		warn("Point lies outside the region");
		breject_clicked = EINA_FALSE;
	}
}

static void __callui_view_circle_reject_move(call_view_data_t *vd, callui_app_data_t *ad, int x, int y)
{
	Evas_Object *lock_reject = __callui_view_circle_get_reject_layout(vd);

	Evas_Coord	point_distance = 0;

	if (breject_clicked) {
		Evas_Object *circle_bg = _callui_edje_object_part_get(lock_reject, "reject_inner_circle_bg");

		point_distance = __callui_view_circle_get_distance(ad, reject_button_center_x, x, y);
		if (point_distance < outer_circle_radius) {
			evas_object_move(circle_bg, reject_button_center_x-point_distance, button_center_y-point_distance);
			evas_object_resize(circle_bg, point_distance*2, point_distance*2);
			evas_object_image_fill_set(circle_bg, 0, 0, point_distance*2, point_distance*2);
		} else {
			evas_object_move(circle_bg, reject_outer_circle_x, outer_circle_y);
			evas_object_resize(circle_bg, outer_circle_width, outer_circle_height);
			evas_object_image_fill_set(circle_bg, 0, 0, outer_circle_width, outer_circle_height);
		}
	} else {
		warn("Initial click is not in correct region");
	}
}

static void __callui_view_circle_reject_up(call_view_data_t *vd, callui_app_data_t *ad, int x, int y)
{
	dbg("..");
	Evas_Object *lock_reject = __callui_view_circle_get_reject_layout(vd);
	Evas_Coord	point_distance = 0;

	if (breject_clicked) {
		dbg("Initial click is in right region");
		Evas_Object *circle_bg = _callui_edje_object_part_get(lock_reject, "reject_inner_circle_bg");
		point_distance = __callui_view_circle_get_distance(ad, reject_button_center_x, x, y);
		if (point_distance >= outer_circle_radius) {
			evas_object_move(circle_bg, reject_outer_circle_x, outer_circle_y);
			evas_object_resize(circle_bg, outer_circle_width, outer_circle_height);
			evas_object_image_fill_set(circle_bg, 0, 0, outer_circle_width, outer_circle_height);
			__callui_view_circle_handle_reject(vd, ad);
		} else {
			elm_object_signal_emit(lock_reject, "outer_circle,hide", "outer-circle");
			elm_object_signal_emit(lock_reject, "inner_circle,show", "inner-circle");
		}
	} else {
		warn("Initial click is not in correct region");
	}
}

void _callui_view_circle_accept_reject_reset(void *data)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	Evas_Object *lock_accept = __callui_view_circle_get_accept_layout(vd);
	Evas_Object *lock_reject = __callui_view_circle_get_reject_layout(vd);

	baccept_clicked = EINA_FALSE;
	breject_clicked = EINA_FALSE;

	elm_object_signal_emit(lock_reject, "inner_circle,show", "inner-circle");
	elm_object_signal_emit(lock_accept, "inner_circle,show", "inner-circle");

}

static void __callui_view_circle_reject_focus_key_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("..");
	Evas_Event_Key_Down *ev = event_info;
	if (!ev) return;
	if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
		return;

	if ((!strcmp(ev->keyname, "Return")) ||
		(!strcmp(ev->keyname, "KP_Enter"))) {
		dbg("..");
	}
}

Evas_Object *_callui_view_circle_create_reject_layout(callui_app_data_t *ad, void *data)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	Evas_Object *lock_reject = __callui_view_circle_get_reject_layout(vd);
	Evas_Object *focus = NULL;
	Evas_Object *inner_circle = NULL;
	Evas_Object *outer_circle = NULL;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;

	if (lock_reject != NULL) {
		evas_object_del(lock_reject);
		__callui_view_circle_set_reject_layout(vd, NULL);
	}

	lock_reject = _callui_load_edj(ad->win, EDJ_NAME, GRP_LOCK_REJECT);
	evas_object_resize(lock_reject, ad->root_w, ad->root_h);
	evas_object_move(lock_reject, 0, 0);

	__callui_view_circle_set_reject_layout(vd, lock_reject);

	elm_object_signal_emit(lock_reject, "outer_circle,hide", "outer-circle");

	elm_object_part_text_set(lock_reject, "reject_text", _("IDS_CALL_BUTTON_REJECT"));

	evas_object_event_callback_add(lock_reject, EVAS_CALLBACK_MOUSE_DOWN, __callui_view_circle_mouse_down_cb, ad);
	evas_object_event_callback_add(lock_reject, EVAS_CALLBACK_MOUSE_MOVE, __callui_view_circle_mouse_move_cb, ad);
	evas_object_event_callback_add(lock_reject, EVAS_CALLBACK_MOUSE_UP, __callui_view_circle_mouse_up_cb, ad);
	evas_object_event_callback_add(lock_reject, EVAS_CALLBACK_MULTI_DOWN, __callui_view_circle_multi_down_cb, ad);
	evas_object_event_callback_add(lock_reject, EVAS_CALLBACK_MULTI_MOVE, __callui_view_circle_multi_move_cb, ad);
	evas_object_event_callback_add(lock_reject, EVAS_CALLBACK_MULTI_UP, __callui_view_circle_multi_up_cb, ad);

	focus = _callui_view_create_focus_layout(ad->win);
	evas_object_event_callback_add(focus, EVAS_CALLBACK_KEY_DOWN, __callui_view_circle_reject_focus_key_down, vd);
	elm_object_part_content_set(lock_reject, "reject_inner_circle_focus", focus);

	inner_circle = _callui_edje_object_part_get(lock_reject, "reject_inner_circle");
	evas_object_geometry_get(inner_circle, &x, &y, &width, &height);
	dbg("inner circle[%p] geometry: x = %d y = %d w1 =%d h1 =%d\n", inner_circle, x, y, width, height);
	reject_button_center_x = x+(width/2);
	dbg("Button center X[%d] Y[%d]", reject_button_center_x, button_center_y);

	outer_circle = _callui_edje_object_part_get(lock_reject, "rect_outer_circle");
	evas_object_geometry_get(outer_circle, &x, &y, &width, &height);
	reject_outer_circle_x = x;

	evas_object_show(lock_reject);

	return lock_reject;
}

static void __callui_view_circle_accept_focus_key_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("..");
	Evas_Event_Key_Down *ev = event_info;

	if (!ev) return;
	if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
		return;

	if ((!strcmp(ev->keyname, "Return")) ||
		(!strcmp(ev->keyname, "KP_Enter"))) {
		dbg("..");
	}
}

Evas_Object *_callui_view_circle_create_accept_layout(callui_app_data_t *ad, void *data)
{
	dbg("..");
	call_view_data_t *vd = (call_view_data_t *)data;
	Evas_Object *lock_accept = __callui_view_circle_get_accept_layout(vd);
	Evas_Object *focus = NULL;
	Evas_Object *inner_circle = NULL;
	Evas_Object *outer_circle = NULL;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;

	if (lock_accept != NULL) {
		evas_object_del(lock_accept);
		__callui_view_circle_set_accept_layout(vd, NULL);
	}

	lock_accept = _callui_load_edj(ad->win, EDJ_NAME, GRP_LOCK_ACCEPT);
	evas_object_resize(lock_accept, ad->root_w, ad->root_h);
	evas_object_move(lock_accept, 0, 0);

	__callui_view_circle_set_accept_layout(vd, lock_accept);

	elm_object_signal_emit(lock_accept, "outer_circle,hide", "outer-circle");

	elm_object_part_text_set(lock_accept, "accept_text", _("IDS_CALL_BUTTON_ACCEPT"));
	evas_object_event_callback_add(lock_accept, EVAS_CALLBACK_MOUSE_DOWN, __callui_view_circle_mouse_down_cb, ad);
	evas_object_event_callback_add(lock_accept, EVAS_CALLBACK_MOUSE_MOVE, __callui_view_circle_mouse_move_cb, ad);
	evas_object_event_callback_add(lock_accept, EVAS_CALLBACK_MOUSE_UP, __callui_view_circle_mouse_up_cb, ad);
	evas_object_event_callback_add(lock_accept, EVAS_CALLBACK_MULTI_DOWN, __callui_view_circle_multi_down_cb, ad);
	evas_object_event_callback_add(lock_accept, EVAS_CALLBACK_MULTI_MOVE, __callui_view_circle_multi_move_cb, ad);
	evas_object_event_callback_add(lock_accept, EVAS_CALLBACK_MULTI_UP, __callui_view_circle_multi_up_cb, ad);

	focus = _callui_view_create_focus_layout(ad->win);
	evas_object_event_callback_add(focus, EVAS_CALLBACK_KEY_DOWN, __callui_view_circle_accept_focus_key_down, ad);
	elm_object_part_content_set(lock_accept, "accept_inner_circle_focus", focus);
	elm_object_focus_set(focus, EINA_TRUE);

	inner_circle = _callui_edje_object_part_get(lock_accept, "accept_inner_circle");
	evas_object_geometry_get(inner_circle, &x, &y, &width, &height);
	dbg("inner circle[%p] geometry: x = %d y = %d w1 =%d h1 =%d\n", inner_circle, x, y, width, height);
	button_center_y = y+(height/2);
	accept_button_center_x = x+(width/2);
	button_radius = width/2;
	dbg("Button center X[%d] Y[%d]", accept_button_center_x, button_center_y);

	outer_circle = _callui_edje_object_part_get(lock_accept, "rect_outer_circle");
	evas_object_geometry_get(outer_circle, &x, &y, &width, &height);
	dbg("outer circle[%p] geometry: x = %d y = %d w1 =%d h1 =%d\n", outer_circle, x, y, width, height);
	accept_outer_circle_x = x;
	outer_circle_y = y;
	outer_circle_radius = width/2;
	outer_circle_width = width;
	outer_circle_height = height;
	dbg("Outer Circle X[%d] Y[%d] Radius[%d]", accept_outer_circle_x, outer_circle_y, outer_circle_radius);
	evas_object_show(lock_accept);

	return lock_accept;
}
