/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 *//* stack.h - simple stacking */

#ifndef HOEDOWN_STACK_H
#define HOEDOWN_STACK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********
 * TYPES *
 *********/

struct hoedown_stack {
	void** item;
	size_t size;
	size_t asize;
};
typedef struct hoedown_stack hoedown_stack;

/*************
 * FUNCTIONS *
 *************/

/* hoedown_stack_init: initialize a stack */
void hoedown_stack_init(hoedown_stack* st, size_t initial_size);

/* hoedown_stack_uninit: free internal data of the stack */
void hoedown_stack_uninit(hoedown_stack* st);

/* hoedown_stack_grow: increase the allocated size to the given value */
void hoedown_stack_grow(hoedown_stack* st, size_t neosz);

/* hoedown_stack_push: push an item to the top of the stack */
void hoedown_stack_push(hoedown_stack* st, void* item);

/* hoedown_stack_pop: retrieve and remove the item at the top of the stack */
void* hoedown_stack_pop(hoedown_stack* st);

/* hoedown_stack_top: retrieve the item at the top of the stack */
void* hoedown_stack_top(const hoedown_stack* st);

#ifdef __cplusplus
}
#endif

#endif /** HOEDOWN_STACK_H **/
