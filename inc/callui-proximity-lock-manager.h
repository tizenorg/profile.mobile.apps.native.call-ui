/*
 * Copyright (c) 2009-2016 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __CALLUI_PROXIMITY_LOCK_MANAGER_H_
#define __CALLUI_PROXIMITY_LOCK_MANAGER_H_

#include "callui-lock-manager.h"

/**
 * @brief Gets proximity sensor supported status
 *
 * @return true when proximity sensor is supported, otherwise false
 */
bool _callui_proximity_lock_manager_is_supported();

/**
 * @brief Create proximity lock manager
 *
 * @param[in] lock_h The lock handle
 *
 * @see lock_data_t
 */
void _callui_proximity_lock_manager_init(lock_data_t *lock_h);

#endif /** __CALLUI_PROXIMITY_LOCK_MANAGER_H_ */
