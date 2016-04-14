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

#include "callui-view-lock-screen.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-view-dialing.h"
#include "callui-view-single-call.h"
#include "callui-view-multi-call-conf.h"
#include "callui-view-multi-call-split.h"
#include "callui-view-layout.h"
#include "callui-keypad.h"
#include "callui-view-manager.h"
#include "callui-view-elements.h"
#include "callui-common.h"
#include "callui-sound-manager.h"

typedef struct _lock_sccreen_data {
	Evas_Object *layout;
	Evas_Object *hit_rect;

	Ecore_Timer *no_lock_timer;
	bool is_locked;

} lock_screen_data_t;

static bool __callui_lock_screen_create_layout(lock_screen_data_t *priv);

/**
 * @brief Start timer
 *
 * @param[in] lock_screen_priv      Lock screen
 *
 */
static void __callui_lock_screen_start_timer(lock_screen_data_t *lock_screen_priv);

/**
 * @brief Stop Timer
 *
 * @param[in] lock_screen_priv      Lock screen
 *
 */
static void __callui_lock_screen_stop_timer(lock_screen_data_t *lock_screen_priv);

/**
 * @brief Delete screen layout
 *
 * @param[in] lock_h      Lock screen handle
 *
 */
static void __callui_lock_screen_delete_layout(void *lock_h);

/**
 * @brief Check if lock screen is shown
 *
 * @param[in] lock_h      Lock screen handle
 *
 * @return Lock screen state
 *
 */
static bool __callui_lock_screen_is_lockscreen_shown(void *lock_h);

/**
 * @brief Show lock screen layout
 *
 * @param[in]    lock_screen_priv      Lock screen
 *
 */
static void __callui_lock_screen_show_layout(lock_screen_data_t *lock_screen_priv);

/**
 * @brief Hide lock screen layout
 *
 * @param[in]    lock_screen_priv      Lock screen
 *
 */
static void __callui_lock_screen_hide_layout(lock_screen_data_t *lock_screen_priv);
static void *__callui_lock_screen_create();
static void __callui_lock_screen_start(void *lock_h);
static void __callui_lock_screen_stop(void *lock_h, bool force);
static bool __callui_lock_screen_is_lcd_off(void *lock_h);
static void __callui_lock_screen_set_callback_on_unlock(void *lock_h, unlock_cb_t callback, void* data){};

static void __callui_lock_screen_show_layout(lock_screen_data_t *lock_screen_priv)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_screen_priv);
	callui_app_data_t *ad = _callui_get_app_data();

	callui_audio_state_type_e audio_state = _callui_sdm_get_audio_state(ad->sound_manager);
	if (audio_state == CALLUI_AUDIO_STATE_SPEAKER) {
		dbg("Speaker ON. Do not display lockscreen.");
		return;
	}
	if (EINA_TRUE == _callui_keypad_get_show_status(ad->keypad)) {
		dbg("Keypad is ON. Do not display lockscreen.");
		return;
	}

	evas_object_raise(lock_screen_priv->layout);
	evas_object_show(lock_screen_priv->layout);

#ifdef _DBUS_DISPLAY_DEVICE_TIMEOUT_
	if (_callui_vm_get_cur_view_type(ad->view_manager) != CALLUI_VIEW_DIALLING) {
		dbg("lcd show");
		callui_app_data_t *ad = _callui_get_app_data();
		_callui_display_set_timeout(ad->display, CALLUI_DISPLAY_TIMEOUT_LS_SET);
	}
#endif

	lock_screen_priv->is_locked = true;
	return;
}

static void __callui_lock_screen_hide_layout(lock_screen_data_t *lock_screen_priv)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_screen_priv);
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);

	evas_object_hide(lock_screen_priv->layout);
	lock_screen_priv->is_locked = false;

#ifdef _DBUS_DISPLAY_DEVICE_TIMEOUT_
	if (_callui_vm_get_cur_view_type(ad->view_manager) != CALLUI_VIEW_DIALLING) {
		dbg("lcd hide");
		callui_app_data_t *ad = _callui_get_app_data();
		_callui_display_set_timeout(ad->display, CALLUI_DISPLAY_TIMEOUT_SET);
	}
#endif

}

static Evas_Object *__callui_lock_screen_create_contents(Evas_Object *parent, char *grpname)
{
	Evas_Object *eo = NULL;

	/* load edje */
	eo = _callui_load_edj(parent, EDJ_NAME, grpname);
	if (eo == NULL)
		return NULL;
	return eo;
}

static Eina_Bool __lock_timeout_cb(void *data)
{
	dbg("__lock_timeout_cb");
	lock_screen_data_t *priv = (lock_screen_data_t *)data;
	callui_app_data_t *ad = _callui_get_app_data();

	priv->no_lock_timer = NULL;

	callui_view_type_e type = _callui_vm_get_cur_view_type(ad->view_manager);
	if (type >= CALLUI_VIEW_MULTICALL_LIST) {
		return ECORE_CALLBACK_CANCEL;
	} else if (type == CALLUI_VIEW_INCOMING_CALL || type == CALLUI_VIEW_INCOMING_CALL_NOTI) {
		return ECORE_CALLBACK_RENEW;
	}

	__callui_lock_screen_show_layout(priv);
	return ECORE_CALLBACK_CANCEL;
}

static void __callui_lock_screen_icon_double_clicked_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
	dbg("__callui_lock_screen_icon_double_clicked_cb");
	lock_screen_data_t *priv = (lock_screen_data_t *)data;

	__callui_lock_screen_hide_layout(priv);
	priv->no_lock_timer = ecore_timer_add(3.0, __lock_timeout_cb, priv);
	return;
}

static void __callui_lock_screen_user_action_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	dbg("__callui_lock_screen_user_action_cb");
	lock_screen_data_t *priv = (lock_screen_data_t *)data;
	DELETE_ECORE_TIMER(priv->no_lock_timer);
	priv->no_lock_timer = ecore_timer_add(3.0, __lock_timeout_cb, priv);

	return;
}

static bool __callui_lock_screen_create_layout(lock_screen_data_t *priv)
{
	dbg( "lock screen create!!");
	callui_app_data_t *ad = _callui_get_app_data();
	Evas_Object *layout = NULL;
	Evas_Object *hit_rect = NULL;
	CALLUI_RETURN_VALUE_IF_FAIL(priv, false);

	/* Make Hit rectangle to refresh lock timer */
	hit_rect = evas_object_rectangle_add(evas_object_evas_get(ad->win));
	evas_object_color_set(hit_rect, 0, 0, 0, 0);
	evas_object_resize(hit_rect, ad->root_w, ad->root_h);
	evas_object_move(hit_rect, 0, 0);
	evas_object_repeat_events_set(hit_rect, EINA_TRUE);
	evas_object_propagate_events_set(hit_rect, EINA_FALSE);
	evas_object_event_callback_add(hit_rect, EVAS_CALLBACK_MOUSE_DOWN, __callui_lock_screen_user_action_cb, priv);
	evas_object_show(hit_rect);
	priv->hit_rect = hit_rect;

	layout = __callui_lock_screen_create_contents(ad->main_ly, "lock-screen");
	if (NULL == layout) {
		warn("layout NULL!!!");
	}

	evas_object_resize(layout, ad->root_w, ad->root_h);
	evas_object_move(layout, 0, 0);
	elm_object_domain_translatable_part_text_set(layout, "lock-text", CALLUI_TEXT_DOMAIN, "IDS_CALL_NPBODY_DOUBLE_TAP_THE_LOCK_ICON_TO_UNLOCK_YOUR_DEVICE");

	edje_object_signal_callback_add(_EDJ(layout), "mouse,down,1,double", "lock-icon", __callui_lock_screen_icon_double_clicked_cb, priv);

	priv->layout = layout;

	__callui_lock_screen_show_layout(priv);

	return true;
}

static void __callui_lock_screen_start_timer(lock_screen_data_t *lock_screen_priv)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_screen_priv);

	DELETE_ECORE_TIMER(lock_screen_priv->no_lock_timer);

	lock_screen_priv->no_lock_timer = ecore_timer_add(3.0, __lock_timeout_cb, lock_screen_priv);
	return;
}

static void __callui_lock_screen_stop_timer(lock_screen_data_t *lock_screen_priv)
{
	CALLUI_RETURN_IF_FAIL(lock_screen_priv);

	DELETE_ECORE_TIMER(lock_screen_priv->no_lock_timer);
	return;
}

static void __callui_lock_screen_delete_layout(void *lock_h)
{
	dbg("lock screen delete!!");
	CALLUI_RETURN_IF_FAIL(lock_h);
	lock_screen_data_t *lock_screen_handle = (lock_screen_data_t *)lock_h;
	evas_object_del(lock_screen_handle->layout);
	evas_object_del(lock_screen_handle->hit_rect);
	DELETE_ECORE_TIMER(lock_screen_handle->no_lock_timer);
	free(lock_screen_handle);

	return;
}

static bool __callui_lock_screen_is_lockscreen_shown(void *lock_h)
{
	dbg("..");
	CALLUI_RETURN_VALUE_IF_FAIL(lock_h, false);
	lock_screen_data_t *lock_screen_handle = (lock_screen_data_t *)lock_h;
	return lock_screen_handle->is_locked;
}

static void *__callui_lock_screen_create()
{
	lock_screen_data_t *lock_screen_handle = calloc(1, sizeof(lock_screen_data_t));
	CALLUI_RETURN_NULL_IF_FAIL(lock_screen_handle);
	bool ret = __callui_lock_screen_create_layout(lock_screen_handle);
	if (!ret) {
		free(lock_screen_handle);
		return NULL;
	}
	__callui_lock_screen_stop(lock_screen_handle, true);
	return lock_screen_handle;
}

static void __callui_lock_screen_start(void *lock_h)
{
	CALLUI_RETURN_IF_FAIL(lock_h);
	lock_screen_data_t *lock_screen_handle = (lock_screen_data_t *)lock_h;
	__callui_lock_screen_show_layout(lock_screen_handle);
	__callui_lock_screen_start_timer(lock_screen_handle);
}

static void __callui_lock_screen_stop(void *lock_h, bool force)
{
	CALLUI_RETURN_IF_FAIL(lock_h);
	lock_screen_data_t *lock_screen_handle = (lock_screen_data_t *)lock_h;
	__callui_lock_screen_hide_layout(lock_screen_handle);
	__callui_lock_screen_stop_timer(lock_screen_handle);
}

static bool __callui_lock_screen_is_lcd_off(void *lock_h)
{
	return false;
}

void _callui_lock_screen_init(lock_data_t *lock_h)
{
	lock_h->create = __callui_lock_screen_create;
	lock_h->start = __callui_lock_screen_start;
	lock_h->stop = __callui_lock_screen_stop;
	lock_h->is_started = __callui_lock_screen_is_lockscreen_shown;
	lock_h->is_lcd_off = __callui_lock_screen_is_lcd_off;
	lock_h->set_unlock_cb = __callui_lock_screen_set_callback_on_unlock;
	lock_h->destroy = __callui_lock_screen_delete_layout;
}
