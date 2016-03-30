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

#ifndef __CALLUI_H_
#define __CALLUI_H_

#include <Elementary.h>

#include "callui-view-manager.h"
#include "callui-lock-manager.h"
#include "callui-keypad.h"
#include "callui-manager.h"
#include "callui-view-quickpanel.h"
#include "callui-action-bar.h"

struct appdata {
	Evas_Object *win;
	Evas_Object *win_conformant;
	Evas_Object *nf;
	Evas_Object *main_ly;

	callui_qp_mc_h qp_minicontrol;

	Evas_Object *ctxpopup;
	Evas_Object *second_call_popup;
	Evas_Object *bt_popup;

	callui_vm_h view_manager;

	bool multi_call_list_end_clicked;

	lock_data_t *lock_handle;
	bool start_lock_manager_on_resume;
	bool on_background;

	int root_w;	/**<Width of a root window */
	int root_h;	/**<Height of a root window */

	bool waiting_dialing;

	Ecore_Event_Handler *downkey_handler;
	Ecore_Event_Handler *upkey_handler;
	Ecore_Timer *earset_key_longpress_timer;

	callui_keypad_h keypad;
	callui_action_bar_h action_bar;

	callui_manager_h call_manager;
	callui_state_provider_h state_provider;
	callui_sound_manager_h sound_manager;
};

callui_app_data_t *_callui_get_app_data();

#endif// __CALLUI_H_
