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

#ifndef __CALLUI_COMMON_DEFINES_H__
#define __CALLUI_COMMON_DEFINES_H__

#define _DBUS_DISPLAY_DEVICE_TIMEOUT_

#ifndef CALLUI_EXPORT_API
#define CALLUI_EXPORT_API __attribute__ ((visibility("default")))
#endif

#define PACKAGE	"org.tizen.call-ui"

#define CALL_EDJ_NAME		"/edje/call.edj"
#define CALL_THEME_EDJ_NAME	"/edje/call_theme.edj"

#define EDJ_NAME	_callui_common_get_call_edj_path()
#define CALL_THEME	_callui_common_get_call_theme_path()

#ifndef _EDJ
#define _EDJ(obj) elm_layout_edje_get(obj)
#endif

#undef _
#define _(s) gettext(s)

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
#define GRP_ENDCALL_ADD_CONTACT_BTN "add_contact_button"

#define	CALLUI_DISPLAY_NAME_LENGTH_MAX			(255+1)			/**< Voiecall Display Name Max Length  */
#define	CALLUI_IMAGE_PATH_LENGTH_MAX			(255+1)			/**< Max File length for image */
#define	CALLUI_RINGTONE_PATH_LENGTH_MAX			(255+1)			/**< Max File length for Ringtone */
#define	CALLUI_VIB_PATH_LENGTH_MAX				(255+1)			/**< Max File length for Vibration */
#define	CALLUI_DATA_LENGTH_MAX					(255+1)			/**< Max data length for misc */
#define	CALLUI_PHONE_NUMBER_LENGTH_MAX			(82+1)			/**< Maximum length of a phone number  */
#define	CALLUI_FORMATTED_NUMBER_LENGTH_MAX		(164+1)			/**< Maximum length of a phone number  */
#define	CALLUI_PHONE_DISP_NUMBER_LENGTH_MAX		(82+10+1)		/**< Maximum length of a display phone number  */
#define	CALLUI_PHONE_NAME_LENGTH_MAX			(80+1)			/**< Maximum length of a calling name  */
#define	CALLUI_PHONE_SUBADDRESS_LENGTH_MAX		(40+1)			/**< Maximum length of a SUB address  */
#define	CALLUI_PHONE_USSD_LENGTH_MAX			(182+1)			/**< Maximum length of a phone number  */
#define	CALLUI_MAX_CALL_GROUP_MEMBER			5				/**< Maximum number of members in a group */
#define	CALLUI_MAX_CALL_MEMBER					8				/**< This is the maximum number of members possible */
#define	CALLUI_INVALID_CALL_ID					0				/**< This is the invalid entry for call ID */

#define	CALLUI_BUF_MEMBER_SIZE 512

#define NO_HANDLE 0

#define CALLUI_KEY_BACK "XF86Back"
#define CALLUI_KEY_MEDIA "XF86AudioMedia"
#define CALLUI_KEY_SELECT "XF86Phone"
#define CALLUI_KEY_POWER "XF86PowerOff"
#define CALLUI_KEY_HOME "XF86Home"
#define CALLUI_KEY_VOLUMEUP "XF86AudioRaiseVolume"
#define CALLUI_KEY_VOLUMEDOWN "XF86AudioLowerVolume"

#define CALLUI_REJ_MSG_MAX_LENGTH (210+1)
#define CALLUI_REJ_MSG_MAX_COUNT 6

#define CALLUI_SAFE_C_CAST(type, value) ((type)(ptrdiff_t)value)

#undef FREE
#define FREE(ptr) \
	do {\
		free(ptr);\
		ptr = NULL;\
	} while (0)

#undef G_FREE
#define G_FREE(ptr) \
	do {\
		g_free(ptr);\
		ptr = NULL;\
	} while (0)

#undef DELETE_EVAS_OBJECT
#define DELETE_EVAS_OBJECT(x) \
	do { \
		if (x != NULL) { \
			evas_object_del(x); \
			x = NULL; \
		} \
	} while (0)

#undef DELETE_ECORE_TIMER
#define DELETE_ECORE_TIMER(x) \
	do { \
		if (x != NULL) { \
			ecore_timer_del(x); \
			x = NULL; \
		} \
	} while (0)

#endif /* __CALLUI_COMMON_DEFINES_H__ */
