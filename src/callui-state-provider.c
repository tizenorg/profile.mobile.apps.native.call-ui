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

#include <Eina.h>
#include <call-manager.h>
#include <call-manager-extension.h>
#include <contacts.h>
#include <sys/sysinfo.h>

#include "callui-state-provider.h"
#include "callui-state-provider-priv.h"
#include "callui-debug.h"
#include "callui-common.h"
#include "callui-model-utils-priv.h"
#include "callui-listeners-collection.h"

#define CALLUI_NO_HANDLE 0
#define CALLUI_DEFAULT_PERSON_ID_TXT "default"

struct __callui_state_provider {
	cm_client_h cm_handler;

	callui_call_state_data_t *st_data_list[CALLUI_CALL_DATA_TYPE_MAX];

	callui_call_state_data_t *last_ended_call_data;

	_callui_listeners_coll_t call_state_lc;
	_callui_listeners_coll_t last_call_end_lc;
};
typedef struct __callui_state_provider _callui_state_provider_t;

static callui_result_e __callui_stp_init(callui_state_provider_h stp, cm_client_h cm_client);
static void __callui_stp_deinit(callui_state_provider_h stp);

static void __get_contact_info_from_contact_srv(callui_contact_data_t *ct_info);
static callui_result_e __convert_cm_call_event_type(cm_call_event_e cm_call_event, callui_call_event_type_e *call_event_type);
static callui_sim_slot_type_e __convert_cm_sim_type(cm_multi_sim_slot_type_e cm_sim_type);
static void __free_call_event_data(callui_state_provider_h stp);
static callui_call_state_data_t *__call_data_create(cm_call_data_t* cm_call_data);
static callui_result_e __call_data_init(callui_call_state_data_t *callui_call_data, cm_call_data_t* cm_call_data);
static callui_result_e __update_stp_call_data(callui_call_state_data_t **stp_call_data, cm_call_data_t* cm_call_data);
static callui_result_e __update_stp_all_call_data(callui_state_provider_h stp,
		cm_call_data_t *cm_incom,
		cm_call_data_t *cm_active,
		cm_call_data_t *cm_held);
static void __call_event_cb(cm_call_event_e call_event, cm_call_event_data_t *call_state_data, void *user_data);
static void __call_state_event_handler_func(_callui_listener_t *listener, va_list args);
static void __list_free_cb(gpointer data);
static callui_result_e __conf_call_data_init(callui_conf_call_data_t *conf_call_data, cm_conf_call_data_t *cm_conf_data);
static callui_conf_call_data_t * __conf_call_data_create(cm_conf_call_data_t *cm_conf_data);

static void __get_contact_info_from_contact_srv(callui_contact_data_t *ct_info)
{
	CALLUI_RETURN_IF_FAIL(ct_info);

	contacts_error_e err = CONTACTS_ERROR_NONE;
	contacts_record_h person_record = NULL;

	int person_id = ct_info->person_id;
	int res = contacts_db_get_record(_contacts_person._uri, person_id, &person_record);
	if (res != CONTACTS_ERROR_NONE) {
		err("contacts_db_get_record error %d", err);
	} else {
		char *name = NULL;
		char *img_path = NULL;

		/* Get display name */
		res = contacts_record_get_str(person_record, _contacts_person.display_name, &name);
		if (res != CONTACTS_ERROR_NONE) {
			err("contacts_record_get_str(display name) error %d", err);
		} else {
			g_strlcpy(ct_info->call_disp_name, name, CALLUI_DISPLAY_NAME_LENGTH_MAX);
			free(name);
		}

		/* Get caller id path */
		res = contacts_record_get_str(person_record, _contacts_person.image_thumbnail_path, &img_path);
		if (res != CONTACTS_ERROR_NONE) {
			err("contacts_record_get_str(caller id path) error %d", err);
		} else {
			g_strlcpy(ct_info->caller_id_path, img_path, CALLUI_IMAGE_PATH_LENGTH_MAX);
			free(img_path);
		}
		contacts_record_destroy(person_record, TRUE);
	}

	sec_dbg("contact index:[%d]", ct_info->person_id);
	sec_dbg("display name:[%s]", ct_info->call_disp_name);
	sec_dbg("img path:[%s]", ct_info->caller_id_path);

	return;
}

static callui_result_e __convert_cm_call_event_type(cm_call_event_e cm_call_event,
		callui_call_event_type_e *call_event_type)
{
	switch (cm_call_event) {
	case CM_CALL_EVENT_IDLE:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_END;
		break;
	case CM_CALL_EVENT_DIALING:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_DIALING;
		break;
	case CM_CALL_EVENT_ACTIVE:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_ACTIVE;
		break;
	case CM_CALL_EVENT_HELD:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_HELD;
		break;
	case CM_CALL_EVENT_ALERT:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_ALERT;
		break;
	case CM_CALL_EVENT_INCOMING:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_INCOMING;
		break;
	case CM_CALL_EVENT_WAITING:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_WAITING;
		break;
	case CM_CALL_EVENT_JOIN:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_JOIN;
		break;
	case CM_CALL_EVENT_SPLIT:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_SPLIT;
		break;
	case CM_CALL_EVENT_SWAPPED:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_SWAPPED;
		break;
	case CM_CALL_EVENT_RETRIEVED:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_RETRIEVED;
		break;
	case CM_CALL_EVENT_SAT_CALL_CONTROL:
		*call_event_type = CALLUI_CALL_EVENT_TYPE_SAT_CALL_CONTROL;
		break;
	default:
		err("Undefined call event type [%d]", cm_call_event);
		return CALLUI_RESULT_FAIL;
	}
	return CALLUI_RESULT_OK;
}

static callui_sim_slot_type_e __convert_cm_sim_type(cm_multi_sim_slot_type_e cm_sim_type)
{
	switch (cm_sim_type) {
	case CM_SIM_SLOT_1_E:
		return CALLUI_SIM_SLOT_1;
	case CM_SIM_SLOT_2_E:
		return CALLUI_SIM_SLOT_2;
	default:
		return CALLUI_SIM_SLOT_DEFAULT;
	}
}

static void __free_call_event_data(callui_state_provider_h stp)
{
	int i = 0;
	for (; i < CALLUI_CALL_DATA_TYPE_MAX; i++) {
		FREE(stp->st_data_list[i]);
	}
}

static callui_call_state_data_t *__call_data_create(cm_call_data_t* cm_call_data)
{
	callui_call_state_data_t *call_data = calloc(1, sizeof(callui_call_state_data_t));
	CALLUI_RETURN_NULL_IF_FAIL(call_data);

	int res = __call_data_init(call_data, cm_call_data);
	if (res != CALLUI_RESULT_OK) {
		err("Init call data failed. res[%d]", res);
		FREE(call_data);
	}
	return call_data;
}

static callui_result_e __call_data_init(callui_call_state_data_t *callui_call_data,
		cm_call_data_t* cm_call_data)
{
	callui_call_data->call_id = CALLUI_NO_HANDLE;
	int res = cm_call_data_get_call_id(cm_call_data, &callui_call_data->call_id);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	char *call_number = NULL;
	res = cm_call_data_get_call_number(cm_call_data, &call_number);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));
	if (call_number) {
		snprintf(callui_call_data->call_num, CALLUI_PHONE_NUMBER_LENGTH_MAX, "%s", call_number);
	}
	// XXX: according to documentation it must be free, but it leads to crash
//	free(call_number);

	res = cm_call_data_get_call_member_count(cm_call_data, &callui_call_data->conf_member_count);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	gboolean is_emergency;
	res = cm_call_data_is_emergency_call(cm_call_data, &is_emergency);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));
	callui_call_data->is_emergency = is_emergency;

	res = cm_call_data_get_start_time(cm_call_data, &callui_call_data->start_time);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	cm_call_state_e call_state;
	res = cm_call_data_get_call_state(cm_call_data, &call_state);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	callui_call_data->is_dialing = false;
	if(call_state == CM_CALL_STATE_DIALING) {
		callui_call_data->is_dialing = true;
	}
	callui_call_data->call_ct_info.person_id = -1;
	res = cm_call_data_get_person_id(cm_call_data, &callui_call_data->call_ct_info.person_id);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	if (callui_call_data->call_ct_info.person_id != -1) {
		dbg("contact exists with index:[%d]", callui_call_data->call_ct_info.person_id);
		__get_contact_info_from_contact_srv(&callui_call_data->call_ct_info);
	}

	if (strlen(callui_call_data->call_ct_info.caller_id_path) <= 0) {
		snprintf(callui_call_data->call_ct_info.caller_id_path,
				CALLUI_IMAGE_PATH_LENGTH_MAX, "%s", CALLUI_DEFAULT_PERSON_ID_TXT);
	}

	return CALLUI_RESULT_OK;
}

static callui_result_e __update_stp_call_data(callui_call_state_data_t **stp_call_data, cm_call_data_t* cm_call_data)
{
	if (!cm_call_data) {
		FREE(*stp_call_data);
		return CALLUI_RESULT_OK;
	}
	callui_call_state_data_t *tmp_call_data = __call_data_create(cm_call_data);
	CALLUI_RETURN_VALUE_IF_FAIL(tmp_call_data, CALLUI_RESULT_FAIL);

	free(*stp_call_data);
	*stp_call_data = tmp_call_data;

	return CALLUI_RESULT_OK;
}

static callui_result_e __update_stp_all_call_data(callui_state_provider_h stp,
		cm_call_data_t *cm_incom,
		cm_call_data_t *cm_active,
		cm_call_data_t *cm_held)
{
	dbg("Update incoming call data");
	int res = __update_stp_call_data(&(stp->st_data_list[CALLUI_CALL_DATA_TYPE_INCOMING]), cm_incom);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	dbg("Update active call data");
	res = __update_stp_call_data(&(stp->st_data_list[CALLUI_CALL_DATA_TYPE_ACTIVE]), cm_active);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	dbg("Update held call data");
	res = __update_stp_call_data(&(stp->st_data_list[CALLUI_CALL_DATA_TYPE_HELD]), cm_held);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return res;
}

static void __call_event_cb(cm_call_event_e call_event, cm_call_event_data_t *call_state_data, void *user_data)
{
	CALLUI_RETURN_IF_FAIL(call_state_data);
	CALLUI_RETURN_IF_FAIL(user_data);

	callui_call_event_type_e event_type;
	CALLUI_RETURN_IF_FAIL(__convert_cm_call_event_type(call_event, &event_type) == CALLUI_RESULT_OK);

	dbg("Call event changed on [%d]", event_type);

	callui_state_provider_h stp = (callui_state_provider_h)user_data;

	unsigned int call_id = 0;
	int res = cm_call_event_data_get_call_id(call_state_data, &call_id);
	CALLUI_RETURN_IF_FAIL(res == CM_ERROR_NONE);

	cm_multi_sim_slot_type_e cm_sim_slot;
	res = cm_call_event_data_get_sim_slot(call_state_data, &cm_sim_slot);
	CALLUI_RETURN_IF_FAIL(res == CM_ERROR_NONE);
	callui_sim_slot_type_e sim_slot_type = __convert_cm_sim_type(cm_sim_slot);

	cm_call_data_t *cm_incom = NULL;
	cm_call_data_t *cm_active = NULL;
	cm_call_data_t *cm_held = NULL;

	res = cm_call_event_data_get_active_call(call_state_data, &cm_active);
	CALLUI_RETURN_IF_FAIL(res == CM_ERROR_NONE);
	res = cm_call_event_data_get_incom_call(call_state_data, &cm_incom);
	CALLUI_RETURN_IF_FAIL(res == CM_ERROR_NONE);
	res = cm_call_event_data_get_held_call(call_state_data, &cm_held);
	CALLUI_RETURN_IF_FAIL(res == CM_ERROR_NONE);

	FREE(stp->last_ended_call_data);

	switch (event_type) {
	case CALLUI_CALL_EVENT_TYPE_END:
	{
		if (cm_incom == NULL && cm_active == NULL && cm_held == NULL) {
			if (stp->st_data_list[CALLUI_CALL_DATA_TYPE_ACTIVE]) {
				stp->last_ended_call_data = stp->st_data_list[CALLUI_CALL_DATA_TYPE_ACTIVE];
				stp->st_data_list[CALLUI_CALL_DATA_TYPE_ACTIVE] = NULL;
			} else if (stp->st_data_list[CALLUI_CALL_DATA_TYPE_HELD]) {
				stp->last_ended_call_data = stp->st_data_list[CALLUI_CALL_DATA_TYPE_HELD];
				stp->st_data_list[CALLUI_CALL_DATA_TYPE_HELD] = NULL;
			}
		}
	}
	case CALLUI_CALL_EVENT_TYPE_ACTIVE:
	case CALLUI_CALL_EVENT_TYPE_INCOMING:
	case CALLUI_CALL_EVENT_TYPE_DIALING:
	case CALLUI_CALL_EVENT_TYPE_HELD:
	case CALLUI_CALL_EVENT_TYPE_RETRIEVED:
	case CALLUI_CALL_EVENT_TYPE_SWAPPED:
	case CALLUI_CALL_EVENT_TYPE_JOIN:
	case CALLUI_CALL_EVENT_TYPE_SPLIT:
		res = __update_stp_all_call_data(stp, cm_incom, cm_active, cm_held);
		break;
	default:
		return;
	}

	if (res == CALLUI_RESULT_OK) {
		_callui_listeners_coll_call_listeners(&stp->call_state_lc, event_type, call_id, sim_slot_type);
	}
}

static callui_result_e __callui_stp_init(callui_state_provider_h stp, cm_client_h cm_client)
{
	stp->cm_handler = cm_client;

	int res = _callui_listeners_coll_init(&stp->call_state_lc);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);
	res = _callui_listeners_coll_init(&stp->last_call_end_lc);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = contacts_connect();
	CALLUI_RETURN_VALUE_IF_FAIL(res == CONTACTS_ERROR_NONE, CALLUI_RESULT_FAIL);

	cm_call_data_t *cm_incom = NULL;
	cm_call_data_t *cm_active = NULL;
	cm_call_data_t *cm_held = NULL;

	res = cm_get_all_calldata(stp->cm_handler, &cm_incom, &cm_active, &cm_held);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	__update_stp_all_call_data(stp, cm_incom, cm_active, cm_held);

	cm_call_data_free(cm_incom);
	cm_call_data_free(cm_active);
	cm_call_data_free(cm_held);

	return _callui_utils_convert_cm_res(cm_set_call_event_cb(stp->cm_handler, __call_event_cb, stp));
}

static void __callui_stp_deinit(callui_state_provider_h stp)
{
	cm_unset_call_event_cb(stp->cm_handler);

	contacts_disconnect();

	_callui_listeners_coll_deinit(&stp->call_state_lc);
	_callui_listeners_coll_deinit(&stp->last_call_end_lc);

	__free_call_event_data(stp);

	FREE(stp->last_ended_call_data);
}

callui_state_provider_h _callui_stp_create(cm_client_h cm_client)
{
	CALLUI_RETURN_NULL_IF_FAIL(cm_client);

	callui_state_provider_h stp = calloc(1, sizeof(_callui_state_provider_t));
	CALLUI_RETURN_NULL_IF_FAIL(stp);

	callui_result_e res = __callui_stp_init(stp, cm_client);
	if (res != CALLUI_RESULT_OK) {
		__callui_stp_deinit(stp);
		FREE(stp);
	}

	return stp;
}

void _callui_stp_destroy(callui_state_provider_h stp)
{
	CALLUI_RETURN_IF_FAIL(stp);

	__callui_stp_deinit(stp);

	free(stp);
}

const callui_call_state_data_t *_callui_stp_get_call_data(callui_state_provider_h stp,
		callui_call_data_type_e call_data_type)
{
	CALLUI_RETURN_NULL_IF_FAIL(stp);
	CALLUI_RETURN_NULL_IF_FAIL((call_data_type >= CALLUI_CALL_DATA_TYPE_ACTIVE &&
			call_data_type < CALLUI_CALL_DATA_TYPE_MAX));

	return stp->st_data_list[call_data_type];
}

static void __call_state_event_handler_func(_callui_listener_t *listener, va_list args)
{
	callui_call_event_type_e call_event_type = va_arg(args, callui_call_event_type_e);
	unsigned int id = va_arg(args, unsigned int);
	callui_sim_slot_type_e sim_slot = va_arg(args, callui_sim_slot_type_e);

	((callui_call_state_event_cb)(listener->cb_func))(listener->cb_data, call_event_type, id, sim_slot);
}

callui_result_e _callui_stp_add_call_state_event_cb(callui_state_provider_h stp,
		callui_call_state_event_cb cb_func,
		void *cb_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(stp, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(cb_func, CALLUI_RESULT_INVALID_PARAM);

	return _callui_listeners_coll_add_listener(&stp->call_state_lc,
			__call_state_event_handler_func, cb_func, cb_data);
}

callui_result_e _callui_stp_remove_call_state_event_cb(callui_state_provider_h stp,
		callui_call_state_event_cb cb_func,
		void *cb_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(stp, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(cb_func, CALLUI_RESULT_INVALID_PARAM);

	return _callui_listeners_coll_remove_listener(&stp->call_state_lc, (void *)cb_func, cb_data);
}

static void __list_free_cb(gpointer data)
{
	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;
	cm_conf_call_data_free(call_data);
}

static callui_conf_call_data_t * __conf_call_data_create(cm_conf_call_data_t *cm_conf_data)
{
	callui_conf_call_data_t *conf_data = (callui_conf_call_data_t *)calloc(1, sizeof(callui_conf_call_data_t));
	CALLUI_RETURN_NULL_IF_FAIL(conf_data);

	int res = __conf_call_data_init(conf_data, cm_conf_data);
	if (res != CALLUI_RESULT_OK) {
		err("Init conference call data failed. res[%d]", res);
		FREE(conf_data);
	}
	return conf_data;
}

static callui_result_e __conf_call_data_init(callui_conf_call_data_t *conf_call_data,
		cm_conf_call_data_t *cm_conf_data)
{
	int res = cm_conf_call_data_get_call_id(cm_conf_data, &conf_call_data->call_id);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	char *call_number = NULL;
	res = cm_conf_call_data_get_call_number(cm_conf_data, &call_number);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));
	if (call_number) {
		snprintf(conf_call_data->call_num, CALLUI_PHONE_NUMBER_LENGTH_MAX, "%s", call_number);
	}
	// XXX: according to documentation it must be free, but it leads to crash
//	free(call_number);

	conf_call_data->call_ct_info.person_id = -1;
	res = cm_conf_call_data_get_person_id(cm_conf_data, &conf_call_data->call_ct_info.person_id);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	if (conf_call_data->call_ct_info.person_id != -1) {
		dbg("contact exists with index:[%d]", conf_call_data->call_ct_info.person_id);
		__get_contact_info_from_contact_srv(&conf_call_data->call_ct_info);
	}

	if (strlen(conf_call_data->call_ct_info.caller_id_path) <= 0) {
		snprintf(conf_call_data->call_ct_info.caller_id_path,
				CALLUI_IMAGE_PATH_LENGTH_MAX, "%s", CALLUI_DEFAULT_PERSON_ID_TXT);
	}

	return CALLUI_RESULT_OK;
}

Eina_List *_callui_stp_get_conference_call_list(callui_state_provider_h stp)
{
	CALLUI_RETURN_NULL_IF_FAIL(stp);

	// XXX: must be initialized with NULL. If not an there is no conference calls
	// cm_get_conference_call_list return CM_ERROR_NONE and pointer will be not changed.
	GSList *call_list = NULL;
	Eina_List *res_list = NULL;

	int res = cm_get_conference_call_list(stp->cm_handler, &call_list);
	CALLUI_RETURN_NULL_IF_FAIL(res == CM_ERROR_NONE);

	if (call_list) {
		int list_len = g_slist_length(call_list);
		res = CALLUI_RESULT_OK;

		int idx = 0;
		for (; idx < list_len; idx++) {
			callui_conf_call_data_t *conf_call_data = __conf_call_data_create(g_slist_nth_data(call_list, idx));
			if(conf_call_data == NULL) {
				err("Create conference call data failed.");
				res = CALLUI_RESULT_ALLOCATION_FAIL;
				break;
			}
			res_list = eina_list_append(res_list, conf_call_data);
			if (res_list == NULL) {
				err("Append item to list failed.");
				res = CALLUI_RESULT_FAIL;
				break;
			}
		}

		if (res != CALLUI_RESULT_OK) {
			Eina_List *l;
			callui_conf_call_data_t *data;
			EINA_LIST_FOREACH(res_list, l, data) {
				free(data);
			}
			res_list = eina_list_free(res_list);
		}
		g_slist_free_full(call_list, __list_free_cb);
	}

	return res_list;
}

struct tm*_callui_stp_get_call_duration(callui_state_provider_h stp,
		callui_call_data_type_e call_data_type)
{
	CALLUI_RETURN_NULL_IF_FAIL(stp);
	CALLUI_RETURN_NULL_IF_FAIL((call_data_type >= CALLUI_CALL_DATA_TYPE_ACTIVE &&
			call_data_type < CALLUI_CALL_DATA_TYPE_MAX));
	CALLUI_RETURN_NULL_IF_FAIL(stp->st_data_list[call_data_type]);

	struct tm *time = calloc(1, sizeof (struct tm));
	CALLUI_RETURN_NULL_IF_FAIL(time);

	long curr_time = 0;
	struct sysinfo info;
	if (sysinfo(&info) == 0) {
		curr_time = info.uptime;
	}

	long call_time = curr_time - stp->st_data_list[call_data_type]->start_time;
	gmtime_r((const time_t *)&call_time, time);

	return time;
}

const callui_call_state_data_t *_callui_stp_get_last_ended_call_data(callui_state_provider_h stp)
{
	CALLUI_RETURN_NULL_IF_FAIL(stp);

	return stp->last_ended_call_data;
}

bool _callui_stp_is_any_calls_available(callui_state_provider_h stp)
{
	CALLUI_RETURN_VALUE_IF_FAIL(stp, false);

	if (stp->st_data_list[CALLUI_CALL_DATA_TYPE_INCOMING])
		dbg("CALLUI_CALL_DATA_TYPE_INCOMING - NOT NULL");
	if (stp->st_data_list[CALLUI_CALL_DATA_TYPE_ACTIVE])
		dbg("CALLUI_CALL_DATA_TYPE_ACTIVE - NOT NULL");
	if (stp->st_data_list[CALLUI_CALL_DATA_TYPE_HELD])
		dbg("CALLUI_CALL_DATA_TYPE_HELD - NOT NULL");

	return (stp->st_data_list[CALLUI_CALL_DATA_TYPE_INCOMING] ||
			stp->st_data_list[CALLUI_CALL_DATA_TYPE_ACTIVE] ||
			stp->st_data_list[CALLUI_CALL_DATA_TYPE_HELD]);
}
