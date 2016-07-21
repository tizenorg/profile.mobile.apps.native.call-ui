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

#ifndef __CALLUI_WINDOW_H__
#define __CALLUI_WINDOW_H__

#include <Elementary.h>
#include <stdbool.h>

#include "callui-common-types.h"

typedef enum {
	CALLUI_WIN_SIZE_FULLSCREEN,
	CALLUI_WIN_SIZE_ACTIVE_NOTI
} callui_win_size_type_e;

typedef enum {
	CALLUI_WIN_SCREEN_MODE_DEFAULT,
	CALLUI_WIN_SCREEN_MODE_ALWAYS_ON
} callui_win_screen_mode_e;

typedef enum {
	CALLUI_WIN_KEYGRAB_UNKNOWN,
	CALLUI_WIN_KEYGRAB_SHARED,
	CALLUI_WIN_KEYGRAB_TOPMOST,
	CALLUI_WIN_KEYGRAB_EXCLUSIVE,
	CALLUI_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE
} callui_win_keygrab_mode_e;

typedef struct __callui_window *callui_window_h;

typedef struct appdata callui_app_data_t;

/**
 * @brief Creates window instance
 *
 * @param[in]	appdata		Application data
 *
 * @return window handle
 */
callui_window_h _callui_window_create(callui_app_data_t *appdata);

/**
 * @brief Destroys window instance
 *
 * @param[in]	window_h	Window handle
 *
 */
void _callui_window_destroy(callui_window_h window_h);

/**
 * @brief Gets window Evas_Object
 *
 * @param[in]	window_h	Window handle
 *
 * @return window Evas_Object on success or NULL otherwise
 */
Evas_Object *_callui_window_get_eo(callui_window_h window_h);

/**
 * @brief Gets content parent Evas_Object
 *
 * @param[in]	window_h	Window handle
 *
 * @return content parent Evas_Object on success or NULL otherwise
 */
Evas_Object *_callui_window_get_content_parent(callui_window_h window_h);

/**
 * @brief Sets content to window
 *
 * @param[in]	window_h	Window handle
 * @param[in]	content		Content to set
 */
void _callui_window_set_content(callui_window_h window_h, Evas_Object *content);

/**
 * @brief Sets window size type
 *
 * @param[in]	window_h	Window handle
 * @param[in]	size_type	Window size type
 */
void _callui_window_set_size_type(callui_window_h window_h, callui_win_size_type_e size_type);

/**
 * @brief Sets rotation lock state
 *
 * @param[in]	window_h	Window handle
 * @param[in]	is_locked	Rotation lock state
 */
callui_result_e _callui_window_set_rotation_locked(callui_window_h window_h, bool is_locked);

/**
 * @brief Gets window rotation
 *
 * @param[in]	window_h	Window handle
 * @param[in]	is_locked	Rotation lock enable state
 *
 * @return The rotation of the window, in degrees (0-360), counter-clockwise.
 */
int _callui_window_get_rotation(callui_window_h window_h);

/**
 * @brief Sets window top level priority
 *
 * @param[in]	window_h		Window handle
 * @param[in]	is_toplevel	Top level priority enable state
 *
 * @return CALLUI_RESULT_OK on success or another result otherwise
 */
callui_result_e _callui_window_set_top_level_priority(callui_window_h window_h, bool is_toplevel);

/**
 * @brief Sets window's screen mode.
 *
 * @param[in]	window_h	Window handle
 * @param[in]	is_toplevel	Top level priority enable state
 *
 * @return CALLUI_RESULT_OK on success or another result otherwise
 */
callui_result_e _callui_window_set_screen_mode(callui_window_h window_h,
		callui_win_screen_mode_e screen_mode);

/**
 * @brief Gets screen geometry details for the screen that a window is on.
 *
* @param[in]	window_h	Window handle
 * @param[out]	x			Where to return the horizontal offset value. May be null.
 * @param[out]	y			Where to return the vertical offset value. May be null.
 * @param[out]	w			Where to return the width value. May be null.
 * @param[out]	h			Where to return the height value. May be null.
 */
void _callui_window_get_screen_size(callui_window_h window_h, int *x, int *y, int *w, int *h);

/**
 * @brief Activates window.
 *
 * @param[in]	window_h	Window handle
 */
void _callui_window_activate(callui_window_h window_h);

/**
 * @brief Lowers a window.
 *
 * @param[in]	window_h	Window handle
 */
void _callui_window_minimize(callui_window_h window_h);

/**
 * @brief Sets keygrab value of the window
 *
 * @param[in]	window_h	Window handle
 * @param[in]	key			Key to set grabbing
 * @param[in]	mode		Window keygrab mode
 *
 * @return CALLUI_RESULT_OK on success or another result otherwise
 */
callui_result_e _callui_window_set_keygrab_mode(callui_window_h window_h,
		const char *key,
		callui_win_keygrab_mode_e mode);

/**
 * @brief Unsets keygrab value of the window
 *
 * @param[in]	window_h	Window handle
 * @param[in]	key			Key to unset grabbing
 *
 * @return CALLUI_RESULT_OK on success or another result otherwise
 */
callui_result_e _callui_window_unset_keygrab_mode(callui_window_h window_h, const char *key);

/**
 * @brief Sets visibility state of indicator
 *
 * @param[in]	window_h	Window handle
 * @param[in]	is_visible	Visibility state of indicator
 *
 * @return CALLUI_RESULT_OK on success or another result otherwise
 */
callui_result_e _callui_window_set_indicator_visible(callui_window_h window_h, bool is_visible);

/**
 * @brief Sets mode of displaying application window above lock screen
 *
 * @param[in]	window_h			Window handle
 * @param[in]	is_above_lockscreen	Above lock screen displaying state
 *
 * @return CALLUI_RESULT_OK on success or another result otherwise
 */
callui_result_e _callui_window_set_above_lockscreen_mode(callui_window_h window_h, bool is_above_lockscreen);

/**
 * @brief Sets the quick panel for application window disable/enable state
 * @remark By default quick panel is enable
 *
 * @param[in]	window_h			Window handle
 * @param[in]	is_disable			Quick panel disable state to set
 *
 * @return CALLUI_RESULT_OK on success or another result otherwise
 */
callui_result_e _callui_window_set_quickpanel_disable(callui_window_h window_h, bool is_disable);

#endif /* __CALLUI_WINDOW_H__ */
