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


#ifndef __VCUI_VIEW_QUICKPANEL_H_
#define __VCUI_VIEW_QUICKPANEL_H_

/**
 * @brief Create quickpanel view
 *
 * @return quickpanel view
 *
 */
call_view_data_t *_callui_view_qp_new();

/**
 * @brief Hide quick panel
 *
 * @param[in] ad                  Application data
 *
 */
void _callui_view_qp_hide(callui_app_data_t *ad);

/**
 * @brief Update text
 *
 * @param[in] txt_status          The status text
 * @param[in] count               The number of active calls
 * @param[in] eo                  Quick panel layout
 *
 */
void _callui_view_qp_update_text(char *txt_status, int count, Evas_Object *eo);

/**
 * @brief Create quick panel mute button
 *
 * @param[in] data                Application data
 * @param[in] bdisable            Disable state
 *
 * @return button
 *
 */
Evas_Object *_callui_create_quickpanel_mute_button(void *data, Eina_Bool bdisable);

/**
 * @brief Create quick panel speaker button
 *
 * @param[in] data               Application data
 * @param[in] bdisable           Speaker state
 *
 * @return button
 *
 */
Evas_Object *_callui_create_quickpanel_speaker_button(void *data, Eina_Bool bdisable);

/**
 * @brief Create quick panel if it not exist or update.
 *
 */
void _callui_view_quickpanel_change();

/**
 * @brief Set call timer
 *
 * @param[in] qp_layout          Quick panel layout
 * @param[in] pcall_timer        Timer
 *
 */
void _callui_view_qp_set_call_timer(Evas_Object *qp_layout, char *pcall_timer);

#endif				/*__VCUI_VIEW_QUICKPANEL_H_*/
