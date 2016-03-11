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

#ifndef __CALLUI_STATE_PROVIDER_PRIV_H__
#define __CALLUI_STATE_PROVIDER_PRIV_H__

#include <call-manager.h>

typedef struct __callui_state_provider *callui_state_provider_h;

callui_state_provider_h _callui_stp_create(cm_client_h cm_client);

void _callui_stp_destroy(callui_state_provider_h sp_handler);

#endif /* __CALLUI_CALL_STATE_PROVIDER_PRIV_H__ */
