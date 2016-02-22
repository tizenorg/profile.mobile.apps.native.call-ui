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

#include "callui-common.h"
#include "callui.h"
#include "callui-view-elements.h"
#include "callui-view-multi-call-list.h"
#include "callui-view-layout.h"

#define CALLUI_APP_DATA_NAME "multi_call_app_data"

struct _callui_view_mc_list {
	call_view_data_base_t base_view;

	Evas_Object *call_genlist;
	GSList *call_list;
	Elm_Genlist_Item_Class *call_genlist_itc;
};
typedef struct _callui_view_mc_list _callui_view_mc_list_t;

static int __callui_view_multi_call_list_oncreate(call_view_data_base_t *view_data, void *appdata);
static int __callui_view_multi_call_list_onupdate(call_view_data_base_t *view_data);
static int __callui_view_multi_call_list_ondestroy(call_view_data_base_t *view_data);

static int __create_main_content(callui_view_mc_list_h vd);
static int __update_displayed_data(callui_view_mc_list_h vd);

static void __caller_genlist_init_item_class(callui_view_mc_list_h vd);
static void __caller_genlist_deinit_item_class(callui_view_mc_list_h vd);

static int __caller_genlist_add(callui_view_mc_list_h vd);
static void __caller_genlist_delete(callui_view_mc_list_h vd);
static void __caller_genlist_clear(callui_view_mc_list_h vd);
static void __caller_genlist_fill(callui_view_mc_list_h vd);

static Evas_Object *__caller_genlist_content_cb(void *data, Evas_Object *obj, const char *part);
static char *__caller_genlist_txt_cb(void *data, Evas_Object *obj, const char *part);

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __split_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info);

static void __back_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void __list_free_cb(gpointer data);

callui_view_mc_list_h _callui_view_multi_call_list_new()
{
	callui_view_mc_list_h mc_list_view = calloc(1, sizeof(_callui_view_mc_list_t));
	CALLUI_RETURN_NULL_IF_FAIL(mc_list_view);

	mc_list_view->base_view.onCreate = __callui_view_multi_call_list_oncreate;
	mc_list_view->base_view.onUpdate = __callui_view_multi_call_list_onupdate;
	mc_list_view->base_view.onDestroy = __callui_view_multi_call_list_ondestroy;

	return mc_list_view;
}

static int __callui_view_multi_call_list_oncreate(call_view_data_base_t *view_data, void *appdata)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);
	CALLUI_RETURN_VALUE_IF_FAIL(appdata, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_list_h vd = (callui_view_mc_list_h)view_data;
	view_data->ad = (callui_app_data_t *)appdata;

	int res = __create_main_content(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return __update_displayed_data(vd);
}

static int __callui_view_multi_call_list_onupdate(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_list_h vd = (callui_view_mc_list_h)view_data;

	return __update_displayed_data(vd);
}

static int __update_displayed_data(callui_view_mc_list_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	g_slist_free_full(vd->call_list, __list_free_cb);

	int res = cm_get_conference_call_list(ad->cm_handle, &vd->call_list);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CM_ERROR_NONE, CALLUI_RESULT_FAIL);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->call_list, CALLUI_RESULT_FAIL);

	__caller_genlist_clear(vd);

	__caller_genlist_init_item_class(vd);
	__caller_genlist_fill(vd);
	__caller_genlist_deinit_item_class(vd);

	evas_object_show(vd->base_view.contents);

	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);

	return CALLUI_RESULT_OK;
}

static int __callui_view_multi_call_list_ondestroy(call_view_data_base_t *view_data)
{
	CALLUI_RETURN_VALUE_IF_FAIL(view_data, CALLUI_RESULT_INVALID_PARAM);

	callui_view_mc_list_h vd = (callui_view_mc_list_h)view_data;
	callui_app_data_t *ad = vd->base_view.ad;

	__caller_genlist_delete(vd);

	__caller_genlist_deinit_item_class(vd);

	g_slist_free_full(vd->call_list, __list_free_cb);

	if (vd->base_view.contents) {
		evas_object_del(vd->base_view.contents);
		vd->base_view.contents = NULL;
	}

	elm_object_part_content_unset(ad->main_ly, "elm.swallow.content");

	return CALLUI_RESULT_OK;
}

static int __create_main_content(callui_view_mc_list_h vd)
{
	callui_app_data_t *ad = vd->base_view.ad;

	vd->base_view.contents = _callui_load_edj(ad->main_ly, EDJ_NAME, GRP_MULTICALL);
	CALLUI_RETURN_VALUE_IF_FAIL(vd->base_view.contents, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", vd->base_view.contents);

	// TODO: replace this into view manager in nearest future
	eext_object_event_callback_add(vd->base_view.contents, EEXT_CALLBACK_BACK, __back_btn_click_cb, ad);

	Evas_Object *back_btn = elm_button_add(vd->base_view.contents);
	CALLUI_RETURN_VALUE_IF_FAIL(back_btn, CALLUI_RESULT_ALLOCATION_FAIL);
	elm_object_style_set(back_btn, "bottom");
	elm_object_text_set(back_btn, _("IDS_CALL_BUTTON_RETURN_TO_CALL_SCREEN_ABB"));
	elm_object_part_content_set(vd->base_view.contents, "bottom_btn", back_btn);
	evas_object_smart_callback_add(back_btn, "clicked", __back_btn_click_cb, ad);
	evas_object_show(back_btn);

	int res = __caller_genlist_add(vd);
	CALLUI_RETURN_VALUE_IF_FAIL(res == CALLUI_RESULT_OK, res);

	return CALLUI_RESULT_OK;
}

static void __end_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_app_data_t *ad = (callui_app_data_t *)evas_object_data_get(obj, CALLUI_APP_DATA_NAME);
	CALLUI_RETURN_IF_FAIL(ad);

	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;

	unsigned int call_id = 0;
	cm_conf_call_data_get_call_id(call_data, &call_id);

	int ret = cm_end_call(ad->cm_handle, call_id, CALL_RELEASE_TYPE_BY_CALL_HANDLE);
	if (ret != CM_ERROR_NONE) {
		err("cm_end_call() get failed with err[%d]", ret);
	}
	ad->multi_call_list_end_clicked = true;
}

static void __split_call_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	CALLUI_RETURN_IF_FAIL(data);
	callui_app_data_t *ad = (callui_app_data_t *)evas_object_data_get(obj, CALLUI_APP_DATA_NAME);
	CALLUI_RETURN_IF_FAIL(ad);

	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;

	unsigned int call_id = 0;
	cm_conf_call_data_get_call_id(call_data, &call_id);

	int ret = cm_split_call(ad->cm_handle, call_id);
	if (ret != CM_ERROR_NONE) {
		err("cm_split_call() get failed with err[%d]", ret);
	}
}

static Evas_Object *__caller_genlist_content_cb(void *data, Evas_Object *obj, const char *part)
{
	CALLUI_RETURN_NULL_IF_FAIL(data);
	callui_app_data_t *ad = (callui_app_data_t *)evas_object_data_get(obj, CALLUI_APP_DATA_NAME);
	CALLUI_RETURN_NULL_IF_FAIL(ad);

	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;

	Evas_Object *img = NULL;
	if ((strcmp(part, "elm.swallow.end") == 0)) {
		img = elm_image_add(obj);
		elm_image_file_set(img, EDJ_NAME, GROUP_CALL_BTN_IC_END);
		evas_object_size_hint_min_set(img, ELM_SCALE_SIZE(CALL_BTN_IC_BG_SIZE), ELM_SCALE_SIZE(CALL_BTN_IC_BG_SIZE));
		evas_object_propagate_events_set(img, EINA_FALSE);
		evas_object_smart_callback_add(img, "clicked", __end_call_btn_click_cb, call_data);
		evas_object_data_set(img, CALLUI_APP_DATA_NAME, ad);
	} else if (strcmp(part, "elm.swallow.icon") == 0) {
		if (ad->held || ad->active->member_count < 3) {
			return NULL;
		}
		img = elm_image_add(obj);
		elm_image_file_set(img, EDJ_NAME, GROUP_CALL_BTN_IC_SPLIT);
		evas_object_size_hint_min_set(img, ELM_SCALE_SIZE(CALL_BTN_IC_BG_SIZE), ELM_SCALE_SIZE(CALL_BTN_IC_BG_SIZE));
		evas_object_propagate_events_set(img, EINA_FALSE);
		evas_object_smart_callback_add(img, "clicked", __split_call_btn_click_cb, call_data);
		evas_object_data_set(img, CALLUI_APP_DATA_NAME, ad);
	} else if ((strcmp(part, "elm.swallow.icon.0") == 0)) {
		call_contact_data_t ct_info = {0,};
		char *file_path = "default";
		int person_id = -1;

		int res = cm_conf_call_data_get_person_id(call_data, &person_id);
		if (res == CM_ERROR_NONE && person_id != -1) {
			_callui_common_get_contact_info(person_id, &ct_info);
			if (strlen(ct_info.caller_id_path) > 0) {
				file_path = ct_info.caller_id_path;
			}
		}
		img = _callui_create_thumbnail_with_size(obj, file_path, THUMBNAIL_98, true);
	}
	return img;
}

static char *__caller_genlist_txt_cb(void *data, Evas_Object *obj, const char *part)
{
	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;
	int person_id = -1;
	char *call_number = NULL;
	dbg("__caller_genlist_txt_cb %s ", part);
	if (strcmp(part, "elm.text") == 0) {
		cm_conf_call_data_get_person_id(call_data, &person_id);
		if (person_id != -1) {
			call_contact_data_t ct_info = { 0 };
			_callui_common_get_contact_info(person_id, &ct_info);
			if (strlen(ct_info.call_disp_name) > 0) {
				return strdup(ct_info.call_disp_name);
			}
		}
		cm_conf_call_data_get_call_number(call_data, &call_number);
		return strdup(call_number);
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

static int __caller_genlist_add(callui_view_mc_list_h vd)
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

static void __caller_genlist_delete(callui_view_mc_list_h vd)
{
	if (vd->call_genlist != NULL) {
		evas_object_del(vd->call_genlist);
		vd->call_genlist = NULL;
	}
}

static void __caller_genlist_clear(callui_view_mc_list_h vd)
{
	if (vd->call_genlist != NULL) {
		elm_genlist_clear(vd->call_genlist);
	}
}

static void __caller_genlist_fill(callui_view_mc_list_h vd)
{
	CALLUI_RETURN_IF_FAIL(vd->call_list);

	int list_len = g_slist_length(vd->call_list);
	Elm_Object_Item *item = NULL;
	cm_conf_call_data_t *call_data = NULL;
	int idx = 0;

	for (; idx < list_len; idx++) {
		call_data = (cm_conf_call_data_t *)g_slist_nth_data(vd->call_list, idx);
		item = elm_genlist_item_append(vd->call_genlist, vd->call_genlist_itc, (void *)call_data, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
		elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	}
}

static void __back_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;
	_callui_vm_auto_change_view(ad->view_manager_handle);
}

static void __list_free_cb(gpointer data)
{
	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;
	cm_conf_call_data_free(call_data);
}
