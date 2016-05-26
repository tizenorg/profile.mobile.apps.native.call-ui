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

#ifndef __CALLUI_DISPLAY_H__
#define __CALLUI_DISPLAY_H__

#include "callui-common-types.h"

typedef enum {
	CALLUI_DISPLAY_TIMEOUT_DEFAULT = 0,
	CALLUI_DISPLAY_TIMEOUT_SET,
	CALLUI_DISPLAY_TIMEOUT_UNSET,
	CALLUI_DISPLAY_TIMEOUT_LS_SET	/* After lock-screen comes in Connected state display goes to OFF in 5 secs */
} callui_display_timeout_e;

typedef enum {
	CALLUI_DISPLAY_ON = 0,
	CALLUI_DISPLAY_ON_LOCK,
	CALLUI_DISPLAY_ON_UNLOCK,
	CALLUI_DISPLAY_UNLOCK,
	CALLUI_DISPLAY_OFF_SLEEP_LOCK,
	CALLUI_DISPLAY_OFF_SLEEP_UNLOCK,
	CALLUI_DISPLAY_OFF
} callui_display_control_e;

typedef struct __callui_display *callui_display_h;

typedef struct appdata callui_app_data_t;

callui_display_h _callui_display_create(callui_app_data_t *appdata);

void _callui_display_destroy(callui_display_h disp);

callui_result_e _callui_display_set_timeout(callui_display_h disp, callui_display_timeout_e timeout_type);

callui_result_e _callui_display_set_control_state(callui_display_h disp, callui_display_control_e state);

callui_result_e _callui_display_get_control_state(callui_display_h disp, callui_display_control_e *state);

#endif /* __CALLUI_DISPLAY_H__ */
