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

#ifndef __CALLUI_VIEW_INCOMING_CALL_H_
#define __CALLUI_VIEW_INCOMING_CALL_H_

#include "callui-view-manager.h"
#include "callui.h"

/**
 * @brief Draw screen for incoming call
 *
 * @param[in]   ad            Call ui app data
 * @param[in]   vd            Call view data
 *
 */
void _callui_view_incoming_call_draw_screen(callui_app_data_t *ad, call_view_data_t *vd);

/**
 * @brief Create layout for incoming call
 *
 * @param[in]   vd            Call view data
 * @param[in]   appdata       Application data
 *
 */
int _callui_view_incoming_call_oncreate(call_view_data_t *view_data, void *appdata);

/**
 * @brief Destroy incoming call
 *
 * @param[in]   data         View data
 *
 */
void _callui_view_incoming_call_ondestroy(void *data);

#endif /* __CALLUI_VIEW_INCOMING_CALL_H_ */
