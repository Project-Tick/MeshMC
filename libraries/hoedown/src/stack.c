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
 */

#include "hoedown/stack.h"

#include "hoedown/buffer.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void hoedown_stack_init(hoedown_stack* st, size_t initial_size)
{
	assert(st);

	st->item = NULL;
	st->size = st->asize = 0;

	if (!initial_size)
		initial_size = 8;

	hoedown_stack_grow(st, initial_size);
}

void hoedown_stack_uninit(hoedown_stack* st)
{
	assert(st);

	free(st->item);
}

void hoedown_stack_grow(hoedown_stack* st, size_t neosz)
{
	assert(st);

	if (st->asize >= neosz)
		return;

	st->item = hoedown_realloc(st->item, neosz * sizeof(void*));
	memset(st->item + st->asize, 0x0, (neosz - st->asize) * sizeof(void*));

	st->asize = neosz;

	if (st->size > neosz)
		st->size = neosz;
}

void hoedown_stack_push(hoedown_stack* st, void* item)
{
	assert(st);

	if (st->size >= st->asize)
		hoedown_stack_grow(st, st->size * 2);

	st->item[st->size++] = item;
}

void* hoedown_stack_pop(hoedown_stack* st)
{
	assert(st);

	if (!st->size)
		return NULL;

	return st->item[--st->size];
}

void* hoedown_stack_top(const hoedown_stack* st)
{
	assert(st);

	if (!st->size)
		return NULL;

	return st->item[st->size - 1];
}
