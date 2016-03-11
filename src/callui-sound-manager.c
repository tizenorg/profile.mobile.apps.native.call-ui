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

#include "callui-sound-manager.h"
#include "callui-sound-manager-priv.h"
#include "callui-debug.h"
#include "callui-common.h"
#include "callui-model-utils-priv.h"
#include "callui-listeners-collection.h"

struct __callui_sound_manager {
	cm_client_h cm_handler;

	_callui_listeners_coll_t audio_state_lc;
	_callui_listeners_coll_t mute_status_lc;
};
typedef struct __callui_sound_manager _callui_sound_manager_t;

static callui_result_e __callui_sdm_init(callui_sound_manager_h sdm, cm_client_h cm_client);
static void __callui_sdm_deinit(callui_sound_manager_h sdm);

static callui_audio_state_type_e __convert_cm_audio_state(cm_audio_state_type_e state_type);
static void __callui_audio_state_changed_cb(cm_audio_state_type_e audio_state, void *user_data);
static void __callui_mute_state_changed_cb(cm_mute_status_e mute_status, void *user_data);

static void __audio_state_change_handler_func(_callui_listener_t *listener, va_list args);
static void __mute_status_change_handler_func(_callui_listener_t *listener, va_list args);

static callui_audio_state_type_e __convert_cm_audio_state(cm_audio_state_type_e state_type)
{
	switch (state_type) {
	case CM_AUDIO_STATE_NONE_E:
		return CALLUI_AUDIO_STATE_NONE;
	case CM_AUDIO_STATE_SPEAKER_E:
		return CALLUI_AUDIO_STATE_SPEAKER;
	case CM_AUDIO_STATE_RECEIVER_E:
		return CALLUI_AUDIO_STATE_RECEIVER;
	case CM_AUDIO_STATE_EARJACK_E:
		return CALLUI_AUDIO_STATE_EARJACK;
	case CM_AUDIO_STATE_BT_E:
		return CALLUI_AUDIO_STATE_BT;
	default:
		err("Unknown audio state type [%d]", state_type);
		return CALLUI_AUDIO_STATE_NONE;
	}
}

static void __callui_audio_state_changed_cb(cm_audio_state_type_e audio_state, void *user_data)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	dbg("AUDIO STATE CHANGED [%d]", audio_state);

	callui_sound_manager_h sdm = (callui_sound_manager_h)user_data;

	if (audio_state == CM_AUDIO_STATE_NONE_E) {
		err("Unhandled state[%d]", audio_state);
		return;
	}

	callui_audio_state_type_e as = __convert_cm_audio_state(audio_state);
	_callui_listeners_coll_call_listeners(&sdm->audio_state_lc, as);
}

static void __callui_mute_state_changed_cb(cm_mute_status_e mute_status, void *user_data)
{
	CALLUI_RETURN_IF_FAIL(user_data);

	dbg("MUTE STATE CHANGED [%d]", mute_status);

	callui_sound_manager_h sdm = (callui_sound_manager_h)user_data;

	bool is_enable = false;
	if (mute_status == CM_MUTE_STATUS_ON) {
		is_enable = true;
	}
	_callui_listeners_coll_call_listeners(&sdm->mute_status_lc, is_enable);
}

static callui_result_e __callui_sdm_init(callui_sound_manager_h sdm, cm_client_h cm_client)
{
	sdm->cm_handler = cm_client;

	int res = _callui_listeners_coll_init(&sdm->audio_state_lc);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);
	res = _callui_listeners_coll_init(&sdm->mute_status_lc);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	res = cm_set_audio_state_changed_cb(sdm->cm_handler, __callui_audio_state_changed_cb, sdm);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	res = cm_set_mute_status_cb(sdm->cm_handler, __callui_mute_state_changed_cb, sdm);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, _callui_utils_convert_cm_res(res));

	return CALLUI_RESULT_OK;
}

static void __callui_sdm_deinit(callui_sound_manager_h sdm)
{
	cm_unset_audio_state_changed_cb(sdm->cm_handler);
	cm_unset_mute_status_cb(sdm->cm_handler);

	_callui_listeners_coll_deinit(&sdm->audio_state_lc);
	_callui_listeners_coll_deinit(&sdm->mute_status_lc);

	sdm->cm_handler = NULL;
}

callui_sound_manager_h _callui_sdm_create(cm_client_h cm_client)
{
	CALLUI_RETURN_NULL_IF_FAIL(cm_client);

	callui_sound_manager_h sdm = calloc(1, sizeof(_callui_sound_manager_t));
	CALLUI_RETURN_NULL_IF_FAIL(sdm);

	callui_result_e res = __callui_sdm_init(sdm, cm_client);
	if (res != CALLUI_RESULT_OK) {
		__callui_sdm_deinit(sdm);
		FREE(sdm);
	}
	return sdm;
}

void _callui_sdm_destroy(callui_sound_manager_h sdm)
{
	CALLUI_RETURN_IF_FAIL(sdm);

	__callui_sdm_deinit(sdm);

	free(sdm);
}

callui_result_e _callui_sdm_set_speaker_state(callui_sound_manager_h sdm, bool is_enable)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);

	if (is_enable) {
		return _callui_utils_convert_cm_res(cm_speaker_on(sdm->cm_handler));
	} else {
		return _callui_utils_convert_cm_res(cm_speaker_off(sdm->cm_handler));
	}
}

callui_result_e _callui_sdm_set_bluetooth_state(callui_sound_manager_h sdm, bool is_enable)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);

	if (is_enable) {
		return _callui_utils_convert_cm_res(cm_bluetooth_on(sdm->cm_handler));
	} else {
		return _callui_utils_convert_cm_res(cm_bluetooth_off(sdm->cm_handler));
	}
}

callui_result_e _callui_sdm_set_mute_state(callui_sound_manager_h sdm, bool is_enable)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(cm_set_mute_state(sdm->cm_handler, is_enable));
}

callui_result_e _callui_sdm_get_mute_state(callui_sound_manager_h sdm, bool *is_enable)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(is_enable, CALLUI_RESULT_INVALID_PARAM);

	cm_mute_status_e mutel_status = CM_MUTE_STATUS_MAX;
	callui_result_e res = _callui_utils_convert_cm_res(cm_get_mute_status(sdm->cm_handler, &mutel_status));
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	*is_enable = false;
	if (mutel_status == CM_MUTE_STATUS_ON) {
		*is_enable = true;
	}

	return res;
}

callui_result_e _callui_sdm_start_dtmf(callui_sound_manager_h sdm, unsigned char dtmf_digit)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(cm_start_dtmf(sdm->cm_handler, dtmf_digit));
}

callui_result_e _callui_sdm_stop_dtmf(callui_sound_manager_h sdm)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);

	return _callui_utils_convert_cm_res(cm_stop_dtmf(sdm->cm_handler));
}

callui_result_e _callui_sdm_get_audio_state(callui_sound_manager_h sdm,
		callui_audio_state_type_e *audio_state)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(audio_state, CALLUI_RESULT_INVALID_PARAM);

	cm_audio_state_type_e snd_path = CM_AUDIO_STATE_NONE_E;
	callui_result_e res = _callui_utils_convert_cm_res(cm_get_audio_state(sdm->cm_handler, &snd_path));
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	*audio_state = __convert_cm_audio_state(snd_path);

	return res;
}

static void __audio_state_change_handler_func(_callui_listener_t *listener, va_list args)
{
	callui_audio_state_type_e status = va_arg(args, callui_audio_state_type_e);
	((audio_state_changed_cb)(listener->cb_func))(listener->cb_data, status);
}

static void __mute_status_change_handler_func(_callui_listener_t *listener, va_list args)
{
	bool status = va_arg(args, int);
	((mute_state_changed_cb)(listener->cb_func))(listener->cb_data, status);
}

callui_result_e _callui_sdm_add_audio_state_changed_cb(callui_sound_manager_h sdm,
		audio_state_changed_cb cb_func, void *cb_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(cb_func, CALLUI_RESULT_INVALID_PARAM);

	return _callui_listeners_coll_add_listener(&sdm->audio_state_lc,
			__audio_state_change_handler_func, cb_func, cb_data);
}

callui_result_e _callui_sdm_remove_audio_state_changed_cb(callui_sound_manager_h sdm,
		audio_state_changed_cb cb_func, void *cb_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(cb_func, CALLUI_RESULT_INVALID_PARAM);

	return _callui_listeners_coll_remove_listener(&sdm->audio_state_lc, (void *)cb_func, cb_data);
}

callui_result_e _callui_sdm_add_mute_state_changed_cb(callui_sound_manager_h sdm,
		mute_state_changed_cb cb_func, void *cb_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(cb_func, CALLUI_RESULT_INVALID_PARAM);

	return _callui_listeners_coll_add_listener(&sdm->mute_status_lc,
			__mute_status_change_handler_func, cb_func, cb_data);
}

callui_result_e _callui_sdm_remove_mute_state_changed_cb(callui_sound_manager_h sdm,
		mute_state_changed_cb cb_func, void *cb_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(sdm, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(cb_func, CALLUI_RESULT_INVALID_PARAM);

	return _callui_listeners_coll_remove_listener(&sdm->mute_status_lc, (void *)cb_func, cb_data);
}
