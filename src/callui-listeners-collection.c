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

#include <stdarg.h>

#include "callui-listeners-collection.h"
#include "callui-debug.h"

static callui_result_e __search_listener(Eina_List *listener_list, void *cb_func, void *cb_data);
static void __delete_empty_listeners(_callui_listeners_coll_t *listeners_coll);

callui_result_e _callui_listeners_coll_init(_callui_listeners_coll_t *listeners_coll)
{
	CALLUI_RETURN_VALUE_IF_FAIL(listeners_coll, CALLUI_RESULT_INVALID_PARAM);

	listeners_coll->list = NULL;
	listeners_coll->is_need_validate = false;
	listeners_coll->is_locked = false;
	listeners_coll->is_initialized = true;

	return CALLUI_RESULT_OK;
}

callui_result_e _callui_listeners_coll_deinit(_callui_listeners_coll_t *listeners_coll)
{
	CALLUI_RETURN_VALUE_IF_FAIL(listeners_coll, CALLUI_RESULT_INVALID_PARAM);

	Eina_List *l;
	_callui_listener_t *data;

	EINA_LIST_FOREACH(listeners_coll->list, l, data) {
		free(data);
	}
	listeners_coll->list = eina_list_free(listeners_coll->list);

	listeners_coll->is_need_validate = false;
	listeners_coll->is_locked = false;
	listeners_coll->is_initialized = false;

	return CALLUI_RESULT_OK;
}

static callui_result_e __search_listener(Eina_List *listener_list, void *cb_func, void *cb_data)
{
	Eina_List *l;
	_callui_listener_t *data;

	EINA_LIST_FOREACH(listener_list, l, data) {
		if (data && data->cb_func == cb_func && data->cb_data == cb_data) {
			return CALLUI_RESULT_ALREADY_REGISTERED;
		}
	}
	return CALLUI_RESULT_OK;
}

static void __delete_empty_listeners(_callui_listeners_coll_t *listeners_coll)
{
	Eina_List *l;
	Eina_List *l_next;
	_callui_listener_t *data;

	EINA_LIST_FOREACH_SAFE(listeners_coll->list, l, l_next, data) {
		if (data == NULL) {
			listeners_coll->list = eina_list_remove_list(listeners_coll->list, l);
		}
	}
	debug_leave();
}

callui_result_e _callui_listeners_coll_add_listener(_callui_listeners_coll_t *listeners_coll,
		cb_func_handler func_handler,
		void *cb_func,
		void *cb_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(listeners_coll, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(func_handler, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(cb_func, CALLUI_RESULT_INVALID_PARAM);

	CALLUI_RETURN_VALUE_IF_FAIL(listeners_coll->is_initialized, CALLUI_RESULT_FAIL);

	callui_result_e res = __search_listener(listeners_coll->list, cb_func, cb_data);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	_callui_listener_t *listener = (_callui_listener_t *)calloc(1, sizeof(_callui_listener_t));
	CALLUI_RETURN_VALUE_IF_FAIL(listener, CALLUI_RESULT_ALLOCATION_FAIL);

	listener->handler = func_handler;
	listener->cb_func = cb_func;
	listener->cb_data = cb_data;

	listeners_coll->list = eina_list_append(listeners_coll->list, listener);
	if (listeners_coll->list == NULL) {
		err("Listener list is empty");
		free(listener);
		return CALLUI_RESULT_FAIL;
	}
	return res;
}

callui_result_e _callui_listeners_coll_call_listeners(_callui_listeners_coll_t *listeners_coll, ...)
{
	CALLUI_RETURN_VALUE_IF_FAIL(listeners_coll, CALLUI_RESULT_INVALID_PARAM);

	CALLUI_RETURN_VALUE_IF_FAIL(listeners_coll->is_initialized, CALLUI_RESULT_FAIL);

	listeners_coll->is_locked = true;

	Eina_List *l;
	_callui_listener_t *data;

	EINA_LIST_FOREACH(listeners_coll->list, l, data) {
		if(data != NULL) {
			dbg("calling listener start...");
			va_list args;
			va_start(args, listeners_coll);
			data->handler(data, args);
			va_end(args);
			dbg("... calling listener done");
		}
	}

	listeners_coll->is_locked = false;

	if (listeners_coll->is_need_validate) {
		dbg("need validate");
		__delete_empty_listeners(listeners_coll);
		listeners_coll->is_need_validate = false;
	}
	debug_leave();
	return CALLUI_RESULT_OK;
}

callui_result_e _callui_listeners_coll_remove_listener(_callui_listeners_coll_t *listeners_coll,
		void *cb_func,
		void *cb_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(listeners_coll, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(cb_func, CALLUI_RESULT_INVALID_PARAM);

	CALLUI_RETURN_VALUE_IF_FAIL(listeners_coll->is_initialized, CALLUI_RESULT_FAIL);

	Eina_List *l;
	Eina_List *l_next;
	_callui_listener_t *data;

	EINA_LIST_FOREACH_SAFE(listeners_coll->list, l, l_next, data) {
		if(cb_func == data->cb_func && cb_data == data->cb_data) {
			free(data);
			if (listeners_coll->is_locked) {
				listeners_coll->is_need_validate = true;
				eina_list_data_set(l, NULL);
			} else {
				listeners_coll->list = eina_list_remove_list(listeners_coll->list, l);
			}
			return CALLUI_RESULT_OK;
		}
	}
	return CALLUI_RESULT_NOT_REGISTERED;
}
