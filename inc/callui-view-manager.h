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

#ifndef __CALLUI_VM_H_
#define __CALLUI_VM_H_

#include <Elementary.h>

typedef enum {
	VIEW_UNDEFINED_TYPE = -1,
	VIEW_DIALLING_VIEW,	/**< Dialling view*/
	VIEW_INCOMING_LOCK_VIEW,/**< Incoming lock view*/
	VIEW_INCALL_ONECALL_VIEW,/**< Incoming single call view*/
	VIEW_INCALL_MULTICALL_SPLIT_VIEW,/**< Multicall split view */
	VIEW_INCALL_MULTICALL_CONF_VIEW,/**< Multicall conference view */
	VIEW_INCALL_MULTICALL_LIST_VIEW,/**< Multicall list view */
	VIEW_ENDCALL_VIEW,/**< End call view */
	VIEW_QUICKPANEL_VIEW,/**< Quick panel view */
	VIEW_MAX/**< Last view */
} callui_view_id_t;

struct _view_data;

typedef int (*create_cb)	(struct _view_data *view_data, unsigned int param1, void *param2, void *appdata);
typedef int (*update_cb)	(struct _view_data *view_data, void *update_data);
typedef int (*destroy_cb)	(struct _view_data *view_data);
typedef int (*show_cb)		(struct _view_data *view_data, void *appdata);
typedef int (*hide_cb)		(struct _view_data *view_data);
typedef int (*rotate_cb)	(struct _view_data *view_data);

typedef struct _view_data {
	callui_view_id_t type;	//CM_UI to do removed 	vcui_app_call_data_t *app_data;

	create_cb onCreate;
	update_cb onUpdate;
	destroy_cb onDestroy;
	show_cb onShow;
	hide_cb onHide;
	rotate_cb onRotate;

	Evas_Object *layout;
	void *priv;
} call_view_data_t;

typedef struct _view_manager_data view_manager_data_t;

/**
 * @brief Initialize view manager
 *
 * @return Manager data
 *
 */
view_manager_data_t *_callvm_init();

/**
 * @brief Change view
 *
 * @param[in]    view_id                 View id
 * @param[in]    param1                  Param to show view
 * @param[in]    param2                  Param to show view
 * @param[in]    appdata                 Application data
 *
 */
void _callvm_view_change(callui_view_id_t view_id, unsigned int param1, void *param2, void *appdata);

/**
 * @brief Get top view ID
 *
 * @param[in]    view_manager_handle      Manager handle
 *
 * @return view id
 *
 */
callui_view_id_t _callvm_get_top_view_id(view_manager_data_t *view_manager_handle);

/**
 * @brief View auto changed
 *
 * @param[in]    appdata                  Application data
 *
 */
void _callvm_view_auto_change(void *appdata);

/**
 * @brief Terminate app or change view
 *
 * @param[in]    appdata                  Application data
 *
 */
void _callvm_terminate_app_or_view_change(void *appdata);

/**
 * @brief Get call view data
 *
 * @param[in]    ad                       Application data
 * @param[in]    view_id                  View ID
 *
 * @return view data
 *
 */
call_view_data_t * _callvm_get_call_view_data(void *ad, callui_view_id_t view_id);

/**
 * @brief Set call view data
 *
 * @param[in]    ad                       Application data
 * @param[in]    view_id                  View ID
 * @param[in]    vd                       View data
 *
 */
void _callvm_set_call_view_data(void *appdata, callui_view_id_t view_id,call_view_data_t *vd);

/**
 * @brief Get view layout
 *
 * @param[in]    ad                      Application data
 *
 * @return view layout
 *
 */
Evas_Object *_callvm_get_view_layout(void *appdata);

/**
 * @brief Reset call view data
 *
 * @param[in]    ad                      Application data
 * @param[in]    view_id                 View ID
 *
 */
void _callvm_reset_call_view_data(void *appdata, callui_view_id_t view_id);

#endif //__CALLUI_VM_H_
