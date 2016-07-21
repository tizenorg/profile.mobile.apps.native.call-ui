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

#ifndef __UI_VIEW_QUICKPANEL_H__
#define __UI_VIEW_QUICKPANEL_H__

#include <Eina.h>

typedef struct _callui_qp_mc *callui_qp_mc_h;

typedef struct appdata callui_app_data_t;

/**
 * @brief Creates quick panel mini control instance
 *
 * @param[in]	ad	Application data
 *
 * @return Quick panel mini control handle
 */
callui_qp_mc_h _callui_qp_mc_create(callui_app_data_t *ad);

/**
 * @brief Destroys quick panel mini control view instance
 *
 * @param[in]	qp	Quick panel mini control handle
 *
 * @return Quick panel mini control view handle
 */
void _callui_qp_mc_destroy(callui_qp_mc_h qp);

#endif	/*__UI_VIEW_QUICKPANEL_H__*/
