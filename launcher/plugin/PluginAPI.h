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
 * MMCOContext — the runtime context passed to mmco_init().
 *
 * This is the ONLY interface plugins have into MeshMC. All interaction
 * goes through function pointers in this struct. Plugins MUST NOT call
 * Qt, MeshMC internals, or any symbol not exposed here.
 *
 * The context is valid for the lifetime of the plugin (until mmco_unload()
 * returns). Pointers obtained through the API (e.g. strings from getters)
 * are valid until the next API call on the same module unless documented
 * otherwise.
 */

#pragma once

#include "plugin/PluginHooks.h"
#include <cstdint>
#include <cstddef>

/*

 */

/* Forward-declare callback typedefs used by the context */
typedef void (*MMCOHttpCallback)(void* user_data, int status_code,
								 const void* response_body,
								 size_t response_size);
typedef void (*MMCOMenuActionCallback)(void* user_data);
typedef void (*MMCODirEntryCallback)(void* user_data, const char* entry_name,
									 int is_dir);

/* UI widget callback types */
typedef void (*MMCOButtonCallback)(void* user_data);
typedef void (*MMCOTreeSelectionCallback)(void* user_data, int row);

struct MMCOContext {
	/* ABI guard */
	uint32_t struct_size; /* sizeof(MMCOContext) for forward compat */
	uint32_t abi_version; /* MMCO_ABI_VERSION */
	void* module_handle;  /* Opaque handle to identify this module */

	void (*log_info)(void* mh, const char* msg);
	void (*log_warn)(void* mh, const char* msg);
	void (*log_error)(void* mh, const char* msg);
	void (*log_debug)(void* mh, const char* msg);

	int (*hook_register)(void* mh, uint32_t hook_id, MMCOHookCallback callback,
						 void* user_data);
	int (*hook_unregister)(void* mh, uint32_t hook_id,
						   MMCOHookCallback callback);

	const char* (*setting_get)(void* mh, const char* key);
	int (*setting_set)(void* mh, const char* key, const char* value);

	/* Enumeration */
	int (*instance_count)(void* mh);
	const char* (*instance_get_id)(void* mh, int index);

	/* Properties (by instance ID) */
	const char* (*instance_get_name)(void* mh, const char* id);
	int (*instance_set_name)(void* mh, const char* id, const char* name);
	const char* (*instance_get_path)(void* mh, const char* id);
	const char* (*instance_get_game_root)(void* mh, const char* id);
	const char* (*instance_get_mods_root)(void* mh, const char* id);
	const char* (*instance_get_icon_key)(void* mh, const char* id);
	int (*instance_set_icon_key)(void* mh, const char* id, const char* key);
	const char* (*instance_get_type)(void* mh, const char* id);
	const char* (*instance_get_notes)(void* mh, const char* id);
	int (*instance_set_notes)(void* mh, const char* id, const char* notes);

	/* State queries */
	int (*instance_is_running)(void* mh, const char* id);
	int (*instance_can_launch)(void* mh, const char* id);
	int (*instance_has_crashed)(void* mh, const char* id);
	int (*instance_has_update)(void* mh, const char* id);
	int64_t (*instance_get_total_play_time)(void* mh, const char* id);
	int64_t (*instance_get_last_play_time)(void* mh, const char* id);
	int64_t (*instance_get_last_launch)(void* mh, const char* id);

	/* Actions */
	int (*instance_launch)(void* mh, const char* id, int online);
	int (*instance_kill)(void* mh, const char* id);
	int (*instance_delete)(void* mh, const char* id);

	/* Groups */
	const char* (*instance_get_group)(void* mh, const char* id);
	int (*instance_set_group)(void* mh, const char* id, const char* group);
	int (*instance_group_count)(void* mh);
	const char* (*instance_group_at)(void* mh, int index);

	/* Pack Profile / Components */
	int (*instance_component_count)(void* mh, const char* id);
	const char* (*instance_component_get_uid)(void* mh, const char* id,
											  int index);
	const char* (*instance_component_get_name)(void* mh, const char* id,
											   int index);
	const char* (*instance_component_get_version)(void* mh, const char* id,
												  int index);
	const char* (*instance_get_mc_version)(void* mh, const char* id);

	/* Instance directories */
	const char* (*instance_get_jar_mods_dir)(void* mh, const char* id);
	const char* (*instance_get_resource_packs_dir)(void* mh, const char* id);
	const char* (*instance_get_texture_packs_dir)(void* mh, const char* id);
	const char* (*instance_get_shader_packs_dir)(void* mh, const char* id);
	const char* (*instance_get_worlds_dir)(void* mh, const char* id);

	int (*mod_count)(void* mh, const char* instance_id, const char* type);
	const char* (*mod_get_name)(void* mh, const char* instance_id,
								const char* type, int index);
	const char* (*mod_get_version)(void* mh, const char* instance_id,
								   const char* type, int index);
	const char* (*mod_get_filename)(void* mh, const char* instance_id,
									const char* type, int index);
	const char* (*mod_get_description)(void* mh, const char* instance_id,
									   const char* type, int index);
	int (*mod_is_enabled)(void* mh, const char* instance_id, const char* type,
						  int index);
	int (*mod_set_enabled)(void* mh, const char* instance_id, const char* type,
						   int index, int enabled);
	int (*mod_remove)(void* mh, const char* instance_id, const char* type,
					  int index);
	int (*mod_install)(void* mh, const char* instance_id, const char* type,
					   const char* filepath);
	int (*mod_refresh)(void* mh, const char* instance_id, const char* type);

	int (*world_count)(void* mh, const char* instance_id);
	const char* (*world_get_name)(void* mh, const char* instance_id, int index);
	const char* (*world_get_folder)(void* mh, const char* instance_id,
									int index);
	int64_t (*world_get_seed)(void* mh, const char* instance_id, int index);
	int (*world_get_game_type)(void* mh, const char* instance_id, int index);
	int64_t (*world_get_last_played)(void* mh, const char* instance_id,
									 int index);
	int (*world_delete)(void* mh, const char* instance_id, int index);
	int (*world_rename)(void* mh, const char* instance_id, int index,
						const char* new_name);
	int (*world_install)(void* mh, const char* instance_id,
						 const char* filepath);
	int (*world_refresh)(void* mh, const char* instance_id);

	int (*account_count)(void* mh);
	const char* (*account_get_profile_name)(void* mh, int index);
	const char* (*account_get_profile_id)(void* mh, int index);
	const char* (*account_get_type)(void* mh, int index);
	int (*account_get_state)(void* mh, int index);
	int (*account_is_active)(void* mh, int index);
	int (*account_get_default_index)(void* mh);

	int (*java_count)(void* mh);
	const char* (*java_get_version)(void* mh, int index);
	const char* (*java_get_arch)(void* mh, int index);
	const char* (*java_get_path)(void* mh, int index);
	int (*java_is_recommended)(void* mh, int index);
	const char* (*instance_get_java_version)(void* mh, const char* id);

	/* Sandboxed (relative to plugin data dir) */
	const char* (*fs_plugin_data_dir)(void* mh);
	int64_t (*fs_read)(void* mh, const char* rel_path, void* buf, size_t sz);
	int (*fs_write)(void* mh, const char* rel_path, const void* data,
					size_t sz);
	int (*fs_exists)(void* mh, const char* rel_path);

	/* Absolute path operations */
	int (*fs_mkdir)(void* mh, const char* abs_path);
	int (*fs_exists_abs)(void* mh, const char* abs_path);
	int (*fs_remove)(void* mh, const char* abs_path);
	int (*fs_copy_file)(void* mh, const char* src, const char* dst);
	int64_t (*fs_file_size)(void* mh, const char* abs_path);
	int (*fs_list_dir)(void* mh, const char* abs_path, int type,
					   MMCODirEntryCallback callback, void* user_data);

	int (*zip_compress_dir)(void* mh, const char* zip_path,
							const char* dir_path);
	int (*zip_extract)(void* mh, const char* zip_path, const char* target_dir);

	int (*http_get)(void* mh, const char* url, MMCOHttpCallback callback,
					void* user_data);
	int (*http_post)(void* mh, const char* url, const void* body,
					 size_t body_size, const char* content_type,
					 MMCOHttpCallback callback, void* user_data);

	void (*ui_show_message)(void* mh, int type, const char* title,
							const char* msg);
	int (*ui_add_menu_item)(void* mh, void* menu_handle, const char* label,
							const char* icon_name,
							MMCOMenuActionCallback callback, void* user_data);

	/* Returns the user-chosen path, or nullptr on cancel */
	const char* (*ui_file_open_dialog)(void* mh, const char* title,
									   const char* filter);
	const char* (*ui_file_save_dialog)(void* mh, const char* title,
									   const char* default_name,
									   const char* filter);
	/* Returns user text, or nullptr on cancel */
	const char* (*ui_input_dialog)(void* mh, const char* title,
								   const char* prompt,
								   const char* default_value);
	/* Returns 1=Yes, 0=No */
	int (*ui_confirm_dialog)(void* mh, const char* title, const char* message);

	/* Register an action in the main window's instance toolbar.
	 * text/tooltip are shown in the toolbar; icon_name refers to a themed icon;
	 * page_id is the page opened via showInstanceWindow(). */
	int (*ui_register_instance_action)(void* mh, const char* text,
									   const char* tooltip,
									   const char* icon_name,
									   const char* page_id);

	/* Register a callback-based action in the instance toolbar.
	 * Unlike ui_register_instance_action which opens a settings page,
	 * this calls the given callback when the button is clicked. */
	int (*ui_register_instance_action_cb)(void* mh, const char* text,
										  const char* tooltip,
										  const char* icon_name,
										  void (*cb)(void* ud), void* ud);

	/* Create a page widget. Returns opaque page handle. */
	void* (*ui_page_create)(void* mh, const char* page_id,
							const char* display_name, const char* icon_name);

	/* Add the created page to the page list from a hook event. */
	int (*ui_page_add_to_list)(void* mh, void* page_handle,
							   void* page_list_handle);

	/* Layouts: type 0=vertical, 1=horizontal */
	void* (*ui_layout_create)(void* mh, void* parent, int type);
	int (*ui_layout_add_widget)(void* mh, void* layout, void* widget);
	int (*ui_layout_add_layout)(void* mh, void* parent_layout,
								void* child_layout);
	int (*ui_layout_add_spacer)(void* mh, void* layout, int horizontal);
	int (*ui_page_set_layout)(void* mh, void* page, void* layout);

	/* Button */
	void* (*ui_button_create)(void* mh, void* parent, const char* text,
							  const char* icon_name,
							  MMCOButtonCallback callback, void* user_data);
	int (*ui_button_set_enabled)(void* mh, void* button, int enabled);
	int (*ui_button_set_text)(void* mh, void* button, const char* text);

	/* Label */
	void* (*ui_label_create)(void* mh, void* parent, const char* text);
	int (*ui_label_set_text)(void* mh, void* label, const char* text);

	/* Tree widget (table-like list with columns) */
	void* (*ui_tree_create)(void* mh, void* parent, const char** column_names,
							int column_count,
							MMCOTreeSelectionCallback on_select,
							void* user_data);
	int (*ui_tree_clear)(void* mh, void* tree);
	int (*ui_tree_add_row)(void* mh, void* tree, const char** values,
						   int col_count);
	int (*ui_tree_selected_row)(void* mh, void* tree);
	int (*ui_tree_set_row_data)(void* mh, void* tree, int row, int64_t data);
	int64_t (*ui_tree_get_row_data)(void* mh, void* tree, int row);
	int (*ui_tree_row_count)(void* mh, void* tree);

	const char* (*get_app_version)(void* mh);
	const char* (*get_app_name)(void* mh);
	int64_t (*get_timestamp)(void* mh);

	/* S15 — Launch Modifiers (only valid inside INSTANCE_PRE_LAUNCH hooks) */

	/* Set an environment variable for the current launching instance.
	 * The variable is injected via qputenv() before launch and removed
	 * via qunsetenv() after the instance stops. */
	int (*launch_set_env)(void* mh, const char* key, const char* value);

	/* Prepend a wrapper command for the current launching instance.
	 * If the instance already has a wrapper, this command is prepended. */
	int (*launch_prepend_wrapper)(void* mh, const char* wrapper_cmd);

	/* S16 — Application Settings (read-only global settings) */

	/* Read a global application setting by key. Returns the value as a
	 * string, or nullptr if the key does not exist. The returned pointer
	 * is valid until the next API call on the same module. */
	const char* (*app_setting_get)(void* mh, const char* key);
	/* S17 — News API */

	/* Returns the total number of loaded news entries across all feeds.
	 * Returns -1 if the news system is not available. */
	int (*news_get_entry_count)(void* mh);

	/* Returns the title of the news entry at the given index.
	 * Returns nullptr if index is out of range. */
	const char* (*news_get_entry_title)(void* mh, int index);

	/* Returns the URL link of the news entry at the given index.
	 * Returns nullptr if index is out of range. */
	const char* (*news_get_entry_link)(void* mh, int index);

	/* Returns the HTML/text content of the news entry at the given index.
	 * Returns nullptr if index is out of range. */
	const char* (*news_get_entry_content)(void* mh, int index);

	/* Returns the author of the news entry at the given index.
	 * Returns nullptr if index is out of range. */
	const char* (*news_get_entry_author)(void* mh, int index);

	/* Returns the publication date of the news entry at the given index
	 * as an ISO 8601 string (e.g. "2026-05-08T14:30:00").
	 * Returns nullptr if index is out of range. */
	const char* (*news_get_entry_date)(void* mh, int index);

	/* Returns the feed URL index for the news entry at the given index.
	 * This identifies which feed the entry came from. */
	int (*news_get_entry_feed_index)(void* mh, int index);

	/* Registers an additional RSS feed URL to be fetched alongside the
	 * default feed. Returns 0 on success, -1 on failure.
	 * The URL is stored for the lifetime of the application. */
	int (*news_add_feed_url)(void* mh, const char* url);

	/* Returns the number of registered feed URLs (including the default). */
	int (*news_get_feed_count)(void* mh);

	/* Returns the feed URL at the given index.
	 * Returns nullptr if index is out of range. */
	const char* (*news_get_feed_url)(void* mh, int index);

	/* Triggers a reload of all news feeds. Non-blocking — results arrive
	 * via the MMCO_HOOK_NEWS_UPDATED hook. Returns 0 on success. */
	int (*news_reload)(void* mh);

	/* S18 — Plugin icon set (ABI 2+) */

	/* Resolve a logical icon name from the calling module's bundled
	 * icon set into a Qt resource path that can be passed to the
	 * ui_* widget creators above (which forward to QIcon::fromTheme()
	 * and QIcon::QIcon(QString)).
	 *
	 * Returns a string of the form ":/plugins/<icon_set>/<name>" or
	 * nullptr if the module did not declare an icon_set_resource or
	 * the icon does not exist. The pointer is valid until the next
	 * API call on the same module.
	 *
	 * Example:
	 *   const char* iconPath = ctx->ui_plugin_icon(MMCO_MH, "settings");
	 *   ctx->ui_button_create(MMCO_MH, parent, "Settings", iconPath,
	 *                         cb, ud);
	 */
	const char* (*ui_plugin_icon)(void* mh, const char* name);
};
