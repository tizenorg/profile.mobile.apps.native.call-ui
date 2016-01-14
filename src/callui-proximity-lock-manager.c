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

#include <sensor.h>

#include "callui-proximity-lock-manager.h"
#include "callui-common.h"

#define SENSOR_INTERVAL 100

typedef enum {
	PLM_LCD_NONE,
	PLM_LCD_ON,
	PLM_LCD_OFF
} proximity_lock_manager_state;

typedef struct proximity_lock {
	sensor_h sensor;
	sensor_listener_h sensor_listener;
	bool is_started;
	proximity_lock_manager_state state;
	unlock_cb_t unlock_cb;
	void *cb_data;
} proximity_lock_t;

/**
 * @brief Create proximity lock manager
 *
 * @return The proximity lock handle as void pointer
 *
 * @see proximity_lock_t
 */
static void *__callui_proximity_lock_manager_create();

/**
 * @brief Starts proximity sensor listener
 *
 * @param[in]   lock_h The proximity lock handle
 *
 * @see proximity_lock_t
 */
static void __callui_proximity_lock_manager_start(void *lock_h);

/**
 * @brief Stops proximity sensor listener
 *
 * @param[in]   lock_h The proximity lock handle
 * @param[in]   force  force stops when true, otherwise just stops
 *
 * @see proximity_lock_t
 */
static void __callui_proximity_lock_manager_force_stop(void *lock_h, bool force);

/**
 * @brief Stops proximity sensor listener
 *
 * @param[in]   lock_h The proximity lock handle
 *
 * @see proximity_lock_t
 */
static void __callui_proximity_lock_manager_stop(void *lock_h);

/**
 * @brief Gets proximity lock status
 *
 * @param[in]   lock_h The proximity lock handle
 *
 * @return true when proximity lock manager is started, otherwise false
 *
 * @see proximity_lock_t
 */
static bool __callui_proximity_lock_manager_is_started(void *lock_h);

/**
 * @brief Destroy proximity lock manager
 *
 * @param[in]   proximity_h The proximity lock handle
 *
 * @see proximity_lock_t
 */
static void __callui_proximity_lock_manager_destroy(void *lock_h);
static bool __callui_proximity_lock_manager_is_lcd_off(void *lock_h);
static void __callui_proximity_lock_manager_set_callback_on_unlock(void *lock_h, unlock_cb_t callback, void *data);

static int __callui_proximity_lock_manager_create_listener(proximity_lock_t *proximity_h);
static void __callui_proximity_lock_manager_cb(sensor_h sensor, sensor_event_s *sensor_data, void *user_data);

static int __callui_proximity_lock_manager_create_listener(proximity_lock_t *proximity_h)
{
	dbg("..");
	int ret = SENSOR_ERROR_NONE;
	CALLUI_RETURN_VALUE_IF_FAIL(proximity_h, SENSOR_ERROR_INVALID_PARAMETER);
	ret = sensor_create_listener(proximity_h->sensor, &proximity_h->sensor_listener);
	if (ret != SENSOR_ERROR_NONE) {
		err("sensor_create_listener() failed(%d)", ret);
		return ret;
	}
	sensor_listener_set_option(proximity_h->sensor_listener, SENSOR_OPTION_ALWAYS_ON);
	sensor_listener_set_event_cb(proximity_h->sensor_listener, SENSOR_INTERVAL, __callui_proximity_lock_manager_cb, proximity_h);
	return ret;
}

static void __callui_proximity_lock_manager_cb(sensor_h sensor, sensor_event_s *sensor_data, void *user_data)
{
	CALLUI_RETURN_IF_FAIL(user_data);
	proximity_lock_t *proximity_h = user_data;
	CALLUI_RETURN_IF_FAIL(sensor_data);
	float value = sensor_data->values[0];
	dbg("changed proximity sensor value = %f", value);
	if (value > 0) {
		if (proximity_h->state == PLM_LCD_OFF) {
			_callui_common_dvc_control_lcd_state(LCD_ON);
			proximity_h->state = PLM_LCD_ON;
			if (proximity_h->unlock_cb) {
				proximity_h->unlock_cb(proximity_h->cb_data);
				proximity_h->unlock_cb = NULL;
				proximity_h->cb_data = NULL;
			}
		}
	} else {
		if (_callui_common_get_lcd_state() != LCD_OFF) {
			_callui_common_dvc_control_lcd_state(LCD_OFF);
			proximity_h->state = PLM_LCD_OFF;
		}
	}
}

static void *__callui_proximity_lock_manager_create()
{
	dbg("..");
	proximity_lock_t *proximity_h = calloc(1, sizeof(proximity_lock_t));
	CALLUI_RETURN_NULL_IF_FAIL(proximity_h);
	proximity_h->is_started = false;
	proximity_h->state = PLM_LCD_NONE;
	proximity_h->unlock_cb = NULL;
	proximity_h->cb_data = NULL;

	int ret = SENSOR_ERROR_NONE;
	ret = sensor_get_default_sensor(SENSOR_PROXIMITY, &proximity_h->sensor);
	if (ret != SENSOR_ERROR_NONE) {
		err("sensor_get_default_sensor() failed(%d)", ret);
		free(proximity_h);
		return NULL;
	}
	ret = __callui_proximity_lock_manager_create_listener(proximity_h);
	if (ret != SENSOR_ERROR_NONE) {
		err("create listener failed(%d)", ret);
		free(proximity_h);
		return NULL;
	}
	return proximity_h;
}

static void __callui_proximity_lock_manager_start(void *lock_h)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h);
	proximity_lock_t *proximity_h = (proximity_lock_t *)lock_h;
	int ret = sensor_listener_start(proximity_h->sensor_listener);
	if (ret != SENSOR_ERROR_NONE) {
		err("sensor_listener_start() failed(%d)", ret);
	}
#ifdef _DBUS_DVC_LSD_TIMEOUT_
	_callui_common_dvc_set_lcd_timeout(LCD_TIMEOUT_SET);
#endif

	proximity_h->is_started = true;
}

static void __callui_proximity_lock_manager_force_stop(void *lock_h, bool force)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h);
	proximity_lock_t *proximity_h = (proximity_lock_t *)lock_h;
	if (!force && __callui_proximity_lock_manager_is_lcd_off(proximity_h)) {
		__callui_proximity_lock_manager_set_callback_on_unlock(proximity_h, __callui_proximity_lock_manager_stop, proximity_h);
	} else {
		__callui_proximity_lock_manager_stop(proximity_h);
	}
}

static void __callui_proximity_lock_manager_stop(void *lock_h)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h);
	proximity_lock_t *proximity_h = (proximity_lock_t *)lock_h;
	int ret = sensor_listener_stop(proximity_h->sensor_listener);
	if (ret != SENSOR_ERROR_NONE) {
		err("sensor_listener_stop() failed(%d)", ret);
	}
	proximity_h->is_started = false;
	proximity_h->state = PLM_LCD_NONE;
}

static bool __callui_proximity_lock_manager_is_started(void *lock_h)
{
	dbg("..");
	CALLUI_RETURN_VALUE_IF_FAIL(lock_h, false);
	proximity_lock_t *proximity_h = (proximity_lock_t *)lock_h;
	return proximity_h->is_started;
}

static void __callui_proximity_lock_manager_destroy(void *lock_h)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h);
	proximity_lock_t *proximity_h = (proximity_lock_t *)lock_h;
	sensor_listener_unset_event_cb(proximity_h->sensor_listener);
	sensor_destroy_listener(proximity_h->sensor_listener);
	free(proximity_h);
}

static bool __callui_proximity_lock_manager_is_lcd_off(void *lock_h)
{
	dbg("..");
	CALLUI_RETURN_VALUE_IF_FAIL(lock_h, false);
	proximity_lock_t *proximity_h = (proximity_lock_t *)lock_h;
	return (proximity_h->state == PLM_LCD_OFF);
}

static void __callui_proximity_lock_manager_set_callback_on_unlock(void *lock_h, unlock_cb_t callback, void *data)
{
	dbg("..");
	CALLUI_RETURN_IF_FAIL(lock_h || callback || data);
	proximity_lock_t *proximity_h = (proximity_lock_t *)lock_h;
	proximity_h->unlock_cb = callback;
	proximity_h->cb_data = data;
}

bool _callui_proximity_lock_manager_is_supported()
{
	bool is_supported = false;
	sensor_is_supported(SENSOR_PROXIMITY, &is_supported);
	return is_supported;
}

void _callui_proximity_lock_manager_init(lock_data_t *lock_h)
{
	lock_h->create = __callui_proximity_lock_manager_create;
	lock_h->start = __callui_proximity_lock_manager_start;
	lock_h->stop = __callui_proximity_lock_manager_force_stop;
	lock_h->is_started = __callui_proximity_lock_manager_is_started;
	lock_h->is_lcd_off = __callui_proximity_lock_manager_is_lcd_off;
	lock_h->set_unlock_cb = __callui_proximity_lock_manager_set_callback_on_unlock;
	lock_h->destroy = __callui_proximity_lock_manager_destroy;
}
