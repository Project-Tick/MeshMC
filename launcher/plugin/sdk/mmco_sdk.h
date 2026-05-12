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
 * MeshMC Plugin SDK — Single-include header for .mmco module development.
 *
 * USAGE:
 *   1. #include "mmco_sdk.h" in your plugin source (this is the ONLY
 *      include you need — Qt and MeshMC types are provided automatically)
 *   2. Define mmco_module_info, mmco_init(), and mmco_unload()
 *   3. Compile as a shared library with the .mmco extension
 *   4. Place the .mmco file in one of the search paths:
 *        - <app_dir>/mmcmodules/       (in-tree / portable)
 *        - ~/.local/lib/mmcmodules/    (user-local on Linux)
 *        - /usr/local/lib/mmcmodules/  (system-wide)
 *        - /usr/lib/mmcmodules/        (distro packages)
 *
 * Plugins MUST NOT:
 *   - Directly #include Qt or MeshMC headers (use this SDK header instead)
 *   - Fork or exec processes
 *
 * Plugins CAN:
 *   - Use Qt types and widgets (provided through this header)
 *   - Use MeshMC types (BasePage, BaseInstance, etc.)
 *   - Register for hooks to observe/modify launcher behaviour
 *   - Read/write their own settings (namespaced automatically)
 *   - Query and manage instances fully (launch, stop, mods, worlds, etc.)
 *   - Query accounts and Java installations
 *   - Make HTTP requests through the provided API
 *   - Build UI pages (as QWidget + BasePage subclasses)
 *   - Show dialogs (file chooser, input, confirm, message)
 *   - Create/extract zip archives (via MMCZip)
 *   - Perform filesystem operations
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QPointer>
#include <QTimer>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolBar>
#include <QAction>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QJsonDocument>
#include <QLocale>
#include "Application.h"
#include "BaseInstance.h"
#include "InstanceList.h"
#include "MMCZip.h"
#include "icons/IconList.h"
#include "minecraft/MinecraftInstance.h"
#include "ui/pages/BasePage.h"

#define MMCO_MAGIC 0x4D4D434F
#define MMCO_VERSION "8.0.0"
#define MMCO_ABI_VERSION 2
#define MMCO_EXTENSION ".mmco"
#define MMCO_FLAG_NONE 0x00000000
#define MMCO_TRAILER_MAGIC 0x53434D4D /* ASCII "MMCS" — see MMCOFormat.h */
#define MMCO_VERNUM                                                            \
	0x08000000L /* MMNNRRSM: major minor revision status modified */
#define MMCO_VER_MAJOR 8
#define MMCO_VER_MINOR 0
#define MMCO_VER_REVISION 0
#define MMCO_VER_STATUS 0 /* 0=devel, 1-E=beta, F=Release (DEPRECATED) */
#define MMCO_VER_STATUSH                                                       \
	0x0 /* Hex values: 0=devel, 1-9=beta, A-E=Release Candidate, F=Release */
#define MMCO_VER_MODIFIED 0 /* non-zero if modified externally from mmco */

/* Symbol visibility for .mmco shared libraries */
#if defined(_WIN32) || defined(__CYGWIN__)
#define MMCO_EXPORT __declspec(dllexport)
#else
#define MMCO_EXPORT __attribute__((visibility("default")))
#endif

/*
 * Optional dependency on another .mmco module.
 */
struct MMCODependency {
	const char* name;
	const char* min_version; /* nullptr or "" = any version */
	uint32_t optional;		 /* non-zero = optional dep */
};

struct MMCOModuleInfo {
	uint32_t magic;
	uint32_t abi_version;
	const char* name;
	const char* version;
	const char* author;
	const char* description;
	const char* license;
	uint32_t flags;
	const char* code_link;

	/* Icon set, dependency table, GPG signing key — see MMCOFormat.h */
	const char* icon_set_resource;
	const MMCODependency* dependencies;
	uint32_t dependency_count;
	const char* signing_key_id;
};

enum MMCOHookId : uint32_t {
	MMCO_HOOK_APP_INITIALIZED = 0x0100,
	MMCO_HOOK_APP_SHUTDOWN = 0x0101,
	MMCO_HOOK_INSTANCE_PRE_LAUNCH = 0x0200,
	MMCO_HOOK_INSTANCE_POST_LAUNCH = 0x0201,
	MMCO_HOOK_INSTANCE_CREATED = 0x0202,
	MMCO_HOOK_INSTANCE_REMOVED = 0x0203,
	MMCO_HOOK_SETTINGS_CHANGED = 0x0300,
	MMCO_HOOK_CONTENT_PRE_DOWNLOAD = 0x0400,
	MMCO_HOOK_CONTENT_POST_DOWNLOAD = 0x0401,
	MMCO_HOOK_NETWORK_PRE_REQUEST = 0x0500,
	MMCO_HOOK_NETWORK_POST_REQUEST = 0x0501,
	MMCO_HOOK_UI_MAIN_READY = 0x0600,
	MMCO_HOOK_UI_CONTEXT_MENU = 0x0601,
	MMCO_HOOK_UI_INSTANCE_PAGES = 0x0602,
	MMCO_HOOK_UI_GLOBAL_SETTINGS_PAGES = 0x0603,

	/* News */
	MMCO_HOOK_NEWS_UPDATED = 0x0700, /* payload: nullptr */
};

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
	const char* method;
	int status_code;
};

struct MMCOMenuEvent {
	const char* context;
	void* menu_handle;
};

struct MMCOInstancePagesEvent {
	const char* instance_id;
	const char* instance_name;
	const char* instance_path;
	void* page_list_handle;
	void* instance_handle;
};

/*
 * Payload for MMCO_HOOK_UI_MAIN_READY (ABI 2+).
 *
 * Direct opaque handles to the main window's long-lived widgets, so
 * plugins can wire callbacks without scanning qApp->allWidgets().
 *
 *   main_window       — QMainWindow*  (MainWindow*)
 *   news_toolbar      — QToolBar*
 *   more_news_action  — QAction*       (the "More News..." action)
 *   news_label_button — QToolButton*   (the clickable news headline)
 *
 * All handles are owned by MeshMC and stay valid for the lifetime of the
 * main window. Plugins must NOT delete or take ownership of them.
 */
struct MMCOUiMainReadyPayload {
	void* main_window;
	void* news_toolbar;
	void* more_news_action;
	void* news_label_button;
};

struct MMCOGlobalSettingsPagesEvent {
	void* page_list_handle;
};

typedef int (*MMCOHookCallback)(void* module_handle, uint32_t hook_id,
								void* payload, void* user_data);

typedef void (*MMCOHttpCallback)(void* user_data, int status_code,
								 const void* response_body,
								 size_t response_size);
typedef void (*MMCOMenuActionCallback)(void* user_data);
typedef void (*MMCODirEntryCallback)(void* user_data, const char* entry_name,
									 int is_dir);
typedef void (*MMCOButtonCallback)(void* user_data);
typedef void (*MMCOTreeSelectionCallback)(void* user_data, int row);

/* S19 — System Tray activation reason
 *   0=Unknown, 1=Trigger (single click), 2=DoubleClick,
 *   3=MiddleClick, 4=Context (right click).
 * Mirrors QSystemTrayIcon::ActivationReason 1..4. */
typedef void (*MMCOTrayActivationCallback)(void* user_data, int reason);

/* S20 — Main-window close-event filter.
 *   Return 0 to let the close proceed, 1 to swallow it (hide-to-tray). */
typedef int (*MMCOMainWindowCloseCallback)(void* user_data);

struct MMCOContext {
	/* ABI guard */
	uint32_t struct_size;
	uint32_t abi_version;
	void* module_handle;

	/* S1 — Logging */
	void (*log_info)(void* mh, const char* msg);
	void (*log_warn)(void* mh, const char* msg);
	void (*log_error)(void* mh, const char* msg);
	void (*log_debug)(void* mh, const char* msg);

	/* S2 — Hooks */
	int (*hook_register)(void* mh, uint32_t hook_id, MMCOHookCallback callback,
						 void* user_data);
	int (*hook_unregister)(void* mh, uint32_t hook_id,
						   MMCOHookCallback callback);

	/* S3 — Settings */
	const char* (*setting_get)(void* mh, const char* key);
	int (*setting_set)(void* mh, const char* key, const char* value);

	/* S4 — Instance Management */
	int (*instance_count)(void* mh);
	const char* (*instance_get_id)(void* mh, int index);
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
	int (*instance_is_running)(void* mh, const char* id);
	int (*instance_can_launch)(void* mh, const char* id);
	int (*instance_has_crashed)(void* mh, const char* id);
	int (*instance_has_update)(void* mh, const char* id);
	int64_t (*instance_get_total_play_time)(void* mh, const char* id);
	int64_t (*instance_get_last_play_time)(void* mh, const char* id);
	int64_t (*instance_get_last_launch)(void* mh, const char* id);
	int (*instance_launch)(void* mh, const char* id, int online);
	int (*instance_kill)(void* mh, const char* id);
	int (*instance_delete)(void* mh, const char* id);
	const char* (*instance_get_group)(void* mh, const char* id);
	int (*instance_set_group)(void* mh, const char* id, const char* group);
	int (*instance_group_count)(void* mh);
	const char* (*instance_group_at)(void* mh, int index);
	int (*instance_component_count)(void* mh, const char* id);
	const char* (*instance_component_get_uid)(void* mh, const char* id,
											  int index);
	const char* (*instance_component_get_name)(void* mh, const char* id,
											   int index);
	const char* (*instance_component_get_version)(void* mh, const char* id,
												  int index);
	const char* (*instance_get_mc_version)(void* mh, const char* id);
	const char* (*instance_get_jar_mods_dir)(void* mh, const char* id);
	const char* (*instance_get_resource_packs_dir)(void* mh, const char* id);
	const char* (*instance_get_texture_packs_dir)(void* mh, const char* id);
	const char* (*instance_get_shader_packs_dir)(void* mh, const char* id);
	const char* (*instance_get_worlds_dir)(void* mh, const char* id);

	/* S5 — Mod Management (type:
	 * "loader","core","resourcepack","texturepack","shaderpack") */
	int (*mod_count)(void* mh, const char* instance_id, const char* type);
	const char* (*mod_get_name)(void* mh, const char* iid, const char* type,
								int index);
	const char* (*mod_get_version)(void* mh, const char* iid, const char* type,
								   int index);
	const char* (*mod_get_filename)(void* mh, const char* iid, const char* type,
									int index);
	const char* (*mod_get_description)(void* mh, const char* iid,
									   const char* type, int index);
	int (*mod_is_enabled)(void* mh, const char* iid, const char* type,
						  int index);
	int (*mod_set_enabled)(void* mh, const char* iid, const char* type,
						   int index, int enabled);
	int (*mod_remove)(void* mh, const char* iid, const char* type, int index);
	int (*mod_install)(void* mh, const char* iid, const char* type,
					   const char* filepath);
	int (*mod_refresh)(void* mh, const char* iid, const char* type);

	/* S6 — World Management */
	int (*world_count)(void* mh, const char* instance_id);
	const char* (*world_get_name)(void* mh, const char* iid, int index);
	const char* (*world_get_folder)(void* mh, const char* iid, int index);
	int64_t (*world_get_seed)(void* mh, const char* iid, int index);
	int (*world_get_game_type)(void* mh, const char* iid, int index);
	int64_t (*world_get_last_played)(void* mh, const char* iid, int index);
	int (*world_delete)(void* mh, const char* iid, int index);
	int (*world_rename)(void* mh, const char* iid, int index, const char* name);
	int (*world_install)(void* mh, const char* iid, const char* filepath);
	int (*world_refresh)(void* mh, const char* iid);

	/* S7 — Account Management (read-only) */
	int (*account_count)(void* mh);
	const char* (*account_get_profile_name)(void* mh, int index);
	const char* (*account_get_profile_id)(void* mh, int index);
	const char* (*account_get_type)(void* mh, int index);
	int (*account_get_state)(void* mh, int index);
	int (*account_is_active)(void* mh, int index);
	int (*account_get_default_index)(void* mh);

	/* S8 — Java Management (read-only) */
	int (*java_count)(void* mh);
	const char* (*java_get_version)(void* mh, int index);
	const char* (*java_get_arch)(void* mh, int index);
	const char* (*java_get_path)(void* mh, int index);
	int (*java_is_recommended)(void* mh, int index);
	const char* (*instance_get_java_version)(void* mh, const char* id);

	/* S9 — Filesystem */
	const char* (*fs_plugin_data_dir)(void* mh);
	int64_t (*fs_read)(void* mh, const char* rel_path, void* buf, size_t sz);
	int (*fs_write)(void* mh, const char* rel_path, const void* data,
					size_t sz);
	int (*fs_exists)(void* mh, const char* rel_path);
	int (*fs_mkdir)(void* mh, const char* abs_path);
	int (*fs_exists_abs)(void* mh, const char* abs_path);
	int (*fs_remove)(void* mh, const char* abs_path);
	int (*fs_copy_file)(void* mh, const char* src, const char* dst);
	int64_t (*fs_file_size)(void* mh, const char* abs_path);
	int (*fs_list_dir)(void* mh, const char* abs_path, int type,
					   MMCODirEntryCallback callback, void* user_data);

	/* S10 — Zip / Archive */
	int (*zip_compress_dir)(void* mh, const char* zip_path,
							const char* dir_path);
	int (*zip_extract)(void* mh, const char* zip_path, const char* target_dir);

	/* S11 — Network */
	int (*http_get)(void* mh, const char* url, MMCOHttpCallback callback,
					void* user_data);
	int (*http_post)(void* mh, const char* url, const void* body,
					 size_t body_size, const char* content_type,
					 MMCOHttpCallback callback, void* user_data);

	/* S12 — UI: Dialogs */
	void (*ui_show_message)(void* mh, int type, const char* title,
							const char* msg);
	int (*ui_add_menu_item)(void* mh, void* menu_handle, const char* label,
							const char* icon_name, MMCOMenuActionCallback cb,
							void* ud);
	const char* (*ui_file_open_dialog)(void* mh, const char* title,
									   const char* filter);
	const char* (*ui_file_save_dialog)(void* mh, const char* title,
									   const char* default_name,
									   const char* filter);
	const char* (*ui_input_dialog)(void* mh, const char* title,
								   const char* prompt,
								   const char* default_value);
	int (*ui_confirm_dialog)(void* mh, const char* title, const char* message);
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
										  MMCOMenuActionCallback cb, void* ud);

	/* S13 — UI: Page Builder */
	void* (*ui_page_create)(void* mh, const char* page_id,
							const char* display_name, const char* icon_name);
	int (*ui_page_add_to_list)(void* mh, void* page, void* page_list_handle);
	void* (*ui_layout_create)(void* mh, void* parent, int type);
	int (*ui_layout_add_widget)(void* mh, void* layout, void* widget);
	int (*ui_layout_add_layout)(void* mh, void* parent_layout,
								void* child_layout);
	int (*ui_layout_add_spacer)(void* mh, void* layout, int horizontal);
	int (*ui_page_set_layout)(void* mh, void* page, void* layout);
	void* (*ui_button_create)(void* mh, void* parent, const char* text,
							  const char* icon_name, MMCOButtonCallback cb,
							  void* ud);
	int (*ui_button_set_enabled)(void* mh, void* button, int enabled);
	int (*ui_button_set_text)(void* mh, void* button, const char* text);
	void* (*ui_label_create)(void* mh, void* parent, const char* text);
	int (*ui_label_set_text)(void* mh, void* label, const char* text);
	void* (*ui_tree_create)(void* mh, void* parent, const char** column_names,
							int column_count, MMCOTreeSelectionCallback cb,
							void* ud);
	int (*ui_tree_clear)(void* mh, void* tree);
	int (*ui_tree_add_row)(void* mh, void* tree, const char** values,
						   int col_count);
	int (*ui_tree_selected_row)(void* mh, void* tree);
	int (*ui_tree_set_row_data)(void* mh, void* tree, int row, int64_t data);
	int64_t (*ui_tree_get_row_data)(void* mh, void* tree, int row);
	int (*ui_tree_row_count)(void* mh, void* tree);

	/* S14 — Utility */
	const char* (*get_app_version)(void* mh);
	const char* (*get_app_name)(void* mh);
	int64_t (*get_timestamp)(void* mh);

	/* S15 — Launch Modifiers (only valid inside INSTANCE_PRE_LAUNCH hooks) */
	int (*launch_set_env)(void* mh, const char* key, const char* value);
	int (*launch_prepend_wrapper)(void* mh, const char* wrapper_cmd);

	/* S16 — Application Settings (read-only global settings) */
	const char* (*app_setting_get)(void* mh, const char* key);

	/* S17 — News API */
	int (*news_get_entry_count)(void* mh);
	const char* (*news_get_entry_title)(void* mh, int index);
	const char* (*news_get_entry_link)(void* mh, int index);
	const char* (*news_get_entry_content)(void* mh, int index);
	const char* (*news_get_entry_author)(void* mh, int index);
	const char* (*news_get_entry_date)(void* mh, int index);
	int (*news_get_entry_feed_index)(void* mh, int index);
	int (*news_add_feed_url)(void* mh, const char* url);
	int (*news_get_feed_count)(void* mh);
	const char* (*news_get_feed_url)(void* mh, int index);
	int (*news_reload)(void* mh);

	/* S18 — Plugin icon set */

	/* Resolve a logical icon name from the calling module's bundled
	 * icon set into a Qt resource path (e.g. ":/plugins/MyPlugin/foo").
	 * Returns nullptr if the module did not declare icon_set_resource
	 * or the icon does not exist. The returned pointer is valid until
	 * the next API call on the same module. */
	const char* (*ui_plugin_icon)(void* mh, const char* name);

	/* S19 — System Tray (additive; no ABI bump) */
	void* (*tray_create)(void* mh, const char* icon_name, const char* tooltip);
	int (*tray_destroy)(void* mh, void* tray_handle);
	int (*tray_is_available)(void* mh);
	int (*tray_set_icon)(void* mh, void* tray_handle, const char* icon_name);
	int (*tray_set_tooltip)(void* mh, void* tray_handle, const char* tooltip);
	int (*tray_set_visible)(void* mh, void* tray_handle, int visible);
	/* icon_type: 0=None, 1=Info, 2=Warning, 3=Critical.
	 * tray_handle may be nullptr — a transient hidden tray is used. */
	int (*tray_show_message)(void* mh, void* tray_handle, const char* title,
							 const char* message, int icon_type, int msecs);
	int (*tray_set_menu)(void* mh, void* tray_handle, void* menu_handle);
	int (*tray_set_activation_cb)(void* mh, void* tray_handle,
								  MMCOTrayActivationCallback cb, void* ud);
	void* (*tray_menu_create)(void* mh);
	int (*tray_menu_destroy)(void* mh, void* menu_handle);
	int (*tray_menu_clear)(void* mh, void* menu_handle);
	int (*tray_menu_add_separator)(void* mh, void* menu_handle);
	void* (*tray_menu_add_action)(void* mh, void* menu_handle,
								  const char* label, const char* icon_name,
								  MMCOMenuActionCallback cb, void* ud);
	int (*tray_menu_action_set_enabled)(void* mh, void* action_handle,
										int enabled);
	int (*tray_menu_action_set_text)(void* mh, void* action_handle,
									 const char* text);
	/* Create a nested submenu under `parent_menu`. The returned handle
	 * is a QMenu* — pass it to the other tray_menu_* helpers. The
	 * submenu is parented to the parent menu and freed automatically
	 * when the parent menu is destroyed. */
	void* (*tray_menu_add_submenu)(void* mh, void* parent_menu,
								   const char* label, const char* icon_name);

	/* S20 — Main window helpers (additive) */
	int (*main_window_install_close_filter)(void* mh,
											MMCOMainWindowCloseCallback cb,
											void* user_data);
	int (*main_window_show)(void* mh);
	int (*main_window_hide)(void* mh);
	int (*main_window_is_visible)(void* mh);
};

/*
 * MMCO_DEFINE_MODULE — emit the mmco_module_info struct.
 *
 * Accepts 5 to 7 positional arguments:
 *   1. name
 *   2. version
 *   3. author
 *   4. description
 *   5. license            (SPDX identifier)
 *   6. code_link          (optional, source URL — pass nullptr to skip)
 *   7. icon_set_resource  (optional, logical icon-set name — see
 *                          MMCOModuleInfo::icon_set_resource)
 *
 * Modules that need to declare dependencies or a signing key id should
 * use MMCO_DEFINE_MODULE_EX() below and supply them explicitly.
 */

#define MMCO_DEFINE_MODULE_7(mod_name, mod_version, mod_author, mod_desc,      \
							 mod_license, mod_code_link, mod_icon_set)         \
	extern "C" MMCO_EXPORT MMCOModuleInfo mmco_module_info = {                 \
		MMCO_MAGIC,	   MMCO_ABI_VERSION, mod_name,	  mod_version,             \
		mod_author,	   mod_desc,		 mod_license, MMCO_FLAG_NONE,          \
		mod_code_link, mod_icon_set,	 nullptr,	  0u,                      \
		nullptr}

#define MMCO_DEFINE_MODULE_6(mod_name, mod_version, mod_author, mod_desc,      \
							 mod_license, mod_code_link)                       \
	extern "C" MMCO_EXPORT MMCOModuleInfo mmco_module_info = {                 \
		MMCO_MAGIC,	   MMCO_ABI_VERSION, mod_name,	  mod_version,             \
		mod_author,	   mod_desc,		 mod_license, MMCO_FLAG_NONE,          \
		mod_code_link, nullptr,			 nullptr,	  0u,                      \
		nullptr}

#define MMCO_DEFINE_MODULE_5(mod_name, mod_version, mod_author, mod_desc,      \
							 mod_license)                                      \
	extern "C" MMCO_EXPORT MMCOModuleInfo mmco_module_info = {                 \
		MMCO_MAGIC, MMCO_ABI_VERSION, mod_name,	   mod_version,                \
		mod_author, mod_desc,		  mod_license, MMCO_FLAG_NONE,             \
		nullptr,	nullptr,		  nullptr,	   0u,                         \
		nullptr}

/*
 * Full-fledged variant for modules that need to declare an icon set,
 * a dependency table, and / or a signing-key id explicitly.
 *
 *   MMCO_DEFINE_MODULE_EX("MyMod", "1.0", "Me", "desc", "MIT",
 *                         "https://example.org", "my_icons",
 *                         my_deps_array, 2,
 *                         "ABCD1234...");
 *
 * Pass nullptr / 0 for any field you don't use.
 */
#define MMCO_DEFINE_MODULE_EX(mod_name, mod_version, mod_author, mod_desc,     \
							  mod_license, mod_code_link, mod_icon_set,        \
							  mod_deps_ptr, mod_deps_count, mod_signing_key)   \
	extern "C" MMCO_EXPORT MMCOModuleInfo mmco_module_info = {                 \
		MMCO_MAGIC,		MMCO_ABI_VERSION, mod_name,		mod_version,           \
		mod_author,		mod_desc,		  mod_license,	MMCO_FLAG_NONE,        \
		mod_code_link,	mod_icon_set,	  mod_deps_ptr, mod_deps_count,        \
		mod_signing_key}

#define MMCO_EXPAND(x) x
#define MMCO_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define MMCO_DEFINE_MODULE(...)                                                \
	MMCO_EXPAND(MMCO_GET_MACRO(__VA_ARGS__, MMCO_DEFINE_MODULE_7,              \
							   MMCO_DEFINE_MODULE_6,                           \
							   MMCO_DEFINE_MODULE_5)(__VA_ARGS__))

#define MMCO_LOG(ctx, msg) (ctx)->log_info((ctx)->module_handle, (msg))
#define MMCO_WARN(ctx, msg) (ctx)->log_warn((ctx)->module_handle, (msg))
#define MMCO_ERR(ctx, msg) (ctx)->log_error((ctx)->module_handle, (msg))
#define MMCO_DBG(ctx, msg) (ctx)->log_debug((ctx)->module_handle, (msg))

/* Shorthand to call API functions with module handle */
#define MMCO_MH (ctx->module_handle)
