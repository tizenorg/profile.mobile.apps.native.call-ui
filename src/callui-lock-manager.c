/*
 * Copyright (c) 2009-2016 Samsung Electronics Co., Ltd All Rights Reserved
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

#include "callui-lock-manager.h"
#include "callui-debug.h"
#include "callui-proximity-lock-manager.h"
#include "callui-view-lock-screen.h"
#include "callui-common.h"

lock_data_t *_callui_lock_manager_create()
{
	dbg("..");
	lock_data_t *lock_h;
	lock_h = calloc(1, sizeof(lock_data_t));
	CALLUI_RETURN_NULL_IF_FAIL(lock_h);
	bool proximity_supported = _callui_proximity_lock_manager_is_supported();
	if (proximity_supported) {
		_callui_proximity_lock_manager_init(lock_h);
	} else {
		_callui_lock_screen_init(lock_h);
	}
	lock_h->handle = lock_h->create();
	if (!lock_h->handle) {
		free(lock_h);
		return NULL;
	}
	return lock_h;
}

void _callui_lock_manager_start(lock_data_t *lock_h)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h);
	if (_callui_is_on_handsfree_mode() || _callui_lock_manager_is_started(lock_h) || _callui_is_on_background()) {
		return;
	}
	lock_h->start(lock_h->handle);
}

void _callui_lock_manager_stop(lock_data_t *lock_h)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h);
	lock_h->stop(lock_h->handle, false);
}

void _callui_lock_manager_force_stop(lock_data_t *lock_h)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h);
	lock_h->stop(lock_h->handle, true);
}

bool _callui_lock_manager_is_started(lock_data_t *lock_h)
{
	dbg("..");
	CALLUI_RETURN_VALUE_IF_FAIL(lock_h, false);
	return lock_h->is_started(lock_h->handle);
}

void _callui_lock_manager_destroy(lock_data_t *lock_h)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h);
	lock_h->destroy(lock_h->handle);
	free(lock_h);
}

bool _callui_lock_manager_is_lcd_off(lock_data_t *lock_h)
{
	dbg("..");
	CALLUI_RETURN_VALUE_IF_FAIL(lock_h, false);
	return lock_h->is_lcd_off(lock_h->handle);
}

void _callui_lock_manager_set_callback_on_unlock(lock_data_t *lock_h, unlock_cb_t callback, void* data)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h);
	lock_h->set_unlock_cb(lock_h->handle, callback, data);
}
