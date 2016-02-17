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

#ifndef __UI_VIEW_QUICKPANEL_H_
#define __UI_VIEW_QUICKPANEL_H_

#include <Eina.h>

typedef struct _callui_qp_mc *callui_qp_mc_h;

callui_qp_mc_h _callui_qp_mc_create(callui_app_data_t *ad);

void _callui_qp_mc_destroy(callui_qp_mc_h qp);

void _callui_qp_mc_update_mute_status(callui_qp_mc_h qp, Eina_Bool is_disable);

void _callui_qp_mc_update_speaker_status(callui_qp_mc_h qp, Eina_Bool is_disable);

void _callui_qp_mc_update_calltime_status(callui_qp_mc_h qp, char *call_timer);

void _callui_qp_mc_update(callui_qp_mc_h qp);

#endif	/*__UI_VIEW_QUICKPANEL_H_*/
