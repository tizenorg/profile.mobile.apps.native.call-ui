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

#ifndef __CALLUI_SOUND_MANAGER_H__
#define __CALLUI_SOUND_MANAGER_H__

#include <stdbool.h>

#include "callui-common-types.h"

typedef enum {
	CALLUI_AUDIO_STATE_NONE,		/**< None */
	CALLUI_AUDIO_STATE_SPEAKER,		/**< System LoudSpeaker path */
	CALLUI_AUDIO_STATE_RECEIVER,	/**< System Receiver */
	CALLUI_AUDIO_STATE_EARJACK,		/**< Earjack path */
	CALLUI_AUDIO_STATE_BT			/**< System BT Headset path */
} callui_audio_state_type_e;

typedef struct __callui_sound_manager *callui_sound_manager_h;

/* Callback functions prototypes */
typedef void (*audio_state_changed_cb)(void *user_data, callui_audio_state_type_e state);
typedef void (*mute_state_changed_cb)(void *user_data, bool is_enable);

callui_result_e _callui_sdm_set_speaker_state(callui_sound_manager_h sdm, bool is_enable);

callui_result_e _callui_sdm_set_bluetooth_state(callui_sound_manager_h sdm, bool is_enable);

callui_audio_state_type_e _callui_sdm_get_audio_state(callui_sound_manager_h sdm);

callui_result_e _callui_sdm_start_dtmf(callui_sound_manager_h sdm, unsigned char dtmf_digit);

callui_result_e _callui_sdm_stop_dtmf(callui_sound_manager_h sdm);

callui_result_e _callui_sdm_set_mute_state(callui_sound_manager_h sdm, bool is_enable);

bool _callui_sdm_get_mute_state(callui_sound_manager_h sdm);

callui_result_e _callui_sdm_add_audio_state_changed_cb(callui_sound_manager_h sdm,
		audio_state_changed_cb cb_func, void *user_data);

callui_result_e _callui_sdm_remove_audio_state_changed_cb(callui_sound_manager_h sdm,
		audio_state_changed_cb cb_func, void *user_data);

callui_result_e _callui_sdm_add_mute_state_changed_cb(callui_sound_manager_h sdm,
		mute_state_changed_cb cb_func, void *user_data);

callui_result_e _callui_sdm_remove_mute_state_changed_cb(callui_sound_manager_h sdm,
		mute_state_changed_cb cb_func, void *user_data);

#endif /* __CALLUI_CALL_SOUND_MANAGER_H__ */
