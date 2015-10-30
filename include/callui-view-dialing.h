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

#ifndef __CALLUI_DIALING_VIEW_H_
#define __CALLUI_DIALING_VIEW_H_

#include "callui.h"
#include "callui-view-manager.h"

typedef struct callui_view_dialing_priv callui_view_dialing_priv_t;

/**
 * @brief Create dialing view
 *
 * @param[in]    ad     Application data
 *
 * @return New dialing view data
 *
 */
call_view_data_t *_callui_dialing_view_dialing_new(callui_app_data_t *ad);

#endif //__CALLUI_DIALING_VIEW_H_
