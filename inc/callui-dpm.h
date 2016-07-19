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


#ifndef __CALLUI_DPM_H__
#define __CALLUI_DPM_H__

#include <stdbool.h>

typedef struct __callui_dpm *callui_dpm_h;

/**
 * @brief Creates device policy manager
 *
 * @return Device policy manager instance
 *
 */
callui_dpm_h _callui_dpm_create();

/**
 * @brief Destroys device policy manager
 *
 * @param[in]	dpm_handler		Device policy manager instance
 *
 */
void _callui_dpm_destroy(callui_dpm_h dpm_handler);

/**
 * @brief Checks enforce change password state
 *
 * @param[in]	dpm_handler		Device policy manager instance
 *
 */
bool _callui_dpm_is_need_enforce_change_password(callui_dpm_h dpm_handler);

#endif /* __CALLUI_DPM_H__ */
