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

#ifndef __CALLUI_ONECALL_CIRCLE_VIEW_H_
#define __CALLUI_ONECALL_CIRCLE_VIEW_H_

#include "callui-view-incoming-call.h"
#include "callui.h"

#include <Elementary.h>

/**
 * @brief Create reject layout
 *
 * @param[in]	ad		Application data
 * @param[in]	vd		Incoming call lock view hander
 *
 * @return CALLUI_RESULT_OK on success or error code otherwise
 *
 */
int _callui_view_circle_create_reject_layout(callui_app_data_t *ad, callui_view_incoming_call_h vd);

/**
 * @brief Create accept layout
 *
 * @param[in]	ad		Application data
 * @param[in]	vd		Incoming call lock view hander
 *
 * @return CALLUI_RESULT_OK on success or error code otherwise
 *
 */
int _callui_view_circle_create_accept_layout(callui_app_data_t *ad, callui_view_incoming_call_h vd);

#endif
