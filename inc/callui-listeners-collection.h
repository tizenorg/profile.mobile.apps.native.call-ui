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

#ifndef __CALLUI_LISTENERS_SET_H__
#define __CALLUI_LISTENERS_SET_H__

#include <Eina.h>
#include <stdbool.h>

#include "callui-common-types.h"

typedef struct _callui_listener _callui_listener_t;

typedef void (*cb_func_handler)(_callui_listener_t *listener, va_list args);

struct _callui_listener {
	cb_func_handler handler;
	void *cb_func;
	void *cb_data;
};

struct _callui_listeners_coll {
	Eina_List *list;
	bool is_need_validate;
	bool is_locked;
	bool is_initialized;
};
typedef struct _callui_listeners_coll _callui_listeners_coll_t;

callui_result_e _callui_listeners_coll_init(_callui_listeners_coll_t *listeners_col);

callui_result_e _callui_listeners_coll_deinit(_callui_listeners_coll_t *listeners_col);

callui_result_e _callui_listeners_coll_add_listener(_callui_listeners_coll_t *listeners_col,
		cb_func_handler func_handler,
		void *cb_func,
		void *cb_data);

callui_result_e _callui_listeners_coll_call_listeners(_callui_listeners_coll_t *listeners_col, ...);

callui_result_e _callui_listeners_coll_remove_listener(_callui_listeners_coll_t *listeners_col,
		void *cb_func,
		void *cb_data);

#endif /* __CALLUI_LISTENERS_SET_H__ */
