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

#ifndef __CALLUI_LOCK_MANAGER_H_
#define __CALLUI_LOCK_MANAGER_H_

#include <stdbool.h>

/**
 * @brief The lock handle
 */
typedef struct lock_data lock_data_t;

/**
 * @brief Called when lcd unlock
 */
typedef void (*unlock_cb_t)(void *data);

/**
 * @brief Called when lock manager creates
 */
typedef void *(*lock_create) ();

/**
 * @brief Called when lock manager starts
 */
typedef void (*lock_start) (void *lock_h);

/**
 * @brief Called when need lock status
 */
typedef bool (*lock_is_started) (void *lock_h);

/**
 * @brief Called when lock manager stops
 */
typedef void (*lock_stop) (void *lock_h, bool force);

/**
 * @brief Called when need lcd status
 */
typedef bool (*lock_is_lcd_off) (void *lock_h);

/**
 * @brief Called when need set unlock callback
 */
typedef void (*lock_set_unlock_cb) (void *lock_h, unlock_cb_t callback, void* data);

/**
 * @brief Called when lock manager destroys
 */
typedef void (*lock_destroy) (void *lock_h);

struct lock_data {
	lock_create create;
	lock_start start;
	lock_stop stop;
	lock_is_started is_started;
	lock_is_lcd_off is_lcd_off;
	lock_set_unlock_cb set_unlock_cb;
	lock_destroy destroy;
	void *handle;
};

/**
 * @brief Create lock manager
 * @return The lock handle
 * @see lock_data_t
 */
lock_data_t *_callui_lock_manager_create();

/**
 * @brief Starts lock manager
 * @param[in]   lock_h The lock handle
 * @see lock_data_t
 */
void _callui_lock_manager_start(lock_data_t *lock_h);

/**
 * @brief Stops lock manager
 * @param[in]   lock_h The lock handle
 * @see lock_data_t
 */
void _callui_lock_manager_stop(lock_data_t *lock_h);

/**
 * @brief Force stops lock manager
 * @param[in]   lock_h The lock handle
 * @see lock_data_t
 */
void _callui_lock_manager_force_stop(lock_data_t *lock_h);

/**
 * @brief Gets lock status
 * @param[in]   lock_h The lock handle
 * @return true when proximity lock manager is started, otherwise false
 * @see lock_data_t
 */
bool _callui_lock_manager_is_started(lock_data_t *lock_h);

/**
 * @brief Destroy lock manager
 * @param[in]   lock_h The lock handle
 * @see lock_data_t
 */
void _callui_lock_manager_destroy(lock_data_t *lock_h);

/**
 * @brief Gets proximity lcd status
 * @param[in]   lock_h The lock handle
 * @return true when proximity manager turned off lcd, otherwise false
 * @see proximity_lock_t
 */
bool _callui_lock_manager_is_lcd_off(lock_data_t *lock_h);

/**
 * @brief Sets callback to be run on unlock
 * @param[in]   lock_h     The lock handle
 * @param[in]   callback   The callback function to run on unlock
 * @param[in]   data       The user data to be passed to the callback function
 * @see lock_data_t
 * @see unlock_cb_t
 */
void _callui_lock_manager_set_callback_on_unlock(lock_data_t *lock_h, unlock_cb_t callback, void *data);

#endif /** __CALLUI_LOCK_MANAGER_H_ */
