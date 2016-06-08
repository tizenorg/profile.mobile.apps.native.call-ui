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

#ifndef __CALLUI_VIEW_ELEMENTS_H_
#define __CALLUI_VIEW_ELEMENTS_H_

#include <Elementary.h>

#include "callui.h"
#include "callui-view-elements-defines.h"

typedef enum {
	CALLUI_CID_TYPE_SINGLE,
	CALLUI_CID_TYPE_CONFERENCE,
	CALLUI_CID_TYPE_EMERGENCY,
	CALLUI_CID_TYPE_MESSAGE,
	CALLUI_CID_TYPE_COUNT
} callui_cid_type_e;

typedef enum {
	CALLUI_CID_SIZE_DEFAULT,
	CALLUI_CID_SIZE_TINY,
	CALLUI_CID_SIZE_SMALL,
	CALLUI_CID_SIZE_MEDIUM,
	CALLUI_CID_SIZE_BIG,
	CALLUI_CID_SIZE_COUNT
} callui_cid_size_e;

/**
 * @brief Load edj from file
 *
 * @param[in]    parent    Parent
 * @param[in]    file      File name
 * @param[in]    group     Group name
 *
 * @return layout
 *
 */
Evas_Object *_callui_load_edj(Evas_Object *parent, const char *file, const char *group);

/**
 * @brief Get object from part
 *
 * @param[in]    parent    Parent
 * @param[in]    part      Part name
 *
 * @return layout
 *
 */
Evas_Object *_callui_edje_object_part_get(Evas_Object *parent, const char *part);

/**
 * @brief Create end call button
 *
 * @param[in]    parent    Parent object
 * @param[in]    cb_func   Callback function
 * @param[in]    data      App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_end_call_button(Evas_Object *parent, Evas_Smart_Cb cb_func, void *data);

/**
 * @brief Create caller id thumbnail
 *
 * @param[in]    parent     Parent layout
 * @param[in]    type       Thumbnail type
 * @param[in]    type       Image path (used only for CALLUI_CID_TYPE_SINGLE if image path is available, in other case it does not taken into account)
 *
 * @return Created thumbnail or success or NULL otherwise
 *
 */
Evas_Object *_callui_create_cid_thumbnail(Evas_Object *parent, callui_cid_type_e type, const char *path);

/**
 * @brief Create caller id thumbnail with predefined size
 *
 * @param[in]    parent     Parent layout
 * @param[in]    type       Thumbnail type
 * @param[in]    size       Predefined size type
 * @param[in]    type       Image path (used only for CALLUI_CID_TYPE_SINGLE if image path is available, in other case it does not taken into account)
 *
 * @return Created thumbnail or success or NULL otherwise
 *
 */
Evas_Object *_callui_create_cid_thumbnail_with_size(Evas_Object *parent,
		callui_cid_type_e type,
		callui_cid_size_e size,
		const char *path);

/**
 * @brief Create info name
 *
 * @param[in]    data       App data
 * @param[in]    name       Info name
 *
 */
void _callui_show_caller_info_name(void *data, const char *name);

/**
 * @brief Create info number
 *
 * @param[in]    data        App data
 * @param[in]    number      Info number
 *
 */
void _callui_show_caller_info_number(void *data, const char *number);

/**
 * @brief Create info message
 *
 * @param[in]    data        App data
 * @param[in]    txt         Text of message
 *
 */
void _callui_show_caller_info_message(void *data, const char *txt);

/**
 * @brief Show caller info status
 *
 * @param[in]    data        Application data
 * @param[in]    status      Text status
 *
 * @return layout
 *
 */
Evas_Object *_callui_show_caller_info_status(void *data, const char *status);

/**
 * @brief Show caller info
 *
 * @param[in]    contents    Parent layout
 * @param[in]    path        Image path
 *
 * @return layout
 *
 */
Evas_Object *_callui_show_caller_id(Evas_Object *contents, const callui_call_data_t *call_data);

/**
 * @brief Create more popup
 *
 * @param[in]    data         View data
 *
 */
void _callui_load_more_option(void *data);

/**
 * @brief Create popup after second call to choose needed action
 *
 * @param[in]    ad           Application data
 *
 */
void _callui_load_second_call_popup(callui_app_data_t *ad);

/**
 * @brief Create bluetooth popup
 *
 * @param[in]    ad           Application data
 *
 */
void _callui_load_bluetooth_popup(callui_app_data_t *ad);

/**
 * @brief Create and set background layout
 *
 * @param[in]    app_data		Application data pointer
 * @param[in]    parent       	Parent object
 * @param[in]    part       	Part name in parent to be set reject message button
 *
 */
int _callui_create_reject_msg_button(void *app_data, Evas_Object *parent, const char *part);

#endif //__CALLUI_VIEW_ELEMENTS_H_
