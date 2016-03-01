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


#ifndef _CALLUI_VIEW_KEYPAD_H_
#define _CALLUI_VIEW_KEYPAD_H_

#include <Elementary.h>

typedef struct _keypad_widget_data *keypad_widget_h;

typedef void (*show_state_change_cd)(void *data, Eina_Bool visibility);

/**
 * @brief Create keypad layout
 *
 * @param[in]	parent		Parent evas object
 * @param[in]	appdata		Application data
 *
 * @return CALLUI_RESULT_OK on success or error code otherwise
 *
 */
keypad_widget_h _callui_keypad_create(Evas_Object *parent, void *appdata);

/**
 * @brief Delete keypad layout
 *
 * @param[in]	keypad		Keypad handler
 *
 */
void _callui_keypad_destroy(keypad_widget_h keypad);

/**
 * @brief Clear input keypad value
 *
 * @param[in]	keypad		Keypad handler
 *
 */
void _callui_keypad_clear_input(keypad_widget_h keypad);

/**
 * @brief Get show keypad status
 *
 * @param[in]	keypad		Keypad handler
 *
 * @return show status
 *
 */
Eina_Bool _callui_keypad_get_show_status(keypad_widget_h keypad);

/**
 * @brief Show keypad
 *
 * @param[in]	keypad		Keypad handler
 *
 */
void _callui_keypad_show(keypad_widget_h keypad);

/**
 * @brief Hide keypad
 *
 * @param[in]	keypad		Keypad handler
 *
 */
void _callui_keypad_hide(keypad_widget_h keypad);

/**
 * @brief Hide keypad without close animation
 *
 * @param[in]	keypad		Keypad handler
 *
 */
void _callui_keypad_hide_immediately(keypad_widget_h keypad);

/**
 * @brief Add keypad show status change callback
 *
 * @param[in]	keypad		Keypad handler
 * @param[in]	cb_func		Callback function
 * @param[in]	cb_data		Callback data
 */
void _callui_keypad_show_status_change_callback_set(keypad_widget_h keypad,
		show_state_change_cd cb_func,
		void *cb_data);

#endif	// _CALLUI_VIEW_KEYPAD_H_
