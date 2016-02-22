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

#include <dlog.h>

#ifndef __CALLUI_DEBUG_H__
#define __CALLUI_DEBUG_H__

#ifndef CALLUI_LOG_TAG
#define CALLUI_LOG_TAG "CALLUI"
#endif

#define info(fmt,args...)   { __dlog_print(LOG_ID_MAIN, DLOG_INFO,  CALLUI_LOG_TAG, "%s: %s(%d) > " fmt "\n", __FILE__, __func__, __LINE__, ##args); }
#define dbg(fmt,args...)    { __dlog_print(LOG_ID_MAIN, DLOG_DEBUG, CALLUI_LOG_TAG, "%s: %s(%d) > " fmt "\n", __FILE__, __func__, __LINE__, ##args); }
#define warn(fmt,args...)   { __dlog_print(LOG_ID_MAIN, DLOG_WARN,  CALLUI_LOG_TAG, "%s: %s(%d) > " fmt "\n", __FILE__, __func__, __LINE__, ##args); }
#define err(fmt,args...)    { __dlog_print(LOG_ID_MAIN, DLOG_ERROR, CALLUI_LOG_TAG, "%s: %s(%d) > " fmt "\n", __FILE__, __func__, __LINE__, ##args); }
#define fatal(fmt,args...)  { __dlog_print(LOG_ID_MAIN, DLOG_FATAL, CALLUI_LOG_TAG, "%s: %s(%d) > " fmt "\n", __FILE__, __func__, __LINE__, ##args); }

#define sec_err(fmt, arg...)  {SECURE_LOGE(CALLUI_LOG_TAG, "%s: %s(%d) > " fmt "\n", __FILE__, __func__, __LINE__, ##arg); }
#define sec_warn(fmt, arg...) {SECURE_LOGW(CALLUI_LOG_TAG, "%s: %s(%d) > " fmt "\n", __FILE__, __func__, __LINE__, ##arg); }
#define sec_dbg(fmt, arg...)  {SECURE_LOGD(CALLUI_LOG_TAG, "%s: %s(%d) > " fmt "\n", __FILE__, __func__, __LINE__, ##arg); }

#define debug_trace(fmt, args...)	do { info(fmt, ##args); } while (0)
#define debug_enter()				do { debug_trace(" * Enter * "); } while (0)
#define debug_leave()				do { debug_trace(" * Leave * "); } while (0)

#ifndef CALLUI_RETURN_IF_FAIL
#define CALLUI_RETURN_IF_FAIL(check_condition)	{\
	if (!(check_condition)) \
	{ \
		err("%s: Failed", #check_condition); \
		return;	\
	} \
}
#endif

#ifndef CALLUI_RETURN_VALUE_IF_FAIL
#define CALLUI_RETURN_VALUE_IF_FAIL(check_condition, value)	{\
	if (!(check_condition)) \
	{ \
		err("%s: Failed", #check_condition); \
		return value;	\
	} \
}
#endif

#ifndef CALLUI_RETURN_NULL_IF_FAIL
#define CALLUI_RETURN_NULL_IF_FAIL(check_condition)	{\
	if (!(check_condition)) \
	{ \
		err("%s: Failed", #check_condition); \
		return NULL;	\
	} \
}
#endif

#endif /* __CALLUI_DEBUG_H__ */
