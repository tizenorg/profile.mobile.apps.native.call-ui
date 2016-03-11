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

#include <call-manager.h>
#include <call-manager-extension.h>

#include "callui-manager.h"
#include "callui-common.h"
#include "callui-sound-manager-priv.h"
#include "callui-state-provider-priv.h"
#include "callui-model-utils-priv.h"

struct __callui_manager {
	cm_client_h cm_handler;

	callui_sound_manager_h sound_manager;
	callui_state_provider_h state_provider;
};
typedef struct __callui_manager _callui_manager_t;

static callui_result_e __callui_manager_init(callui_manager_h cm_handler);
static void __callui_manager_deinit(callui_manager_h cm_handler);
static cm_call_release_type_e __convert_app_release_type(callui_call_release_type_e type);
static cm_call_answer_type_e __convert_app_answer_type(callui_call_answer_type_e type);

static cm_call_release_type_e __convert_app_release_type(callui_call_release_type_e type)
{
	switch (type) {
	case CALLUI_CALL_RELEASE_TYPE_BY_CALL_HANDLE:
		return CALL_RELEASE_TYPE_BY_CALL_HANDLE;
	case CALLUI_CALL_RELEASE_TYPE_ALL_CALLS:
		return CALL_RELEASE_TYPE_ALL_CALLS;
	case CALLUI_CALL_RELEASE_TYPE_ALL_HOLD_CALLS:
		return CALL_RELEASE_TYPE_ALL_HOLD_CALLS;
	case CALLUI_CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS:
		return CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS;
	default:
		err("undefined call release type [%d]", type);
		return CALL_RELEASE_TYPE_BY_CALL_HANDLE;
	}
}

static cm_call_answer_type_e __convert_app_answer_type(callui_call_answer_type_e type)
{
	switch (type) {
	case CALLUI_CALL_ANSWER_TYPE_NORMAL:
		return CALL_ANSWER_TYPE_NORMAL;
	case CALLUI_CALL_ANSWER_TYPE_HOLD_ACTIVE_AND_ACCEPT:
		return CALL_ANSWER_TYPE_HOLD_ACTIVE_AND_ACCEPT;
	case CALLUI_CALL_ANSWER_TYPE_RELEASE_ACTIVE_AND_ACCEPT:
		return CALL_ANSWER_TYPE_RELEASE_ACTIVE_AND_ACCEPT;
	case CALLUI_CALL_ANSWER_TYPE_RELEASE_HOLD_AND_ACCEPT:
		return CALL_ANSWER_TYPE_RELEASE_HOLD_AND_ACCEPT;
	case CALLUI_CALL_ANSWER_TYPE_RELEASE_ALL_AND_ACCEPT:
		return CALL_ANSWER_TYPE_RELEASE_ALL_AND_ACCEPT;
	default:
		err("undefined call answer type [%d]", type);
		return CALL_ANSWER_TYPE_NORMAL;
	}
}

static callui_result_e __callui_manager_init(callui_manager_h cm_handler)
{
	callui_result_e res = _callui_utils_convert_cm_res(cm_init(&cm_handler->cm_handler));
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	cm_handler->sound_manager = _callui_sdm_create(cm_handler->cm_handler);
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler->sound_manager, CALLUI_RESULT_FAIL);

	cm_handler->state_provider = _callui_stp_create(cm_handler->cm_handler);
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler->state_provider, CALLUI_RESULT_FAIL);

	return res;
}

static void __callui_manager_deinit(callui_manager_h cm_handler)
{
	if (cm_handler->state_provider) {
		_callui_stp_destroy(cm_handler->state_provider);
		cm_handler->state_provider = NULL;
	}

	if (cm_handler->sound_manager) {
		_callui_sdm_destroy(cm_handler->sound_manager);
		cm_handler->sound_manager = NULL;
	}

	if (cm_handler->cm_handler) {
		cm_deinit(cm_handler->cm_handler);
		cm_handler->cm_handler = NULL;
	}
}

callui_manager_h _callui_manager_create()
{
	callui_manager_h cm_handler = calloc(1, sizeof(_callui_manager_t));
	CALLUI_RETURN_NULL_IF_FAIL(cm_handler);

	callui_result_e res = __callui_manager_init(cm_handler);
	if (res != CALLUI_RESULT_OK) {
		__callui_manager_deinit(cm_handler);
		FREE(cm_handler);
	}
	return cm_handler;
}

void _callui_manager_destroy(callui_manager_h cm_handler)
{
	CALLUI_RETURN_IF_FAIL(cm_handler);

	__callui_manager_deinit(cm_handler);

	free(cm_handler);
}

static cm_multi_sim_slot_type_e __convert_callui_sim_type(callui_sim_slot_type_e sim_type)
{
	switch (sim_type) {
	case CALLUI_SIM_SLOT_1:
		return CM_SIM_SLOT_1_E;
	case CALLUI_SIM_SLOT_2:
		return CM_SIM_SLOT_2_E;
	default:
		return CM_SIM_SLOT_DEFAULT_E;
	}
}

callui_result_e _callui_manager_dial_voice_call(callui_manager_h cm_handler, const char *number, callui_sim_slot_type_e sim_slot)
{
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(number, CALLUI_RESULT_INVALID_PARAM);

	char *temp_number = (char *)number;
	return _callui_utils_convert_cm_res(
			cm_dial_call(cm_handler->cm_handler, temp_number, CM_CALL_TYPE_VOICE, __convert_callui_sim_type(sim_slot)));
}

callui_result_e _callui_manager_end_call(callui_manager_h cm_handler, unsigned int call_id, callui_call_release_type_e release_type)
{
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(call_id <= 0, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(
			cm_end_call(cm_handler->cm_handler, call_id, __convert_app_release_type(release_type)));
}

callui_result_e _callui_manager_swap_call(callui_manager_h cm_handler)
{
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(cm_swap_call(cm_handler->cm_handler));
}

callui_result_e _callui_manager_hold_call(callui_manager_h cm_handler)
{
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(cm_hold_call(cm_handler->cm_handler));
}

callui_result_e _callui_manager_unhold_call(callui_manager_h cm_handler)
{
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(cm_unhold_call(cm_handler->cm_handler));
}

callui_result_e _callui_manager_join_call(callui_manager_h cm_handler)
{
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(cm_join_call(cm_handler->cm_handler));
}

callui_result_e _callui_manager_split_call(callui_manager_h cm_handler, unsigned int call_id)
{
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(call_id <= 0, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(
			cm_split_call(cm_handler->cm_handler, call_id));
}

callui_result_e _callui_manager_answer_call(callui_manager_h cm_handler, callui_call_answer_type_e ans_type)
{
	CALLUI_RETURN_VALUE_IF_FAIL(cm_handler, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(
			cm_answer_call(cm_handler->cm_handler, __convert_app_answer_type(ans_type)));

}

callui_sound_manager_h _callui_manager_get_sound_manager(callui_manager_h cm_handler)
{
	CALLUI_RETURN_NULL_IF_FAIL(cm_handler);

	return cm_handler->sound_manager;
}

callui_state_provider_h _callui_manager_get_state_provider(callui_manager_h cm_handler)
{
	CALLUI_RETURN_NULL_IF_FAIL(cm_handler);

	return cm_handler->state_provider;
}
