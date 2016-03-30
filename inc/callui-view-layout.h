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


#ifndef _CALLUI_VIEW_LAYOUT_H_
#define _CALLUI_VIEW_LAYOUT_H_

#define COLOR_BG_ALPHA	0 0 0 0

#define VC_FONT_REGULAR			"Tizen:style=Regular"
#define VC_TEXT_CLASS_REGULAR	"tizen"

/*
 * NBEAT-HD GUI Widget Dimensions
 * In Portrait mode
 */
#define MAIN_SCREEN_H		1280
#define MAIN_SCREEN_W		720
#define INDICATOR_HEIGHT	40

#define MAIN_LAYOUT_W		MAIN_SCREEN_W
#define MAIN_LAYOUT_H		MAIN_SCREEN_H

#define NORMAL_HOLD_FONT_SIZE	36

#define CALLER_INFO_L_PAD			0
#define CALLER_INFO_WIDTH			MAIN_SCREEN_W
#define CALLER_INFO_T_PAD			0
#define CALLER_INFO_HEIGHT_LAYOUT_H	712
#define CALLER_INFO_HEIGHT			(CALLER_INFO_HEIGHT_LAYOUT_H+INDICATOR_HEIGHT)
#define CALLER_INFO_MAIN_WIDTH		656

#define CALLER_INFO_STATUS_L_PAD	32
#define CALLER_INFO_STATUS_T_PAD	(INDICATOR_HEIGHT+24)
#define CALLER_INFO_STATUS_WIDTH	CALLER_INFO_MAIN_WIDTH
#define CALLER_INFO_STATUS_HEIGHT	54

#define CALL_STATUS_MAX_TEXT_WIDTH	CALLER_INFO_MAIN_WIDTH

#define VC_CALLER_ID_LAYOUT_L_PAD	CALLER_INFO_STATUS_L_PAD
#define VC_CALLER_ID_LAYOUT_WIDTH	CALLER_INFO_MAIN_WIDTH
#define VC_CALLER_ID_LAYOUT_T_PAD	(CALLER_INFO_STATUS_T_PAD+CALLER_INFO_STATUS_HEIGHT+33)
#define VC_CALLER_ID_LAYOUT_HEIGHT	348

#define CALLER_INFO_NAME_L_PAD		CALLER_INFO_STATUS_L_PAD
#define CALLER_INFO_NAME_WIDTH		CALLER_INFO_MAIN_WIDTH
#define CALLER_INFO_NAME_T_PAD		(VC_CALLER_ID_LAYOUT_T_PAD+VC_CALLER_ID_LAYOUT_HEIGHT+42)
#define CALLER_INFO_NAME_HEIGHT		86

#define CALLER_INFO_EMERGENCY_HEIGHT	86

#define CALLER_INFO_NAME_1LINE_T_PAD	(VC_CALLER_ID_LAYOUT_T_PAD+VC_CALLER_ID_LAYOUT_HEIGHT+71)
#define CALLER_INFO_NAME_1LINE_HEIGHT	86

#define CALLER_INFO_NUMBER_L_PAD	CALLER_INFO_NAME_L_PAD
#define CALLER_INFO_NUMBER_WIDTH	CALLER_INFO_MAIN_WIDTH
#define CALLER_INFO_NUMBER_T_PAD	(CALLER_INFO_NAME_T_PAD+CALLER_INFO_NAME_HEIGHT+8)
#define CALLER_INFO_NUMBER_HEIGHT	54

#define CALLER_INFO_MIN_WIDTH	50

#define STYLE_ICN_T_PAD		37
#define STYLE_ICN_B			99
#define STYLE_TXT_T_PAD		103
#define STYLE_TXT_B			140
#define STYLE_TXT_L_PAD		41
#define STYLE_TXT_R			199
#define STYLE_TXT_WIDTH		158
#define STYLE_TXT_HEIGHT	37

#define STYLE_ONOFF_BAR_T_PAD	150
#define STYLE_ONOFF_BAR_B		156
#define STYLE_ONOFF_BAR_L_PAD	50
#define STYLE_ONOFF_BAR_R		190

#define MTLOCK_CALL_ICON_W	94
#define MTLOCK_CALL_ICON_H	94

#define MTLOCK_ACCEPT_OUTER_CIRCLE_BG_L_PAD		(-129)
#define MTLOCK_ACCEPT_OUTER_CIRCLE_BG_WIDTH		520
#define MTLOCK_ACCEPT_OUTER_CIRCLE_BG_T_PAD		(CALLER_INFO_HEIGHT-51)
#define MTLOCK_ACCEPT_OUTER_CIRCLE_BG_HEIGHT	520

#define MTLOCK_ACCEPT_INNER_CIRCLE_L_PAD		38
#define MTLOCK_ACCEPT_INNER_CIRCLE_WIDTH		186
#define MTLOCK_ACCEPT_INNER_CIRCLE_T_PAD		(CALLER_INFO_HEIGHT+116)
#define MTLOCK_ACCEPT_INNER_CIRCLE_HEIGHT		186

#define MTLOCK_ACCEPT_INNER_CIRCLE_BG_L_PAD		38
#define MTLOCK_ACCEPT_INNER_CIRCLE_BG_WIDTH		273
#define MTLOCK_ACCEPT_INNER_CIRCLE_BG_T_PAD		MTLOCK_ACCEPT_INNER_CIRCLE_T_PAD
#define MTLOCK_ACCEPT_INNER_CIRCLE_BG_HEIGHT	186

#define MTLOCK_REJECT_OUTER_CIRCLE_BG_L_PAD		329
#define MTLOCK_REJECT_OUTER_CIRCLE_BG_WIDTH		MTLOCK_ACCEPT_OUTER_CIRCLE_BG_WIDTH
#define MTLOCK_REJECT_OUTER_CIRCLE_BG_T_PAD		MTLOCK_ACCEPT_OUTER_CIRCLE_BG_T_PAD
#define MTLOCK_REJECT_OUTER_CIRCLE_BG_HEIGHT	MTLOCK_ACCEPT_OUTER_CIRCLE_BG_HEIGHT

#define MTLOCK_REJECT_INNER_CIRCLE_BG_L_PAD		MAIN_SCREEN_W-MTLOCK_ACCEPT_INNER_CIRCLE_BG_L_PAD-MTLOCK_REJECT_INNER_CIRCLE_BG_WIDTH
#define MTLOCK_REJECT_INNER_CIRCLE_BG_WIDTH		MTLOCK_ACCEPT_INNER_CIRCLE_BG_WIDTH
#define MTLOCK_REJECT_INNER_CIRCLE_BG_T_PAD		MTLOCK_ACCEPT_INNER_CIRCLE_BG_T_PAD
#define MTLOCK_REJECT_INNER_CIRCLE_BG_HEIGHT	MTLOCK_ACCEPT_INNER_CIRCLE_BG_HEIGHT

#define MTLOCK_REJECT_INNER_CIRCLE_L_PAD	496
#define MTLOCK_REJECT_INNER_CIRCLE_WIDTH	MTLOCK_ACCEPT_INNER_CIRCLE_WIDTH
#define MTLOCK_REJECT_INNER_CIRCLE_T_PAD	MTLOCK_ACCEPT_INNER_CIRCLE_T_PAD
#define MTLOCK_REJECT_INNER_CIRCLE_HEIGHT	MTLOCK_ACCEPT_INNER_CIRCLE_HEIGHT

#define MTLOCK_REJECT_MSG_BG_L_PAD		0
#define MTLOCK_REJECT_MSG_BG_WIDTH		MAIN_SCREEN_W
#define MTLOCK_REJECT_MSG_BG_HEIGHT		104
#define MTLOCK_REJECT_MSG_BG_T_PAD		(MAIN_SCREEN_H-MTLOCK_REJECT_MSG_BG_HEIGHT)

#define MTLOCK_REJECT_MSG_ARW_L_PAD		340
#define MTLOCK_REJECT_MSG_ARW_WIDTH		40
#define MTLOCK_REJECT_MSG_ARW_HEIGHT	18
#define MTLOCK_REJECT_MSG_ARW_TOP_PAD	12
#define MTLOCK_REJECT_MSG_ARW_T_PAD		(MTLOCK_REJECT_MSG_BG_T_PAD+MTLOCK_REJECT_MSG_ARW_TOP_PAD)

#define MTLOCK_REJECT_MSG_DOWN_ARW_T_PAD	MAIN_SCREEN_H

#define MTLOCK_REJECT_MSG_TEXT_L_PAD		0
#define MTLOCK_REJECT_MSG_TEXT_WIDTH		MAIN_SCREEN_W
#define MTLOCK_REJECT_MSG_TEXT_ARW_T_PAD	10
#define MTLOCK_REJECT_MSG_TEXT_T_PAD		(MTLOCK_REJECT_MSG_BG_T_PAD+MTLOCK_REJECT_MSG_ARW_HEIGHT+MTLOCK_REJECT_MSG_TEXT_ARW_T_PAD+MTLOCK_REJECT_MSG_ARW_TOP_PAD)
#define MTLOCK_REJECT_MSG_TEXT_HEIGHT		52

#define MTLOCK_REJECT_MSG_TOP_VISIBLE_AREA_W	(MTLOCK_REJECT_MSG_TEXT_HEIGHT+MTLOCK_REJECT_MSG_ARW_HEIGHT)

#define MTLOCK_ACTIVE_NOTI_CALL_HEIGHT 350

#define ITEM_SIZE_H 120

#define REJ_MSG_LIST_CREATE_MSG_BTN_H		150
#define REJ_MSG_LIST_CREATE_MSG_BTN_L_PAD	32
#define REJ_MSG_LIST_CREATE_MSG_BTN_R_PAD	REJ_MSG_LIST_CREATE_MSG_BTN_L_PAD
#define REJ_MSG_LIST_CREATE_MSG_BTN_T_PAD	17
#define REJ_MSG_LIST_CREATE_MSG_BTN_B_PAD	REJ_MSG_LIST_CREATE_MSG_BTN_T_PAD

#define MTLOCK_REJECT_MSG_LIST_L_PAD	0
#define MTLOCK_REJECT_MSG_LIST_WIDTH	MAIN_SCREEN_W
#define MTLOCK_REJECT_MSG_LIST_T_PAD	(MAIN_SCREEN_H+MTLOCK_REJECT_MSG_ARW_HEIGHT)
#define MTLOCK_REJECT_MSG_LIST_1ITEM_HEIGHT	86
#define MTLOCK_REJECT_MSG_LIST_1ITEM_NEW_HEIGHT	120
#define MTLOCK_REJECT_MSG_LIST_HEIGHT	(MTLOCK_REJECT_MSG_LIST_1ITEM_NEW_HEIGHT*6)

#define MULTI_LIST_HEIGHT MAIN_LAYOUT_H

/* Bottom reject call button*/
#define BOTTOM_BTN_BG_L_PAD		0
#define BOTTOM_BTN_BG_WIDTH		MAIN_SCREEN_W
#define BOTTOM_BTN_BG_T_PAD		(CALLER_INFO_HEIGHT+ACTION_BAR_LAYOUT_HEIGHT)
#define BOTTOM_BTN_BG_HEIGHT	ENDCALL_BTN_BG_H

/* Six buttons layout*/
#define ACTION_BAR_LAYOUT_L_PAD	0
#define ACTION_BAR_LAYOUT_WIDTH	MAIN_SCREEN_W
#define ACTION_BAR_LAYOUT_T_PAD	(CALLER_INFO_HEIGHT)
#define ACTION_BAR_LAYOUT_HEIGHT	376

/* Six buttons layout - single button*/
#define BTN_LAYOUT_SINGLE_BTN_WIDTH		240
#define BTN_LAYOUT_SINGLE_BTN_HEIGHT	188

#define BTN_LAYOUT_GAP		2

#define BTN_LAYOUT_TOP_FIRST_BTN_L_PAD		0
#define BTN_LAYOUT_TOP_SECOND_BTN_L_PAD		(BTN_LAYOUT_TOP_FIRST_BTN_L_PAD+BTN_LAYOUT_SINGLE_BTN_WIDTH+(BTN_LAYOUT_GAP/2))
#define BTN_LAYOUT_TOP_THIRD_BTN_L_PAD		(BTN_LAYOUT_TOP_SECOND_BTN_L_PAD+BTN_LAYOUT_SINGLE_BTN_WIDTH)

#define BTN_LAYOUT_BOTTOM_FIRST_BTN_L_PAD	0
#define BTN_LAYOUT_BOTTOM_SECOND_BTN_L_PAD	(BTN_LAYOUT_BOTTOM_FIRST_BTN_L_PAD+BTN_LAYOUT_SINGLE_BTN_WIDTH+(BTN_LAYOUT_GAP/2))
#define BTN_LAYOUT_BOTTOM_THIRD_BTN_L_PAD	(BTN_LAYOUT_BOTTOM_SECOND_BTN_L_PAD+BTN_LAYOUT_SINGLE_BTN_WIDTH)

#define BTN_LAYOUT_TOP_BTNS_T_PAD	0
#define BTN_LAYOUT_BOTTOM_BTNS_T_PAD	BTN_LAYOUT_TOP_BTNS_T_PAD + BTN_LAYOUT_SINGLE_BTN_HEIGHT + (BTN_LAYOUT_GAP/2)

#define QP_CALL_ICON_HEIGHT		77
#define QP_CALL_ICON_WIDTH		119
#define QP_CONTENT_ICON_T_PAD	13
#define QP_CONTENT_ICON_HEIGHT	50
/*
 * End of NBEAT-HD GUI Widget Dimensions
 */

/*
 * Relative positions
 * In Portrait mode
 */
/* Relative X & Y positions of widgets w.r.t. Full Screen */
#define REL_W(x)		((x)/MAIN_SCREEN_W)
#define REL_H(y)		((y)/MAIN_LAYOUT_H)

/* Relative X & Y positions of widgets w.r.t. Quick-panel button layout */
#define REL_QP_BTN_W(x)		((x)/QP_CALL_ICON_WIDTH)
#define REL_QP_BTN_H(y)		((y)/QP_CALL_ICON_HEIGHT)

/* Relative X & Y positions of buttons w.r.t. Six-Button layout */
#define REL_BTN_LY_W(x)		((x)/ACTION_BAR_LAYOUT_WIDTH)
#define REL_BTN_LY_H(y)		((y)/ACTION_BAR_LAYOUT_HEIGHT)

/* Relative X & Y postions of elements w.r.t. Caller info layout */
#define REL_CALLER_INFO_W(y)	((y)/CALLER_INFO_WIDTH)
#define REL_CALLER_INFO_H(y)	((y)/CALLER_INFO_HEIGHT)

/* Relative Y postions of elements w.r.t. Multi list layout */
#define REL_MULTI_LIST_H(y)		((y)/MULTI_LIST_HEIGHT)

/* Relative X & Y postions of elements w.r.t. Reject Message layout */
#define REL_REJECT_MSG_W(x)		((x)/MAIN_SCREEN_W)
#define REL_REJECT_MSG_H(y)		((y)/MAIN_SCREEN_H)

/* Relative X & Y postions of elements w.r.t. End Call button background */
#define REL_ENDCALL_BTN_BG_W(x)		((x)/ENDCALL_BTN_BG_W)
#define REL_ENDCALL_BTN_BG_H(y)		((y)/ENDCALL_BTN_BG_H)

/* Relative X & Y postions of elements w.r.t. Keypad Arrow background */
#define REL_KEYPAD_ARROW_BG_W(x)	((x)/KEYPAD_ARROW_BTN_BG_W)
#define REL_KEYPAD_ARROW_BG_H(y)	((y)/KEYPAD_ARROW_BTN_BG_H)

/* Relative Y postions of elements w.r.t. Keypad area */
#define REL_KEYPAD_AREA_H(y)		((y)/KEYPAD_AREA_H)

#define REL_REJECT_MSG_SIZE_H(y)	(1.0+(y)/MAIN_SCREEN_H)
/*
 * End of Relative positions
 */


/*
 * MO/Dialing view & MT/Incoming view
 * In Portrait mode
 */
#define BUTTON_REJECT_SIZE_H	78
#define BUTTON_OFFSET_B			22
#define BUTTON_OFFSET_T			20
#define IMAGE_CLOSE				18
#define IMAGE_OFFSET			12

#define VC_CALLER_ID_ICON_L_PAD	186
#define VC_CALLER_ID_LAYOUT_L	REL_W(VC_CALLER_ID_ICON_L_PAD)
#define VC_CALLER_ID_LAYOUT_R	REL_W(VC_CALLER_ID_ICON_L_PAD+VC_CALLER_ID_LAYOUT_HEIGHT)

#define VC_CALLER_ID_LAYOUT_T	REL_CALLER_INFO_H(VC_CALLER_ID_LAYOUT_T_PAD)
#define VC_CALLER_ID_LAYOUT_B	REL_CALLER_INFO_H(VC_CALLER_ID_LAYOUT_T_PAD+VC_CALLER_ID_LAYOUT_HEIGHT)

/*
 * End of MO/Dialing view & MT/Incoming view
 */

/*
 * MT LOCK View - check
 * In Portrait mode
 */
#define MTLOCK_ACCEPT_CIRCLE_RECT_WIDTH		MAIN_SCREEN_W
#define MTLOCK_ACCEPT_CIRCLE_RECT_HEIGHT	313

#define MTLOCK_ACCEPT_INNER_CIRCLE_L	REL_W(MTLOCK_ACCEPT_INNER_CIRCLE_L_PAD)
#define MTLOCK_ACCEPT_INNER_CIRCLE_R	REL_W(MTLOCK_ACCEPT_INNER_CIRCLE_L_PAD+MTLOCK_ACCEPT_INNER_CIRCLE_WIDTH)
#define MTLOCK_ACCEPT_INNER_CIRCLE_T	REL_H(MTLOCK_ACCEPT_INNER_CIRCLE_T_PAD)
#define MTLOCK_ACCEPT_INNER_CIRCLE_B	REL_H(MTLOCK_ACCEPT_INNER_CIRCLE_T_PAD+MTLOCK_ACCEPT_INNER_CIRCLE_HEIGHT)

#define MTLOCK_ACCEPT_INNER_CIRCLE_BG_L	REL_W(MTLOCK_ACCEPT_INNER_CIRCLE_BG_L_PAD)
#define MTLOCK_ACCEPT_INNER_CIRCLE_BG_R	REL_W(MTLOCK_ACCEPT_INNER_CIRCLE_BG_L_PAD+MTLOCK_ACCEPT_INNER_CIRCLE_BG_WIDTH)
#define MTLOCK_ACCEPT_INNER_CIRCLE_BG_T	REL_H(MTLOCK_ACCEPT_INNER_CIRCLE_BG_T_PAD)
#define MTLOCK_ACCEPT_INNER_CIRCLE_BG_B	REL_H(MTLOCK_ACCEPT_INNER_CIRCLE_BG_T_PAD+MTLOCK_ACCEPT_INNER_CIRCLE_BG_HEIGHT)

#define MTLOCK_ACCEPT_OUTER_CIRCLE_L	REL_W(MTLOCK_ACCEPT_OUTER_CIRCLE_BG_L_PAD)
#define MTLOCK_ACCEPT_OUTER_CIRCLE_R	REL_W(MTLOCK_ACCEPT_OUTER_CIRCLE_BG_L_PAD+MTLOCK_ACCEPT_OUTER_CIRCLE_BG_WIDTH)
#define MTLOCK_ACCEPT_OUTER_CIRCLE_T	REL_H(MTLOCK_ACCEPT_OUTER_CIRCLE_BG_T_PAD)
#define MTLOCK_ACCEPT_OUTER_CIRCLE_B	REL_H(MTLOCK_ACCEPT_OUTER_CIRCLE_BG_T_PAD+MTLOCK_ACCEPT_OUTER_CIRCLE_BG_HEIGHT)

#define MTLOCK_ACCEPT_OUTER_CIRCLE_CLIP_RECT_T	REL_H(CALLER_INFO_HEIGHT)
#define MTLOCK_ACCEPT_OUTER_CIRCLE_CLIP_RECT_B	REL_H(MAIN_LAYOUT_H)

#define MTLOCK_REJECT_INNER_CIRCLE_L	REL_W(MTLOCK_REJECT_INNER_CIRCLE_L_PAD)
#define MTLOCK_REJECT_INNER_CIRCLE_R	REL_W(MTLOCK_REJECT_INNER_CIRCLE_L_PAD+MTLOCK_REJECT_INNER_CIRCLE_WIDTH)
#define MTLOCK_REJECT_INNER_CIRCLE_T	REL_H(MTLOCK_REJECT_INNER_CIRCLE_T_PAD)
#define MTLOCK_REJECT_INNER_CIRCLE_B	REL_H(MTLOCK_REJECT_INNER_CIRCLE_T_PAD+MTLOCK_REJECT_INNER_CIRCLE_HEIGHT)

#define MTLOCK_REJECT_INNER_CIRCLE_BG_L		REL_W(MTLOCK_REJECT_INNER_CIRCLE_BG_L_PAD)
#define MTLOCK_REJECT_INNER_CIRCLE_BG_R		REL_W(MTLOCK_REJECT_INNER_CIRCLE_BG_L_PAD+MTLOCK_REJECT_INNER_CIRCLE_BG_WIDTH)
#define MTLOCK_REJECT_INNER_CIRCLE_BG_T		REL_H(MTLOCK_REJECT_INNER_CIRCLE_BG_T_PAD)
#define MTLOCK_REJECT_INNER_CIRCLE_BG_B		REL_H(MTLOCK_REJECT_INNER_CIRCLE_BG_T_PAD+MTLOCK_REJECT_INNER_CIRCLE_BG_HEIGHT)

#define MTLOCK_REJECT_OUTER_CIRCLE_L	REL_W(MTLOCK_REJECT_OUTER_CIRCLE_BG_L_PAD)
#define MTLOCK_REJECT_OUTER_CIRCLE_R	REL_W(MTLOCK_REJECT_OUTER_CIRCLE_BG_L_PAD+MTLOCK_REJECT_OUTER_CIRCLE_BG_WIDTH)
#define MTLOCK_REJECT_OUTER_CIRCLE_T	REL_H(MTLOCK_REJECT_OUTER_CIRCLE_BG_T_PAD)
#define MTLOCK_REJECT_OUTER_CIRCLE_B	REL_H(MTLOCK_REJECT_OUTER_CIRCLE_BG_T_PAD+MTLOCK_REJECT_OUTER_CIRCLE_BG_HEIGHT)

#define MTLOCK_REJECT_OUTER_CIRCLE_CLIP_RECT_T	REL_H(CALLER_INFO_HEIGHT)
#define MTLOCK_REJECT_OUTER_CIRCLE_CLIP_RECT_B	REL_H(MAIN_LAYOUT_H)

#define MTLOCK_REJECT_MSG_BG_L		REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_BG_L_PAD)
#define MTLOCK_REJECT_MSG_BG_R		REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_BG_L_PAD+MTLOCK_REJECT_MSG_BG_WIDTH)
#define MTLOCK_REJECT_MSG_BG_T		REL_REJECT_MSG_H(MTLOCK_REJECT_MSG_BG_T_PAD)
#define MTLOCK_REJECT_MSG_BG_B		REL_REJECT_MSG_H(MTLOCK_REJECT_MSG_BG_T_PAD+MTLOCK_REJECT_MSG_BG_HEIGHT)

#define MTLOCK_REJECT_MSG_ARW_L		REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_ARW_L_PAD)
#define MTLOCK_REJECT_MSG_ARW_R		REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_ARW_L_PAD+MTLOCK_REJECT_MSG_ARW_WIDTH)
#define MTLOCK_REJECT_MSG_ARW_T		REL_REJECT_MSG_H(MTLOCK_REJECT_MSG_ARW_T_PAD)

#define MTLOCK_REJECT_MSG_ARW_CLOSE_T	(1.0 + (IMAGE_OFFSET/MAIN_SCREEN_H))

#define MTLOCK_REJECT_MSG_ARW_B			REL_REJECT_MSG_H(MTLOCK_REJECT_MSG_ARW_T_PAD+MTLOCK_REJECT_MSG_ARW_HEIGHT)
#define MTLOCK_REJECT_MSG_ARW_CLOSE_B	REL_REJECT_MSG_SIZE_H(IMAGE_CLOSE+IMAGE_OFFSET)

#define MTLOCK_REJECT_MSG_TEXT_L	REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_TEXT_L_PAD)
#define MTLOCK_REJECT_MSG_TEXT_R	REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_TEXT_L_PAD+MTLOCK_REJECT_MSG_TEXT_WIDTH)
#define MTLOCK_REJECT_MSG_TEXT_T	REL_REJECT_MSG_H(MTLOCK_REJECT_MSG_TEXT_T_PAD)
#define MTLOCK_REJECT_MSG_TEXT_B	REL_REJECT_MSG_H(MTLOCK_REJECT_MSG_TEXT_T_PAD+MTLOCK_REJECT_MSG_TEXT_HEIGHT)

#define MTLOCK_REJECT_MSG_LIST_L	REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_LIST_L_PAD)
#define MTLOCK_REJECT_MSG_LIST_R	REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_LIST_L_PAD+MTLOCK_REJECT_MSG_LIST_WIDTH)

#define MTLOCK_REJECT_MSG_DOWN_ARROW_T	(1.0)
#define MTLOCK_REJECT_MSG_DOWN_ARROW_B	REL_REJECT_MSG_SIZE_H(IMAGE_CLOSE+IMAGE_OFFSET)

#define MTLOCK_REJECT_MSG_BTN_L		REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_LIST_L_PAD)
#define MTLOCK_REJECT_MSG_BTN_R		REL_REJECT_MSG_W(MTLOCK_REJECT_MSG_LIST_L_PAD+MTLOCK_REJECT_MSG_LIST_WIDTH)
#define MTLOCK_REJECT_MSG_BTN_T		REL_REJECT_MSG_SIZE_H(IMAGE_CLOSE+IMAGE_OFFSET)
#define MTLOCK_REJECT_MSG_BTN_B		REL_REJECT_MSG_SIZE_H(BUTTON_REJECT_SIZE_H+BUTTON_OFFSET_B+BUTTON_OFFSET_T+IMAGE_CLOSE+IMAGE_OFFSET)

#define REJECT_MSG		(BUTTON_REJECT_SIZE_H + BUTTON_OFFSET_B + BUTTON_OFFSET_T + IMAGE_CLOSE + IMAGE_OFFSET)
#define MTLOCK_REJECT_MSG_LIST_NEW_T		REL_REJECT_MSG_SIZE_H(REJECT_MSG)

#define MTLOCK_REJECT_MSG_LIST_6ITEMS_B		REL_REJECT_MSG_SIZE_H((ITEM_SIZE_H*6)+REJECT_MSG)
#define MTLOCK_REJECT_MSG_LIST_5ITEMS_B		REL_REJECT_MSG_SIZE_H((ITEM_SIZE_H*5)+REJECT_MSG)
#define MTLOCK_REJECT_MSG_LIST_4ITEMS_B		REL_REJECT_MSG_SIZE_H((ITEM_SIZE_H*4)+REJECT_MSG)
#define MTLOCK_REJECT_MSG_LIST_3ITEMS_B		REL_REJECT_MSG_SIZE_H((ITEM_SIZE_H*3)+REJECT_MSG)
#define MTLOCK_REJECT_MSG_LIST_2ITEMS_B		REL_REJECT_MSG_SIZE_H((ITEM_SIZE_H*2)+REJECT_MSG)
#define MTLOCK_REJECT_MSG_LIST_1ITEM_B		REL_REJECT_MSG_SIZE_H((ITEM_SIZE_H*1)+REJECT_MSG)
/*
 * End of MT LOCK View
 */

/*
 * Action Bar Button Layout
 * In Portrait mode
 */
#define TOP_FIRST_BUTTON_L		REL_BTN_LY_W(BTN_LAYOUT_TOP_FIRST_BTN_L_PAD)
#define TOP_FIRST_BUTTON_R		REL_BTN_LY_W(BTN_LAYOUT_TOP_FIRST_BTN_L_PAD + BTN_LAYOUT_SINGLE_BTN_WIDTH-(BTN_LAYOUT_GAP/2))
#define TOP_FIRST_BUTTON_T		REL_BTN_LY_H(BTN_LAYOUT_TOP_BTNS_T_PAD)
#define TOP_FIRST_BUTTON_B		REL_BTN_LY_H(BTN_LAYOUT_TOP_BTNS_T_PAD+BTN_LAYOUT_SINGLE_BTN_HEIGHT-(BTN_LAYOUT_GAP/2))

#define TOP_SECOND_BUTTON_L		REL_BTN_LY_W(BTN_LAYOUT_TOP_SECOND_BTN_L_PAD)
#define TOP_SECOND_BUTTON_R		REL_BTN_LY_W(BTN_LAYOUT_TOP_SECOND_BTN_L_PAD+BTN_LAYOUT_SINGLE_BTN_WIDTH-BTN_LAYOUT_GAP)
#define TOP_SECOND_BUTTON_T		REL_BTN_LY_H(BTN_LAYOUT_TOP_BTNS_T_PAD)
#define TOP_SECOND_BUTTON_B		TOP_FIRST_BUTTON_B

#define TOP_THIRD_BUTTON_L		REL_BTN_LY_W(BTN_LAYOUT_TOP_THIRD_BTN_L_PAD)
#define TOP_THIRD_BUTTON_R		REL_BTN_LY_W(BTN_LAYOUT_TOP_THIRD_BTN_L_PAD+BTN_LAYOUT_SINGLE_BTN_WIDTH-(BTN_LAYOUT_GAP/2))
#define TOP_THIRD_BUTTON_T		REL_BTN_LY_H(BTN_LAYOUT_TOP_BTNS_T_PAD)
#define TOP_THIRD_BUTTON_B		TOP_FIRST_BUTTON_B

#define BOTTOM_FIRST_BUTTON_L	REL_BTN_LY_W(BTN_LAYOUT_BOTTOM_FIRST_BTN_L_PAD)
#define BOTTOM_FIRST_BUTTON_R	REL_BTN_LY_W(BTN_LAYOUT_BOTTOM_FIRST_BTN_L_PAD + BTN_LAYOUT_SINGLE_BTN_WIDTH-(BTN_LAYOUT_GAP/2))
#define BOTTOM_FIRST_BUTTON_T	REL_BTN_LY_H(BTN_LAYOUT_BOTTOM_BTNS_T_PAD)
#define BOTTOM_FIRST_BUTTON_B	REL_BTN_LY_H(BTN_LAYOUT_BOTTOM_BTNS_T_PAD+BTN_LAYOUT_SINGLE_BTN_HEIGHT-BTN_LAYOUT_GAP)

#define BOTTOM_SECOND_BUTTON_L	REL_BTN_LY_W(BTN_LAYOUT_BOTTOM_SECOND_BTN_L_PAD)
#define BOTTOM_SECOND_BUTTON_R	REL_BTN_LY_W(BTN_LAYOUT_BOTTOM_SECOND_BTN_L_PAD+BTN_LAYOUT_SINGLE_BTN_WIDTH-BTN_LAYOUT_GAP)
#define BOTTOM_SECOND_BUTTON_T	REL_BTN_LY_H(BTN_LAYOUT_BOTTOM_BTNS_T_PAD)
#define BOTTOM_SECOND_BUTTON_B	BOTTOM_FIRST_BUTTON_B

#define BOTTOM_THIRD_BUTTON_L	REL_BTN_LY_W(BTN_LAYOUT_BOTTOM_THIRD_BTN_L_PAD)
#define BOTTOM_THIRD_BUTTON_R	REL_BTN_LY_W(BTN_LAYOUT_BOTTOM_THIRD_BTN_L_PAD+BTN_LAYOUT_SINGLE_BTN_WIDTH-(BTN_LAYOUT_GAP/2))
#define BOTTOM_THIRD_BUTTON_T	REL_BTN_LY_H(BTN_LAYOUT_BOTTOM_BTNS_T_PAD)
#define BOTTOM_THIRD_BUTTON_B	BOTTOM_FIRST_BUTTON_B

#define ACTION_BAR_LAYOUT_L	REL_W(ACTION_BAR_LAYOUT_L_PAD)
#define ACTION_BAR_LAYOUT_R	REL_W(ACTION_BAR_LAYOUT_L_PAD+ACTION_BAR_LAYOUT_WIDTH)
#define ACTION_BAR_LAYOUT_T	REL_H(ACTION_BAR_LAYOUT_T_PAD)
#define ACTION_BAR_LAYOUT_B	REL_H(ACTION_BAR_LAYOUT_T_PAD+ACTION_BAR_LAYOUT_HEIGHT)
/*
 * End of Action Bar Button Layout
 */

/*
 * QUICK-PANEL layout
 * In Portrait mode
 */
#define CALL_QUICKPANEL_BTN_ICON_T	REL_QP_BTN_H(QP_CONTENT_ICON_T_PAD)
#define CALL_QUICKPANEL_BTN_ICON_B	REL_QP_BTN_H(QP_CONTENT_ICON_T_PAD+QP_CONTENT_ICON_HEIGHT)
/*
 * End of QUICK-PANEL layout
 */

/*
 * Caller info layout
 * In Portrait mode
 */

#define CALLER_INFO_L		REL_W(CALLER_INFO_L_PAD)
#define CALLER_INFO_R		REL_W(CALLER_INFO_L_PAD+CALLER_INFO_WIDTH)
#define CALLER_INFO_T		REL_H(CALLER_INFO_T_PAD)
#define CALLER_INFO_B		REL_H(CALLER_INFO_T_PAD+CALLER_INFO_HEIGHT)

#define CALLER_INFO_NAME_L	REL_W(CALLER_INFO_NAME_L_PAD)
#define CALLER_INFO_NAME_R	REL_W(CALLER_INFO_NAME_L_PAD+CALLER_INFO_NAME_WIDTH)
#define CALLER_INFO_NAME_T	REL_CALLER_INFO_H(CALLER_INFO_NAME_T_PAD)
#define CALLER_INFO_NAME_B	REL_CALLER_INFO_H(CALLER_INFO_NAME_T_PAD+CALLER_INFO_NAME_HEIGHT)
#define CALLER_INFO_NAME_EMERGENCY_B	REL_CALLER_INFO_H(CALLER_INFO_NAME_T_PAD+CALLER_INFO_EMERGENCY_HEIGHT)

#define CALLER_INFO_NAME_1LINE_T	REL_CALLER_INFO_H(CALLER_INFO_NAME_1LINE_T_PAD)
#define CALLER_INFO_NAME_1LINE_B	REL_CALLER_INFO_H(CALLER_INFO_NAME_1LINE_T_PAD+CALLER_INFO_NAME_1LINE_HEIGHT)

#define CALLER_INFO_NUMBER_L	REL_W(CALLER_INFO_NUMBER_L_PAD)
#define CALLER_INFO_NUMBER_R	REL_W(CALLER_INFO_NUMBER_L_PAD+CALLER_INFO_NUMBER_WIDTH)
#define CALLER_INFO_NUMBER_T	REL_CALLER_INFO_H(CALLER_INFO_NUMBER_T_PAD)
#define CALLER_INFO_NUMBER_B	REL_CALLER_INFO_H(CALLER_INFO_NUMBER_T_PAD+CALLER_INFO_NUMBER_HEIGHT)

#define CALLER_INFO_STATUS_L	REL_W(CALLER_INFO_STATUS_L_PAD)
#define CALLER_INFO_STATUS_R	REL_W(CALLER_INFO_STATUS_WIDTH)
#define CALLER_INFO_STATUS_T	REL_H(CALLER_INFO_STATUS_T_PAD)
#define CALLER_INFO_STATUS_B	REL_H(CALLER_INFO_STATUS_T_PAD+CALLER_INFO_STATUS_HEIGHT)
/*
 * End of Caller info layout
 */

/*
* Lock Screen Layout
*/
#define LOCK_SCREEN_LAYER_WIDTH		MAIN_SCREEN_W
#define LOCK_SCREEN_LAYER_HEIGHT	MAIN_SCREEN_H

#define LOCK_SCREEN_ICON_TOP_PAD	420
#define LOCK_SCREEN_ICON_HEIGHT		270

#define REL_LOCK_SCREEN_W(x)		((x)/LOCK_SCREEN_LAYER_WIDTH)
#define REL_LOCK_SCREEN_H(y)		((y)/LOCK_SCREEN_LAYER_HEIGHT)

#define LOCK_SCREEN_ICON_L			REL_LOCK_SCREEN_W(0)
#define LOCK_SCREEN_ICON_R			REL_LOCK_SCREEN_W(LOCK_SCREEN_LAYER_WIDTH)
#define LOCK_SCREEN_ICON_T			REL_LOCK_SCREEN_H(LOCK_SCREEN_ICON_TOP_PAD)
#define LOCK_SCREEN_ICON_B			REL_LOCK_SCREEN_H(LOCK_SCREEN_ICON_TOP_PAD+LOCK_SCREEN_ICON_HEIGHT)

#define LOCK_SCREEN_VERTICAL_GAP		20
#define LOCK_SCREEN_TEXT_LEFT_PADDING	15
#define LOCK_SCREEN_TEXT_RIGHT_PADDING	15
#define LOCK_SCREEN_TEXT_HEIGHT			144

#define LOCK_SCREEN_TEXT_L	REL_LOCK_SCREEN_W(LOCK_SCREEN_TEXT_LEFT_PADDING)
#define LOCK_SCREEN_TEXT_R	REL_LOCK_SCREEN_W(LOCK_SCREEN_LAYER_WIDTH-LOCK_SCREEN_TEXT_RIGHT_PADDING)
#define LOCK_SCREEN_TEXT_T	REL_LOCK_SCREEN_H(LOCK_SCREEN_ICON_TOP_PAD+LOCK_SCREEN_ICON_HEIGHT+LOCK_SCREEN_VERTICAL_GAP)
#define LOCK_SCREEN_TEXT_B	REL_LOCK_SCREEN_H(LOCK_SCREEN_ICON_TOP_PAD+LOCK_SCREEN_ICON_HEIGHT+LOCK_SCREEN_VERTICAL_GAP+LOCK_SCREEN_TEXT_HEIGHT)
/*
* End of Lock Screen Layout
*/

/* Keypad */
#define KEYPAD_H				588
#define KEYPAD_AREA_H			(MAIN_SCREEN_H - ENDCALL_BTN_BG_H)
#define KEYPAD_AREA_W			MAIN_SCREEN_W

#define KEYPAD_ARROW_BTN_BG_H	80
#define KEYPAD_ARROW_BTN_BG_W	MAIN_SCREEN_W

#define KEYPAD_BTN_ICON_W		108
#define KEYPAD_BTN_NUMBER_H		71
#define KEYPAD_BTN_LETTERS_H	36
#define KEYPAD_DIVIDER_SIZE		2
#define KEYPAD_EACH_BTN_W		239
#define KEYPAD_EACH_BTN_H		145

#define KEYPAD_ARROW_H		44
#define KEYPAD_ARROW_W		KEYPAD_ARROW_H

#define KEYPAD_SWEEP_AREA_T	0
#define KEYPAD_SWEEP_AREA_B	REL_KEYPAD_AREA_H(KEYPAD_AREA_H-KEYPAD_H)

#define KEYPAD_ARROW_BG_T	REL_KEYPAD_AREA_H(KEYPAD_AREA_H-KEYPAD_H-KEYPAD_ARROW_BTN_BG_H)
#define KEYPAD_ARROW_BG_B	REL_KEYPAD_AREA_H(KEYPAD_AREA_H-KEYPAD_H)

#define KEYPAD_ARROW_T		REL_KEYPAD_ARROW_BG_H((KEYPAD_ARROW_BTN_BG_H-KEYPAD_ARROW_H)/2)
#define KEYPAD_ARROW_B		REL_KEYPAD_ARROW_BG_H(((KEYPAD_ARROW_BTN_BG_H-KEYPAD_ARROW_H)/2)+KEYPAD_ARROW_H)
#define KEYPAD_ARROW_L		REL_KEYPAD_ARROW_BG_W((KEYPAD_ARROW_BTN_BG_W-KEYPAD_ARROW_W)/2)
#define KEYPAD_ARROW_R		REL_KEYPAD_ARROW_BG_W(((KEYPAD_ARROW_BTN_BG_W-KEYPAD_ARROW_W)/2)+KEYPAD_ARROW_W)

#define KEYPAD_BTNS_LAYOUT_T	REL_KEYPAD_AREA_H(KEYPAD_AREA_H-KEYPAD_H)
#define KEYPAD_BTNS_LAYOUT_B	REL_KEYPAD_AREA_H(KEYPAD_AREA_H)

#define KEYPAD_TXT_B			(1-REL_H(KEYPAD_H + ENDCALL_BTN_BG_H))

#define KEYPAD_BTN_OFFSET_X		65
#define KEYPAD_BTN_OFFSET_Y		18

/* Manage button */
#define MANAGE_BTN_SIZE			50
#define MANAGE_BTN_R_OFFSET		22
#define MANAGE_BTN_B_OFFSET		98

#define MANAGE_BTN_L	(1-REL_CALLER_INFO_W(MANAGE_BTN_SIZE + MANAGE_BTN_R_OFFSET))
#define MANAGE_BTN_T	(1-REL_CALLER_INFO_H(MANAGE_BTN_SIZE + MANAGE_BTN_B_OFFSET))
#define MANAGE_BTN_R	(1-REL_CALLER_INFO_W(MANAGE_BTN_R_OFFSET))
#define MANAGE_BTN_B	(1-REL_CALLER_INFO_H(MANAGE_BTN_B_OFFSET))

/* Back button */
#define BACK_BTN_SIZE 496

#define BACK_BTN_L		((1-REL_W(BACK_BTN_SIZE))/2)
#define BACK_BTN_R		(1-BACK_BTN_L)

/* One hold in conference layout */
#define ONE_HOLD_H		712
#define ONE_HOLD_B		REL_H(ONE_HOLD_H)

/* Indicator */
#define INDICATOR_B		REL_H(INDICATOR_HEIGHT)

/* Caller info layout */
#define CALLER_INF_LAYOUT_OFFSET	10
#define CALLER_INF_LAYOUT_B_OFFSET	62
#define CALLER_INF_LAYOUT_H			248
#define CALLER_INF_LAYOUT_W			700

/* Hold info */
#define HOLD_LAYOUT_L	REL_W(CALLER_INF_LAYOUT_OFFSET)
#define HOLD_LAYOUT_T	(CALLER_INF_LAYOUT_OFFSET/ONE_HOLD_H)
#define HOLD_LAYOUT_R	(1-REL_W(CALLER_INF_LAYOUT_OFFSET))
#define HOLD_LAYOUT_B	(HOLD_LAYOUT_T+(CALLER_INF_LAYOUT_H/ONE_HOLD_H))

/* Caller info */
#define CALLER_INF_L_OFFSET			22
#define CALLER_INF_T_OFFSET			10
#define CALLER_INF_R_OFFSET			22
#define CALLER_INF_B_OFFSET			30
#define CALLER_INF_ID_T_OFFSET		16
#define CALLER_INF_TEXT_L_OFFSET	32
#define CALLER_INF_SUBTEXT_T_OFFSET	6
#define CALLER_INF_STATUS_H			54
#define CALLER_INF_ID_SIZE			138
#define CALLER_INF_MAIN_TEXT_H		76

#define CALLER_INF_L	(CALLER_INF_L_OFFSET/CALLER_INF_LAYOUT_W)

#define CALLER_INF_STATUS_T		(CALLER_INF_T_OFFSET/CALLER_INF_LAYOUT_H)
#define CALLER_INF_STATUS_R		((CALLER_INF_L_OFFSET+CALLER_INF_ID_SIZE+CALLER_INF_TEXT_L_OFFSET)/CALLER_INF_LAYOUT_W)
#define CALLER_INF_STATUS_B		((CALLER_INF_T_OFFSET+CALLER_INF_STATUS_H)/CALLER_INF_LAYOUT_H)

#define CALLER_INF_ID_T		((CALLER_INF_T_OFFSET+CALLER_INF_STATUS_H+CALLER_INF_ID_T_OFFSET)/CALLER_INF_LAYOUT_H)
#define CALLER_INF_ID_R		((CALLER_INF_L_OFFSET+CALLER_INF_ID_SIZE)/CALLER_INF_LAYOUT_W)
#define CALLER_INF_ID_B		((CALLER_INF_T_OFFSET+CALLER_INF_STATUS_H+CALLER_INF_ID_SIZE+CALLER_INF_ID_T_OFFSET)/CALLER_INF_LAYOUT_H)

#define CALLER_INF_TEXT_L		((CALLER_INF_L+CALLER_INF_ID_SIZE+CALLER_INF_TEXT_L_OFFSET)/CALLER_INF_LAYOUT_W)
#define CALLER_INF_SUBTEXT_T	((CALLER_INF_T_OFFSET+CALLER_INF_STATUS_H+CALLER_INF_ID_T_OFFSET+CALLER_INF_MAIN_TEXT_H+CALLER_INF_SUBTEXT_T_OFFSET)/CALLER_INF_LAYOUT_H)

/* Merge and swap buttons */
#define MERGE_SWAP_BTN_W	350
#define MERGE_SWAP_BTN_H	122

#define MERGE_SWAP_BTN_T	HOLD_LAYOUT_B
#define MERGE_SWAP_BTN_B	(MERGE_SWAP_BTN_T+(MERGE_SWAP_BTN_H/ONE_HOLD_H))

#define MERGE_BTN_L		REL_W(CALLER_INF_LAYOUT_OFFSET)
#define MERGE_BTN_R		REL_W(CALLER_INF_LAYOUT_OFFSET+MERGE_SWAP_BTN_W)

#define SWAP_BTN_L		REL_W(CALLER_INF_LAYOUT_OFFSET+MERGE_SWAP_BTN_W)
#define SWAP_BTN_R		(1-REL_W(CALLER_INF_LAYOUT_OFFSET))

/* Merge and swap buttons content */
#define MS_BTN_ICON_T_OFFSET	10

#define MS_BTN_ICON_W	54
#define MS_BTN_ICON_H	48

#define MS_BTN_ICON_L	(((MERGE_SWAP_BTN_W/2)-(MS_BTN_ICON_W/2))/MERGE_SWAP_BTN_W)
#define MS_BTN_ICON_T	(MS_BTN_ICON_T_OFFSET/MERGE_SWAP_BTN_H)
#define MS_BTN_ICON_R	(((MERGE_SWAP_BTN_W/2)+(MS_BTN_ICON_W/2))/MERGE_SWAP_BTN_W)
#define MS_BTN_ICON_B	((MS_BTN_ICON_T_OFFSET+MS_BTN_ICON_H)/MERGE_SWAP_BTN_H)

#define MS_BTN_TEXT_B	((MERGE_SWAP_BTN_H-MS_BTN_ICON_T_OFFSET)/MERGE_SWAP_BTN_H)

/* Active Info */
#define ACTIVE_INFO_LAYOUT_T_OFFSET	8

#define ACTIVE_INFO_L	REL_W(CALLER_INF_LAYOUT_OFFSET)
#define ACTIVE_INFO_T	(MERGE_SWAP_BTN_B+(ACTIVE_INFO_LAYOUT_T_OFFSET/ONE_HOLD_H))
#define ACTIVE_INFO_R	(1-REL_W(CALLER_INF_LAYOUT_OFFSET))
#define ACTIVE_INFO_B	(ACTIVE_INFO_T+(CALLER_INF_LAYOUT_H/ONE_HOLD_H))

/* Arrow */
// TODO: Need to change on Manage Btn size after On Hold and Active layouts will be separated on two
#define ARROW_R_OFFSET	12
#define ARROW_B_OFFSET	68

#define ARROW_SIZE	50

#define ARROW_L		(1-((ARROW_SIZE+ARROW_R_OFFSET)/CALLER_INF_LAYOUT_W))
#define ARROW_T		(1-((ARROW_SIZE+ARROW_B_OFFSET)/CALLER_INF_LAYOUT_H))
#define ARROW_R		(1-(ARROW_R_OFFSET/CALLER_INF_LAYOUT_W))
#define ARROW_B		(1-(ARROW_B_OFFSET/CALLER_INF_LAYOUT_H))

/* End call bottom button BG */
#define ENDCALL_BTN_BG_H		152
#define ENDCALL_BTN_BG_W		MAIN_SCREEN_W

#define ENDCALL_BTN_BG_T_PAD	(MAIN_SCREEN_H-ENDCALL_BTN_BG_H)
#define ENDCALL_BTN_BG_L_PAD	0

#define ENDCALL_BTN_BG_L		REL_W(ENDCALL_BTN_BG_L_PAD)
#define ENDCALL_BTN_BG_R		REL_W(ENDCALL_BTN_BG_W)
#define ENDCALL_BTN_BG_T		REL_H(ENDCALL_BTN_BG_T_PAD)
#define ENDCALL_BTN_BG_B		REL_H(ENDCALL_BTN_BG_T_PAD+ENDCALL_BTN_BG_H)

/* End call bottom button */
#define ENDCALL_BTN_SIZE		104
#define ENDCALL_BTN_L_OFFSET	((ENDCALL_BTN_BG_W-ENDCALL_BTN_SIZE)/2)
#define ENDCALL_BTN_T_OFFSET	((ENDCALL_BTN_BG_H-ENDCALL_BTN_SIZE)/2)

#define ENDCALL_BTN_L			REL_ENDCALL_BTN_BG_W(ENDCALL_BTN_L_OFFSET)
#define ENDCALL_BTN_R			REL_ENDCALL_BTN_BG_W(ENDCALL_BTN_L_OFFSET+ENDCALL_BTN_SIZE)
#define ENDCALL_BTN_T			REL_ENDCALL_BTN_BG_H(ENDCALL_BTN_T_OFFSET)
#define ENDCALL_BTN_B			REL_ENDCALL_BTN_BG_H(ENDCALL_BTN_T_OFFSET+ENDCALL_BTN_SIZE)

#endif /*_CALLUI_VIEW_LAYOUT_H_*/
