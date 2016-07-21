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

#ifndef __CALLUI_STATE_PROVIDER_H__
#define __CALLUI_STATE_PROVIDER_H__

#include <stdbool.h>
#include <time.h>

#include "callui-common-types.h"
#include "callui-manager.h"

#define CALLUI_DISPLAY_NAME_LENGTH_MAX		(255+1)
#define CALLUI_IMAGE_PATH_LENGTH_MAX		(255+1)
#define CALLUI_PHONE_NUMBER_LENGTH_MAX		(82+1)
#define CALLUI_PHONE_DISP_NUMBER_LENGTH_MAX	(82+10+1)

typedef enum {
	CALLUI_CALL_DATA_ACTIVE = 0,		/**< Active call data*/
	CALLUI_CALL_DATA_HELD,				/**< Held call data*/
	CALLUI_CALL_DATA_INCOMING,			/**< Incoming call data*/
	CALLUI_CALL_DATA_COUNT
} callui_call_data_type_e;

typedef enum {
	CALLUI_CALL_EVENT_END = 0,
	CALLUI_CALL_EVENT_DIALING,
	CALLUI_CALL_EVENT_ACTIVE,
	CALLUI_CALL_EVENT_HELD,
	CALLUI_CALL_EVENT_ALERT,
	CALLUI_CALL_EVENT_INCOMING,
	CALLUI_CALL_EVENT_WAITING,
	CALLUI_CALL_EVENT_JOIN,
	CALLUI_CALL_EVENT_SPLIT,
	CALLUI_CALL_EVENT_SWAPPED,
	CALLUI_CALL_EVENT_RETRIEVED,
	CALLUI_CALL_EVENT_SAT_CALL_CONTROL
} callui_call_event_type_e;

typedef struct __callui_state_provider *callui_state_provider_h;

struct _callui_contact_data_t {
	int person_id;												/**< Contact index of the Caller */
	char call_disp_name[CALLUI_DISPLAY_NAME_LENGTH_MAX];		/**< Caller display name */
	char caller_id_path[CALLUI_IMAGE_PATH_LENGTH_MAX];			/**< Caller image path */
};
typedef struct _callui_contact_data_t callui_contact_data_t;

struct _callui_conf_call_data_t {
	unsigned int call_id;
	char call_num[CALLUI_PHONE_NUMBER_LENGTH_MAX];
	callui_contact_data_t call_ct_info;
};
typedef struct _callui_conf_call_data_t callui_conf_call_data_t;

struct _callui_call_data_t {
	callui_call_data_type_e type;
	unsigned int call_id;
	char call_num[CALLUI_PHONE_NUMBER_LENGTH_MAX];
	char call_disp_num[CALLUI_PHONE_DISP_NUMBER_LENGTH_MAX];
	long start_time;
	bool is_dialing;
	bool is_emergency;
	callui_contact_data_t call_ct_info;
	int conf_member_count;
};
typedef struct _callui_call_data_t callui_call_data_t;

/* Callback functions prototypes */
typedef void (*callui_call_state_event_cb)(void *user_data,
		callui_call_event_type_e call_event_type,
		unsigned int call_id,
		callui_sim_slot_type_e sim_type,
		void *event_info);

/**
 * @brief Gets call data
 *
 * @param[in]	stp_h			State provider handle
 * @param[in]	call_data_type	Call data type
 *
 * @return Call data on success or NULL otherwise
 */
const callui_call_data_t *_callui_stp_get_call_data(callui_state_provider_h stp_h,
		callui_call_data_type_e call_data_type);

/**
 * @brief Gets conference call members list
 *
 * @param[in]	stp_h		State provider handle
 *
 * @return Conference call members list on success or NULL otherwise
 */
Eina_List *_callui_stp_get_conference_call_list(callui_state_provider_h stp_h);

/**
 * @brief Gets call duration time
 *
 * @param[in]	stp_h			State provider handle
 * @param[in]	call_data_type	Call data type
 *
 * @return Call duration time structure on success or NULL otherwise
 */
struct tm *_callui_stp_get_call_duration(callui_state_provider_h stp_h,
		callui_call_data_type_e call_data_type);

/**
 * @brief Gets status if any calls are available
 *
 * @param[in]	stp_h		State provider handle
 *
 * @return true if there are any calls and false otherwise
 */
bool _callui_stp_is_any_calls_available(callui_state_provider_h stp_h);

/**
 * @brief Adds call state change event listener
 *
 * @param[in]	stp_h		State provider handle
 * @param[in]	cb_func		User callback function
 * @param[in]	cb_data		User callback data
 *
 * @return CALLUI_RESULT_OK on success or another result otherwise
 */
callui_result_e _callui_stp_add_call_state_event_cb(callui_state_provider_h stp_h,
		callui_call_state_event_cb cb_func,
		void *cb_data);

/**
 * @brief Removes call state change event listener
 *
 * @param[in]	stp_h		State provider handle
 * @param[in]	cb_func		User callback function
 * @param[in]	cb_data		User callback data
 *
 * @return CALLUI_RESULT_OK on success or another result otherwise
 */
callui_result_e _callui_stp_remove_call_state_event_cb(callui_state_provider_h stp_h,
		callui_call_state_event_cb cb_func,
		void *cb_data);

#endif /* __CALLUI_CALL_STATE_PROVIDER_H__ */
