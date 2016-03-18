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

#define	CALLUI_END_TYPE_KEY		"END_TYPE"
#define	CALLUI_END_TYPE_SINGLE_CALL_END	"SINGLE_CALL_END"
#define	CALLUI_END_TYPE_CONF_CALL_END	"CONF_CALL_END"

typedef enum {
	THUMBNAIL_98,
	THUMBNAIL_138,
	CONFERENCE_THUMBNAIL_138
} thumbnail_type;

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
 * @brief Create top first button
 *
 * @param[in]    ad       App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_top_first_button(callui_app_data_t *ad);

/**
 * @brief Create disabled top first button
 *
 * @param[in]    ad       App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_top_first_button_disabled(callui_app_data_t *ad);

/**
 * @brief Create top second button
 *
 * @param[in]    ad       App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_top_second_button(callui_app_data_t *ad);

/**
 * @brief Create disabled top second button
 *
 * @param[in]    ad       App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_top_second_button_disabled(callui_app_data_t *ad);

/**
 * @brief Create top third button
 *
 * @param[in]    ad       App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_top_third_button(callui_app_data_t *ad);

/**
 * @brief Create disabled top third button
 *
 * @param[in]    ad       App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_top_third_button_disabled(callui_app_data_t *ad);

/**
 * @brief Create bottom first button
 *
 * @param[in]    ad       App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_bottom_first_button(callui_app_data_t *ad);

/**
 * @brief Create disabled bottom first button
 *
 * @param[in]    ad        App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_bottom_first_button_disabled(callui_app_data_t *ad);

/**
 * @brief Create bottom second button
 *
 * @param[in]    ad        App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_bottom_second_button(callui_app_data_t *ad);

/**
 * @brief Create disabled bottom second button
 *
 * @param[in]    ad        App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_bottom_second_button_disabled(callui_app_data_t *ad);

/**
 * @brief Create bottom third button
 *
 * @param[in]    ad        App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_bottom_third_button(callui_app_data_t *ad);

/**
 * @brief Create disabled bottom third button
 *
 * @param[in]    ad        App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_bottom_third_button_disabled(callui_app_data_t *ad);

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
 * @brief Destroy end call button
 *
 * @param[in]    parent    Parent object
 *
 */
void _callui_destroy_end_call_button(Evas_Object *parent);

/**
 * @brief Create bg layout
 *
 * @param[in]    parent     Parent layout
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_bg_layout(Evas_Object *parent);

/**
 * @brief Create thumbnail
 *
 * @param[in]    parent     Parent layout
 * @param[in]    path       Image path
 * @param[in]    type       Image type
 *
 * @return layout
 *
 */

Evas_Object *_callui_create_thumbnail(Evas_Object *parent, const char *path, thumbnail_type type);

/**
 * @brief Create thumbnail with size
 *
 * @param[in]    parent     Parent layout
 * @param[in]    path       Image path
 * @param[in]    type       Image type
 * @param[in]    type       Image size
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_thumbnail_with_size(Evas_Object *parent, const char *path, thumbnail_type type, bool set_size);

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
Evas_Object *_callui_show_caller_id(Evas_Object *contents, const char *path);

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
 * @brief Create extra volume notification popup
 *
 */
void _callui_create_extravolume_notify_popup(void);

/**
 * @brief Create toast message
 *
 * @param[in]    string       The messages to be posted
 *
 */
void _callui_create_toast_message(char *string);

/**
 * @brief Speaker button callback
 *
 * @param[in]    data         Application data
 * @param[in]    obj          Parent object
 * @param[in]    event_info   The event's name string
 *
 */
void _callui_spk_btn_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Mute button callback
 *
 * @param[in]    data         Application data
 * @param[in]    obj          Parent object
 * @param[in]    event_info   The event's name string
 *
 */
void _callui_mute_btn_cb(void *data, Evas_Object *obj, void *event_info);

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
