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
#include "callui-debug.h"
#include "callui-view-dialing.h"
#include "callui-view-single-call.h"
#include "callui-view-callend.h"
#include "callui-view-incoming-call-noti.h"
#include "callui-view-incoming-call.h"
#include "callui-view-multi-call-split.h"
#include "callui-view-multi-call-conf.h"
#include "callui-view-multi-call-list.h"
#include "callui-view-quickpanel.h"
#include "callui-common.h"
#include "callui-manager.h"
#include "callui-state-provider.h"

typedef call_view_data_base_t *(*new_view_data_cb) ();

struct _callui_vm {
	call_view_data_base_t *cur_view;
	callui_view_type_e cur_view_type;
	callui_app_data_t *ad;

	bool conf_call_ended;
	bool paused;
	bool check_conf_memeber_count;
};
typedef struct _callui_vm callui_vm_t;

static callui_result_e __callui_vm_init(callui_vm_h vm, callui_app_data_t *ad);
static void __callui_vm_deinit(callui_vm_h vm);

static callui_result_e __destroy_cur_view(callui_vm_h vm);
static callui_result_e __create_update_view(callui_vm_h vm, callui_view_type_e type);
static call_view_data_base_t *__allocate_view(callui_view_type_e view_type);
static callui_result_e __change_view(callui_vm_h vm, callui_view_type_e type);
static void __update_cur_view(callui_vm_h vm);
static callui_result_e __auto_change_view(callui_vm_h vm, callui_call_data_t *call_data);
static void __call_state_event_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info);
static void __end_call_called_cb(void *user_data, unsigned int call_id, callui_call_release_type_e release_type);

static call_view_data_base_t *__allocate_view(callui_view_type_e view_type)
{
	switch (view_type)
	{
	case CALLUI_VIEW_DIALLING:
		return (call_view_data_base_t *)_callui_dialing_view_dialing_new();
	case CALLUI_VIEW_INCOMING_CALL_NOTI:
		return (call_view_data_base_t *)_callui_view_incoming_call_noti_new();
	case CALLUI_VIEW_INCOMING_CALL:
		return (call_view_data_base_t *)_callui_view_incoming_call_new();
	case CALLUI_VIEW_SINGLECALL:
		return (call_view_data_base_t *)_callui_view_single_call_new();
	case CALLUI_VIEW_MULTICALL_SPLIT:
		return (call_view_data_base_t *)_callui_view_multi_call_split_new();
	case CALLUI_VIEW_MULTICALL_CONF:
		return (call_view_data_base_t *)_callui_view_multi_call_conf_new();
	case CALLUI_VIEW_MULTICALL_LIST:
		return (call_view_data_base_t *)_callui_view_multi_call_list_new();
	case CALLUI_VIEW_ENDCALL:
		return (call_view_data_base_t *)_callui_view_callend_new();
	default:
		return NULL;
	}
	return NULL;
}

static void _lock_manager_unlock_cb(void *data)
{
	callui_app_data_t *ad = data;

	if (_callui_stp_is_any_calls_available(ad->state_provider)) {
		return;
	}
	__change_view(ad->view_manager, CALLUI_VIEW_ENDCALL);
}

static callui_result_e __auto_change_view(callui_vm_h vm, callui_call_data_t *call_data)
{
	callui_app_data_t *ad = vm->ad;
	callui_result_e res = CALLUI_RESULT_FAIL;

	const callui_call_data_t *active =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	const callui_call_data_t *held =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
	const callui_call_data_t *incom =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);

	if (vm->conf_call_ended && call_data) {
		if (!ad->end_call_data) {
			ad->end_call_data = calloc(1, sizeof(callui_call_data_t));
			CALLUI_RETURN_VALUE_IF_FAIL(ad->end_call_data, CALLUI_RESULT_ALLOCATION_FAIL);
		}
		memcpy(ad->end_call_data, call_data, sizeof(callui_call_data_t));
		res = __change_view(vm, CALLUI_VIEW_ENDCALL);
		vm->conf_call_ended = false;
		return res;
	}

	if (vm->check_conf_memeber_count) {
		vm->check_conf_memeber_count = false;

		if (active && active->conf_member_count > 1) {
			return __change_view(vm, CALLUI_VIEW_MULTICALL_LIST);
		}
	}

	if (incom) {
		callui_view_type_e type = CALLUI_VIEW_INCOMING_CALL;
		callui_view_type_e cur_type = _callui_vm_get_cur_view_type(ad->view_manager);
		if (_callui_common_get_idle_lock_type() == LOCK_TYPE_UNLOCK &&
				active == NULL &&
				held == NULL &&
				(cur_type == CALLUI_VIEW_UNDEFINED || cur_type == CALLUI_VIEW_ENDCALL)) {
			type = CALLUI_VIEW_INCOMING_CALL_NOTI;
		}
		res =__change_view(ad->view_manager, type);
	} else if (active) {
		if (active->is_dialing) {
			res = __change_view(vm, CALLUI_VIEW_DIALLING);
		} else if (held) {
			res = __change_view(vm, CALLUI_VIEW_MULTICALL_SPLIT);
		} else if (active->conf_member_count > 1) {
			res = __change_view(vm, CALLUI_VIEW_MULTICALL_CONF);
		} else {
			res = __change_view(vm, CALLUI_VIEW_SINGLECALL);
		}
	} else if (held) {
		if (held->conf_member_count > 1) {
			res = __change_view(vm, CALLUI_VIEW_MULTICALL_CONF);
		} else {
			res = __change_view(vm, CALLUI_VIEW_SINGLECALL);
		}
	} else {
		if (call_data && call_data->type != CALLUI_CALL_DATA_INCOMING) {

			if (!ad->end_call_data) {
				ad->end_call_data = calloc(1, sizeof(callui_call_data_t));
				CALLUI_RETURN_VALUE_IF_FAIL(ad->end_call_data, CALLUI_RESULT_ALLOCATION_FAIL);
			}
			memcpy(ad->end_call_data, call_data, sizeof(callui_call_data_t));

			if (_callui_lock_manager_is_lcd_off(ad->lock_handle)) {
				_callui_lock_manager_set_callback_on_unlock(ad->lock_handle, _lock_manager_unlock_cb, ad);
			} else {
				_callui_lock_manager_stop(ad->lock_handle);
				res = __change_view(vm, CALLUI_VIEW_ENDCALL);
			}
		} else {
			_callui_common_exit_app();
		}
	}
	return res;
}

static void __call_state_event_cb(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_vm_h vm = user_data;
	callui_app_data_t *ad = vm->ad;

	if (vm->cur_view_type == CALLUI_VIEW_ENDCALL) {
		switch (call_event_type) {
		case CALLUI_CALL_EVENT_END:
			dbg("Ignored. Already in end call view.");
			return;
		case CALLUI_CALL_EVENT_INCOMING:
			elm_object_signal_emit(ad->main_ly, "maximize_no_anim", "app_main_ly");
			break;
		default:
			break;
		}
		_callui_action_bar_set_disabled_state(ad->action_bar, false);
	}
	__auto_change_view(vm, event_info);
}

static void __end_call_called_cb(void *user_data, unsigned int call_id, callui_call_release_type_e release_type)
{
	CALLUI_RETURN_IF_FAIL(user_data);
	callui_vm_h vm = user_data;

	if (release_type == CALLUI_CALL_RELEASE_BY_CALL_HANDLE) {
		if (vm->cur_view_type == CALLUI_VIEW_MULTICALL_LIST) {
			vm->check_conf_memeber_count = true;
		}
		return;
	}

	callui_app_data_t *ad = vm->ad;

	const callui_call_data_t *active =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	const callui_call_data_t *held =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_HELD);
	const callui_call_data_t *incom =
			_callui_stp_get_call_data(ad->state_provider, CALLUI_CALL_DATA_INCOMING);

	if ((active && !held && !incom && (active->conf_member_count > 1)) ||
			(held && !active && !incom && (held->conf_member_count > 1))) {
		vm->conf_call_ended = true;
	}
}

static callui_result_e __callui_vm_init(callui_vm_h vm, callui_app_data_t *ad)
{
	vm->cur_view_type = CALLUI_VIEW_UNDEFINED;
	vm->ad = ad;
	vm->paused = true;

	callui_result_e res = _callui_stp_add_call_state_event_cb(ad->state_provider, __call_state_event_cb, vm);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = _callui_manager_add_end_call_called_cb(ad->call_manager, __end_call_called_cb, vm);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return res;
}

static void __callui_vm_deinit(callui_vm_h vm)
{
	callui_app_data_t *ad = vm->ad;

	_callui_stp_remove_call_state_event_cb(ad->state_provider, __call_state_event_cb, vm);
	_callui_manager_remove_end_call_called_cb(ad->call_manager, __end_call_called_cb, vm);

	__destroy_cur_view(vm);
}

callui_vm_h _callui_vm_create(callui_app_data_t *ad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad, NULL);

	callui_vm_h vm = calloc(1, sizeof(callui_vm_t));
	CALLUI_RETURN_VALUE_IF_FAIL(vm, NULL);

	callui_result_e res = __callui_vm_init(vm, ad);
	if (res != CALLUI_RESULT_OK) {
		FREE(vm);
	}

	return vm;
}

void _callui_vm_destroy(callui_vm_h vm)
{
	CALLUI_RETURN_IF_FAIL(vm);

	__callui_vm_deinit(vm);

	free(vm);
}

callui_view_type_e _callui_vm_get_cur_view_type(callui_vm_h vm)
{
	if (!vm) {
		err("vm is NULL");
		return CALLUI_VIEW_UNDEFINED;
	}
	return vm->cur_view_type;
}

static callui_result_e __destroy_cur_view(callui_vm_h vm)
{
	callui_result_e res = CALLUI_RESULT_FAIL;
	call_view_data_base_t *view = vm->cur_view;

	if (!view) {
		dbg("Current view is NULL");
		return CALLUI_RESULT_OK;
	}

	if (view->destroy) {
		res = view->destroy(view);
	} else {
		warn("destroy() is not set! Possible memory leak");
	}

	vm->cur_view = NULL;
	vm->cur_view_type = CALLUI_VIEW_UNDEFINED;

	return res;
}

static callui_result_e __create_update_view(callui_vm_h vm, callui_view_type_e type)
{
	call_view_data_base_t *view = vm->cur_view;
	callui_result_e res;

	if (!view) {
		dbg("Try create new view [%d]", type);

		view = __allocate_view(type);
		CALLUI_RETURN_VALUE_IF_FAIL(view, CALLUI_RESULT_FAIL);

		if (!view->create) {
			err("create() is NULL");
			free(view);
			return CALLUI_RESULT_FAIL;
		}

		res = view->create(view, vm->ad);

		if (res != CALLUI_RESULT_OK) {
			err("create() failed! res[%d]", res);
			free(view);
			return CALLUI_RESULT_FAIL;
		}
		vm->cur_view = view;

	} else {
		vm->cur_view->update_flags |= CALLUI_UF_DATA_REFRESH;
		if (!vm->paused) {
			__update_cur_view(vm);
		}
	}
	return CALLUI_RESULT_OK;
}

static callui_result_e __change_view(callui_vm_h vm, callui_view_type_e type)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vm, CALLUI_RESULT_INVALID_PARAM);

	if ((type <= CALLUI_VIEW_UNDEFINED) || (type >= CALLUI_VIEW_COUNT)) {
		err("Invalid view type [%d]", type);
		return CALLUI_RESULT_INVALID_PARAM;
	}
	info("Change view: [%d] -> [%d]", vm->cur_view_type, type);

	callui_result_e res;
	callui_view_type_e last_view_type = vm->cur_view_type;

	if ((last_view_type != CALLUI_VIEW_UNDEFINED) && (last_view_type != type)) {
		dbg("destroy [%d]", last_view_type);
		res = __destroy_cur_view(vm);
		CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);
	}

	res = __create_update_view(vm, type);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	vm->cur_view_type = type;

	if (type == CALLUI_VIEW_DIALLING
			|| type == CALLUI_VIEW_INCOMING_CALL
			|| type == CALLUI_VIEW_INCOMING_CALL_NOTI) {
		elm_win_activate(vm->ad->win);
	}

	evas_object_show(vm->ad->win);

	return res;
}

callui_result_e _callui_vm_change_view(callui_vm_h vm, callui_view_type_e type)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vm, CALLUI_RESULT_INVALID_PARAM);

	if ((type <= CALLUI_VIEW_UNDEFINED) || (type >= CALLUI_VIEW_COUNT)) {
		err("Invalid view type [%d]", type);
		return CALLUI_RESULT_INVALID_PARAM;
	}

	return __change_view(vm, type);
}

callui_result_e _callui_vm_auto_change_view(callui_vm_h vm)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vm, CALLUI_RESULT_INVALID_PARAM);

	return __auto_change_view(vm, NULL);
}

static void __update_cur_view(callui_vm_h vm)
{
	call_view_data_base_t *cur_view = vm->cur_view;

	if (cur_view && cur_view->update && cur_view->update_flags) {
		cur_view->update(vm->cur_view);
		cur_view->update_flags = 0;
	}
}

callui_result_e _callui_vm_pause(callui_vm_h vm)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vm, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(!vm->paused, CALLUI_RESULT_FAIL);

	vm->paused = true;

	call_view_data_base_t *cur_view = vm->cur_view;

	if (cur_view && cur_view->pause) {
		cur_view->pause(vm->cur_view);
	}

	return CALLUI_RESULT_OK;
}

callui_result_e _callui_vm_resume(callui_vm_h vm)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vm, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(vm->paused, CALLUI_RESULT_FAIL);

	vm->paused = false;

	call_view_data_base_t *cur_view = vm->cur_view;
	if (cur_view && cur_view->resume) {
		cur_view->resume(vm->cur_view);
	}
	__update_cur_view(vm);

	return CALLUI_RESULT_OK;
}

callui_result_e _callui_vm_update_language(callui_vm_h vm)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vm, CALLUI_RESULT_INVALID_PARAM);

	call_view_data_base_t *cur_view = vm->cur_view;

	if (cur_view) {
		cur_view->update_flags |= CALLUI_UF_LANG_CHANGE;
		if (!vm->paused) {
			__update_cur_view(vm);
		}
	}

	return CALLUI_RESULT_OK;
}
