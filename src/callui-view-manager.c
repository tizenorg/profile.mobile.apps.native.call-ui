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
#include "callui-view-dialing.h"
#include "callui-view-single-call.h"
#include "callui-view-callend.h"
#include "callui-view-incoming-lock.h"
#include "callui-view-multi-call-split.h"
#include "callui-view-multi-call-conf.h"
#include "callui-view-multi-call-list.h"
#include "callui-view-quickpanel.h"

typedef call_view_data_t *(*new_view_data_cb) ();

struct _view_manager_data {
	new_view_data_cb func_new[VIEW_MAX];
	call_view_data_t *view_st[VIEW_MAX];
	callui_view_id_t view_before_top;
	callui_view_id_t view_top;
};

static void __callvm_init_view_register_function(view_manager_data_t *vm_data, callui_view_id_t view_id, call_view_data_t *(*view_new) ())
{
	vm_data->func_new[view_id] = view_new;
}

view_manager_data_t *_callvm_init()
{
	view_manager_data_t *vm_data = NULL;

	vm_data = (view_manager_data_t *) calloc(1, sizeof(view_manager_data_t));
	CALLUI_RETURN_VALUE_IF_FAIL(vm_data, NULL);
	__callvm_init_view_register_function(vm_data, VIEW_DIALLING_VIEW, _callui_dialing_view_dialing_new);
	__callvm_init_view_register_function(vm_data, VIEW_INCOMING_LOCK_VIEW, _callui_view_incoming_lock_new);
	__callvm_init_view_register_function(vm_data, VIEW_INCALL_ONECALL_VIEW, _callui_view_single_call_new);
	__callvm_init_view_register_function(vm_data, VIEW_INCALL_MULTICALL_SPLIT_VIEW, _callui_view_multi_call_split_new);
	__callvm_init_view_register_function(vm_data, VIEW_INCALL_MULTICALL_CONF_VIEW, _callui_view_multi_call_conf_new);
	__callvm_init_view_register_function(vm_data, VIEW_INCALL_MULTICALL_LIST_VIEW, _callui_view_multi_call_list_new);
	__callvm_init_view_register_function(vm_data, VIEW_QUICKPANEL_VIEW, _callui_view_qp_new);
	__callvm_init_view_register_function(vm_data, VIEW_ENDCALL_VIEW, _callui_view_callend_new);
	vm_data->view_top = VIEW_UNDEFINED_TYPE;
	return vm_data;
}

callui_view_id_t _callvm_get_top_view_id(view_manager_data_t *view_manager_handle)
{
	return view_manager_handle->view_top;
}

call_view_data_t *_callvm_get_call_view_data(void *appdata, callui_view_id_t view_id)
{

	callui_app_data_t *ad = (callui_app_data_t *) appdata;
	if (ad->view_manager_handle->view_st[view_id]) {
		return (call_view_data_t *) ad->view_manager_handle->view_st[view_id];
	}

	return NULL;
}

void _callvm_set_call_view_data(void *appdata, callui_view_id_t view_id, call_view_data_t *vd)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *) appdata;

	if (vd) {
		(ad->view_manager_handle->view_st[view_id]) = vd;
	}
}

Evas_Object *_callvm_get_view_layout(void *appdata)
{
	Evas_Object *layout = NULL;
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	CALLUI_RETURN_VALUE_IF_FAIL(ad, NULL);
	layout = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");

	return layout;
}


void _callvm_reset_call_view_data(void *appdata, callui_view_id_t view_id)
{
	dbg("..");
	callui_app_data_t *ad = (callui_app_data_t *) appdata;

	info("view:[%d] -> [%d]", ad->view_manager_handle->view_top, view_id);
	ad->view_manager_handle->view_st[view_id] = NULL;
}

static void _callvm_hide_view(call_view_data_t *view)
{
	CALLUI_RETURN_IF_FAIL(view);

	hide_cb hide = view->onHide;
	destroy_cb destroy = view->onDestroy;

	if (hide) {
		hide(view);
	}
	if (destroy) {
		destroy(view);
	}
}

static call_view_data_t *_callvm_show_view(call_view_data_t *view, new_view_data_cb func_new, unsigned int param1, void *param2, void *ad)
{
	if (view == NULL) {
		dbg("Create view data");

		view = func_new(ad);
	}
	CALLUI_RETURN_VALUE_IF_FAIL(view, NULL);

	if (view->layout == NULL) {
		dbg("Create layout");

		create_cb create = view->onCreate;
		CALLUI_RETURN_VALUE_IF_FAIL(create, NULL);
		create(view, param1, param2, ad);
	} else {
		dbg("Update layout");

		update_cb update = view->onUpdate;
		CALLUI_RETURN_VALUE_IF_FAIL(update, NULL);
		update(view, param2);
	}

	return view;
}

void _callvm_view_change(callui_view_id_t view_id, unsigned int param1, void *param2, void *appdata)
{
	CALLUI_RETURN_IF_FAIL(appdata);
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	info("view:[%d] -> [%d]", ad->view_manager_handle->view_top, view_id);
	if ((view_id <= VIEW_UNDEFINED_TYPE) || view_id >= VIEW_MAX) {
		err("[=========== ERROR!!!! Invalid View ID : %d =================]", view_id);
		return;
	}

	call_view_data_t **views = ad->view_manager_handle->view_st;

	callui_view_id_t last_view_id = ad->view_manager_handle->view_top;
	ad->view_manager_handle->view_before_top = last_view_id;	/* hold the current top window in the before_top pointer */
	ad->view_manager_handle->view_top = view_id;	/* set the new top window to the view_id which is passed... this step enables in setting 00:00:00 as timer */
	new_view_data_cb func_new = ad->view_manager_handle->func_new[view_id];


	if ((last_view_id != VIEW_UNDEFINED_TYPE) && (last_view_id != view_id)) {
		dbg("hide & destroy [%d]", last_view_id);
		_callvm_hide_view(views[last_view_id]);
		views[last_view_id] = NULL;
	}
	views[view_id] = _callvm_show_view(views[view_id], func_new, param1, param2, appdata);

	if (view_id == VIEW_DIALLING_VIEW || view_id == VIEW_INCOMING_LOCK_VIEW) {
		elm_win_activate(ad->win);
	}
	if (view_id != VIEW_ENDCALL_VIEW) {
		_callui_view_quickpanel_change();
	}

	evas_object_show(ad->win);
	dbg("End");
}

void _callvm_view_auto_change(void *appdata)
{
	dbg("_callvm_view_auto_change");
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	if (ad == NULL) {
		err("Invalid AppData");
		return;
	}

	if (ad->multi_call_list_end_clicked) {
		ad->multi_call_list_end_clicked = false;
		if (ad->active && ad->active->member_count > 1) {
			_callvm_view_change(VIEW_INCALL_MULTICALL_LIST_VIEW, 0, NULL, ad);
			return;
		}
	}

	if (ad->incom) {
		_callvm_view_change(VIEW_INCOMING_LOCK_VIEW, 0, NULL, ad);
		return;
	} else if (ad->active) {
		if (CM_CALL_STATE_DIALING == ad->active->call_state) {
			_callvm_view_change(VIEW_DIALLING_VIEW, 0, NULL, ad);
		} else if (ad->held) {
			_callvm_view_change(VIEW_INCALL_MULTICALL_SPLIT_VIEW, 0, NULL, ad);
		} else if (ad->active->member_count > 1) {
			_callvm_view_change(VIEW_INCALL_MULTICALL_CONF_VIEW, 0, NULL, ad);
		} else {
			_callvm_view_change(VIEW_INCALL_ONECALL_VIEW, 0, NULL, ad);
		}
		return;
	} else if (ad->held) {
		if (ad->held->member_count > 1) {
			_callvm_view_change(VIEW_INCALL_MULTICALL_CONF_VIEW, 0, NULL, ad);
		} else {
			_callvm_view_change(VIEW_INCALL_ONECALL_VIEW, 0, NULL, ad);
		}
	} else {
		err("No call exist");
	}
	return;
}

static void __callvm_terminate_app(void *data)
{
	dbg("..");
	lock_data_t *lock_h = data;
	_callui_lock_manager_destroy(lock_h);
	elm_exit();
}

void _callvm_terminate_app_or_view_change(void *appdata)
{
	callui_app_data_t *ad = (callui_app_data_t *)appdata;
	view_manager_data_t *vd = ad->view_manager_handle;
	int i = 0;
	if ((NULL == ad->active) && (NULL == ad->incom) && (NULL == ad->held)) {
		warn("No call exist. App will be terminated");
		for (i = 0; i < VIEW_MAX; i++) {
			if (vd->view_st[i] != NULL) {
				dbg("hide & destory [%d]", i);
				vd->view_st[i]->onHide(vd->view_st[i]);
				vd->view_st[i]->onDestroy(vd->view_st[i]);
			}
		}
		vd->view_top = VIEW_UNDEFINED_TYPE;
		if (_callui_lock_manager_is_lcd_off(ad->lock_handle)) {
			_callui_lock_manager_set_callback_on_unlock(ad->lock_handle, __callvm_terminate_app, ad->lock_handle);
		} else {
			__callvm_terminate_app(ad->lock_handle);
		}
	} else {
		_callvm_view_auto_change(ad);
	}
	return;
}

