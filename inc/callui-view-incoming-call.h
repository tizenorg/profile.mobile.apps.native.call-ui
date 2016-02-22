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

#ifndef __CALLUI_VIEW_INCOMING_LOCK_H__
#define __CALLUI_VIEW_INCOMING_LOCK_H__

#include <Elementary.h>

typedef struct _callui_view_incoming_call *callui_view_incoming_call_h;

/**
 * @brief Creates Incoming call view instance
 *
 * @return View data handler
 */
callui_view_incoming_call_h _callui_view_incoming_call_new();

/**
 * @brief Sets accept layout to Incoming call view
 *
 * @param[in]	vd		View data handler
 * @param[in]	layout	Accept layout
 *
 * @return CALLUI_RESULT_OK on success or error code otherwise
 */
int _callui_view_incoming_call_set_accept_layout(callui_view_incoming_call_h vd, Evas_Object *layout);

/**
 * @brief Gets accept layout from Incoming call view
 *
 * @param[in]	vd		View data handler
 *
 * @return Accept layout
 */
Evas_Object *_callui_view_incoming_call_get_accept_layout(callui_view_incoming_call_h vd);

/**
 * @brief Sets reject layout to Incoming call view
 *
 * @param[in]	vd		View data handler
 * @param[in]	layout	Reject layout
 *
 * @return CALLUI_RESULT_OK on success or error code otherwise
 */
int _callui_view_incoming_call_set_reject_layout(callui_view_incoming_call_h vd, Evas_Object *layout);

/**
 * @brief Gets reject layout from Incoming call view
 *
 * @param[in]	vd		View data handler
 *
 * @return Reject layout
 */
Evas_Object *_callui_view_incoming_call_get_reject_layout(callui_view_incoming_call_h vd);

#endif /* __CALLUI_VIEW_INCOMING_LOCK_H__ */
