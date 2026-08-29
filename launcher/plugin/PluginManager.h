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

#include "plugin/PluginLoader.h"
#include "plugin/PluginMetadata.h"
#include "plugin/PluginHooks.h"
#include "plugin/PluginAPI.h"

#include "news/NewsEntry.h"

#include <QObject>
#include <QMutex>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QMultiMap>
#include <QSet>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <functional>

class NewsChecker;
class QAction;
class QEvent;
class QMenu;
class QSystemTrayIcon;
class QWidget;

/*
 * PluginManager — owns the plugin lifecycle and provides the bridge
 * between loaded .mmco modules and MeshMC internals.
 *
 * Responsibilities:
 *   - Discover and load modules via PluginLoader
 *   - Build MMCOContext for each module (populating function pointers)
 *   - Call mmco_init() / mmco_unload()
 *   - Dispatch hooks to registered callbacks
 *   - Implement the API functions that back MMCOContext
 */

class Application;

class PluginManager : public QObject
{
	Q_OBJECT

  public:
	explicit PluginManager(Application* app, QObject* parent = nullptr);
	~PluginManager() override;

	/*
	 * Discover and initialise all modules. Called once during
	 * Application startup, after subsystems are ready.
	 */
	void initializeAll();

	/*
	 * Gracefully shut down all modules (calls mmco_unload() in
	 * reverse load order, then dlclose).
	 */
	void shutdownAll();

	/*
	 * Dispatch a hook to all registered listeners.
	 * Returns true if any callback signalled cancellation (non-zero return).
	 */
	bool dispatchHook(uint32_t hook_id, void* payload = nullptr);

	/*
	 * Query loaded modules.
	 */
	const QVector<PluginMetadata>& modules() const
	{
		return m_modules;
	}
	int moduleCount() const
	{
		return m_modules.size();
	}

	/*
	 * Disable / enable management.
	 *
	 * The disabled-set is persisted in the application settings under
	 * the key "plugins.disabled" as a comma-separated list of module
	 * names (case-insensitive). Toggling a module DOES NOT load or
	 * unload anything at runtime — the change takes effect on the next
	 * launcher start. PluginsDialog calls these to mutate the set; the
	 * dialog warns the user that a restart is required.
	 */
	bool isModuleDisabled(const QString& moduleName) const;
	void setModuleDisabled(const QString& moduleName, bool disabled);
	QSet<QString> disabledModuleNames() const;

	/*
	 * ScratchString — the per-module scratch buffer that backs every
	 * `const char*` getter in the plugin API.
	 *
	 * The ABI contract is "the returned pointer stays valid until the
	 * next API call by the same module". This used to be a plain
	 * std::string member of ModuleRuntime, which is only correct while
	 * every call happens on the GUI thread: as soon as a hook callback
	 * runs on a worker thread, two threads write the same buffer and a
	 * reader can be handed a pointer that the other thread has already
	 * reallocated.
	 *
	 * The bytes therefore live in thread-local storage keyed by the
	 * ScratchString instance, which turns the contract into "valid
	 * until the next API call by the same module *on the same thread*"
	 * — what callers actually rely on, and safe for background hooks.
	 * Call sites are untouched: assignment, c_str(), data() and
	 * assign() all forward to the calling thread's own copy.
	 */
	class ScratchString
	{
	  public:
		ScratchString() = default;
		~ScratchString();

		/* Non-copyable: the identity of the object is the storage key. */
		ScratchString(const ScratchString&) = delete;
		ScratchString& operator=(const ScratchString&) = delete;

		ScratchString& operator=(std::string value);

		/* Binary-safe fill, used for skin/cape PNG blobs. */
		void assign(const char* bytes, size_t size);

		const char* c_str() const;
		const char* data() const;
	};

	/*
	 * ModuleRuntime — the opaque object behind module_handle.
	 * Lets static API callbacks find their way back to the manager.
	 * Public so helper functions in the .cpp can use it.
	 */
	struct ModuleRuntime {
		PluginManager* manager;
		int moduleIndex;
		/* Written once during initializeAll(), read-only afterwards —
		 * safe to hand out from any thread. */
		std::string dataDir;
		ScratchString tempString;
	};

	Application* m_app;

	/*
	 * QObject event filter — installed on the main window when at least
	 * one plugin has registered a close-event callback via
	 * main_window_install_close_filter(). Intercepts QCloseEvent and
	 * lets registered callbacks decide whether to accept or swallow it.
	 */
	bool eventFilter(QObject* watched, QEvent* event) override;

  signals:
	void moduleLoaded(const QString& name);
	void moduleUnloaded(const QString& name);
	void moduleError(const QString& name, const QString& error);

  private:
	/* Build an MMCOContext for a specific module */
	MMCOContext buildContext(PluginMetadata& meta);

	/* Ensure the plugin data directory exists */
	void ensurePluginDataDir(PluginMetadata& meta);

	/* ── API implementation (static callbacks wired into MMCOContext) ── */
	/* These are static so they can be used as C function pointers.
	 * They resolve the PluginManager instance via the module_handle,
	 * which is actually a pointer to a ModuleRuntime struct.
	 */

	struct HookRegistration {
		void* module_handle;
		MMCOHookCallback callback;
		void* user_data;
		/* MMCO_HOOK_FLAG_* — 0 for anything registered through the
		 * plain hook_register, which keeps the inline behaviour. */
		uint32_t flags;
	};

	/* Runs the background-flagged callbacks of one dispatch as a
	 * SequentialTask behind a ProgressDialog, one row per module.
	 * Returns true if one of them vetoed the operation. */
	bool runBackgroundHooks(uint32_t hook_id, void* payload,
							const QVector<HookRegistration>& regs);

	/* Static API trampolines — Section 1: Logging */
	static void api_log_info(void* mh, const char* msg);
	static void api_log_warn(void* mh, const char* msg);
	static void api_log_error(void* mh, const char* msg);
	static void api_log_debug(void* mh, const char* msg);

	/* Section 2: Hooks */
	static int api_hook_register(void* mh, uint32_t hook_id,
								 MMCOHookCallback cb, void* ud);
	static int api_hook_unregister(void* mh, uint32_t hook_id,
								   MMCOHookCallback cb);

	/* Section 32: Background hooks + progress reporting */
	static int api_hook_register_ex(void* mh, uint32_t hook_id,
									MMCOHookCallback cb, void* ud,
									uint32_t flags);
	static int api_progress_report(void* mh, const char* status,
								   const char* details, int64_t current,
								   int64_t total);

	/* Section 3: Settings */
	static const char* api_setting_get(void* mh, const char* key);
	static int api_setting_set(void* mh, const char* key, const char* value);

	/* Section 4: Instance Management */
	static int api_instance_count(void* mh);
	static const char* api_instance_get_id(void* mh, int index);
	static const char* api_instance_get_name(void* mh, const char* id);
	static int api_instance_set_name(void* mh, const char* id,
									 const char* name);
	static const char* api_instance_get_path(void* mh, const char* id);
	static const char* api_instance_get_game_root(void* mh, const char* id);
	static const char* api_instance_get_mods_root(void* mh, const char* id);
	static const char* api_instance_get_icon_key(void* mh, const char* id);
	static int api_instance_set_icon_key(void* mh, const char* id,
										 const char* key);
	static const char* api_instance_get_type(void* mh, const char* id);
	static const char* api_instance_get_notes(void* mh, const char* id);
	static int api_instance_set_notes(void* mh, const char* id,
									  const char* notes);
	static int api_instance_is_running(void* mh, const char* id);
	static int api_instance_can_launch(void* mh, const char* id);
	static int api_instance_has_crashed(void* mh, const char* id);
	static int api_instance_has_update(void* mh, const char* id);
	static int api_instance_set_update_available(void* mh, const char* id,
												 int value);
	static int api_instance_component_set_version(void* mh, const char* id,
												  const char* uid,
												  const char* version);
	static int api_http_get_with_headers(void* mh, const char* url,
										 const char* const* headers,
										 int header_count,
										 MMCOHttpCallback callback,
										 void* user_data);
	/* Section 31: Subprocess execution */
	static int api_process_run(void* mh, const char* program,
							   const char* const* args, int arg_count,
							   const char* working_dir, const char* stdin_data,
							   int stdin_size, char* out_buf, int out_buf_size,
							   int* out_exit_code, int timeout_ms);
	static int64_t api_instance_get_total_play_time(void* mh, const char* id);
	static int64_t api_instance_get_last_play_time(void* mh, const char* id);
	static int64_t api_instance_get_last_launch(void* mh, const char* id);
	static int api_instance_launch(void* mh, const char* id, int online);
	static int api_instance_kill(void* mh, const char* id);
	static int api_instance_delete(void* mh, const char* id);
	static const char* api_instance_get_group(void* mh, const char* id);
	static int api_instance_set_group(void* mh, const char* id,
									  const char* group);
	static int api_instance_group_count(void* mh);
	static const char* api_instance_group_at(void* mh, int index);
	static int api_instance_component_count(void* mh, const char* id);
	static const char* api_instance_component_get_uid(void* mh, const char* id,
													  int idx);
	static const char* api_instance_component_get_name(void* mh, const char* id,
													   int idx);
	static const char*
	api_instance_component_get_version(void* mh, const char* id, int idx);
	static const char* api_instance_get_mc_version(void* mh, const char* id);
	static const char* api_instance_get_jar_mods_dir(void* mh, const char* id);
	static const char* api_instance_get_resource_packs_dir(void* mh,
														   const char* id);
	static const char* api_instance_get_texture_packs_dir(void* mh,
														  const char* id);
	static const char* api_instance_get_shader_packs_dir(void* mh,
														 const char* id);
	static const char* api_instance_get_worlds_dir(void* mh, const char* id);

	/* Section 5: Mod Management */
	static int api_mod_count(void* mh, const char* inst, const char* type);
	static const char* api_mod_get_name(void* mh, const char* inst,
										const char* type, int idx);
	static const char* api_mod_get_version(void* mh, const char* inst,
										   const char* type, int idx);
	static const char* api_mod_get_filename(void* mh, const char* inst,
											const char* type, int idx);
	static const char* api_mod_get_description(void* mh, const char* inst,
											   const char* type, int idx);
	static int api_mod_is_enabled(void* mh, const char* inst, const char* type,
								  int idx);
	static int api_mod_set_enabled(void* mh, const char* inst, const char* type,
								   int idx, int e);
	static int api_mod_remove(void* mh, const char* inst, const char* type,
							  int idx);
	static int api_mod_install(void* mh, const char* inst, const char* type,
							   const char* path);
	static int api_mod_refresh(void* mh, const char* inst, const char* type);

	/* Section 6: World Management */
	static int api_world_count(void* mh, const char* inst);
	static const char* api_world_get_name(void* mh, const char* inst, int idx);
	static const char* api_world_get_folder(void* mh, const char* inst,
											int idx);
	static int64_t api_world_get_seed(void* mh, const char* inst, int idx);
	static int api_world_get_game_type(void* mh, const char* inst, int idx);
	static int64_t api_world_get_last_played(void* mh, const char* inst,
											 int idx);
	static int api_world_delete(void* mh, const char* inst, int idx);
	static int api_world_rename(void* mh, const char* inst, int idx,
								const char* name);
	static int api_world_install(void* mh, const char* inst, const char* path);
	static int api_world_refresh(void* mh, const char* inst);

	/* Section 7: Account Management */
	static int api_account_count(void* mh);
	static const char* api_account_get_profile_name(void* mh, int idx);
	static const char* api_account_get_profile_id(void* mh, int idx);
	static const char* api_account_get_type(void* mh, int idx);
	static int api_account_get_state(void* mh, int idx);
	static int api_account_is_active(void* mh, int idx);
	static int api_account_get_default_index(void* mh);

	/* Section 8: Java Management */
	static int api_java_count(void* mh);
	static const char* api_java_get_version(void* mh, int idx);
	static const char* api_java_get_arch(void* mh, int idx);
	static const char* api_java_get_path(void* mh, int idx);
	static int api_java_is_recommended(void* mh, int idx);
	static const char* api_instance_get_java_version(void* mh, const char* id);

	/* Section 9: Filesystem */
	static const char* api_fs_plugin_data_dir(void* mh);
	static int64_t api_fs_read(void* mh, const char* rel, void* buf, size_t sz);
	static int api_fs_write(void* mh, const char* rel, const void* data,
							size_t sz);
	static int api_fs_exists(void* mh, const char* rel);
	static int api_fs_mkdir(void* mh, const char* path);
	static int api_fs_exists_abs(void* mh, const char* path);
	static int api_fs_remove(void* mh, const char* path);
	static int api_fs_copy_file(void* mh, const char* src, const char* dst);
	static int64_t api_fs_file_size(void* mh, const char* path);
	static int api_fs_list_dir(void* mh, const char* path, int type,
							   MMCODirEntryCallback cb, void* ud);

	/* Section 10: Zip */
	static int api_zip_compress_dir(void* mh, const char* zip, const char* dir);
	static int api_zip_extract(void* mh, const char* zip, const char* target);

	/* Section 11: Network */
	static int api_http_get(void* mh, const char* url, MMCOHttpCallback cb,
							void* ud);
	static int api_http_post(void* mh, const char* url, const void* body,
							 size_t body_sz, const char* ct,
							 MMCOHttpCallback cb, void* ud);

	/* Section 12: UI Dialogs */
	static void api_ui_show_message(void* mh, int type, const char* title,
									const char* msg);
	static int api_ui_add_menu_item(void* mh, void* menu_handle,
									const char* label, const char* icon,
									MMCOMenuActionCallback cb, void* ud);
	static const char* api_ui_file_open_dialog(void* mh, const char* title,
											   const char* filter);
	static const char* api_ui_file_save_dialog(void* mh, const char* title,
											   const char* def,
											   const char* filter);
	static const char* api_ui_input_dialog(void* mh, const char* title,
										   const char* prompt, const char* def);
	static int api_ui_confirm_dialog(void* mh, const char* title,
									 const char* msg);
	static int api_ui_register_instance_action(void* mh, const char* text,
											   const char* tooltip,
											   const char* icon_name,
											   const char* page_id);
	static int api_ui_register_instance_action_cb(void* mh, const char* text,
												  const char* tooltip,
												  const char* icon_name,
												  void (*cb)(void* ud),
												  void* ud);

	/* Section 13: UI Page Builder */
	static void* api_ui_page_create(void* mh, const char* id, const char* name,
									const char* icon);
	static int api_ui_page_add_to_list(void* mh, void* page, void* list);
	static void* api_ui_layout_create(void* mh, void* parent, int type);
	static int api_ui_layout_add_widget(void* mh, void* layout, void* widget);
	static int api_ui_layout_add_layout(void* mh, void* parent, void* child);
	static int api_ui_layout_add_spacer(void* mh, void* layout, int horizontal);
	static int api_ui_page_set_layout(void* mh, void* page, void* layout);
	static void* api_ui_button_create(void* mh, void* parent, const char* text,
									  const char* icon, MMCOButtonCallback cb,
									  void* ud);
	static int api_ui_button_set_enabled(void* mh, void* btn, int enabled);
	static int api_ui_button_set_text(void* mh, void* btn, const char* text);
	static void* api_ui_label_create(void* mh, void* parent, const char* text);
	static int api_ui_label_set_text(void* mh, void* label, const char* text);
	static void* api_ui_tree_create(void* mh, void* parent, const char** cols,
									int ncols, MMCOTreeSelectionCallback cb,
									void* ud);
	static int api_ui_tree_clear(void* mh, void* tree);
	static int api_ui_tree_add_row(void* mh, void* tree, const char** vals,
								   int ncols);
	static int api_ui_tree_selected_row(void* mh, void* tree);
	static int api_ui_tree_set_row_data(void* mh, void* tree, int row,
										int64_t data);
	static int64_t api_ui_tree_get_row_data(void* mh, void* tree, int row);
	static int api_ui_tree_row_count(void* mh, void* tree);

	/* Section 14: Utility */
	static const char* api_get_app_version(void* mh);
	static const char* api_get_app_name(void* mh);
	static int64_t api_get_timestamp(void* mh);

	/* Section 15: Launch Modifiers */
	static int api_launch_set_env(void* mh, const char* key, const char* value);
	static int api_launch_prepend_wrapper(void* mh, const char* wrapper_cmd);

	/* Section 16: Application Settings */
	static const char* api_app_setting_get(void* mh, const char* key);

	/* Section 21: Application Settings — write side (ABI 3+) */
	static int api_app_setting_set(void* mh, const char* key,
								   const char* value);
	static int api_app_setting_register(void* mh, const char* key,
										const char* default_value);
	static int api_app_setting_contains(void* mh, const char* key);

	/* Section 22: Themed icon resolution (ABI 3+) */
	static const char* api_ui_themed_icon(void* mh, const char* name);

	/* Section 23: Instance running-state signal bridge (ABI 3+) */
	static int api_instance_running_register(void* mh, const char* instance_id,
											 MMCOInstanceRunningCallback cb,
											 void* ud);
	static int api_instance_running_unregister(void* mh,
											   const char* instance_id);

	/* Section 24: Per-instance settings (ABI 3+) */
	static const char* api_instance_setting_get(void* mh,
												const char* instance_id,
												const char* key);
	static int api_instance_setting_set(void* mh, const char* instance_id,
										const char* key, const char* value);
	static int api_instance_setting_register(void* mh, const char* instance_id,
											 const char* key,
											 const char* default_value);
	static int api_instance_setting_register_override(void* mh,
													  const char* instance_id,
													  const char* key,
													  const char* gate_key);
	static int api_instance_setting_reset(void* mh, const char* instance_id,
										  const char* key);
	static int api_instance_setting_contains(void* mh, const char* instance_id,
											 const char* key);

	/* Section 25: Account / skin / cape access (ABI 3+) */
	static const char* api_account_get_id_by_index(void* mh, int index);
	static int api_account_is_msa_by_id(void* mh, const char* account_id);
	static const char* api_account_get_access_token(void* mh,
													const char* account_id);
	static const char* api_account_get_current_cape_id(void* mh,
													   const char* account_id);
	static const char* api_account_get_skin_variant(void* mh,
													const char* account_id);
	static int64_t api_account_get_skin_blob(void* mh, const char* account_id,
											 const void** out_ptr);
	static int api_account_cape_count(void* mh, const char* account_id);
	static const char* api_account_cape_get_id(void* mh, const char* account_id,
											   int index);
	static const char*
	api_account_cape_get_alias(void* mh, const char* account_id, int index);
	static int64_t api_account_cape_get_blob(void* mh, const char* account_id,
											 int index, const void** out_ptr);
	static int api_account_set_skin_variant(void* mh, const char* account_id,
											const char* variant);
	static int api_account_set_current_cape(void* mh, const char* account_id,
											const char* cape_id);
	static int api_account_set_skin_blob(void* mh, const char* account_id,
										 const void* data, int64_t size);

	/* Section 26: Synchronous task helpers (ABI 3+) */
	static int api_account_skin_upload(void* mh, const char* account_id,
									   const void* png_bytes, int64_t size,
									   const char* variant);
	static int api_account_skin_reset(void* mh, const char* account_id);
	static int api_account_cape_set(void* mh, const char* account_id,
									const char* cape_id);

	/* Section 27: Icon list enumeration (ABI 3+) */
	static int api_icon_list_count(void* mh);
	static const char* api_icon_list_get_key(void* mh, int index);
	static const char* api_icon_list_get_name(void* mh, int index);
	static const char* api_icon_list_get_file_path(void* mh,
												   const char* icon_key);
	static int api_icon_list_save_png(void* mh, const char* icon_key,
									  const char* dest_path);

	/* Section 18: Plugin Icon Set (ABI 2+) */
	static const char* api_ui_plugin_icon(void* mh, const char* name);

	/* Section 19: System Tray (additive) */
	static void* api_tray_create(void* mh, const char* icon_name,
								 const char* tooltip);
	static int api_tray_destroy(void* mh, void* tray_handle);
	static int api_tray_is_available(void* mh);
	static int api_tray_set_icon(void* mh, void* tray_handle,
								 const char* icon_name);
	static int api_tray_set_tooltip(void* mh, void* tray_handle,
									const char* tooltip);
	static int api_tray_set_visible(void* mh, void* tray_handle, int visible);
	static int api_tray_show_message(void* mh, void* tray_handle,
									 const char* title, const char* message,
									 int icon_type, int msecs);
	static int api_tray_set_menu(void* mh, void* tray_handle,
								 void* menu_handle);
	static int api_tray_set_activation_cb(void* mh, void* tray_handle,
										  MMCOTrayActivationCallback cb,
										  void* ud);
	static void* api_tray_menu_create(void* mh);
	static int api_tray_menu_destroy(void* mh, void* menu_handle);
	static int api_tray_menu_clear(void* mh, void* menu_handle);
	static int api_tray_menu_add_separator(void* mh, void* menu_handle);
	static void* api_tray_menu_add_action(void* mh, void* menu_handle,
										  const char* label,
										  const char* icon_name,
										  MMCOMenuActionCallback cb, void* ud);
	static int api_tray_menu_action_set_enabled(void* mh, void* action_handle,
												int enabled);
	static int api_tray_menu_action_set_text(void* mh, void* action_handle,
											 const char* text);
	static void* api_tray_menu_add_submenu(void* mh, void* parent_menu,
										   const char* label,
										   const char* icon_name);

	/* Section 20: Main window helpers */
	static int api_main_window_install_close_filter(
		void* mh, MMCOMainWindowCloseCallback cb, void* user_data);
	static int api_main_window_show(void* mh);
	static int api_main_window_hide(void* mh);
	static int api_main_window_is_visible(void* mh);

	/* S17 — News API.
	 *
	 * Read-only projection of MainWindow's NewsChecker, which owns and
	 * parses every feed. Both helpers cope with there being no main
	 * window yet (startup) or any more (shutdown). */
	NewsChecker* newsChecker() const;
	QList<NewsEntryPtr> newsEntries() const;
	static int api_news_get_entry_count(void* mh);
	static const char* api_news_get_entry_title(void* mh, int index);
	static const char* api_news_get_entry_link(void* mh, int index);
	static const char* api_news_get_entry_content(void* mh, int index);
	static const char* api_news_get_entry_author(void* mh, int index);
	static const char* api_news_get_entry_date(void* mh, int index);
	static int api_news_get_entry_feed_index(void* mh, int index);
	static int api_news_add_feed_url(void* mh, const char* url);
	static int api_news_get_feed_count(void* mh);
	static const char* api_news_get_feed_url(void* mh, int index);
	static int api_news_reload(void* mh);

	/* Helpers */
	static ModuleRuntime* rt(void* mh);

	PluginLoader m_loader;
	QVector<PluginMetadata> m_modules;
	std::vector<std::unique_ptr<ModuleRuntime>> m_runtimes;
	std::vector<MMCOContext> m_contexts;

	/* hook_id -> list of registrations */
	QMultiMap<uint32_t, HookRegistration> m_hooks;

  public:
	/* Registered instance toolbar actions (for MainWindow to consume) */
	struct InstanceAction {
		QString text;
		QString tooltip;
		QString iconName;
		QString pageId;
	};
	const QVector<InstanceAction>& instanceActions() const
	{
		return m_instanceActions;
	}

	struct InstanceCallbackAction {
		QString text;
		QString tooltip;
		QString iconName;
		void (*callback)(void* ud);
		void* userData;
	};
	const QVector<InstanceCallbackAction>& instanceCallbackActions() const
	{
		return m_instanceCallbackActions;
	}

  private:
	QVector<InstanceAction> m_instanceActions;
	QVector<InstanceCallbackAction> m_instanceCallbackActions;

	/* Pending launch modifications (set by plugins during PRE_LAUNCH hooks).
	 *
	 * PRE_LAUNCH callbacks may run on a worker thread while
	 * LaunchController reads the collected values on the GUI thread, so
	 * every access goes through m_launchModMutex. The mutex is
	 * recursive-free: no method below calls another one that locks. */
	mutable QMutex m_launchModMutex;
	QMap<QString, QString> m_pendingLaunchEnv;
	QString m_pendingLaunchWrapper;

	/* S17 — News: no state. NewsChecker holds the feeds and the
	 * entries; see newsChecker() above. */

	/* S19 / S20 — system-tray and main-window helpers state.
	 * All tray icons, menus, actions and close filters are tracked per
	 * owning module so PluginManager can release them en masse when a
	 * module is unloaded — preventing leaks and dangling Qt parents. */
	struct TrayRecord {
		void* module_handle;
		QSystemTrayIcon* icon;
		QObject* guard; /* relay for activation signal */
	};
	struct MenuRecord {
		void* module_handle;
		QMenu* menu;
	};
	struct ActionRecord {
		void* module_handle;
		QAction* action;
	};
	struct CloseFilterRecord {
		void* module_handle;
		MMCOMainWindowCloseCallback cb;
		void* user_data;
	};
	QVector<TrayRecord> m_trayIcons;
	QVector<MenuRecord> m_trayMenus;
	QVector<ActionRecord> m_trayActions;
	QVector<CloseFilterRecord> m_closeFilters;
	bool m_closeFilterInstalled = false;
	QPointer<QWidget> m_filteredMainWindow;

	/* S23 (ABI 3+) — per-module per-instance running-state callbacks.
	 *
	 * Each record owns one QObject `guard` that anchors the Qt
	 * QObject::connect to the resolved BaseInstance's
	 * runningStatusChanged signal. mmco_unload() / module teardown
	 * deletes the guard, which severs the connection automatically
	 * — Qt's normal sender/receiver bookkeeping then guarantees the
	 * plugin's callback can never fire into freed memory.
	 *
	 * A single module may have at most one record per (module_handle,
	 * instance_id) pair; re-registering replaces the existing one. */
	struct InstanceRunningRecord {
		void* module_handle;
		QString instanceId;
		MMCOInstanceRunningCallback cb;
		void* user_data;
		QObject* guard;
	};
	QVector<InstanceRunningRecord> m_instanceRunning;

	/* Resolve the launcher's main window (objectName == "MainWindow"),
	 * cached for the lifetime of the QPointer. Returns nullptr if the
	 * window has not been built yet. */
	QWidget* resolveMainWindow();
	/* Make sure our QObject::eventFilter is installed on the main
	 * window. Safe to call multiple times — installs at most once. */
	void ensureCloseFilterInstalled();
	/* Release all S19/S20/S23 resources owned by the given module
	 * handle.  Called from shutdownAll() right before mmco_unload(). */
	void releaseTrayResourcesForModule(void* module_handle);

	/* Wire the two Application Qt signals we re-publish as hooks:
	 *   • globalSettingsAboutToOpen   ->
	 * MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN • instanceSettingsPageCreated ->
	 * MMCO_HOOK_INSTANCE_SETTINGS_PAGE_CREATED Called once from initializeAll()
	 * after modules are up. The connections are owned by `this` (PluginManager
	 * is a QObject) and severed automatically on destruction. */
	void connectAppSignals();

	bool m_shutdownDone = false;

  public:
	/* Called by LaunchController before/after dispatching PRE_LAUNCH hook */
	void clearPendingLaunchMods();
	QMap<QString, QString> takePendingLaunchEnv();
	QString takePendingLaunchWrapper();
};
