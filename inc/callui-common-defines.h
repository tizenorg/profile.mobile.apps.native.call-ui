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

#ifndef __CALLUI_COMMON_DEFINES_H__
#define __CALLUI_COMMON_DEFINES_H__

#define _DBUS_DISPLAY_DEVICE_TIMEOUT_

#ifndef CALLUI_EXPORT_API
#define CALLUI_EXPORT_API __attribute__ ((visibility("default")))
#endif

#define CALLUI_PACKAGE	"org.tizen.call-ui"

#define CALLUI_CALL_EDJ_NAME		"/edje/call.edj"
#define CALLUI_CALL_THEME_EDJ_NAME	"/edje/call_theme.edj"

#define CALLUI_CALL_EDJ_PATH		_callui_common_get_call_edj_path()
#define CALLUI_CALL_THEME_EDJ_PATH	_callui_common_get_call_theme_path()

#define CALLUI_GROUP_VIEW_MAIN_LY					"view_main_ly"
#define CALLUI_GROUP_CALLER_INFO					"caller_info"
#define CALLUI_GROUP_PART_SWALLOW_CALLER_INFO		"swallow.caller_info"

#define CALLUI_APP_CONTROL_OPERATION_DURING_CALL	"http://tizen.org/appcontrol/oparation/during_call"
#define CALLUI_APP_CONTROL_OPERATION_END_CALL		"http://tizen.org/appcontrol/operation/end_call"
#define CALLUI_APP_CONTROL_OPERATION_QP_RESUME		"http://tizen.org/appcontrol/operation/qp_resume"
#define CALLUI_APP_CONTROL_OPERATION_LS_RESUME		"http://tizen.org/appcontrol/operation/lockscreen_resume"

#define CALLUI_KEY_SELECT	"XF86Phone"
#define CALLUI_KEY_POWER	"XF86PowerOff"

#define CALLUI_REJ_MSG_MAX_LENGTH	(210+1)
#define CALLUI_REJ_MSG_MAX_COUNT	6

#define CALLUI_BUFF_SIZE_HUG	512
#define CALLUI_BUFF_SIZE_LAR	256
#define CALLUI_BUFF_SIZE_BIG	128
#define CALLUI_BUFF_SIZE_MID	64
#define CALLUI_BUFF_SIZE_SML	32
#define CALLUI_BUFF_SIZE_TINY	16

#define CALLUI_PAUSE_LOCK_TIMEOUT_LIMIT_SEC 0.3

#undef SAFE_C_CAST
#define SAFE_C_CAST(type, value) ((type)(ptrdiff_t)value)

#undef _EDJ
#define _EDJ(obj) elm_layout_edje_get(obj)

#undef _
#define _(s) gettext(s)

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

#undef DELETE_ECORE_IDLE_ENTERER
#define DELETE_ECORE_IDLE_ENTERER(x) \
	do { \
		if (x != NULL) { \
			ecore_idle_enterer_del(x); \
			x = NULL; \
		} \
	} while (0)

#undef DELETE_ELM_TRANSIT_HARD
#define DELETE_ELM_TRANSIT_HARD(x) \
	do { \
		if (x != NULL) { \
			elm_transit_del_cb_set(x, NULL, NULL); \
			elm_transit_del(x); \
			x = NULL; \
		} \
	} while (0)

#undef DELETE_ELM_TRANSIT_SOFT
#define DELETE_ELM_TRANSIT_SOFT(x) \
	do { \
		if (x != NULL) { \
			elm_transit_del(x); \
			x = NULL; \
		} \
	} while (0)

#undef STRING_EMPTY
#define STRING_EMPTY(x) !(x && x[0] != '\0')

#endif /* __CALLUI_COMMON_DEFINES_H__ */
