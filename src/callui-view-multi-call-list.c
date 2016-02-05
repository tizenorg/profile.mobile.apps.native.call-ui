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

static Elm_Genlist_Item_Class *itc_call;
#define	CALLUI_VIEW_MULTICALL_LIST_LAYOUT_ID "MULTIVIEWLIST"
typedef struct {
	Evas_Object *contents;
	Evas_Object *ic;
	Evas_Object *multibox_gl;

	GSList *call_list;
} callui_view_multi_call_list_priv_t;

static int __callui_view_multi_call_list_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *param3);
static int __callui_view_multi_call_list_onupdate(call_view_data_t *view_data, void *update_data1);
static int __callui_view_multi_call_list_onhide(call_view_data_t *view_data);
static int __callui_view_multi_call_list_onshow(call_view_data_t *view_data,  void *appdata);
static int __callui_view_multi_call_list_ondestroy(call_view_data_t *view_data);
static int __callui_view_multi_call_list_onrotate(call_view_data_t *view_data);

static call_view_data_t s_view = {
	.type = VIEW_INCALL_MULTICALL_LIST_VIEW,
	.layout = NULL,
	.onCreate = __callui_view_multi_call_list_oncreate,
	.onUpdate = __callui_view_multi_call_list_onupdate,
	.onHide = __callui_view_multi_call_list_onhide,
	.onShow = __callui_view_multi_call_list_onshow,
	.onDestroy = __callui_view_multi_call_list_ondestroy,
	.onRotate = __callui_view_multi_call_list_onrotate,
	.priv = NULL,
};

call_view_data_t *_callui_view_multi_call_list_new(callui_app_data_t *ad)
{
	s_view.priv = calloc(1, sizeof(callui_view_multi_call_list_priv_t));
	CALLUI_RETURN_VALUE_IF_FAIL(s_view.priv, NULL);
	return &s_view;
}

static void __callui_view_multi_call_list_small_end_call_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);
	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;
	CALLUI_RETURN_IF_FAIL(call_data);
	int ret = -1;
	unsigned int call_id = 0;
	cm_conf_call_data_get_call_id(call_data, &call_id);
	ret = cm_end_call(ad->cm_handle, call_id, CALL_RELEASE_TYPE_BY_CALL_HANDLE);
	if (ret != CM_ERROR_NONE) {
		err("cm_end_call() get failed with err[%d]", ret);
	}
	ad->multi_call_list_end_clicked = true;
}

static void __callui_view_multi_call_list_split_call_cb(void *data, Evas_Object *obj, void *event_info)
{
	dbg("..");
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);
	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;
	CALLUI_RETURN_IF_FAIL(call_data);
	int ret = -1;
	unsigned int call_id = 0;

	cm_conf_call_data_get_call_id(call_data, &call_id);
	ret = cm_split_call(ad->cm_handle, call_id);
	if (ret != CM_ERROR_NONE) {
		err("cm_split_call() get failed with err[%d]", ret);
	}
}

static Evas_Object *__callui_view_multi_call_list_gl_icon_get_call(void *data, Evas_Object *obj, const char *part)
{
	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;
	Evas_Object *img = NULL;
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_VALUE_IF_FAIL(ad, NULL);
	dbg("__callui_view_multi_call_list_gl_icon_get_call %s ", part);
	if ((strcmp(part, "elm.swallow.end") == 0)) {
		Evas_Object *img = elm_image_add(obj);
		elm_image_file_set(img, EDJ_NAME, GROUP_CALL_BTN_IC_END);
		evas_object_size_hint_min_set(img, ELM_SCALE_SIZE(CALL_BTN_IC_BG_SIZE), ELM_SCALE_SIZE(CALL_BTN_IC_BG_SIZE));
		evas_object_propagate_events_set(img, EINA_FALSE);
		evas_object_smart_callback_add(img, "clicked", __callui_view_multi_call_list_small_end_call_cb, call_data);
		return img;
	} else if (strcmp(part, "elm.swallow.icon") == 0) {
		if (ad->held || ad->active->member_count < 3) {
			return NULL;
		}
		Evas_Object *img = elm_image_add(obj);
		elm_image_file_set(img, EDJ_NAME, GROUP_CALL_BTN_IC_SPLIT);
		evas_object_size_hint_min_set(img, ELM_SCALE_SIZE(CALL_BTN_IC_BG_SIZE), ELM_SCALE_SIZE(CALL_BTN_IC_BG_SIZE));
		evas_object_propagate_events_set(img, EINA_FALSE);
		evas_object_smart_callback_add(img, "clicked", __callui_view_multi_call_list_split_call_cb, call_data);
		return img;
	} else if ((strcmp(part, "elm.swallow.icon.0") == 0)) {
		call_contact_data_t ct_info = {0,};
		char *file_path = "default";
		int person_id = -1;

		cm_conf_call_data_get_person_id(call_data, &person_id);
		if (person_id != -1) {
			_callui_common_get_contact_info(person_id, &ct_info);
			if (strlen(ct_info.caller_id_path) > 0) {
				file_path = ct_info.caller_id_path;
			}
		}
		img = _callui_create_thumbnail_with_size(obj, file_path, THUMBNAIL_98, true);
		return img;
	} else {
		return NULL;
	}
}

static char *__callui_view_multi_call_list_gl_label_get_call(void *data, Evas_Object *obj, const char *part)
{
	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;
	int person_id = -1;
	char *call_number = NULL;
	dbg("__callui_view_multi_call_list_gl_label_get_call %s ", part);
	if (strcmp(part, "elm.text") == 0) {
		cm_conf_call_data_get_person_id(call_data, &person_id);
		if (person_id != -1) {
			call_contact_data_t ct_info = {0,};
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

static void __callui_view_multi_call_list_genlist_init(void *data)
{
	itc_call = elm_genlist_item_class_new();

	itc_call->item_style = "type1";
	itc_call->func.text_get = __callui_view_multi_call_list_gl_label_get_call;
	itc_call->func.content_get = __callui_view_multi_call_list_gl_icon_get_call;
	itc_call->func.state_get = NULL;
	itc_call->func.del = NULL;
}

static void __callui_view_multi_call_list_genlist_deinit()
{
	elm_genlist_item_class_free(itc_call);
	itc_call = NULL;
}

static void __callui_view_multi_call_list_genlist_add(void *data)
{
	call_view_data_t *vd = data;
	callui_view_multi_call_list_priv_t *priv = (callui_view_multi_call_list_priv_t *)vd->priv;
	Evas_Object *genlist = NULL;

	genlist = elm_genlist_add(priv->contents);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_object_part_content_set(priv->contents, "swl_calllist", genlist);
	priv->multibox_gl = genlist;
}

static void __callui_view_multi_call_list_genlist_delete(void *data)
{
	call_view_data_t *vd = data;
	callui_view_multi_call_list_priv_t *priv = (callui_view_multi_call_list_priv_t *)vd->priv;
	if (priv->multibox_gl != NULL) {
		elm_genlist_clear(priv->multibox_gl);
		evas_object_del(priv->multibox_gl);
		priv->multibox_gl = NULL;
	}
}

void __callui_view_multi_call_list_genlist_item_append(void *data)
{
	dbg("..");
	Elm_Object_Item *item = NULL;
	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_IF_FAIL(ad);
	call_view_data_t *vd = data;
	CALLUI_RETURN_IF_FAIL(vd);
	callui_view_multi_call_list_priv_t *priv = (callui_view_multi_call_list_priv_t *)vd->priv;
	CALLUI_RETURN_IF_FAIL(priv);
	int list_len = 0;
	int idx = 0;
	cm_conf_call_data_t *call_data = NULL;

	CALLUI_RETURN_IF_FAIL(priv->call_list);
	list_len = g_slist_length(priv->call_list);
	for (idx = 0; idx < list_len; idx++) {
		call_data = (cm_conf_call_data_t *)g_slist_nth_data(priv->call_list, idx);
		item = elm_genlist_item_append(priv->multibox_gl, itc_call, (void *)call_data, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
		elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	}
}

static void __callui_view_multi_call_list_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	callui_app_data_t *ad = (callui_app_data_t *)data;
	_callvm_view_auto_change(ad);
}

static Evas_Object *__callui_view_multi_call_list_create_contents(Evas_Object *parent, char *group)
{
	Evas_Object *eo;

	if (parent == NULL) {
		err("ERROR");
		return NULL;
	}

	/* load edje */
	eo = _callui_load_edj(parent, EDJ_NAME, group);
	if (eo == NULL)
		return NULL;

	return eo;
}

static int __callui_view_multi_call_list_oncreate(call_view_data_t *view_data, unsigned int param1, void *param2, void *param3)
{
	dbg("multicall-list view create");

	__callui_view_multi_call_list_onshow(view_data, NULL);

	return 0;
}

static void __callui_view_multi_call_list_free_cb(gpointer data)
{
	cm_conf_call_data_t *call_data = (cm_conf_call_data_t *)data;
	cm_conf_call_data_free(call_data);
}

static int __callui_view_multi_call_list_onupdate(call_view_data_t *view_data, void *update_data1)
{
	dbg("multicall-list view update");

	callui_app_data_t *ad = _callui_get_app_data();
	callui_view_multi_call_list_priv_t *priv = (callui_view_multi_call_list_priv_t *)view_data->priv;
	GSList *list = NULL;

	__callui_view_multi_call_list_genlist_delete(view_data);
	g_slist_free_full(priv->call_list, __callui_view_multi_call_list_free_cb);

	cm_get_conference_call_list(ad->cm_handle, &list);
	CALLUI_RETURN_VALUE_IF_FAIL(list, -1);
	priv->call_list = list;
	__callui_view_multi_call_list_genlist_add(view_data);
	__callui_view_multi_call_list_genlist_init(view_data);
	__callui_view_multi_call_list_genlist_item_append(view_data);
	__callui_view_multi_call_list_genlist_deinit();

	elm_object_signal_emit(priv->contents, "set_portrait", "multicall_list_layout");
	evas_object_show(priv->contents);
	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);
	return 0;
}

static int __callui_view_multi_call_list_onhide(call_view_data_t *view_data)
{
	dbg("multicall-list view hide");
	callui_app_data_t *ad = _callui_get_app_data();
	evas_object_hide(ad->main_ly);
	return 0;
}

static int __callui_view_multi_call_list_onshow(call_view_data_t *view_data,  void *appdata)
{
	dbg("multicall-list view show");

	callui_app_data_t *ad = _callui_get_app_data();
	callui_view_multi_call_list_priv_t *priv = (callui_view_multi_call_list_priv_t *)view_data->priv;
	Evas_Object *back_btn = NULL;

	priv->contents = elm_object_part_content_get(ad->main_ly, "elm.swallow.content");
	if (priv->contents) {
		elm_object_part_content_unset(ad->main_ly, "elm.swallow.content");
		evas_object_del(priv->contents);
		priv->contents = NULL;
	}
	priv->contents = __callui_view_multi_call_list_create_contents(ad->main_ly, GRP_MULTICALL);
	elm_object_part_content_set(ad->main_ly, "elm.swallow.content", priv->contents);
	view_data->layout = priv->contents;

	__callui_view_multi_call_list_genlist_add(view_data);
	__callui_view_multi_call_list_genlist_init(view_data);

	cm_get_conference_call_list(ad->cm_handle, &priv->call_list);
	__callui_view_multi_call_list_genlist_item_append(view_data);
	__callui_view_multi_call_list_genlist_deinit();

	back_btn = elm_button_add(priv->contents);
	elm_object_style_set(back_btn, "bottom");
	elm_object_text_set(back_btn, _("IDS_CALL_BUTTON_RETURN_TO_CALL_SCREEN_ABB"));
	elm_object_part_content_set(priv->contents, "bottom_btn", back_btn);

	evas_object_smart_callback_del(back_btn, "clicked", __callui_view_multi_call_list_back_cb);
	evas_object_smart_callback_add(back_btn, "clicked", __callui_view_multi_call_list_back_cb, ad);

	evas_object_show(back_btn);
	eext_object_event_callback_add(priv->contents, EEXT_CALLBACK_BACK, __callui_view_multi_call_list_back_cb, ad);

	evas_object_name_set(priv->contents, CALLUI_VIEW_MULTICALL_LIST_LAYOUT_ID);
	dbg("[========== MULTIVIEWLIST: priv->contents Addr : [%p] ==========]", priv->contents);

	elm_object_signal_emit(priv->contents, "set_portrait", "multicall_list_layout");

	evas_object_show(priv->contents);
	evas_object_hide(ad->main_ly);
	evas_object_show(ad->main_ly);
	return 0;
}

static int __callui_view_multi_call_list_ondestroy(call_view_data_t *view_data)
{
	dbg("multicall-list view destroy");

	callui_app_data_t *ad = _callui_get_app_data();
	CALLUI_RETURN_VALUE_IF_FAIL(ad, -1);
	call_view_data_t *vd = _callvm_get_call_view_data(ad, VIEW_INCALL_MULTICALL_LIST_VIEW);
	CALLUI_RETURN_VALUE_IF_FAIL(vd, -1);
	callui_view_multi_call_list_priv_t *priv = (callui_view_multi_call_list_priv_t *)vd->priv;

	if (priv != NULL) {
		__callui_view_multi_call_list_genlist_delete(vd);

		g_slist_free_full(priv->call_list, __callui_view_multi_call_list_free_cb);

		if (priv->contents) {
			evas_object_del(priv->contents);
			priv->contents = NULL;
		}
		elm_object_part_content_unset(ad->main_ly, "elm.swallow.content");
		free(priv);
		priv = NULL;
	}
	vd->layout = NULL;
	_callvm_reset_call_view_data(ad, VIEW_INCALL_MULTICALL_LIST_VIEW);
	dbg("complete destroy multi view list");
	return 0;
}

static int __callui_view_multi_call_list_onrotate(call_view_data_t *view_data)
{
	dbg("*** Multi Call List view Rotate ***");

	__callui_view_multi_call_list_onshow(view_data, NULL);
	return 0;
}


