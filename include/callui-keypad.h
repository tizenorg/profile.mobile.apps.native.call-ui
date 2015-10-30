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

/**
 * @brief Get show keypad status
 *
 * @return show status
 *
 */
Eina_Bool _callui_keypad_get_show_status(void);

/**
 * @brief Show keypad layout
 *
 * @param[in]    appdata         Application data
 *
 */
void _callui_keypad_show_layout(void *appdata);

/**
 * @brief Hide keypad layout
 *
 * @param[in]    appdata         Application data
 *
 */
void _callui_keypad_hide_layout(void *appdata);

/**
 * @brief Create keypad layout
 *
 * @param[in]    appdata         Application data
 *
 */
void _callui_keypad_create_layout(void *appdata);

/**
 * @brief Delete keypad layout
 *
 * @param[in]    appdata         Application data
 *
 */
void _callui_keypad_delete_layout(void *appdata);


#endif	// _CALLUI_VIEW_KEYPAD_H_
