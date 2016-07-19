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

#include <dpm/device-policy-manager.h>
#include <dpm/password.h>
#include <stdlib.h>

#include "callui-dpm.h"
#include "callui-debug.h"
#include "callui-common-types.h"
#include "callui-common-defines.h"

struct __callui_dpm {
	device_policy_manager_h dpm;
};

typedef struct __callui_dpm __callui_dpm_t;

static callui_result_e __callui_dpm_init(callui_dpm_h dpm_handler)
{
	dpm_handler->dpm = dpm_manager_create();
	CALLUI_RETURN_VALUE_IF_FAIL(dpm_handler, CALLUI_RESULT_FAIL);

	return CALLUI_RESULT_OK;
}

static void __callui_dpm_deinit(callui_dpm_h dpm_handler)
{
	dpm_manager_destroy(dpm_handler->dpm);
	dpm_handler->dpm = NULL;
}

callui_dpm_h _callui_dpm_create()
{
	callui_dpm_h dpm_handler = calloc(1, sizeof(__callui_dpm_t));
	CALLUI_RETURN_NULL_IF_FAIL(dpm_handler);

	callui_result_e res = __callui_dpm_init(dpm_handler);
	if (res != CALLUI_RESULT_OK) {
		__callui_dpm_deinit(dpm_handler);
		FREE(dpm_handler);
	}
	return dpm_handler;
}

void _callui_dpm_destroy(callui_dpm_h dpm_handler)
{
	CALLUI_RETURN_IF_FAIL(dpm_handler);

	__callui_dpm_deinit(dpm_handler);

	free(dpm_handler);
}

bool _callui_dpm_is_need_enforce_change_password(callui_dpm_h dpm_handler)
{
	CALLUI_RETURN_VALUE_IF_FAIL(dpm_handler, false);

	dpm_password_status_e status = DPM_PASSWORD_STATUS_NORMAL;

	// TODO: Requested function dpm_password_get_status() is not added yet
	// Remove comments from two lower lines when API will be ready
//	int res = dpm_password_get_status(manager, &status);
//	CALLUI_RETURN_VALUE_IF_FAIL(res == DPM_ERROR_NONE, false);

	return (status == DPM_PASSWORD_STATUS_CHANGE_REQUIRED);
}

