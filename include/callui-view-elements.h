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
	THUMBNAIL_186,
	CONFERENCE_THUMBNAIL_186
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
 * @param[in]    data      App data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_end_call_button(Evas_Object *parent, void *data);

/**
 * @brief Destroy end call button
 *
 * @param[in]    parent    Parent object
 *
 */
void _callui_destroy_end_call_button(Evas_Object *parent);

/**
 * @brief Create disabled voice call button
 *
 * @param[in]    data      View data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_voicecall_button_disabled(void *data);

/**
 * @brief Create disabled message button
 *
 * @param[in]    data      View data
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_message_button_disabled(void *data);

/**
 * @brief Create view contact button
 *
 * @param[in]    data       View data
 * @param[in]    ct_id      Contact id
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_view_contact_button(void *data, int ct_id);

/**
 * @brief Create contact button
 *
 * @param[in]    data       View data
 * @param[in]    number     Contact number
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_create_contacts_button(void *data, char *number);

/**
 * @brief Update existing contact button
 *
 * @param[in]    data       View data
 * @param[in]    number     Contact number
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_update_existing_contact_button(void *data, char *number);

/**
 * @brief Create voice call button
 *
 * @param[in]    data       View data
 * @param[in]    number     Contact number
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_voicecall_button(void *data, char *number);

/**
 * @brief Create message button
 *
 * @param[in]    data       View data
 * @param[in]    number     Contact number
 *
 * @return layout
 *
 */
Evas_Object *_callui_create_message_button(void *data, char *number);

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
Evas_Object *_callui_show_caller_id(Evas_Object *contents, char *path);

/**
 * @brief Create focus layout
 *
 * @param[in]    parent       Parent layout
 *
 * @return focus layout
 *
 */
Evas_Object *_callui_view_create_focus_layout(Evas_Object *parent);

/**
 * @brief Create more popup
 *
 * @param[in]    data         View data
 *
 */
void _callui_load_more_option(void *data);

/**
 * @brief Set an attached data pointer to an object with a given string key.
 *
 * @param[in]    obj          The object to attach the data pointer to
 * @param[in]    key          The string key for the data to access it
 * @param[in]    value        The pointer to the data to be attached
 *
 * @return @c 0 on success, otherwise a negative error value
 *
 * @see evas_object_data_set
 *
 */
int _callui_set_object_data(Evas_Object *obj, char *key, void *value);

/**
 * @brief Return an attached data pointer on an Evas object by its given string key.
 *
 * @param[in]    obj          The object to which the data was attached
 * @param[in]    key          The string key the data was stored under
 *
 * @return @c The data pointer stored, or NULL if none was stored
 *
 * @see evas_object_data_get
 *
 */
void *_callui_get_object_data(Evas_Object *obj, char *key);

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
 * @brief Mute button callback
 *
 * @param[in]    data         Application data
 * @param[in]    obj          Parent object
 * @param[in]    event_info   The event's name string
 *
 */
void _callui_mute_btn_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Update speaker button depending on the state
 *
 * @param[in]    data         Application data
 * @param[in]    is_on        Button state
 *
 */
void _callui_update_speaker_btn(callui_app_data_t *ad, Eina_Bool is_on);

/**
 * @brief Update headset button depending on the state
 *
 * @param[in]    data         Application data
 * @param[in]    is_on        Button state
 *
 */
void _callui_update_headset_btn(callui_app_data_t *ad, Eina_Bool is_on);

/**
 * @brief Update mute button depending on the state
 *
 * @param[in]    data         Application data
 * @param[in]    is_on        Button state
 *
 */
void _callui_update_mute_btn(callui_app_data_t *ad, Eina_Bool is_on);

/**
 * @brief Update extra volume button depending on the state
 *
 * @param[in]    data         Application data
 * @param[in]    is_on        Button state
 *
 */
void _callui_update_extra_vol_btn(callui_app_data_t *ad, Eina_Bool is_on);


/**
 * @brief Create and set background layout
 *
 * @param[in]    parent       Parent object
 *
 */
void _callui_set_background_layout(Evas_Object *parent);

/**
 * @brief Gets audio mode
 *
 * @return @c true when callui is on handsfree mode, otherwise false
 *
 */
bool _callui_is_on_handsfree_mode();

/**
 * @brief Gets background state
 *
 * @return @c true when callui is on background, otherwise false
 *
 */
bool _callui_is_on_background();

#endif //__CALLUI_VIEW_ELEMENTS_H_
