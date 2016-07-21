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

#ifndef __CALLUI_STATE_PROVIDER_PRIV_H__
#define __CALLUI_STATE_PROVIDER_PRIV_H__

#include <call-manager.h>

typedef struct __callui_state_provider *callui_state_provider_h;

/**
 * @brief Creates state provider instance
 *
 * @param[in]	cm_client	Platform call manager client handle
 *
 * @return State provider handle on success or NULL otherwise
 */
callui_state_provider_h _callui_stp_create(cm_client_h cm_client);

/**
 * @brief Destroys state provider instance
 *
 * @param[in]	stp_h		State provider handle
 */
void _callui_stp_destroy(callui_state_provider_h stp_h);

#endif /* __CALLUI_CALL_STATE_PROVIDER_PRIV_H__ */
