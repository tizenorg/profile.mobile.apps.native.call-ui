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

#ifndef __CALLUI_VIEW_MANAGER_H__
#define __CALLUI_VIEW_MANAGER_H__

#include <Elementary.h>
#include <Ecore.h>
#include <time.h>

#include "callui-common-types.h"

typedef enum {
	VIEW_TYPE_UNDEFINED = -1,
	VIEW_TYPE_DIALLING,				/**< Dialling view*/
	VIEW_TYPE_INCOMING_CALL_NOTI,	/**< Incoming active notification view*/
	VIEW_TYPE_INCOMING_CALL,		/**< Incoming lock view*/
	VIEW_TYPE_SINGLECALL,			/**< Incoming single call view*/
	VIEW_TYPE_MULTICALL_SPLIT,		/**< Multicall split view */
	VIEW_TYPE_MULTICALL_CONF,		/**< Multicall conference view */
	VIEW_TYPE_MULTICALL_LIST,		/**< Multicall list view */
	VIEW_TYPE_ENDCALL,				/**< End call view */
	VIEW_TYPE_QUICKPANEL,			/**< Quick panel view */
	VIEW_TYPE_MAX					/**< Max view count*/
} callui_view_type_e;

struct _view_data;

typedef callui_result_e (*create_cb) (struct _view_data *view_data, void *appdata);
typedef callui_result_e (*update_cb) (struct _view_data *view_data);
typedef callui_result_e (*destroy_cb) (struct _view_data *view_data);

typedef struct appdata callui_app_data_t;

struct _view_data {
	create_cb onCreate;
	update_cb onUpdate;
	destroy_cb onDestroy;

	callui_app_data_t *ad;

	Evas_Object *contents;

	Ecore_Timer *call_duration_timer;
	struct tm *call_duration_tm;
};
typedef struct _view_data call_view_data_base_t;

typedef struct _callui_vm *callui_vm_h;

/**
 * @brief Create view manager
 *
 * @return view manager handler
 */
callui_vm_h _callui_vm_create(callui_app_data_t *ad);

/**
 * @brief Destroy view manager
 *
 * @param[in]	vm		View manager handler
 */
void _callui_vm_destroy(callui_vm_h vm);

/**
 * @brief Change view
 *
 * @param[in]	vm		View manager handler
 * @param[in]	type	View type
 *
 * @return result CALLUI_RESULT_OK on success
 */
callui_result_e _callui_vm_change_view(callui_vm_h vm, callui_view_type_e type);

/**
 * @brief Auto change view
 *
 * @param[in]	vm		View manager handler
 *
 * @return result CALLUI_RESULT_OK on success
 */
callui_result_e _callui_vm_auto_change_view(callui_vm_h vm);

/**
 * @brief Get top view type
 *
 * @param[in]	vm		View manager handler
 *
 * @return view type
 */
callui_view_type_e _callui_vm_get_cur_view_type(callui_vm_h vm);

#endif /* __CALLUI_VIEW_MANAGER_H__ */
