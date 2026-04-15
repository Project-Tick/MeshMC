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
 *//* html.h - HTML renderer and utilities */

#ifndef HOEDOWN_HTML_H
#define HOEDOWN_HTML_H

#include "document.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************
 * CONSTANTS *
 *************/

typedef enum hoedown_html_flags {
	HOEDOWN_HTML_SKIP_HTML = (1 << 0),
	HOEDOWN_HTML_ESCAPE = (1 << 1),
	HOEDOWN_HTML_HARD_WRAP = (1 << 2),
	HOEDOWN_HTML_USE_XHTML = (1 << 3)
} hoedown_html_flags;

typedef enum hoedown_html_tag {
	HOEDOWN_HTML_TAG_NONE = 0,
	HOEDOWN_HTML_TAG_OPEN,
	HOEDOWN_HTML_TAG_CLOSE
} hoedown_html_tag;

/*********
 * TYPES *
 *********/

struct hoedown_html_renderer_state {
	void* opaque;

	struct {
		int header_count;
		int current_level;
		int level_offset;
		int nesting_level;
	} toc_data;

	hoedown_html_flags flags;

	/* extra callbacks */
	void (*link_attributes)(hoedown_buffer* ob, const hoedown_buffer* url,
							const hoedown_renderer_data* data);
};
typedef struct hoedown_html_renderer_state hoedown_html_renderer_state;

/*************
 * FUNCTIONS *
 *************/

/* hoedown_html_smartypants: process an HTML snippet using SmartyPants for smart
 * punctuation */
void hoedown_html_smartypants(hoedown_buffer* ob, const uint8_t* data,
							  size_t size);

/* hoedown_html_is_tag: checks if data starts with a specific tag, returns the
 * tag type or NONE */
hoedown_html_tag hoedown_html_is_tag(const uint8_t* data, size_t size,
									 const char* tagname);

/* hoedown_html_renderer_new: allocates a regular HTML renderer */
hoedown_renderer* hoedown_html_renderer_new(hoedown_html_flags render_flags,
											int nesting_level)
	__attribute__((malloc));

/* hoedown_html_toc_renderer_new: like hoedown_html_renderer_new, but the
 * returned renderer produces the Table of Contents */
hoedown_renderer* hoedown_html_toc_renderer_new(int nesting_level)
	__attribute__((malloc));

/* hoedown_html_renderer_free: deallocate an HTML renderer */
void hoedown_html_renderer_free(hoedown_renderer* renderer);

#ifdef __cplusplus
}
#endif

#endif /** HOEDOWN_HTML_H **/
