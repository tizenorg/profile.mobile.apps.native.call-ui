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

#ifndef _CALLUI_VIEW_LAYOUT_HD_H_
#define _CALLUI_VIEW_LAYOUT_HD_H_

#define TOTAL_W								720
#define TOTAL_H								1280

#define REL_HD_W(x) ((x)/TOTAL_W)
#define REL_HD_H(y) ((y)/TOTAL_H)

#define INDICATOR_H							58

#define KEYPAD_H							528
#define KEYPAD_AREA_H						(TOTAL_H - ENDCALL_BTN_BG_H)
#define KEYPAD_ARROW_BTN_BG_H				67
#define KEYPAD_BTN_ICON_W					108
#define KEYPAD_BTN_NUMBER_H					71
#define KEYPAD_BTN_LETTERS_H				36
#define KEYPAD_DIVIDER_SIZE					2
#define KEYPAD_EACH_BTN_W					239
#define KEYPAD_EACH_BTN_H					130

#define ENDCALL_BTN_BG_H					108*1280/800

#define KEYPAD_BTN_OFFSET_X					65
#define KEYPAD_BTN_OFFSET_Y					12

//Manage button
#define MANAGE_BTN_SIZE						50
#define MANAGE_BTN_R_OFFSET					22
#define MANAGE_BTN_B_OFFSET					160

#define MANAGE_BTN_L						(1 - REL_HD_W(MANAGE_BTN_SIZE + MANAGE_BTN_R_OFFSET))
#define MANAGE_BTN_T						(1 - REL_HD_H(MANAGE_BTN_SIZE + MANAGE_BTN_B_OFFSET))
#define MANAGE_BTN_R						(1 - REL_HD_W(MANAGE_BTN_R_OFFSET))
#define MANAGE_BTN_B						(1 - REL_HD_H(MANAGE_BTN_B_OFFSET))

//Back button
#define BACK_BTN_SIZE						496

#define BACK_BTN_L							((1 - REL_HD_W(BACK_BTN_SIZE))/2)
#define BACK_BTN_R							(1 - BACK_BTN_L)

#include "callui-view-caller-info-metric.h"

#endif /* _CALLUI_VIEW_LAYOUT_HD_H_ */
