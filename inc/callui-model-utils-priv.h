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

#ifndef __CALLUI_MODEL_UTILS_PRIV_H__
#define __CALLUI_MODEL_UTILS_PRIV_H__

#include <call-manager.h>

#include "callui-common-types.h"

/**
 * @brief Converts platform callmanager result types into application once
 *
 * @param[in]	cm_res	Platform  call manager result type
 *
 * @return Call UI application result type
 */
callui_result_e _callui_utils_convert_cm_res(cm_error_e cm_res);

#endif /* __CALLUI_MODEL_UTILS_PRIV_H__ */
