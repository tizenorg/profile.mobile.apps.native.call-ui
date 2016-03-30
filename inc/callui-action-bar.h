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

#ifndef _CALLUI_ACTION_BAR_H_
#define _CALLUI_ACTION_BAR_H_

#include <Elementary.h>

typedef struct _callui_action_bar *callui_action_bar_h;

typedef struct appdata callui_app_data_t;

/**
 * @brief Create action bar
 *
 * @param[in]	appdata		Application data
 *
 * @return Action bar instance
 *
 */
callui_action_bar_h _callui_action_bar_create(callui_app_data_t *appdata);

/**
 * @brief Delete action bar
 *
 * @param[in]	action_bar	Action bar handler
 *
 */
void _callui_action_bar_destroy(callui_action_bar_h action_bar);

/**
 * @brief Show action bar
 *
 * @param[in]	action_bar	Action bar handler
 *
 */
void _callui_action_bar_show(callui_action_bar_h action_bar);

/**
 * @brief Hide action bar
 *
 * @param[in]	action_bar	Action bar handler
 *
 */
void _callui_action_bar_hide(callui_action_bar_h action_bar);

#endif /* _CALLUI_ACTION_BAR_H_ */
