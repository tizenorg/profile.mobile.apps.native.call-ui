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

#ifndef __CALLUI_INCOMINGLOCK_VIEW_H_
#define __CALLUI_INCOMINGLOCK_VIEW_H_

#include "callui-view-manager.h"
#include "callui.h"

/**
 * @brief Create Incoming lock view
 *
 * @return View data
 *
 */
call_view_data_t *_callui_view_incoming_lock_new();

/**
 * @brief Incoming lock set accept layout
 *
 * @param[in]   vd                     view data
 * @param[in]   layout                 Accept layout
 *
 */
void _callui_view_incoming_lock_set_accept_layout(call_view_data_t *vd, Evas_Object *layout);

/**
 * @brief Incoming lock get accept layout
 *
 * @param[in]   vd                      View data
 *
 * @return Accept layout
 *
 */
Evas_Object *_callui_view_incoming_lock_get_accept_layout(call_view_data_t *vd);

/**
 * @brief Incoming lock get reject layout
 *
 * @param[in]   vd                      View data
 *
 * @return Reject layout
 *
 */
Evas_Object *_callui_view_incoming_lock_get_reject_layout(call_view_data_t *vd);

/**
 * @brief Incoming lock set reject layout
 *
 * @param[in]   vd                      View data
 * @param[in]   layout                  Reject layout
 *
 */
void _callui_view_incoming_lock_set_reject_layout(call_view_data_t *vd, Evas_Object *layout);

/**
 * @brief Send reject message
 *
 * @param[in]   data                    View data
 * @param[in]   call_data               Call data
 *
 */
void _callui_view_incoming_lock_view_send_reject_msg(void *data, call_data_t *call_data);

/**
 * @brief Create item class
 *
 * @return Elm_Genlist_Item_Class
 *
 */
Elm_Genlist_Item_Class * _callui_view_incoming_lock_create_item_class();

/**
 * @brief Create reject message button
 *
 * @param[in]   parent                  Parent object
 * @param[in]   part                    Part to set button
 * @param[in]   data                    View data
 *
 */
void _callui_view_incoming_lock_create_reject_msg_button(Evas_Object *parent, char *part, void *data);

/**
 * @brief Append item to genlist
 *
 * @param[in]   msg_glist               Message genlist
 * @param[in]   itc_reject_msg          Genlist item class
 * @param[in]   index                   Item index
 *
 * @return Item
 *
 */
Elm_Object_Item *_callui_view_incoming_lock_append_genlist_item(Evas_Object *msg_glist, Elm_Genlist_Item_Class * itc_reject_msg, int index);

#define CALLUI_REJECT_MSG_LENGTH_MAX (210+1)
typedef struct {
	Evas_Object *contents;
	Evas_Object *caller_info;
	Evas_Object *dimming_ly;

	Evas_Object *lock_accept;
	Evas_Object *lock_reject;

	Evas_Object *ic;
	Evas_Object *lock_reject_with_msg;
	Evas_Object *reject_msg_gl;
	Evas_Object *msg_glist;

	int reject_with_msg_start_y;
	int reject_with_msg_cur_y;

	int msg_list_height;

	gboolean bmouse_down_pressed;

	Elm_Genlist_Item_Class *itc_reject_msg;
	Evas_Coord y_momentum;
	char reject_msg[CALLUI_REJECT_MSG_LENGTH_MAX];

} incoming_lock_view_priv_t;

#endif //__CALLUI_INCOMINGLOCK_VIEW_H_
