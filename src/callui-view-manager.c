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
#include "callui-view-incoming-call-noti.h"
#include "callui-view-incoming-call.h"
#include "callui-view-multi-call-split.h"
#include "callui-view-multi-call-conf.h"
#include "callui-view-multi-call-list.h"
#include "callui-view-quickpanel.h"
#include "callui-common.h"

typedef call_view_data_base_t *(*new_view_data_cb) ();

struct _callui_vm {
	call_view_data_base_t *cur_view;
	callui_view_type_e cur_view_type;
	callui_app_data_t *ad;
};
typedef struct _callui_vm callui_vm_t;

static int __callui_vm_destroy_cur_view(callui_vm_h vm);
static int __callui_vm_create_update_view(callui_vm_h vm, callui_view_type_e type);

static call_view_data_base_t *_allocate_view(callui_view_type_e view_type)
{
	switch (view_type)
	{
	case VIEW_TYPE_DIALLING:
		return (call_view_data_base_t *)_callui_dialing_view_dialing_new();
	case VIEW_TYPE_INCOMING_CALL_NOTI:
		return (call_view_data_base_t *)_callui_view_incoming_call_noti_new();
	case VIEW_TYPE_INCOMING_CALL:
		return (call_view_data_base_t *)_callui_view_incoming_call_new();
	case VIEW_TYPE_SINGLECALL:
		return (call_view_data_base_t *)_callui_view_single_call_new();
	case VIEW_TYPE_MULTICALL_SPLIT:
		return (call_view_data_base_t *)_callui_view_multi_call_split_new();
	case VIEW_TYPE_MULTICALL_CONF:
		return (call_view_data_base_t *)_callui_view_multi_call_conf_new();
	case VIEW_TYPE_MULTICALL_LIST:
		return (call_view_data_base_t *)_callui_view_multi_call_list_new();
	case VIEW_TYPE_ENDCALL:
		return (call_view_data_base_t *)_callui_view_callend_new();
	default:
		return NULL;
	}
	return NULL;
}

callui_vm_h _callui_vm_create(callui_app_data_t *ad)
{
	CALLUI_RETURN_VALUE_IF_FAIL(ad, NULL);

	callui_vm_h vm = calloc(1, sizeof(callui_vm_t));

	CALLUI_RETURN_VALUE_IF_FAIL(vm, NULL);

	vm->cur_view_type = VIEW_TYPE_UNDEFINED;
	vm->ad = ad;

	return vm;
}

void _callui_vm_destroy(callui_vm_h vm)
{
	CALLUI_RETURN_IF_FAIL(vm);

	__callui_vm_destroy_cur_view(vm);

	free(vm);
}

callui_view_type_e _callui_vm_get_cur_view_type(callui_vm_h vm)
{
	if (!vm) {
		err("vm is NULL");
		return VIEW_TYPE_UNDEFINED;
	}
	return vm->cur_view_type;
}

static int __callui_vm_destroy_cur_view(callui_vm_h vm)
{
	int res = CALLUI_RESULT_FAIL;

	call_view_data_base_t *view = vm->cur_view;

	CALLUI_RETURN_VALUE_IF_FAIL(view, CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(view->onDestroy, CALLUI_RESULT_FAIL);

	if (view->onDestroy) {
		res = view->onDestroy(view);
	}

	vm->cur_view = NULL;
	vm->cur_view_type = VIEW_TYPE_UNDEFINED;

	return res;
}

static int __callui_vm_create_update_view(callui_vm_h vm, callui_view_type_e type)
{
	call_view_data_base_t *view = vm->cur_view;
	int res;

	if (!view) {
		dbg("Try create new view [%d]", type);

		view = _allocate_view(type);
		CALLUI_RETURN_VALUE_IF_FAIL(view, CALLUI_RESULT_FAIL);

		if (!view->onCreate) {
			err("Create callback is NULL");
			free(view);
			return CALLUI_RESULT_FAIL;
		}

		res = view->onCreate(view, vm->ad);

		if (res != CALLUI_RESULT_OK) {
			err("onCreate callback failed! res[%d]", res);
			free(view);
			return CALLUI_RESULT_FAIL;
		}
		vm->cur_view = view;

	} else {
		dbg("Try update view [%d]", type);
		CALLUI_RETURN_VALUE_IF_FAIL(view->onUpdate, CALLUI_RESULT_OK);
		view->onUpdate(view);
	}
	return CALLUI_RESULT_OK;
}

int _callui_vm_change_view(callui_vm_h vm, callui_view_type_e type)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vm, CALLUI_RESULT_INVALID_PARAM);

	if ((type <= VIEW_TYPE_UNDEFINED) || (type >= VIEW_TYPE_MAX)) {
		err("Invalid view type [%d]", type);
		return CALLUI_RESULT_INVALID_PARAM;
	}

	info("Change view: [%d] -> [%d]", vm->cur_view_type, type);

	int res;
	callui_view_type_e last_view_type = vm->cur_view_type;

	if ((last_view_type != VIEW_TYPE_UNDEFINED) && (last_view_type != type)) {
		dbg("destroy [%d]", last_view_type);
		res = __callui_vm_destroy_cur_view(vm);
		CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);
	}

	res = __callui_vm_create_update_view(vm, type);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	vm->cur_view_type = type;

	if (type == VIEW_TYPE_DIALLING || type == VIEW_TYPE_INCOMING_CALL) {
		elm_win_activate(vm->ad->win);
	}

	if (type == VIEW_TYPE_INCOMING_CALL_NOTI) {
		vm->ad->incoming_noti = true;
	}

	if (type != VIEW_TYPE_ENDCALL) {
		// TODO: temp solution. Need to be replaced in future
		_callui_qp_mc_update(vm->ad->qp_minicontrol);
	}

	evas_object_show(vm->ad->win);
	return CALLUI_RESULT_OK;
}

int _callui_vm_auto_change_view(callui_vm_h vm)
{
	CALLUI_RETURN_VALUE_IF_FAIL(vm, CALLUI_RESULT_INVALID_PARAM);

	callui_app_data_t *ad = vm->ad;

	if (ad->multi_call_list_end_clicked) {
		ad->multi_call_list_end_clicked = false;
		if (ad->active && ad->active->member_count > 1) {
			return _callui_vm_change_view(vm, VIEW_TYPE_MULTICALL_LIST);
		}
	}

	int res;
	if (ad->incom) {

		callui_view_type_e type = VIEW_TYPE_INCOMING_CALL;

#ifdef ACTIVE_NOTIFICATION_AVAILABLE
		if (_callui_common_get_idle_lock_type() == LOCK_TYPE_UNLOCK &&
				ad->active == NULL &&
				ad->held == NULL &&
				ad->incoming_noti == false) {
			type = VIEW_TYPE_INCOMING_CALL_NOTI;
		}
#endif
		res =_callui_vm_change_view(ad->view_manager_handle, type);
	} else if (ad->active) {
		if (CM_CALL_STATE_DIALING == ad->active->call_state) {
			res = _callui_vm_change_view(vm, VIEW_TYPE_DIALLING);
		} else if (ad->held) {
			res = _callui_vm_change_view(vm, VIEW_TYPE_MULTICALL_SPLIT);
		} else if (ad->active->member_count > 1) {
			res = _callui_vm_change_view(vm, VIEW_TYPE_MULTICALL_CONF);
		} else {
			res = _callui_vm_change_view(vm, VIEW_TYPE_SINGLECALL);
		}
	} else if (ad->held) {
		if (ad->held->member_count > 1) {
			res = _callui_vm_change_view(vm, VIEW_TYPE_MULTICALL_CONF);
		} else {
			res = _callui_vm_change_view(vm, VIEW_TYPE_SINGLECALL);
		}
	} else {
		err("No call exist");
		res = CALLUI_RESULT_FAIL;
	}
	return res;
}
