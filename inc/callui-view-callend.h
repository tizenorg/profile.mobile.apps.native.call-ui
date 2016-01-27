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

#ifndef _CALLUI_VIEW_CALLEND_H_
#define _CALLUI_VIEW_CALLEND_H_

#include "callui-view-manager.h"

typedef struct callui_endcall_view_priv callui_endcall_view_priv_t;

/**
 * @brief Create end call layout
 *
 * @param[in]    ad              Application data
 *
 * @return End call view data
 *
 */
call_view_data_t *_callui_view_callend_new(callui_app_data_t *ad);

/**
 * @brief Get end call layout
 *
 * @param[in]    vd              View data
 *
 * @return End call layout
 *
 */
Evas_Object *_callui_view_callend_get_layout(call_view_data_t *vd);

#endif //_CALLUI_VIEW_CALLEND_H_
