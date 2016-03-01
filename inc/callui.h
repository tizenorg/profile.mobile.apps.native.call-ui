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

#include <efl_extension.h>
#include <call-manager.h>
#include <call-manager-extension.h>

#include "callui-view-manager.h"
#include "callui-lock-manager.h"
#include "callui-debug.h"
#include "callui-keypad.h"

#define ACTIVE_NOTIFICATION_AVAILABLE 1

#ifndef CALLUI_EXPORT_API
#define CALLUI_EXPORT_API __attribute__ ((visibility("default")))
#endif

#define MSG_PKG		"org.tizen.message"

#define PACKAGE		"org.tizen.call-ui"

#define EDJ_NAME	_callui_common_get_call_edj_path()
#define CALL_THEME	_callui_common_get_call_theme_path()

#ifndef _EDJ
#define _EDJ(obj) elm_layout_edje_get(obj)
#endif

#define CALLUI_TEXT_DOMAIN "call-ui"

#undef _
#define _(s) dgettext(CALLUI_TEXT_DOMAIN, s)

#define GRP_MAIN_LY "main_ly"
#define GRP_KEYPAD "keypad"
#define GRP_MULTICALL "multicall-list"
#define GRP_REJECT_MSG "reject_with_msg"
#define GRP_QUICKPANEL "quickpanel"
#define GRP_SEPARATOR_LAYOUT "separator-layout"
#define GRP_SEPARATOR_LAYOUT_1BUTTON "separator-layout-1button"
#define GRP_SEPARATOR_LAYOUT_2BUTTON "separator-layout-2button"
#define GRP_BUTTON_LAYOUT "button-layout"
#define APP_CONTROL_OPERATION_DURING_CALL "http://tizen.org/appcontrol/oparation/during_call"
#define APP_CONTROL_OPERATION_MESSAGE_REJECT "http://tizen.org/appcontrol/operation/message_reject"
#define APP_CONTROL_OPERATION_END_CALL "http://tizen.org/appcontrol/operation/end_call"
#define GRP_CALLER_INFO "caller-info"
#define GRP_MANAGE_CALLS "manage-calls"
#define GRP_LOCK_ACCEPT "lock_accept"
#define GRP_LOCK_REJECT "lock_reject"
#define GRP_LOCK_REJECT_WITH_MSG "lock_reject_with_msg"
#define GRP_DIMMING_LAYOUT "dimming_ly"
#define GRP_ENDCALL_MAIN_LAYOUT "main_end_call"
#define GRP_ENDCALL_CALL_BACK_BTN "call_back"
#define GRP_ENDCALL_MSG_BTN "message_button"
#define GRP_ENDCALL_CREATE_CONT_BTN "create_contact_button"

#define	 CALLUI_DISPLAY_NAME_LENGTH_MAX			(255+1)			/**< Voiecall Display Name Max Length  */
#define	 CALLUI_IMAGE_PATH_LENGTH_MAX			(255+1)			/**< Max File length for image */
#define	 CALLUI_RINGTONE_PATH_LENGTH_MAX		(255+1)			/**< Max File length for Ringtone */
#define	 CALLUI_VIB_PATH_LENGTH_MAX				(255+1)			/**< Max File length for Vibration */
#define	 CALLUI_DATA_LENGTH_MAX					(255+1)			/**< Max data length for misc */
#define	 CALLUI_PHONE_NUMBER_LENGTH_MAX			(82+1)			/**< Maximum length of a phone number  */
#define	 CALLUI_FORMATTED_NUMBER_LENGTH_MAX		(164+1)			/**< Maximum length of a phone number  */
#define	 CALLUI_PHONE_DISP_NUMBER_LENGTH_MAX	(82+10+1)		/**< Maximum length of a display phone number  */
#define	 CALLUI_PHONE_NAME_LENGTH_MAX			(80+1)			/**< Maximum length of a calling name  */
#define	 CALLUI_PHONE_SUBADDRESS_LENGTH_MAX		(40+1)			/**< Maximum length of a SUB address  */
#define	 CALLUI_PHONE_USSD_LENGTH_MAX			(182+1)			/**< Maximum length of a phone number  */
#define	 CALLUI_MAX_CALL_GROUP_MEMBER			5				/**< Maximum number of members in a group */
#define	 CALLUI_MAX_CALL_MEMBER					8				/**< This is the maximum number of members possible */
#define	 CALLUI_INVALID_CALL_ID					0				/**< This is the invalid entry for call ID */

#define NO_HANDLE 0

//typedef struct appdata callui_app_data_t;
typedef struct _call_contact_data_t {
	gint person_id;			/**< Contact index of the Caller */
	char call_disp_name[CALLUI_DISPLAY_NAME_LENGTH_MAX];			/**< Caller display name */
	char caller_id_path[CALLUI_IMAGE_PATH_LENGTH_MAX];				/**< Caller image path */
} call_contact_data_t;

typedef struct _call_data_t {
	unsigned int call_id;
	cm_call_direction_e call_direction;
	char call_num[CALLUI_PHONE_NUMBER_LENGTH_MAX];
	char call_disp_num[CALLUI_PHONE_DISP_NUMBER_LENGTH_MAX];		/**< Caller display number */
	long start_time;
	cm_call_type_e call_type;					/**< Specifies type of call (voice, data, emergency) */
	cm_call_state_e call_state;					/**< Current Call state */
	int member_count;						/**< Whether Call is in Conference or not and indicate the number of members in that conference*/
	gboolean is_emergency;						/**< is emergency*/
	cm_call_domain_e call_domain;				/**< Current Call domain */
	call_contact_data_t call_ct_info;	/**< Contact information */
}call_data_t;

typedef struct _callui_qp_mc *callui_qp_mc_h;

struct appdata {
	Evas_Object *win;
	Evas_Object *win_conformant;
	Evas_Object *nf;

	/* Main Layout contents Start */
	Evas_Object *main_ly;
	/* Main Layout contents End */
	callui_qp_mc_h qp_minicontrol;

	Evas_Object *ctxpopup;
	Evas_Object *second_call_popup;
	Evas_Object *bt_popup;

	callui_vm_h view_manager_handle;

	bool multi_call_list_end_clicked;

	lock_data_t *lock_handle;
	bool start_lock_manager_on_resume;
	bool on_background;

	int root_w;	/**<Width of a root window */
	int root_h;	/**<Height of a root window */

	Eina_Bool speaker_status;
	Eina_Bool extra_volume_status;
	Eina_Bool extra_volume_status_force_stop;
	Eina_Bool headset_status;
	Eina_Bool earphone_status;
	Eina_Bool mute_status;
	Eina_Bool b_earset_key_longpress;

	cm_client_h cm_handle;		/**<Call manager client handle */
	cm_multi_sim_slot_type_e sim_slot;

	/*call list*/
	call_data_t *incom;
	call_data_t *active;
	call_data_t *held;

	bool incoming_noti;

	bool waiting_dialing;

	/*Call duration*/
	Ecore_Timer *blink_timer;
	Ecore_Timer *ending_timer;
	Ecore_Timer *duration_timer;
	int blink_cnt;
	int current_sec;
	int current_min;
	int current_hour;

	Ecore_Event_Handler *downkey_handler;
	Ecore_Event_Handler *upkey_handler;
	Ecore_Timer *earset_key_longpress_timer;


	callui_keypad_h keypad;
};

callui_app_data_t *_callui_get_app_data();

#endif// __CALLUI_H_
