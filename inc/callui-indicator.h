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

#ifndef __CALLUI_INDICATOR_H__
#define __CALLUI_INDICATOR_H__

#include "callui-common-types.h"

typedef struct __callui_indicator *callui_indicator_h;

typedef struct appdata callui_app_data_t;

/**
 * @brief Creates indicator
 *
 * @param[in]	appdata		Application data
 *
 * @return Indicator instance
 *
 */
callui_indicator_h callui_indicator_create(callui_app_data_t *appdata);

/**
 * @brief Deletes indicator
 *
 * @param[in]	indicator	Indicator handler
 *
 */
void callui_indicator_destroy(callui_indicator_h indicator);

/**
 * @brief Sets indicator active state
 *
 * @param[in]	indicator	Indicator handler
 * @param[in]	is_active	Flag to set indicator state
 */
void callui_indicator_set_active(callui_indicator_h indicator, bool is_active);

/**
 * @brief Force deactivates indicator if it was in active state
 *
 * @param[in]	indicator	Indicator handler
 */
void callui_indicator_force_deativate(callui_indicator_h indicator);

#endif /* __CALLUI_INDICATOR_H__ */
