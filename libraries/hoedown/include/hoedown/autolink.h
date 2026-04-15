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
 *//* autolink.h - versatile autolinker */

#ifndef HOEDOWN_AUTOLINK_H
#define HOEDOWN_AUTOLINK_H

#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************
 * CONSTANTS *
 *************/

typedef enum hoedown_autolink_flags {
	HOEDOWN_AUTOLINK_SHORT_DOMAINS = (1 << 0)
} hoedown_autolink_flags;

/*************
 * FUNCTIONS *
 *************/

/* hoedown_autolink_is_safe: verify that a URL has a safe protocol */
int hoedown_autolink_is_safe(const uint8_t* data, size_t size);

/* hoedown_autolink__www: search for the next www link in data */
size_t hoedown_autolink__www(size_t* rewind_p, hoedown_buffer* link,
							 uint8_t* data, size_t offset, size_t size,
							 hoedown_autolink_flags flags);

/* hoedown_autolink__email: search for the next email in data */
size_t hoedown_autolink__email(size_t* rewind_p, hoedown_buffer* link,
							   uint8_t* data, size_t offset, size_t size,
							   hoedown_autolink_flags flags);

/* hoedown_autolink__url: search for the next URL in data */
size_t hoedown_autolink__url(size_t* rewind_p, hoedown_buffer* link,
							 uint8_t* data, size_t offset, size_t size,
							 hoedown_autolink_flags flags);

#ifdef __cplusplus
}
#endif

#endif /** HOEDOWN_AUTOLINK_H **/
