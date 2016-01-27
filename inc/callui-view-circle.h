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

#include "callui.h"

/**
 * @brief Reset accept/reject layout
 *
 * @param[in]    data     View data
 *
 */
void _callui_view_circle_accept_reject_reset(void *data);

/**
 * @brief Create reject layout
 *
 * @param[in]    ad       Application data
 * @param[in]    data     View data
 *
 * @return Reject layout
 *
 */
Evas_Object *_callui_view_circle_create_reject_layout(callui_app_data_t *ad, void *data);

/**
 * @brief Create accept layout
 *
 * @param[in]    ad       Application data
 * @param[in]    data     View data
 *
 * @return Accept layout
 *
 */
Evas_Object *_callui_view_circle_create_accept_layout(callui_app_data_t *ad, void *data);

#endif
