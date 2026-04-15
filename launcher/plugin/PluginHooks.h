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

#pragma once

#include <cstdint>

/*
 * Hook points where plugins can intercept or extend MeshMC behaviour.
 *
 * Hooks follow the observer pattern: plugins register callbacks for
 * specific hook IDs, and MeshMC dispatches to all registered callbacks
 * when the corresponding event occurs.
 *
 * Hook callbacks receive a generic void* payload whose concrete type
 * depends on the hook ID (documented below).
 */

enum MMCOHookId : uint32_t {
    /* Application lifecycle */
    MMCO_HOOK_APP_INITIALIZED       = 0x0100,  /* payload: nullptr */
    MMCO_HOOK_APP_SHUTDOWN          = 0x0101,  /* payload: nullptr */

    /* Instance lifecycle */
    MMCO_HOOK_INSTANCE_PRE_LAUNCH   = 0x0200,  /* payload: MMCOInstanceInfo* */
    MMCO_HOOK_INSTANCE_POST_LAUNCH  = 0x0201,  /* payload: MMCOInstanceInfo* */
    MMCO_HOOK_INSTANCE_CREATED      = 0x0202,  /* payload: MMCOInstanceInfo* */
    MMCO_HOOK_INSTANCE_REMOVED      = 0x0203,  /* payload: MMCOInstanceInfo* */

    /* Settings */
    MMCO_HOOK_SETTINGS_CHANGED      = 0x0300,  /* payload: MMCOSettingChange* */

    /* Content / mod management */
    MMCO_HOOK_CONTENT_PRE_DOWNLOAD  = 0x0400,  /* payload: MMCOContentEvent* */
    MMCO_HOOK_CONTENT_POST_DOWNLOAD = 0x0401,  /* payload: MMCOContentEvent* */

    /* Network */
    MMCO_HOOK_NETWORK_PRE_REQUEST   = 0x0500,  /* payload: MMCONetworkEvent* */
    MMCO_HOOK_NETWORK_POST_REQUEST  = 0x0501,  /* payload: MMCONetworkEvent* */

    /* UI extension points */
    MMCO_HOOK_UI_MAIN_READY         = 0x0600,  /* payload: nullptr */
    MMCO_HOOK_UI_CONTEXT_MENU       = 0x0601,  /* payload: MMCOMenuEvent* */
    MMCO_HOOK_UI_INSTANCE_PAGES     = 0x0602,  /* payload: MMCOInstancePagesEvent* */
};

/*
 * Hook callback signature.
 *   module_handle: opaque handle identifying the calling module
 *   hook_id:       which hook fired
 *   payload:       hook-specific data (may be nullptr)
 *   user_data:     arbitrary pointer the plugin passed at registration
 *
 * Return 0 to allow the chain to continue, non-zero to signal cancellation
 * (only effective for "pre" hooks).
 */
typedef int (*MMCOHookCallback)(void* module_handle,
                                uint32_t hook_id,
                                void* payload,
                                void* user_data);

/* Payload structures for hooks */

struct MMCOInstanceInfo {
    const char* instance_id;
    const char* instance_name;
    const char* instance_path;
    const char* minecraft_version;
};

struct MMCOSettingChange {
    const char* key;
    const char* old_value;
    const char* new_value;
};

struct MMCOContentEvent {
    const char* instance_id;
    const char* file_name;
    const char* url;
    const char* target_path;
};

struct MMCONetworkEvent {
    const char* url;
    const char* method;     /* "GET", "POST", etc. */
    int         status_code; /* 0 for pre-request */
};

struct MMCOMenuEvent {
    const char* context;     /* "main", "instance", etc. */
    void*       menu_handle; /* Opaque handle for mmco_ui_add_menu_item() */
};

/*
 * Payload for MMCO_HOOK_UI_INSTANCE_PAGES.
 * Plugins receive this when instance page dialogs are being built.
 * They can add pages via the page_list_handle (opaque pointer to
 * QList<BasePage*>*).
 */
struct MMCOInstancePagesEvent {
    const char* instance_id;
    const char* instance_name;
    const char* instance_path;
    void*       page_list_handle;  /* Opaque: QList<BasePage*>* */
    void*       instance_handle;   /* Opaque: InstancePtr raw pointer */
};
