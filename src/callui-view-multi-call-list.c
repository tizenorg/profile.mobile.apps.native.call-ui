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

#include <Elementary.h>
#include <efl_extension.h>

#include "callui-view-multi-call-list.h"
#include "callui-debug.h"
#include "callui.h"
#include "callui-common.h"
#include "callui-view-elements.h"
#include "callui-view-layout.h"
#include "callui-state-provider.h"

#define CALLUI_GROUP_MULTICALL			"multicall_list"
#define CALLUI_GROUP_CALL_BTN_IC_SPLIT	"call_btn_ic_split"
#define CALLUI_GROUP_CALL_BTN_IC_END	"call_btn_ic_end"

#define CALLUI_APP_DATA_NAME			"app_data"

struct _callui_view_mc_list {
	call_view_data_base_t base_view;

	Evas_Object *call_genlist;
	Elm_Genlist_Item_Class *call_genlist_itc;
	Eina_List *conf_call_list;
};
typedef struct _callui_view_mc_list _callui_view_mc_list_t;

static callui_result_e __callui_view_multi_call_list_oncreate(call_view_data_base_t *view_data, Evas_Object *parent, void *appdata);
static callui_result_e __callui_view_multi_call_list_onupdate(call_view_data_base_t *view_data);
static callui_result_e __callui_view_multi_call_list_ondestroy(call_view_data_base_t *view_data);

static callui_result_e __create_main_content(callui_view_mc_list_h vd, Evas_Object *parent);
static callui_result_e __update_displayed_data(callui_view_mc_list_h vd);

static void __caller_genlist_init_item_class(callui_view_mc_list_h vd);
static void __caller_genlist_deinit_item_class(callui_view_mc_list_h vd);

static callui_result_e __caller_genlist_add(callui_view_mc_list_h vd);
static void __caller_genlist_clear(callui_view_mc_list_h vd);
static void __caller_genlist_fill(callui_view_mc_list_h vd);

static Evas_Object *__caller_genlist_content_cb(void *data, Evas_Object *obj, const char *part);
static char *__caller_genlist_txt_cb(void *data, Evas_Object *obj, const char *part);

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __split_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);

static void __back_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool __call_duration_timer_cb(void* data);

callui_view_mc_list_h _callui_view_multi_call_list_new()
{
	callui_view_mc_list_h mc_list_view = calloc(1, sizeof(_callui_view_mc_list_t));
	CALLUI_RETURN_NULL_IF_FAIL(mc_list_view);

	mc_list_view->base_view.create = __callui_view_multi_call_list_oncreate;
	mc_list_view->base_view.update = __callui_view_multi_call_list_onupdate;
	mc_list_view->base_view.destroy = __callui_view_multi_call_list_ondestroy;

	return mc_list_view;
}

static callui_result_e __callui_view_multi_call_list_oncreate(call_view_data_base_t *view_data, Evas_Object *parent, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(parent, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_list_h vd = (callui_view_mc_list_h)view_data;
	view_data->ad = (callui_app_data_t *)appdata;

	callui_result_e res = __create_main_content(vd, parent);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return __update_displayed_data(vd);
}

static callui_result_e __callui_view_multi_call_list_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	return __update_displayed_data((callui_view_mc_list_h)view_data);
}

static void __clear_conference_call_list(Eina_List **conf_list)
{
	Eina_List *l;
	callui_conf_call_data_t *data;

	EINA_LIST_FOREACH(*conf_list, l, data) {
		free(data);
	}
	*conf_list = eina_list_free(*conf_list);
}

static Eina_Bool __call_duration_timer_cb(void* data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(data, ECORE_CALLBACK_CANCEL);

	callui_view_mc_list_h vd = data;

	struct tm *new_tm = _callui_stp_get_call_duration(vd->base_view.ad->state_provider,
			CALLUI_CALL_DATA_ACTIVE);
	if (!new_tm) {
		vd->base_view.call_duration_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	_callui_common_try_update_call_duration_time(vd->base_view.call_duration_tm,
			new_tm,
			_callui_common_set_call_duration_time,
			vd->base_view.contents,
			"call_txt_status");

	free(new_tm);

	return ECORE_CALLBACK_RENEW;
}

static callui_result_e __update_displayed_data(callui_view_mc_list_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	__caller_genlist_clear(vd);

	__clear_conference_call_list(&vd->conf_call_list);

	vd->conf_call_list = _callui_stp_get_conference_call_list(ad->state_provider);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->conf_call_list, CALLUI_RESULT_FAIL);

	FREE(vd->base_view.call_duration_tm);
	vd->base_view.call_duration_tm = _callui_stp_get_call_duration(ad->state_provider, CALLUI_CALL_DATA_ACTIVE);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.call_duration_tm, CALLUI_RESULT_ALLOCATION_FAIL);

	_callui_common_set_call_duration_time(vd->base_view.call_duration_tm, vd->base_view.contents, "call_txt_status");

	DELETE_ECORE_TIMER(vd->base_view.call_duration_timer);
	vd->base_view.call_duration_timer = ecore_timer_add(0.1, __call_duration_timer_cb, vd);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.call_duration_timer, CALLUI_RESULT_ALLOCATION_FAIL);

	__caller_genlist_init_item_class(vd);
	__caller_genlist_fill(vd);
	__caller_genlist_deinit_item_class(vd);

	evas_object_show(vd->base_view.contents);

	return CALLUI_RESULT_OK;
}

static callui_result_e __callui_view_multi_call_list_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_list_h vd = (callui_view_mc_list_h)view_data;

	DELETE_ECORE_TIMER(vd->base_view.call_duration_timer);
	free(vd->base_view.call_duration_tm);

	__clear_conference_call_list(&vd->conf_call_list);

	DELETE_EVAS_OBJECT(vd->base_view.contents);

	free(vd);

	return CALLUI_RESULT_OK;
}

static callui_result_e __create_main_content(callui_view_mc_list_h vd, Evas_Object *parent)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(parent, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_MULTICALL);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(parent, "elm.swallow.content", vd->base_view.contents);

	// TODO: replace this into view manager in nearest future
	eext_object_event_callback_add(vd->base_view.contents, EEXT_CALLBACK_BACK, __back_btn_click_cb, ad);

	Evas_Object *back_btn = elm_button_add(vd->base_view.contents);
	CALLUI_RETURN_VALUE_IF_FAIL(back_btn, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_style_set(back_btn, "bottom");
	elm_object_translatable_text_set(back_btn, "IDS_CALL_BUTTON_RETURN_TO_CALL_SCREEN_ABB");
	elm_object_part_content_set(vd->base_view.contents, "bottom_btn", back_btn);
	evas_object_smart_callback_add(back_btn, "clicked", __back_btn_click_cb, ad);
	evas_object_show(back_btn);

	callui_result_e res = __caller_genlist_add(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return CALLUI_RESULT_OK;
}

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_app_data_t *ad = (callui_app_data_t *)evas_object_data_get(obj, CALLUI_APP_DATA_NAME);
	CALLUI_RETURN_IF_FAIL(ad);

	callui_conf_call_data_t *call_data = (callui_conf_call_data_t *)data;

	callui_result_e res = _callui_manager_end_call(ad->call_manager,
			call_data->call_id,
			CALLUI_CALL_RELEASE_BY_CALL_HANDLE);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_end_call() failed. res[%d]", res);
	}
}

static void __split_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_app_data_t *ad = (callui_app_data_t *)evas_object_data_get(obj, CALLUI_APP_DATA_NAME);
	CALLUI_RETURN_IF_FAIL(ad);

	callui_conf_call_data_t *call_data = (callui_conf_call_data_t *)data;

	callui_result_e res = _callui_manager_split_call(ad->call_manager, call_data->call_id);
	if (res != CALLUI_RESULT_OK) {
		err("_callui_manager_split_call() failed. res[%d]", res);
	}
}

static Evas_Object *__caller_genlist_content_cb(void *data, Evas_Object *obj, const char *part)
{
	CALLUI_RETURN_NULL_IF_FAIL(data);
	callui_app_data_t *ad = (callui_app_data_t *)evas_object_data_get(obj, CALLUI_APP_DATA_NAME);
	CALLUI_RETURN_NULL_IF_FAIL(ad);

	callui_conf_call_data_t *call_data = (callui_conf_call_data_t *)data;

	Evas_Object *img = NULL;
	if ((strcmp(part, "elm.swallow.end") == 0)) {
		img = elm_image_add(obj);
		elm_image_file_set(img, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_CALL_BTN_IC_END);
		evas_object_size_hint_min_set(img, ELM_SCALE_SIZE(MULTI_LIST_LIST_ICON_BG_SIZE), ELM_SCALE_SIZE(MULTI_LIST_LIST_ICON_BG_SIZE));
		evas_object_propagate_events_set(img, EINA_FALSE);
		evas_object_smart_callback_add(img, "clicked", __end_call_btn_click_cb, call_data);
		evas_object_data_set(img, CALLUI_APP_DATA_NAME, ad);
	} else if (strcmp(part, "elm.swallow.icon") == 0) {

		const callui_call_data_t *active = _callui_stp_get_call_data(ad->state_provider,
				CALLUI_CALL_DATA_ACTIVE);
		const callui_call_data_t *held = _callui_stp_get_call_data(ad->state_provider,
				CALLUI_CALL_DATA_HELD);

		if (held || (active && active->conf_member_count < 3)) {
			return NULL;
		}
		img = elm_image_add(obj);
		elm_image_file_set(img, CALLUI_CALL_EDJ_PATH, CALLUI_GROUP_CALL_BTN_IC_SPLIT);
		evas_object_size_hint_min_set(img, ELM_SCALE_SIZE(MULTI_LIST_LIST_ICON_BG_SIZE), ELM_SCALE_SIZE(MULTI_LIST_LIST_ICON_BG_SIZE));
		evas_object_propagate_events_set(img, EINA_FALSE);
		evas_object_smart_callback_add(img, "clicked", __split_call_btn_click_cb, call_data);
		evas_object_data_set(img, CALLUI_APP_DATA_NAME, ad);
	} else if ((strcmp(part, "elm.swallow.icon.0") == 0)) {
		img = _callui_create_cid_thumbnail_with_size(obj,
				CALLUI_CID_TYPE_SINGLE,
				CALLUI_CID_SIZE_TINY,
				call_data->call_ct_info.caller_id_path);
	}
	return img;
}

static char *__caller_genlist_txt_cb(void *data, Evas_Object *obj, const char *part)
{
	callui_conf_call_data_t *call_data = (callui_conf_call_data_t *)data;

	dbg("__caller_genlist_txt_cb %s ", part);
	if (strcmp(part, "elm.text") == 0) {
		if (strlen(call_data->call_ct_info.call_disp_name) > 0) {
			return strdup(call_data->call_ct_info.call_disp_name);
		} else {
			return strdup(call_data->call_num);
		}
	} else {
		return NULL;
	}
}

static void __caller_genlist_init_item_class(callui_view_mc_list_h vd)
{
	vd->call_genlist_itc = elm_genlist_item_class_new();

	vd->call_genlist_itc->item_style = "type1";
	vd->call_genlist_itc->func.text_get = __caller_genlist_txt_cb;
	vd->call_genlist_itc->func.content_get = __caller_genlist_content_cb;
	vd->call_genlist_itc->func.state_get = NULL;
	vd->call_genlist_itc->func.del = NULL;
}

static void __caller_genlist_deinit_item_class(callui_view_mc_list_h vd)
{
	elm_genlist_item_class_free(vd->call_genlist_itc);
	vd->call_genlist_itc = NULL;
}

static callui_result_e __caller_genlist_add(callui_view_mc_list_h vd)
{
	Evas_Object *genlist = elm_genlist_add(vd->base_view.contents);
	CALLUI_RETURN_VALUE_IF_FAIL(genlist, CALLUI_RESULT_ALLOCATION_FAIL);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_part_content_set(vd->base_view.contents, "swl_calllist", genlist);
	evas_object_data_set(genlist, CALLUI_APP_DATA_NAME, vd->base_view.ad);
	vd->call_genlist = genlist;

	return CALLUI_RESULT_OK;
}

static void __caller_genlist_clear(callui_view_mc_list_h vd)
{
	if (vd->call_genlist != NULL) {
		elm_genlist_clear(vd->call_genlist);
	}
}

static void __caller_genlist_fill(callui_view_mc_list_h vd)
{
	CALLUI_RETURN_IF_FAIL(vd->conf_call_list);

	Elm_Object_Item *item;
	Eina_List *l;
	callui_conf_call_data_t *data;

	EINA_LIST_FOREACH(vd->conf_call_list, l, data) {
		item = elm_genlist_item_append(vd->call_genlist,
				vd->call_genlist_itc,
				(void *)data,
				NULL,
				ELM_GENLIST_ITEM_NONE,
				NULL,
				NULL);
		elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	}
}

static void __back_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;
	_callui_vm_auto_change_view(ad->view_manager);
}
