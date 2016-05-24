/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "callui-color-box.h"
#include "callui-debug.h"
#include "callui-common-def.h"

#define CALLUI_COLOR_BOX_BG_PART_NAME "callui.swallow.bg"
#define CALLUI_COLOR_BOX_TIMER_INTERVAL_SEC 0.067
#define CALLUI_COLOR_BOX_ALPHA_STEP -0.04

#define FULL_ALPHA 0xFF

struct __callui_color_box_data {
	Evas_Object *box;
	Evas_Object *scroller;
	Ecore_Timer *timer;
	Eina_Bool update_on_timer;
	Eina_Bool calculating;

	double initial_opacity;
	callui_rgb_color_t initial_color;
};

typedef struct __callui_color_box_data callui_color_box_t;

static void __callui_color_box_update_colors_with_calculate(callui_color_box_t *cbd);
static void __callui_color_box_update_colors_with_timer(callui_color_box_t *cbd);
static void __callui_color_box_update_colors(callui_color_box_t *cbd);

static Eina_Bool __callui_color_box_evas_calculate(callui_color_box_t *cbd);

static Eina_Bool __callui_color_box_update_colors_timer_cb(void *data);

static void __callui_color_box_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void __callui_color_box_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void __callui_color_box_free_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void __callui_color_box_scroller_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void __callui_color_box_scroller_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void __callui_color_box_scroller_free_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

Evas_Object *_callui_color_box_add(Evas_Object *scroller, callui_rgb_color_t *color, double opacity)
{
	debug_enter();
	CALLUI_RETURN_NULL_IF_FAIL(scroller);
	CALLUI_RETURN_NULL_IF_FAIL(color);

	Evas_Object *box = NULL;
	callui_color_box_t *cbd = NULL;

	box = elm_box_add(scroller);
	CALLUI_RETURN_NULL_IF_FAIL(box);

	cbd = calloc(1, sizeof(callui_color_box_t));
	if (!cbd) {
		err("Memory allocation failed failed!");
		evas_object_del(box);
		return NULL;
	}

	cbd->box = box;
	cbd->scroller = scroller;
	cbd->initial_opacity = opacity;
	cbd->initial_color.r = color->r;
	cbd->initial_color.g = color->g;
	cbd->initial_color.b = color->b;

	evas_object_event_callback_add(box, EVAS_CALLBACK_MOVE, __callui_color_box_move_cb, cbd);
	evas_object_event_callback_add(box, EVAS_CALLBACK_RESIZE, __callui_color_box_resize_cb, cbd);
	evas_object_event_callback_add(box, EVAS_CALLBACK_FREE, __callui_color_box_free_cb, cbd);

	evas_object_event_callback_add(scroller, EVAS_CALLBACK_MOVE, __callui_color_box_scroller_move_cb, cbd);
	evas_object_event_callback_add(scroller, EVAS_CALLBACK_RESIZE, __callui_color_box_scroller_resize_cb, cbd);
	evas_object_event_callback_add(scroller, EVAS_CALLBACK_FREE, __callui_color_box_scroller_free_cb, cbd);

	return box;
}

void __callui_color_box_update_colors_with_calculate(callui_color_box_t *cbd)
{
	debug_enter();

	if (cbd->scroller && __callui_color_box_evas_calculate(cbd)) {
		__callui_color_box_update_colors(cbd);
	}
}

void __callui_color_box_update_colors_with_timer(callui_color_box_t *cbd)
{
	if (!cbd->scroller) {
		return;
	}

	if (!cbd->timer) {
		dbg("Starting update timer");
		cbd->timer = ecore_timer_add(CALLUI_COLOR_BOX_TIMER_INTERVAL_SEC,
				__callui_color_box_update_colors_timer_cb, cbd);
	}

	cbd->update_on_timer = EINA_TRUE;
}

void __callui_color_box_update_colors(callui_color_box_t *cbd)
{
	int scroller_y = 0;
	int scroller_h = 0;

	Eina_List *children = NULL;
	Eina_List *l = NULL;
	Evas_Object *subobj = NULL;

	double alpha = cbd->initial_opacity;

	evas_object_geometry_get(cbd->scroller, NULL, &scroller_y, NULL, &scroller_h);

	children = elm_box_children_get(cbd->box);
	EINA_LIST_FOREACH(children, l, subobj) {
		if (elm_object_widget_check(subobj)) {
			Evas_Object *edje = elm_layout_edje_get(subobj);
			if (edje_object_part_exists(edje, CALLUI_COLOR_BOX_BG_PART_NAME)) {

				int subobj_y = 0;
				int subobj_h = 0;
				int r = (int)(alpha * cbd->initial_color.r + 0.5);
				int g = (int)(alpha * cbd->initial_color.g + 0.5);
				int b = (int)(alpha * cbd->initial_color.b + 0.5);
				int a = (int)(alpha * FULL_ALPHA + 0.5);
				Evas_Object *bg = elm_object_part_content_get(subobj, CALLUI_COLOR_BOX_BG_PART_NAME);

				if (!bg) {
					bg = evas_object_rectangle_add(evas_object_evas_get(subobj));
					elm_object_part_content_set(subobj, CALLUI_COLOR_BOX_BG_PART_NAME, bg);
					evas_object_show(bg);
				}
				evas_object_color_set(bg, r, g, b, a);

				evas_object_geometry_get(subobj, NULL, &subobj_y, NULL, &subobj_h);
				if ((subobj_y + subobj_h > scroller_y) && (subobj_y < scroller_y + scroller_h)) {
					alpha += CALLUI_COLOR_BOX_ALPHA_STEP;
				}
			}
		}
	}
	eina_list_free(children);
}

Eina_Bool __callui_color_box_evas_calculate(callui_color_box_t *cbd)
{
	if (cbd->calculating) {
		return EINA_FALSE;
	}

	cbd->calculating = EINA_TRUE;
	evas_smart_objects_calculate(evas_object_evas_get(cbd->box));
	cbd->calculating = EINA_FALSE;

	return EINA_TRUE;
}

Eina_Bool __callui_color_box_update_colors_timer_cb(void *data)
{
	callui_color_box_t *cbd = data;

	if (cbd->update_on_timer) {
		__callui_color_box_update_colors(cbd);
		cbd->update_on_timer = EINA_FALSE;
		return ECORE_CALLBACK_RENEW;
	}

	cbd->timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}

void __callui_color_box_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	callui_color_box_t *cbd = data;

	__callui_color_box_update_colors_with_timer(cbd);
}

void __callui_color_box_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	callui_color_box_t *cbd = data;

	__callui_color_box_update_colors_with_calculate(cbd);
}

void __callui_color_box_free_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	callui_color_box_t *cbd = data;

	if (cbd->scroller) {
		evas_object_event_callback_del(cbd->scroller, EVAS_CALLBACK_MOVE, __callui_color_box_move_cb);
		evas_object_event_callback_del(cbd->scroller, EVAS_CALLBACK_RESIZE, __callui_color_box_resize_cb);
		evas_object_event_callback_del(cbd->scroller, EVAS_CALLBACK_FREE, __callui_color_box_scroller_free_cb);
	}

	DELETE_ECORE_TIMER(cbd->timer);

	free(cbd);
}

void __callui_color_box_scroller_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	callui_color_box_t *cbd = data;

	__callui_color_box_update_colors_with_timer(cbd);
}

void __callui_color_box_scroller_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	callui_color_box_t *cbd = data;

	__callui_color_box_update_colors(cbd);
}

void __callui_color_box_scroller_free_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	callui_color_box_t *cbd = data;

	cbd->scroller = NULL;

	DELETE_ECORE_TIMER(cbd->timer);
}
