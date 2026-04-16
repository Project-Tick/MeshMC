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
 *
 * MMCO Module Format
 *
 * .mmco files are shared libraries (.so/.dylib/.dll) that export a standard
 * ABI via C linkage. They are analogous to Linux kernel modules (.ko) but
 * operate at the launcher layer.
 *
 * Every .mmco module MUST export a symbol named `mmco_module_info` of type
 * MMCOModuleInfo. The loader validates the magic and ABI version before
 * calling any further entry points.
 *
 * The compiler toolchain produces .mmco by compiling C++ sources against
 * the MeshMC Plugin SDK and linking as a shared library with the .mmco
 * extension.
 *
 */

#pragma once

#include <cstdint>

#define MMCO_MAGIC 0x4D4D434F /* "MMCO" in big-endian ASCII */
#define MMCO_ABI_VERSION 1
#define MMCO_EXTENSION ".mmco"

struct MMCOModuleInfo {
	uint32_t magic;			 /* Must be MMCO_MAGIC */
	uint32_t abi_version;	 /* Must match MMCO_ABI_VERSION */
	const char* name;		 /* Human-readable module name */
	const char* version;	 /* Module version string */
	const char* author;		 /* Author / maintainer */
	const char* description; /* Short description */
	const char* license;	 /* SPDX license identifier */
	uint32_t flags;			 /* Reserved for future use, set to 0 */
	const char* code_link;	 /* Optional: URL to source code repository */
};

/* Module flags (reserved, extend as needed) */
#define MMCO_FLAG_NONE 0x00000000

/* Symbol visibility for .mmco shared libraries */
#if defined(_WIN32) || defined(__CYGWIN__)
#define MMCO_EXPORT __declspec(dllexport)
#else
#define MMCO_EXPORT __attribute__((visibility("default")))
#endif

/*
 * Every .mmco module must export these three symbols with C linkage:
 *
 *   extern "C" MMCOModuleInfo mmco_module_info;
 *   extern "C" int  mmco_init(struct MMCOContext* ctx);
 *   extern "C" void mmco_unload(void);
 *
 * mmco_init() returns 0 on success, non-zero on failure.
 */
