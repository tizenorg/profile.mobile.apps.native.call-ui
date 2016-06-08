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

#ifndef __CALLUI_COMMON_TYPES_H__
#define __CALLUI_COMMON_TYPES_H__

#include <msg_types.h>
#include <time.h>

typedef enum {

   CALLUI_RESULT_OK,

   CALLUI_RESULT_FAIL,
   CALLUI_RESULT_INVALID_PARAM,
   CALLUI_RESULT_ALLOCATION_FAIL,
   CALLUI_RESULT_PERMISSION_DENIED,
   CALLUI_RESULT_NOT_SUPPORTED,
   CALLUI_RESULT_NOT_REGISTERED,
   CALLUI_RESULT_ALREADY_REGISTERED,

   CALLUI_RESULT_UNKNOWN_ERROR

} callui_result_e;

typedef struct {
	char text[MAX_MSG_TEXT_LEN];
	time_t date;
}callui_msg_data_t;

#endif /* __CALLUI_COMMON_TYPES_H__ */
