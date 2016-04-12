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

#ifndef __CALLUI_MANAGER_H__
#define __CALLUI_MANAGER_H__

#include <stdbool.h>

#include "callui-common-types.h"

typedef enum {
	CALLUI_CALL_ANSWER_TYPE_NORMAL = 0,					/**< Only single call exist, Accept the Incoming call*/
	CALLUI_CALL_ANSWER_TYPE_HOLD_ACTIVE_AND_ACCEPT,		/**< Put the active call on hold and accepts the call*/
	CALLUI_CALL_ANSWER_TYPE_RELEASE_ACTIVE_AND_ACCEPT,	/**< Releases the active call and accept the call*/
	CALLUI_CALL_ANSWER_TYPE_RELEASE_HOLD_AND_ACCEPT,	/**< Releases the held call and accept the call*/
	CALLUI_CALL_ANSWER_TYPE_RELEASE_ALL_AND_ACCEPT		/**< Releases all calls and accept the call*/
} callui_call_answer_type_e;

typedef enum {
	CALLUI_CALL_RELEASE_TYPE_BY_CALL_HANDLE = 0,	/**< Release call using given call_handle*/
	CALLUI_CALL_RELEASE_TYPE_ALL_CALLS,				/**< Release all Calls*/
	CALLUI_CALL_RELEASE_TYPE_ALL_HOLD_CALLS,		/**< Releases all hold calls*/
	CALLUI_CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS,		/**< Releases all active calls*/
} callui_call_release_type_e;

typedef enum {
	CALLUI_DIAL_SUCCESS = 0,
	CALLUI_DIAL_CANCEL,
	CALLUI_DIAL_FAIL,
	CALLUI_DIAL_FAIL_SS,
	CALLUI_DIAL_FAIL_FDN,
	CALLUI_DIAL_FAIL_FLIGHT_MODE,
} callui_dial_status_e;

typedef enum {
	CALLUI_SIM_SLOT_1 = 0,
	CALLUI_SIM_SLOT_2,
	CALLUI_SIM_SLOT_DEFAULT,
} callui_sim_slot_type_e;

/* Call manager handler type define */
typedef struct __callui_manager *callui_manager_h;

/* Forward declaration */
typedef struct __callui_state_provider *callui_state_provider_h;
typedef struct __callui_sound_manager *callui_sound_manager_h;

/* Callback prototype */
typedef void (*callui_end_call_called_cb)(void *user_data,
		unsigned int call_id,
		callui_call_release_type_e release_type);
typedef void (*callui_dial_status_cb)(void *user_data, callui_dial_status_e dial_status);


callui_manager_h _callui_manager_create();

void _callui_manager_destroy(callui_manager_h cm_handler);

callui_result_e _callui_manager_dial_voice_call(callui_manager_h cm_handler, const char *number, callui_sim_slot_type_e sim_slot);

callui_result_e _callui_manager_end_call(callui_manager_h cm_handler, unsigned int call_id, callui_call_release_type_e release_type);

callui_result_e _callui_manager_swap_call(callui_manager_h cm_handler);

callui_result_e _callui_manager_hold_call(callui_manager_h cm_handler);

callui_result_e _callui_manager_unhold_call(callui_manager_h cm_handler);

callui_result_e _callui_manager_join_call(callui_manager_h cm_handler);

callui_result_e _callui_manager_reject_call(callui_manager_h cm_handler);

callui_result_e _callui_manager_stop_alert(callui_manager_h cm_handler);

callui_result_e _callui_manager_split_call(callui_manager_h cm_handler, unsigned int call_id);

callui_result_e _callui_manager_answer_call(callui_manager_h cm_handler, callui_call_answer_type_e ans_type);

callui_sound_manager_h _callui_manager_get_sound_manager(callui_manager_h cm_handler);

callui_state_provider_h _callui_manager_get_state_provider(callui_manager_h cm_handler);

callui_result_e _callui_manager_add_end_call_called_cb(callui_manager_h cm_handler,
		callui_end_call_called_cb cb_func, void *cb_data);

callui_result_e _callui_manager_remove_end_call_called_cb(callui_manager_h cm_handler,
		callui_end_call_called_cb cb_func, void *cb_data);

callui_result_e _callui_manager_add_dial_status_cb(callui_manager_h cm_handler,
		callui_dial_status_cb cb_func, void *cb_data);

callui_result_e _callui_manager_remove_dial_status_cb(callui_manager_h cm_handler,
		callui_dial_status_cb cb_func, void *cb_data);

#endif /* __CALLUI_CALL_MANAGER_H__ */
