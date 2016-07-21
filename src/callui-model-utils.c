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

#include "callui-model-utils-priv.h"

callui_result_e _callui_utils_convert_cm_res(cm_error_e cm_res)
{
	switch (cm_res) {
	case CM_ERROR_NONE:
		return CALLUI_RESULT_OK;
	case CM_ERROR_OUT_OF_MEMORY:
		return CALLUI_RESULT_ALLOCATION_FAIL;
	case CM_ERROR_INVALID_PARAMETER:
		return CALLUI_RESULT_INVALID_PARAM;
	case CM_ERROR_PERMISSION_DENIED:
		return CALLUI_RESULT_PERMISSION_DENIED;
	case CM_ERROR_NOT_SUPPORTED:
		return CALLUI_RESULT_NOT_SUPPORTED;
	case CM_ERROR_NOT_REGISTERED:
		return CALLUI_RESULT_NOT_REGISTERED;
	case CM_ERROR_ALREADY_REGISTERED:
		return CALLUI_RESULT_ALREADY_REGISTERED;
	case CM_ERROR_OPERATION_FAILED:
		return CALLUI_RESULT_FAIL;
	default:
		return CALLUI_RESULT_UNKNOWN_ERROR;
	}
}
