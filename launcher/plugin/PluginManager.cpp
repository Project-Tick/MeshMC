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

#include "plugin/PluginManager.h"
#include "plugin/PluginDependencyResolver.h"
#include "plugin/PluginSignature.h"
#include "Application.h"
#include "BuildConfig.h"
#include "InstanceList.h"
#include "BaseInstance.h"
#include "MMCZip.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/Component.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/WorldList.h"
#include "minecraft/World.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/auth/AccountData.h"
#include "minecraft/services/SkinUpload.h"
#include "minecraft/services/SkinDelete.h"
#include "minecraft/services/CapeChange.h"
#include "tasks/SequentialTask.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/pages/instance/InstanceSettingsPage.h"
#include "icons/IconList.h"
#include "icons/MMCIcon.h"
#include "java/JavaInstallList.h"
#include "java/JavaInstall.h"
#include "settings/SettingsObject.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QInputDialog>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDomDocument>
#include <QDomNodeList>
#include <QStringList>
#include "news/NewsChecker.h"
#include "ui/MainWindow.h"
#include <net/NetJob.h>
#include "net/Download.h"
#include <QPushButton>
#include <QSpacerItem>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QHeaderView>
#include <QAction>
#include <QCloseEvent>
#include <QIcon>
#include <QSystemTrayIcon>

PluginManager::PluginManager(Application* app, QObject* parent)
	: QObject(parent), m_app(app)
{
}

PluginManager::~PluginManager()
{
	shutdownAll();
}

void PluginManager::initializeAll()
{
	qDebug() << "[PluginManager] Discovering modules...";

	// Configure GPG keyring location and the verification-result cache
	// before discovery — the loader's signature pre-flight runs inside
	// discoverModules() and consumes both.
	if (m_app && m_app->settings()) {
		const QString key = QStringLiteral("plugin.signing.keyring_path");
		if (!m_app->settings()->contains(key))
			m_app->settings()->registerSetting(key, QString());
		const QString path = m_app->settings()->get(key).toString();
		if (!path.isEmpty())
			PluginSignature::setKeyringPath(path);
	}

	// Cache path lives next to the launcher's other settings. The cache
	// is the single biggest reason a second-and-later startup is fast:
	// the (size, mtime) tuple of each .mmco is enough to skip the
	// GpgME round-trip entirely. First-startup cost stays the same.
	{
		const QString cacheDir =
			QStandardPaths::writableLocation(
				QStandardPaths::AppLocalDataLocation);
		if (!cacheDir.isEmpty()) {
			PluginSignature::setCachePath(
				QDir(cacheDir).filePath(QStringLiteral(
					"plugin-signature-cache.json")));
		}
	}

	const QSet<QString> disabled = disabledModuleNames();
	m_modules = m_loader.discoverModules(disabled);
	// Persist the cache once discovery has settled — every newly-seen
	// plugin is now memoised so the next launcher startup skips
	// straight to the cache hit.
	PluginSignature::flushCache();

	if (m_modules.isEmpty()) {
		qDebug() << "[PluginManager] No modules found.";
		return;
	}

	// Resolve dependencies and produce a load order.
	const auto resolved = PluginDependencyResolver::resolve(m_modules);

	// Prepare runtimes and contexts — sized to the full module list so
	// that disabled modules still have a slot (they just never get an
	// active context). Indexing stays consistent with m_modules.
	m_runtimes.resize(static_cast<size_t>(m_modules.size()));
	m_contexts.resize(static_cast<size_t>(m_modules.size()));

	for (int i : resolved.loadOrder) {
		auto& meta = m_modules[i];

		if (meta.disabled) {
			// Defensive — resolver should have excluded these already.
			qDebug() << "[PluginManager] Skipping disabled module:" << meta.name
					 << "-" << meta.disableDetail;
			continue;
		}

		ensurePluginDataDir(meta);

		auto runtime = std::make_unique<ModuleRuntime>();
		runtime->manager = this;
		runtime->moduleIndex = i;
		runtime->dataDir = meta.dataDir.toStdString();
		m_runtimes[i] = std::move(runtime);

		m_contexts[i] = buildContext(meta);
		m_contexts[i].module_handle = m_runtimes[i].get();

		qDebug() << "[PluginManager] Initializing module:" << meta.name;
		int rc = meta.initFunc(&m_contexts[i]);
		if (rc != 0) {
			qWarning() << "[PluginManager] Module" << meta.name
					   << "mmco_init() returned" << rc << "- skipping";
			emit moduleError(meta.name,
							 QString("mmco_init returned %1").arg(rc));
			PluginLoader::unloadModule(meta);
			continue;
		}

		meta.initialized = true;
		qDebug() << "[PluginManager] Module" << meta.name
				 << "initialized successfully";
		emit moduleLoaded(meta.name);
	}

	// Log the modules that were excluded from the load order so users
	// can find them in the launcher logs.
	for (const auto& meta : m_modules) {
		if (meta.disabled) {
			qInfo().noquote() << "[PluginManager] Module" << meta.name
							  << "not loaded:" << meta.disableDetail;
		}
	}

	// Wire the Application Qt signals we re-publish as MMCO hooks
	// (global-settings open, instance-settings-page created). Must
	// happen before APP_INITIALIZED so plugins that register for
	// those hooks inside mmco_init() see the very first dispatch.
	connectAppSignals();

	// Seed the news extra-feed list from BuildConfig so plugins that
	// consume the news API see every feed configured at build time
	// without having to link BuildConfig themselves.
	if (!BuildConfig.NEWS_EXTRA_FEEDS.isEmpty()) {
		const QStringList urls = BuildConfig.NEWS_EXTRA_FEEDS.split(
			QLatin1Char(';'), Qt::SkipEmptyParts);
		for (const QString& url : urls) {
			const QString trimmed = url.trimmed();
			if (!trimmed.isEmpty() && !m_extraFeedUrls.contains(trimmed))
				m_extraFeedUrls.append(trimmed);
		}
	}

	// Fire app-initialized hook
	dispatchHook(MMCO_HOOK_APP_INITIALIZED);
}

bool PluginManager::isModuleDisabled(const QString& moduleName) const
{
	return disabledModuleNames().contains(moduleName.toLower());
}

void PluginManager::setModuleDisabled(const QString& moduleName, bool disabled)
{
	if (!m_app || !m_app->settings() || moduleName.isEmpty())
		return;

	const QString key = QStringLiteral("plugins.disabled");
	if (!m_app->settings()->contains(key))
		m_app->settings()->registerSetting(key, QString());

	QSet<QString> current = disabledModuleNames();
	const QString lname = moduleName.toLower();
	if (disabled)
		current.insert(lname);
	else
		current.remove(lname);

	QStringList list(current.begin(), current.end());
	list.sort();
	m_app->settings()->set(key, list.join(QLatin1Char(',')));
}

QSet<QString> PluginManager::disabledModuleNames() const
{
	QSet<QString> out;
	if (!m_app || !m_app->settings())
		return out;
	const QString key = QStringLiteral("plugins.disabled");

	// SettingsObject::contains() only reports settings that have been
	// explicitly registered with registerSetting() — even if the INI
	// file on disk has a value for that key. If we are the first caller
	// of the session we must register the setting here, otherwise the
	// `get()` below would always return an empty default and the user's
	// disable list would silently be ignored after a restart.
	if (!m_app->settings()->contains(key)) {
		m_app->settings()->registerSetting(key, QString());
	}

	const QString raw = m_app->settings()->get(key).toString();
	for (const QString& part :
		 raw.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
		const QString trimmed = part.trimmed().toLower();
		if (!trimmed.isEmpty())
			out.insert(trimmed);
	}
	return out;
}

void PluginManager::shutdownAll()
{
	if (m_shutdownDone)
		return;
	m_shutdownDone = true;

	// Fire shutdown hook before unloading
	dispatchHook(MMCO_HOOK_APP_SHUTDOWN);

	// Unload in reverse order
	for (int i = m_modules.size() - 1; i >= 0; --i) {
		auto& meta = m_modules[i];
		if (!meta.initialized)
			continue;

		qDebug().noquote() << "[PluginManager] Unloading module:" << meta.name;
		/* Tear down tray icons, menus, actions and close filters owned by
		 * this module *before* invoking mmco_unload(). The plugin may
		 * still hold raw pointers to these QObjects in its C state, but
		 * after this point they are dead — and that is fine because the
		 * plugin won't touch them once mmco_unload() returns. */
		releaseTrayResourcesForModule(m_runtimes[i].get());
		if (meta.unloadFunc) {
			meta.unloadFunc();
		}
		meta.initialized = false;
	}

	/* Belt-and-braces: if the main window outlives PluginManager (Application
	 * teardown is awkward), unhook our event filter so it doesn't fire into
	 * a dead `this`. */
	if (m_closeFilterInstalled && m_filteredMainWindow) {
		m_filteredMainWindow->removeEventFilter(this);
		m_closeFilterInstalled = false;
	}

	// DO NOT call dlclose() or clear() data structures here.
	//
	// Plugin .mmco shared libraries statically link MeshMC_logic
	// which contains global objects with non-trivial destructors
	// (e.g. `const Config BuildConfig`).  Modules are opened with
	// RTLD_NODELETE to prevent their static destructors from running
	// at exit and corrupting the heap.  Calling dlclose() would
	// undo that protection.
	//
	// Additionally, plugin Q_OBJECT classes (e.g. BackupPage) have
	// MOC-generated QMetaObject statics registered in Qt's type
	// system; unmapping that memory causes crashes.
	// The OS reclaims all process memory at exit.
}

bool PluginManager::dispatchHook(uint32_t hook_id, void* payload)
{
	auto range = m_hooks.equal_range(hook_id);
	for (auto it = range.first; it != range.second; ++it) {
		const auto& reg = it.value();
		int rc =
			reg.callback(reg.module_handle, hook_id, payload, reg.user_data);
		if (rc != 0) {
			return true; // cancelled
		}
	}
	return false;
}

MMCOContext PluginManager::buildContext(PluginMetadata& meta)
{
	MMCOContext ctx{};
	ctx.struct_size = sizeof(MMCOContext);
	ctx.abi_version = MMCO_ABI_VERSION;
	ctx.module_handle = nullptr; // set by caller

	// S1 — Logging
	ctx.log_info = api_log_info;
	ctx.log_warn = api_log_warn;
	ctx.log_error = api_log_error;
	ctx.log_debug = api_log_debug;

	// S2 — Hooks
	ctx.hook_register = api_hook_register;
	ctx.hook_unregister = api_hook_unregister;

	// S3 — Settings
	ctx.setting_get = api_setting_get;
	ctx.setting_set = api_setting_set;

	// S4 — Instance Management
	ctx.instance_count = api_instance_count;
	ctx.instance_get_id = api_instance_get_id;
	ctx.instance_get_name = api_instance_get_name;
	ctx.instance_set_name = api_instance_set_name;
	ctx.instance_get_path = api_instance_get_path;
	ctx.instance_get_game_root = api_instance_get_game_root;
	ctx.instance_get_mods_root = api_instance_get_mods_root;
	ctx.instance_get_icon_key = api_instance_get_icon_key;
	ctx.instance_set_icon_key = api_instance_set_icon_key;
	ctx.instance_get_type = api_instance_get_type;
	ctx.instance_get_notes = api_instance_get_notes;
	ctx.instance_set_notes = api_instance_set_notes;
	ctx.instance_is_running = api_instance_is_running;
	ctx.instance_can_launch = api_instance_can_launch;
	ctx.instance_has_crashed = api_instance_has_crashed;
	ctx.instance_has_update = api_instance_has_update;
	ctx.instance_get_total_play_time = api_instance_get_total_play_time;
	ctx.instance_get_last_play_time = api_instance_get_last_play_time;
	ctx.instance_get_last_launch = api_instance_get_last_launch;
	ctx.instance_launch = api_instance_launch;
	ctx.instance_kill = api_instance_kill;
	ctx.instance_delete = api_instance_delete;
	ctx.instance_get_group = api_instance_get_group;
	ctx.instance_set_group = api_instance_set_group;
	ctx.instance_group_count = api_instance_group_count;
	ctx.instance_group_at = api_instance_group_at;
	ctx.instance_component_count = api_instance_component_count;
	ctx.instance_component_get_uid = api_instance_component_get_uid;
	ctx.instance_component_get_name = api_instance_component_get_name;
	ctx.instance_component_get_version = api_instance_component_get_version;
	ctx.instance_get_mc_version = api_instance_get_mc_version;
	ctx.instance_get_jar_mods_dir = api_instance_get_jar_mods_dir;
	ctx.instance_get_resource_packs_dir = api_instance_get_resource_packs_dir;
	ctx.instance_get_texture_packs_dir = api_instance_get_texture_packs_dir;
	ctx.instance_get_shader_packs_dir = api_instance_get_shader_packs_dir;
	ctx.instance_get_worlds_dir = api_instance_get_worlds_dir;

	// S5 — Mod Management
	ctx.mod_count = api_mod_count;
	ctx.mod_get_name = api_mod_get_name;
	ctx.mod_get_version = api_mod_get_version;
	ctx.mod_get_filename = api_mod_get_filename;
	ctx.mod_get_description = api_mod_get_description;
	ctx.mod_is_enabled = api_mod_is_enabled;
	ctx.mod_set_enabled = api_mod_set_enabled;
	ctx.mod_remove = api_mod_remove;
	ctx.mod_install = api_mod_install;
	ctx.mod_refresh = api_mod_refresh;

	// S6 — World Management
	ctx.world_count = api_world_count;
	ctx.world_get_name = api_world_get_name;
	ctx.world_get_folder = api_world_get_folder;
	ctx.world_get_seed = api_world_get_seed;
	ctx.world_get_game_type = api_world_get_game_type;
	ctx.world_get_last_played = api_world_get_last_played;
	ctx.world_delete = api_world_delete;
	ctx.world_rename = api_world_rename;
	ctx.world_install = api_world_install;
	ctx.world_refresh = api_world_refresh;

	// S7 — Account Management
	ctx.account_count = api_account_count;
	ctx.account_get_profile_name = api_account_get_profile_name;
	ctx.account_get_profile_id = api_account_get_profile_id;
	ctx.account_get_type = api_account_get_type;
	ctx.account_get_state = api_account_get_state;
	ctx.account_is_active = api_account_is_active;
	ctx.account_get_default_index = api_account_get_default_index;

	// S8 — Java Management
	ctx.java_count = api_java_count;
	ctx.java_get_version = api_java_get_version;
	ctx.java_get_arch = api_java_get_arch;
	ctx.java_get_path = api_java_get_path;
	ctx.java_is_recommended = api_java_is_recommended;
	ctx.instance_get_java_version = api_instance_get_java_version;

	// S9 — Filesystem
	ctx.fs_plugin_data_dir = api_fs_plugin_data_dir;
	ctx.fs_read = api_fs_read;
	ctx.fs_write = api_fs_write;
	ctx.fs_exists = api_fs_exists;
	ctx.fs_mkdir = api_fs_mkdir;
	ctx.fs_exists_abs = api_fs_exists_abs;
	ctx.fs_remove = api_fs_remove;
	ctx.fs_copy_file = api_fs_copy_file;
	ctx.fs_file_size = api_fs_file_size;
	ctx.fs_list_dir = api_fs_list_dir;

	// S10 — Zip
	ctx.zip_compress_dir = api_zip_compress_dir;
	ctx.zip_extract = api_zip_extract;

	// S11 — Network
	ctx.http_get = api_http_get;
	ctx.http_post = api_http_post;

	// S12 — UI Dialogs
	ctx.ui_show_message = api_ui_show_message;
	ctx.ui_add_menu_item = api_ui_add_menu_item;
	ctx.ui_file_open_dialog = api_ui_file_open_dialog;
	ctx.ui_file_save_dialog = api_ui_file_save_dialog;
	ctx.ui_input_dialog = api_ui_input_dialog;
	ctx.ui_confirm_dialog = api_ui_confirm_dialog;
	ctx.ui_register_instance_action = api_ui_register_instance_action;
	ctx.ui_register_instance_action_cb = api_ui_register_instance_action_cb;

	// S13 — UI Page Builder
	ctx.ui_page_create = api_ui_page_create;
	ctx.ui_page_add_to_list = api_ui_page_add_to_list;
	ctx.ui_layout_create = api_ui_layout_create;
	ctx.ui_layout_add_widget = api_ui_layout_add_widget;
	ctx.ui_layout_add_layout = api_ui_layout_add_layout;
	ctx.ui_layout_add_spacer = api_ui_layout_add_spacer;
	ctx.ui_page_set_layout = api_ui_page_set_layout;
	ctx.ui_button_create = api_ui_button_create;
	ctx.ui_button_set_enabled = api_ui_button_set_enabled;
	ctx.ui_button_set_text = api_ui_button_set_text;
	ctx.ui_label_create = api_ui_label_create;
	ctx.ui_label_set_text = api_ui_label_set_text;
	ctx.ui_tree_create = api_ui_tree_create;
	ctx.ui_tree_clear = api_ui_tree_clear;
	ctx.ui_tree_add_row = api_ui_tree_add_row;
	ctx.ui_tree_selected_row = api_ui_tree_selected_row;
	ctx.ui_tree_set_row_data = api_ui_tree_set_row_data;
	ctx.ui_tree_get_row_data = api_ui_tree_get_row_data;
	ctx.ui_tree_row_count = api_ui_tree_row_count;

	// S14 — Utility
	ctx.get_app_version = api_get_app_version;
	ctx.get_app_name = api_get_app_name;
	ctx.get_timestamp = api_get_timestamp;

	// S15 — Launch Modifiers
	ctx.launch_set_env = api_launch_set_env;
	ctx.launch_prepend_wrapper = api_launch_prepend_wrapper;

	// S16 — Application Settings
	ctx.app_setting_get = api_app_setting_get;

	// S17 — News API
	ctx.news_get_entry_count = api_news_get_entry_count;
	ctx.news_get_entry_title = api_news_get_entry_title;
	ctx.news_get_entry_link = api_news_get_entry_link;
	ctx.news_get_entry_content = api_news_get_entry_content;
	ctx.news_get_entry_author = api_news_get_entry_author;
	ctx.news_get_entry_date = api_news_get_entry_date;
	ctx.news_get_entry_feed_index = api_news_get_entry_feed_index;
	ctx.news_add_feed_url = api_news_add_feed_url;
	ctx.news_get_feed_count = api_news_get_feed_count;
	ctx.news_get_feed_url = api_news_get_feed_url;
	ctx.news_reload = api_news_reload;

	// S18 — Plugin Icon Set (ABI 2+)
	ctx.ui_plugin_icon = api_ui_plugin_icon;

	// S19 — System Tray
	ctx.tray_create = api_tray_create;
	ctx.tray_destroy = api_tray_destroy;
	ctx.tray_is_available = api_tray_is_available;
	ctx.tray_set_icon = api_tray_set_icon;
	ctx.tray_set_tooltip = api_tray_set_tooltip;
	ctx.tray_set_visible = api_tray_set_visible;
	ctx.tray_show_message = api_tray_show_message;
	ctx.tray_set_menu = api_tray_set_menu;
	ctx.tray_set_activation_cb = api_tray_set_activation_cb;
	ctx.tray_menu_create = api_tray_menu_create;
	ctx.tray_menu_destroy = api_tray_menu_destroy;
	ctx.tray_menu_clear = api_tray_menu_clear;
	ctx.tray_menu_add_separator = api_tray_menu_add_separator;
	ctx.tray_menu_add_action = api_tray_menu_add_action;
	ctx.tray_menu_action_set_enabled = api_tray_menu_action_set_enabled;
	ctx.tray_menu_action_set_text = api_tray_menu_action_set_text;
	ctx.tray_menu_add_submenu = api_tray_menu_add_submenu;

	// S20 — Main window helpers
	ctx.main_window_install_close_filter = api_main_window_install_close_filter;
	ctx.main_window_show = api_main_window_show;
	ctx.main_window_hide = api_main_window_hide;
	ctx.main_window_is_visible = api_main_window_is_visible;

	// S21 — Application Settings (write side, ABI 3+)
	ctx.app_setting_set = api_app_setting_set;
	ctx.app_setting_register = api_app_setting_register;
	ctx.app_setting_contains = api_app_setting_contains;

	// S22 — Themed icon resolution (ABI 3+)
	ctx.ui_themed_icon = api_ui_themed_icon;

	// S23 — Instance running-state signal bridge (ABI 3+)
	ctx.instance_running_register = api_instance_running_register;
	ctx.instance_running_unregister = api_instance_running_unregister;

	// S24 — Per-instance settings (ABI 3+)
	ctx.instance_setting_get = api_instance_setting_get;
	ctx.instance_setting_set = api_instance_setting_set;
	ctx.instance_setting_register = api_instance_setting_register;
	ctx.instance_setting_register_override =
		api_instance_setting_register_override;
	ctx.instance_setting_reset = api_instance_setting_reset;
	ctx.instance_setting_contains = api_instance_setting_contains;

	// S25 — Account / skin / cape access (ABI 3+)
	ctx.account_get_id_by_index = api_account_get_id_by_index;
	ctx.account_is_msa_by_id = api_account_is_msa_by_id;
	ctx.account_get_access_token = api_account_get_access_token;
	ctx.account_get_current_cape_id = api_account_get_current_cape_id;
	ctx.account_get_skin_variant = api_account_get_skin_variant;
	ctx.account_get_skin_blob = api_account_get_skin_blob;
	ctx.account_cape_count = api_account_cape_count;
	ctx.account_cape_get_id = api_account_cape_get_id;
	ctx.account_cape_get_alias = api_account_cape_get_alias;
	ctx.account_cape_get_blob = api_account_cape_get_blob;
	ctx.account_set_skin_variant = api_account_set_skin_variant;
	ctx.account_set_current_cape = api_account_set_current_cape;
	ctx.account_set_skin_blob = api_account_set_skin_blob;

	// S26 — Synchronous task helpers (ABI 3+)
	ctx.account_skin_upload = api_account_skin_upload;
	ctx.account_skin_reset = api_account_skin_reset;
	ctx.account_cape_set = api_account_cape_set;

	// S27 — Icon list enumeration (ABI 3+)
	ctx.icon_list_count = api_icon_list_count;
	ctx.icon_list_get_key = api_icon_list_get_key;
	ctx.icon_list_get_name = api_icon_list_get_name;
	ctx.icon_list_get_file_path = api_icon_list_get_file_path;
	ctx.icon_list_save_png = api_icon_list_save_png;

	return ctx;
}

void PluginManager::ensurePluginDataDir(PluginMetadata& meta)
{
	QString baseDir;
#ifdef Q_OS_WIN
	baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
	baseDir = QDir::homePath() + "/.local/share/MeshMC";
#endif
	meta.dataDir = QDir(baseDir).filePath("plugin-data/" + meta.moduleId());
	QDir().mkpath(meta.dataDir);
}

PluginManager::ModuleRuntime* PluginManager::rt(void* mh)
{
	return static_cast<ModuleRuntime*>(mh);
}

void PluginManager::api_log_info(void* mh, const char* msg)
{
	auto* r = rt(mh);
	auto& meta = r->manager->m_modules[r->moduleIndex];
	qInfo().noquote() << "[Plugin:" << meta.name << "]" << msg;
}

void PluginManager::api_log_warn(void* mh, const char* msg)
{
	auto* r = rt(mh);
	auto& meta = r->manager->m_modules[r->moduleIndex];
	qWarning().noquote() << "[Plugin:" << meta.name << "]" << msg;
}

void PluginManager::api_log_error(void* mh, const char* msg)
{
	auto* r = rt(mh);
	auto& meta = r->manager->m_modules[r->moduleIndex];
	qCritical().noquote() << "[Plugin:" << meta.name << "]" << msg;
}

void PluginManager::api_log_debug(void* mh, const char* msg)
{
	auto* r = rt(mh);
	auto& meta = r->manager->m_modules[r->moduleIndex];
	qDebug().noquote() << "[Plugin:" << meta.name << "]" << msg;
}

int PluginManager::api_hook_register(void* mh, uint32_t hook_id,
									 MMCOHookCallback cb, void* ud)
{
	auto* r = rt(mh);
	if (!cb)
		return -1;

	HookRegistration reg;
	reg.module_handle = mh;
	reg.callback = cb;
	reg.user_data = ud;

	r->manager->m_hooks.insert(hook_id, reg);
	return 0;
}

int PluginManager::api_hook_unregister(void* mh, uint32_t hook_id,
									   MMCOHookCallback cb)
{
	auto* r = rt(mh);
	auto& hooks = r->manager->m_hooks;

	auto range = hooks.equal_range(hook_id);
	for (auto it = range.first; it != range.second; ++it) {
		if (it.value().module_handle == mh && it.value().callback == cb) {
			hooks.erase(it);
			return 0;
		}
	}
	return -1;
}

const char* PluginManager::api_setting_get(void* mh, const char* key)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->settings())
		return nullptr;

	auto& meta = r->manager->m_modules[r->moduleIndex];
	QString fullKey =
		QString("plugin.%1.%2").arg(meta.moduleId(), QString::fromUtf8(key));
	QVariant val = app->settings()->get(fullKey);
	if (!val.isValid())
		return nullptr;

	r->tempString = val.toString().toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_setting_set(void* mh, const char* key, const char* value)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->settings())
		return -1;

	auto& meta = r->manager->m_modules[r->moduleIndex];
	QString fullKey =
		QString("plugin.%1.%2").arg(meta.moduleId(), QString::fromUtf8(key));

	// Auto-register the setting if it doesn't exist yet
	if (!app->settings()->contains(fullKey)) {
		app->settings()->registerSetting(fullKey, QString());
	}

	app->settings()->set(fullKey, QString::fromUtf8(value));
	return 0;
}

int PluginManager::api_instance_count(void* mh)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances())
		return 0;
	return app->instances()->count();
}

const char* PluginManager::api_instance_get_id(void* mh, int index)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances())
		return nullptr;

	auto list = app->instances();
	if (index < 0 || index >= list->count())
		return nullptr;

	auto inst = list->at(index);
	if (!inst)
		return nullptr;

	r->tempString = inst->id().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_name(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances())
		return nullptr;

	auto inst = app->instances()->getInstanceById(QString::fromUtf8(id));
	if (!inst)
		return nullptr;

	r->tempString = inst->name().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_path(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances())
		return nullptr;

	auto inst = app->instances()->getInstanceById(QString::fromUtf8(id));
	if (!inst)
		return nullptr;

	r->tempString = inst->instanceRoot().toStdString();
	return r->tempString.c_str();
}

static BaseInstance* resolveInstance(PluginManager::ModuleRuntime* r,
									 const char* id)
{
	auto* app = r->manager->m_app;
	if (!app || !app->instances() || !id)
		return nullptr;
	auto inst = app->instances()->getInstanceById(QString::fromUtf8(id));
	return inst.get();
}

static MinecraftInstance* resolveMC(PluginManager::ModuleRuntime* r,
									const char* id)
{
	return dynamic_cast<MinecraftInstance*>(resolveInstance(r, id));
}

int PluginManager::api_instance_set_name(void* mh, const char* id,
										 const char* name)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	if (!inst || !name)
		return -1;
	inst->setName(QString::fromUtf8(name));
	return 0;
}

const char* PluginManager::api_instance_get_game_root(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	if (!inst)
		return nullptr;
	r->tempString = inst->gameRoot().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_mods_root(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	if (!inst)
		return nullptr;
	r->tempString = inst->modsRoot().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_icon_key(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	if (!inst)
		return nullptr;
	r->tempString = inst->iconKey().toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_instance_set_icon_key(void* mh, const char* id,
											 const char* key)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	if (!inst || !key)
		return -1;
	inst->setIconKey(QString::fromUtf8(key));
	return 0;
}

const char* PluginManager::api_instance_get_type(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	if (!inst)
		return nullptr;
	r->tempString = inst->instanceType().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_notes(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	if (!inst)
		return nullptr;
	r->tempString = inst->notes().toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_instance_set_notes(void* mh, const char* id,
										  const char* notes)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	if (!inst || !notes)
		return -1;
	inst->setNotes(QString::fromUtf8(notes));
	return 0;
}

int PluginManager::api_instance_is_running(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	return inst ? (inst->isRunning() ? 1 : 0) : 0;
}

int PluginManager::api_instance_can_launch(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	return inst ? (inst->canLaunch() ? 1 : 0) : 0;
}

int PluginManager::api_instance_has_crashed(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	return inst ? (inst->hasCrashed() ? 1 : 0) : 0;
}

int PluginManager::api_instance_has_update(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	return inst ? (inst->hasUpdateAvailable() ? 1 : 0) : 0;
}

int64_t PluginManager::api_instance_get_total_play_time(void* mh,
														const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	return inst ? inst->totalTimePlayed() : 0;
}

int64_t PluginManager::api_instance_get_last_play_time(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	return inst ? inst->lastTimePlayed() : 0;
}

int64_t PluginManager::api_instance_get_last_launch(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* inst = resolveInstance(r, id);
	return inst ? inst->lastLaunch() : 0;
}

int PluginManager::api_instance_launch(void* mh, const char* id, int online)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances() || !id)
		return -1;
	auto inst = app->instances()->getInstanceById(QString::fromUtf8(id));
	if (!inst)
		return -1;
	return app->launch(inst, online != 0) ? 0 : -1;
}

int PluginManager::api_instance_kill(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances() || !id)
		return -1;
	auto inst = app->instances()->getInstanceById(QString::fromUtf8(id));
	if (!inst)
		return -1;
	return app->kill(inst) ? 0 : -1;
}

int PluginManager::api_instance_delete(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances() || !id)
		return -1;
	app->instances()->deleteInstance(QString::fromUtf8(id));
	return 0;
}

const char* PluginManager::api_instance_get_group(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances() || !id)
		return nullptr;
	r->tempString =
		app->instances()->getInstanceGroup(QString::fromUtf8(id)).toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_instance_set_group(void* mh, const char* id,
										  const char* group)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances() || !id)
		return -1;
	app->instances()->setInstanceGroup(
		QString::fromUtf8(id), group ? QString::fromUtf8(group) : QString());
	return 0;
}

int PluginManager::api_instance_group_count(void* mh)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances())
		return 0;
	return app->instances()->getGroups().size();
}

const char* PluginManager::api_instance_group_at(void* mh, int index)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->instances())
		return nullptr;
	auto groups = app->instances()->getGroups();
	if (index < 0 || index >= groups.size())
		return nullptr;
	r->tempString = groups.at(index).toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_instance_component_count(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc || !mc->getPackProfile())
		return 0;
	return mc->getPackProfile()->rowCount(QModelIndex());
}

const char*
PluginManager::api_instance_component_get_uid(void* mh, const char* id, int idx)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc || !mc->getPackProfile())
		return nullptr;
	auto* comp = mc->getPackProfile()->getComponent(idx);
	if (!comp)
		return nullptr;
	r->tempString = comp->getID().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_component_get_name(void* mh,
														   const char* id,
														   int idx)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc || !mc->getPackProfile())
		return nullptr;
	auto* comp = mc->getPackProfile()->getComponent(idx);
	if (!comp)
		return nullptr;
	r->tempString = comp->getName().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_component_get_version(void* mh,
															  const char* id,
															  int idx)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc || !mc->getPackProfile())
		return nullptr;
	auto* comp = mc->getPackProfile()->getComponent(idx);
	if (!comp)
		return nullptr;
	r->tempString = comp->getVersion().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_mc_version(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc || !mc->getPackProfile())
		return nullptr;
	r->tempString = mc->getPackProfile()
						->getComponentVersion("net.minecraft")
						.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_jar_mods_dir(void* mh,
														 const char* id)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc)
		return nullptr;
	r->tempString = mc->jarModsDir().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_resource_packs_dir(void* mh,
															   const char* id)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc)
		return nullptr;
	r->tempString = mc->resourcePacksDir().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_texture_packs_dir(void* mh,
															  const char* id)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc)
		return nullptr;
	r->tempString = mc->texturePacksDir().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_shader_packs_dir(void* mh,
															 const char* id)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc)
		return nullptr;
	r->tempString = mc->shaderPacksDir().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_instance_get_worlds_dir(void* mh, const char* id)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc)
		return nullptr;
	r->tempString = mc->worldDir().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_fs_plugin_data_dir(void* mh)
{
	auto* r = rt(mh);
	return r->dataDir.c_str();
}

static bool validateRelativePath(const char* rel)
{
	if (!rel || rel[0] == '\0')
		return false;
	QString p = QString::fromUtf8(rel);
	// Reject path traversal
	if (p.contains("..") || p.startsWith('/') || p.startsWith('\\'))
		return false;
	return true;
}

int64_t PluginManager::api_fs_read(void* mh, const char* rel, void* buf,
								   size_t sz)
{
	if (!validateRelativePath(rel))
		return -1;

	auto* r = rt(mh);
	QString path = QDir(QString::fromStdString(r->dataDir))
					   .filePath(QString::fromUtf8(rel));

	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
		return -1;

	qint64 bytesRead = f.read(static_cast<char*>(buf), static_cast<qint64>(sz));
	return bytesRead;
}

int PluginManager::api_fs_write(void* mh, const char* rel, const void* data,
								size_t sz)
{
	if (!validateRelativePath(rel))
		return -1;

	auto* r = rt(mh);
	QString path = QDir(QString::fromStdString(r->dataDir))
					   .filePath(QString::fromUtf8(rel));

	// Ensure parent directory exists
	QDir().mkpath(QFileInfo(path).absolutePath());

	QFile f(path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return -1;

	qint64 written =
		f.write(static_cast<const char*>(data), static_cast<qint64>(sz));
	return (written == static_cast<qint64>(sz)) ? 0 : -1;
}

int PluginManager::api_fs_exists(void* mh, const char* rel)
{
	if (!validateRelativePath(rel))
		return 0;

	auto* r = rt(mh);
	QString path = QDir(QString::fromStdString(r->dataDir))
					   .filePath(QString::fromUtf8(rel));
	return QFile::exists(path) ? 1 : 0;
}

int PluginManager::api_http_get(void* mh, const char* url, MMCOHttpCallback cb,
								void* ud)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;

	if (!url || !cb)
		return -1;

	// Validate URL scheme (only http/https allowed)
	QString qurl = QString::fromUtf8(url);
	if (!qurl.startsWith("http://") && !qurl.startsWith("https://"))
		return -1;

	auto nam = app->network();
	if (!nam)
		return -1;

	QNetworkRequest request{QUrl(qurl)};
	request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT);

	QNetworkReply* reply = nam->get(request);

	QObject::connect(reply, &QNetworkReply::finished, [reply, cb, ud]() {
		int status =
			reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		QByteArray body = reply->readAll();
		cb(ud, status, body.constData(), static_cast<size_t>(body.size()));
		reply->deleteLater();
	});

	return 0;
}

int PluginManager::api_http_post(void* mh, const char* url, const void* body,
								 size_t body_sz, const char* ct,
								 MMCOHttpCallback cb, void* ud)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;

	if (!url || !cb)
		return -1;

	QString qurl = QString::fromUtf8(url);
	if (!qurl.startsWith("http://") && !qurl.startsWith("https://"))
		return -1;

	auto nam = app->network();
	if (!nam)
		return -1;

	QNetworkRequest request{QUrl(qurl)};
	request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT);
	if (ct)
		request.setHeader(QNetworkRequest::ContentTypeHeader,
						  QString::fromUtf8(ct));

	QByteArray postData(static_cast<const char*>(body),
						static_cast<int>(body_sz));
	QNetworkReply* reply = nam->post(request, postData);

	QObject::connect(reply, &QNetworkReply::finished, [reply, cb, ud]() {
		int status =
			reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		QByteArray respBody = reply->readAll();
		cb(ud, status, respBody.constData(),
		   static_cast<size_t>(respBody.size()));
		reply->deleteLater();
	});

	return 0;
}

void PluginManager::api_ui_show_message(void* mh, int type, const char* title,
										const char* msg)
{
	auto* r = rt(mh);
	auto& meta = r->manager->m_modules[r->moduleIndex];

	QString qtitle =
		QString("[%1] %2").arg(meta.name, QString::fromUtf8(title));
	QString qmsg = QString::fromUtf8(msg);

	switch (type) {
		case 1:
			QMessageBox::warning(nullptr, qtitle, qmsg);
			break;
		case 2:
			QMessageBox::critical(nullptr, qtitle, qmsg);
			break;
		default:
			QMessageBox::information(nullptr, qtitle, qmsg);
			break;
	}
}

int PluginManager::api_ui_add_menu_item(void* /* mh */, void* menu_handle,
										const char* label,
										const char* /* icon */,
										MMCOMenuActionCallback cb, void* ud)
{
	if (!menu_handle || !label || !cb)
		return -1;

	auto* menu = static_cast<QMenu*>(menu_handle);
	QString qlabel = QString::fromUtf8(label);

	QAction* action = menu->addAction(qlabel);
	QObject::connect(action, &QAction::triggered, [cb, ud]() { cb(ud); });

	return 0;
}

const char* PluginManager::api_get_app_version(void* mh)
{
	auto* r = rt(mh);
	r->tempString = BuildConfig.VERSION_STR.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_get_app_name(void* mh)
{
	auto* r = rt(mh);
	r->tempString = BuildConfig.MESHMC_NAME.toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_zip_compress_dir(void* mh, const char* zip,
										const char* dir)
{
	(void)mh;
	if (!zip || !dir)
		return -1;
	bool ok = MMCZip::compressDir(QString::fromUtf8(zip),
								  QString::fromUtf8(dir), nullptr);
	return ok ? 0 : -1;
}

int PluginManager::api_zip_extract(void* mh, const char* zip,
								   const char* target)
{
	(void)mh;
	if (!zip || !target)
		return -1;
	auto result =
		MMCZip::extractDir(QString::fromUtf8(zip), QString::fromUtf8(target));
	return result.has_value() ? 0 : -1;
}

int PluginManager::api_fs_list_dir(void* mh, const char* path, int type,
								   MMCODirEntryCallback cb, void* ud)
{
	(void)mh;
	if (!path || !cb)
		return -1;

	QDir dir(QString::fromUtf8(path));
	if (!dir.exists())
		return -1;

	QDir::Filters filters;
	if (type == 1)
		filters = QDir::Files;
	else if (type == 2)
		filters = QDir::Dirs | QDir::NoDotAndDotDot;
	else
		filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;

	const auto entries = dir.entryInfoList(filters);
	for (const auto& entry : entries) {
		cb(ud, entry.fileName().toUtf8().constData(), entry.isDir() ? 1 : 0);
	}
	return 0;
}

int PluginManager::api_fs_copy_file(void* mh, const char* src, const char* dst)
{
	(void)mh;
	if (!src || !dst)
		return -1;
	return QFile::copy(QString::fromUtf8(src), QString::fromUtf8(dst)) ? 0 : -1;
}

int PluginManager::api_fs_remove(void* mh, const char* path)
{
	(void)mh;
	if (!path)
		return -1;

	QFileInfo fi(QString::fromUtf8(path));
	if (fi.isDir()) {
		return QDir(fi.absoluteFilePath()).removeRecursively() ? 0 : -1;
	}
	return QFile::remove(fi.absoluteFilePath()) ? 0 : -1;
}

int PluginManager::api_fs_mkdir(void* mh, const char* path)
{
	(void)mh;
	if (!path)
		return -1;
	return QDir().mkpath(QString::fromUtf8(path)) ? 0 : -1;
}

int PluginManager::api_fs_exists_abs(void* mh, const char* path)
{
	(void)mh;
	if (!path)
		return 0;
	return QFileInfo::exists(QString::fromUtf8(path)) ? 1 : 0;
}

int64_t PluginManager::api_fs_file_size(void* mh, const char* path)
{
	(void)mh;
	if (!path)
		return -1;
	QFileInfo fi(QString::fromUtf8(path));
	if (!fi.exists())
		return -1;
	return fi.size();
}

int64_t PluginManager::api_get_timestamp(void* mh)
{
	(void)mh;
	return QDateTime::currentSecsSinceEpoch();
}

static std::shared_ptr<ModFolderModel>
resolveModList(PluginManager::ModuleRuntime* r, const char* inst_id,
			   const char* type)
{
	auto* mc = resolveMC(r, inst_id);
	if (!mc || !type)
		return nullptr;

	QString t = QString::fromUtf8(type).toLower();
	if (t == "loader")
		return mc->loaderModList();
	if (t == "core")
		return mc->coreModList();
	if (t == "resourcepack")
		return mc->resourcePackList();
	if (t == "texturepack")
		return mc->texturePackList();
	if (t == "shaderpack")
		return mc->shaderPackList();
	return nullptr;
}

int PluginManager::api_mod_count(void* mh, const char* inst, const char* type)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	return model ? static_cast<int>(model->size()) : 0;
}

const char* PluginManager::api_mod_get_name(void* mh, const char* inst,
											const char* type, int idx)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	if (!model || idx < 0 || idx >= static_cast<int>(model->size()))
		return nullptr;
	r->tempString = model->at(idx).name().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_mod_get_version(void* mh, const char* inst,
											   const char* type, int idx)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	if (!model || idx < 0 || idx >= static_cast<int>(model->size()))
		return nullptr;
	r->tempString = model->at(idx).version().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_mod_get_filename(void* mh, const char* inst,
												const char* type, int idx)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	if (!model || idx < 0 || idx >= static_cast<int>(model->size()))
		return nullptr;
	r->tempString = model->at(idx).filename().fileName().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_mod_get_description(void* mh, const char* inst,
												   const char* type, int idx)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	if (!model || idx < 0 || idx >= static_cast<int>(model->size()))
		return nullptr;
	r->tempString = model->at(idx).description().toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_mod_is_enabled(void* mh, const char* inst,
									  const char* type, int idx)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	if (!model || idx < 0 || idx >= static_cast<int>(model->size()))
		return 0;
	return model->at(idx).enabled() ? 1 : 0;
}

int PluginManager::api_mod_set_enabled(void* mh, const char* inst,
									   const char* type, int idx, int e)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	if (!model || idx < 0 || idx >= static_cast<int>(model->size()))
		return -1;
	QModelIndexList indices;
	indices.append(model->index(idx, 0));
	return model->setModStatus(indices, e ? ModFolderModel::Enable
										  : ModFolderModel::Disable)
			   ? 0
			   : -1;
}

int PluginManager::api_mod_remove(void* mh, const char* inst, const char* type,
								  int idx)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	if (!model || idx < 0 || idx >= static_cast<int>(model->size()))
		return -1;
	QModelIndexList indices;
	indices.append(model->index(idx, 0));
	return model->deleteMods(indices) ? 0 : -1;
}

int PluginManager::api_mod_install(void* mh, const char* inst, const char* type,
								   const char* path)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	if (!model || !path)
		return -1;
	return model->installMod(QString::fromUtf8(path)) ? 0 : -1;
}

int PluginManager::api_mod_refresh(void* mh, const char* inst, const char* type)
{
	auto* r = rt(mh);
	auto model = resolveModList(r, inst, type);
	if (!model)
		return -1;
	return model->update() ? 0 : -1;
}

static std::shared_ptr<WorldList>
resolveWorldList(PluginManager::ModuleRuntime* r, const char* inst_id)
{
	auto* mc = resolveMC(r, inst_id);
	if (!mc)
		return nullptr;
	return mc->worldList();
}

int PluginManager::api_world_count(void* mh, const char* inst)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	return wl ? static_cast<int>(wl->size()) : 0;
}

const char* PluginManager::api_world_get_name(void* mh, const char* inst,
											  int idx)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	if (!wl || idx < 0 || idx >= static_cast<int>(wl->size()))
		return nullptr;
	r->tempString = wl->allWorlds().at(idx).name().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_world_get_folder(void* mh, const char* inst,
												int idx)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	if (!wl || idx < 0 || idx >= static_cast<int>(wl->size()))
		return nullptr;
	r->tempString = wl->allWorlds().at(idx).folderName().toStdString();
	return r->tempString.c_str();
}

int64_t PluginManager::api_world_get_seed(void* mh, const char* inst, int idx)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	if (!wl || idx < 0 || idx >= static_cast<int>(wl->size()))
		return 0;
	return wl->allWorlds().at(idx).seed();
}

int PluginManager::api_world_get_game_type(void* mh, const char* inst, int idx)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	if (!wl || idx < 0 || idx >= static_cast<int>(wl->size()))
		return -1;
	return wl->allWorlds().at(idx).gameType().type;
}

int64_t PluginManager::api_world_get_last_played(void* mh, const char* inst,
												 int idx)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	if (!wl || idx < 0 || idx >= static_cast<int>(wl->size()))
		return 0;
	return wl->allWorlds().at(idx).lastPlayed().toMSecsSinceEpoch();
}

int PluginManager::api_world_delete(void* mh, const char* inst, int idx)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	if (!wl || idx < 0 || idx >= static_cast<int>(wl->size()))
		return -1;
	return wl->deleteWorld(idx) ? 0 : -1;
}

int PluginManager::api_world_rename(void* mh, const char* inst, int idx,
									const char* name)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	if (!wl || idx < 0 || idx >= static_cast<int>(wl->size()) || !name)
		return -1;
	// WorldList doesn't expose rename by index; access the world directly
	auto& worlds = wl->allWorlds();
	World w = worlds.at(idx);
	return w.rename(QString::fromUtf8(name)) ? 0 : -1;
}

int PluginManager::api_world_install(void* mh, const char* inst,
									 const char* path)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	if (!wl || !path)
		return -1;
	wl->installWorld(QFileInfo(QString::fromUtf8(path)));
	return 0;
}

int PluginManager::api_world_refresh(void* mh, const char* inst)
{
	auto* r = rt(mh);
	auto wl = resolveWorldList(r, inst);
	if (!wl)
		return -1;
	return wl->update() ? 0 : -1;
}

int PluginManager::api_account_count(void* mh)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->accounts())
		return 0;
	return app->accounts()->count();
}

const char* PluginManager::api_account_get_profile_name(void* mh, int idx)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->accounts())
		return nullptr;
	auto acc = app->accounts()->at(idx);
	if (!acc)
		return nullptr;
	r->tempString = acc->profileName().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_account_get_profile_id(void* mh, int idx)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->accounts())
		return nullptr;
	auto acc = app->accounts()->at(idx);
	if (!acc)
		return nullptr;
	r->tempString = acc->profileId().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_account_get_type(void* mh, int idx)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->accounts())
		return nullptr;
	auto acc = app->accounts()->at(idx);
	if (!acc)
		return nullptr;
	r->tempString = acc->typeString().toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_account_get_state(void* mh, int idx)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->accounts())
		return -1;
	auto acc = app->accounts()->at(idx);
	if (!acc)
		return -1;
	return static_cast<int>(acc->accountState());
}

int PluginManager::api_account_is_active(void* mh, int idx)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->accounts())
		return 0;
	auto acc = app->accounts()->at(idx);
	return (acc && acc->isActive()) ? 1 : 0;
}

int PluginManager::api_account_get_default_index(void* mh)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->accounts())
		return -1;
	auto def = app->accounts()->defaultAccount();
	if (!def)
		return -1;
	for (int i = 0; i < app->accounts()->count(); ++i) {
		if (app->accounts()->at(i) == def)
			return i;
	}
	return -1;
}

int PluginManager::api_java_count(void* mh)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->javalist() || !app->javalist()->isLoaded())
		return 0;
	return app->javalist()->count();
}

const char* PluginManager::api_java_get_version(void* mh, int idx)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->javalist() || !app->javalist()->isLoaded())
		return nullptr;
	if (idx < 0 || idx >= app->javalist()->count())
		return nullptr;
	auto ver = std::dynamic_pointer_cast<JavaInstall>(app->javalist()->at(idx));
	if (!ver)
		return nullptr;
	r->tempString = ver->id.toString().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_java_get_arch(void* mh, int idx)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->javalist() || !app->javalist()->isLoaded())
		return nullptr;
	if (idx < 0 || idx >= app->javalist()->count())
		return nullptr;
	auto ver = std::dynamic_pointer_cast<JavaInstall>(app->javalist()->at(idx));
	if (!ver)
		return nullptr;
	r->tempString = ver->arch.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_java_get_path(void* mh, int idx)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->javalist() || !app->javalist()->isLoaded())
		return nullptr;
	if (idx < 0 || idx >= app->javalist()->count())
		return nullptr;
	auto ver = std::dynamic_pointer_cast<JavaInstall>(app->javalist()->at(idx));
	if (!ver)
		return nullptr;
	r->tempString = ver->path.toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_java_is_recommended(void* mh, int idx)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->javalist() || !app->javalist()->isLoaded())
		return 0;
	if (idx < 0 || idx >= app->javalist()->count())
		return 0;
	auto ver = std::dynamic_pointer_cast<JavaInstall>(app->javalist()->at(idx));
	return (ver && ver->recommended) ? 1 : 0;
}

const char* PluginManager::api_instance_get_java_version(void* mh,
														 const char* id)
{
	auto* r = rt(mh);
	auto* mc = resolveMC(r, id);
	if (!mc)
		return nullptr;
	r->tempString = mc->getJavaVersion().toString().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_ui_file_open_dialog(void* mh, const char* title,
												   const char* filter)
{
	auto* r = rt(mh);
	QString result = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		title ? QString::fromUtf8(title) : QString(), QString(),
		filter ? QString::fromUtf8(filter) : QString());
	if (result.isEmpty())
		return nullptr;
	r->tempString = result.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_ui_file_save_dialog(void* mh, const char* title,
												   const char* def,
												   const char* filter)
{
	auto* r = rt(mh);
	QString result = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		title ? QString::fromUtf8(title) : QString(),
		def ? QString::fromUtf8(def) : QString(),
		filter ? QString::fromUtf8(filter) : QString());
	if (result.isEmpty())
		return nullptr;
	r->tempString = result.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_ui_input_dialog(void* mh, const char* title,
											   const char* prompt,
											   const char* def)
{
	auto* r = rt(mh);
	bool ok = false;
	QString result = QInputDialog::getText(
		nullptr, title ? QString::fromUtf8(title) : QString(),
		prompt ? QString::fromUtf8(prompt) : QString(), QLineEdit::Normal,
		def ? QString::fromUtf8(def) : QString(), &ok);
	if (!ok)
		return nullptr;
	r->tempString = result.toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_ui_confirm_dialog(void* mh, const char* title,
										 const char* msg)
{
	(void)mh;
	auto ret = QMessageBox::question(
		nullptr, title ? QString::fromUtf8(title) : QString(),
		msg ? QString::fromUtf8(msg) : QString(),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	return ret == QMessageBox::Yes ? 1 : 0;
}

int PluginManager::api_ui_register_instance_action(void* mh, const char* text,
												   const char* tooltip,
												   const char* icon_name,
												   const char* page_id)
{
	(void)mh;
	auto* pm = APPLICATION->pluginManager();
	if (!pm)
		return 0;
	InstanceAction action;
	action.text = text ? QString::fromUtf8(text) : QString();
	action.tooltip = tooltip ? QString::fromUtf8(tooltip) : QString();
	action.iconName = icon_name ? QString::fromUtf8(icon_name) : QString();
	action.pageId = page_id ? QString::fromUtf8(page_id) : QString();
	pm->m_instanceActions.append(action);
	return 1;
}

int PluginManager::api_ui_register_instance_action_cb(
	void* mh, const char* text, const char* tooltip, const char* icon_name,
	void (*cb)(void* ud), void* ud)
{
	(void)mh;
	auto* pm = APPLICATION->pluginManager();
	if (!pm || !cb)
		return 0;
	InstanceCallbackAction action;
	action.text = text ? QString::fromUtf8(text) : QString();
	action.tooltip = tooltip ? QString::fromUtf8(tooltip) : QString();
	action.iconName = icon_name ? QString::fromUtf8(icon_name) : QString();
	action.callback = cb;
	action.userData = ud;
	pm->m_instanceCallbackActions.append(action);
	return 1;
}

#include "ui/pages/BasePage.h"

namespace
{

	class PluginPage : public QWidget, public BasePage
	{
		Q_OBJECT
	  public:
		PluginPage(const QString& pageId, const QString& displayName,
				   const QString& iconName, QWidget* parent = nullptr)
			: QWidget(parent), m_id(pageId), m_displayName(displayName),
			  m_iconName(iconName)
		{
		}

		QString id() const override
		{
			return m_id;
		}
		QString displayName() const override
		{
			return m_displayName;
		}
		QIcon icon() const override
		{
			// Accept either a Qt resource path (":/...") or a themed
			// icon name. Resource paths come from ui_plugin_icon().
			if (m_iconName.startsWith(QLatin1Char(':')))
				return QIcon(m_iconName);
			return QIcon::fromTheme(m_iconName);
		}
		bool shouldDisplay() const override
		{
			return true;
		}

	  private:
		QString m_id;
		QString m_displayName;
		QString m_iconName;
	};

} // anonymous namespace

void* PluginManager::api_ui_page_create(void* mh, const char* id,
										const char* name, const char* iconName)
{
	(void)mh;
	if (!id || !name)
		return nullptr;
	auto* page = new PluginPage(QString::fromUtf8(id), QString::fromUtf8(name),
								iconName ? QString::fromUtf8(iconName)
										 : QStringLiteral("plugin"));
	return static_cast<QWidget*>(page);
}

int PluginManager::api_ui_page_add_to_list(void* mh, void* page, void* list)
{
	(void)mh;
	if (!page || !list)
		return -1;
	auto* pageWidget = static_cast<QWidget*>(page);
	auto* pageBase = dynamic_cast<BasePage*>(pageWidget);
	if (!pageBase)
		return -1;
	auto* pages = static_cast<QList<BasePage*>*>(list);
	pages->append(pageBase);
	return 0;
}

void* PluginManager::api_ui_layout_create(void* mh, void* parent, int type)
{
	(void)mh;
	QWidget* pw = parent ? static_cast<QWidget*>(parent) : nullptr;
	QBoxLayout* layout;
	if (type == 1)
		layout = new QHBoxLayout();
	else
		layout = new QVBoxLayout();
	// Don't set on parent yet — let page_set_layout do that
	(void)pw;
	return layout;
}

int PluginManager::api_ui_layout_add_widget(void* mh, void* layout,
											void* widget)
{
	(void)mh;
	if (!layout || !widget)
		return -1;
	auto* l = static_cast<QBoxLayout*>(layout);
	l->addWidget(static_cast<QWidget*>(widget));
	return 0;
}

int PluginManager::api_ui_layout_add_layout(void* mh, void* parent, void* child)
{
	(void)mh;
	if (!parent || !child)
		return -1;
	auto* p = static_cast<QBoxLayout*>(parent);
	p->addLayout(static_cast<QLayout*>(child));
	return 0;
}

int PluginManager::api_ui_layout_add_spacer(void* mh, void* layout,
											int horizontal)
{
	(void)mh;
	if (!layout)
		return -1;
	auto* l = static_cast<QBoxLayout*>(layout);
	if (horizontal)
		l->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding,
								   QSizePolicy::Minimum));
	else
		l->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum,
								   QSizePolicy::Expanding));
	return 0;
}

int PluginManager::api_ui_page_set_layout(void* mh, void* page, void* layout)
{
	(void)mh;
	if (!page || !layout)
		return -1;
	auto* w = static_cast<QWidget*>(page);
	w->setLayout(static_cast<QLayout*>(layout));
	return 0;
}

void* PluginManager::api_ui_button_create(void* mh, void* parent,
										  const char* text,
										  const char* iconName,
										  MMCOButtonCallback cb, void* ud)
{
	(void)mh;
	auto* btn = new QPushButton(text ? QString::fromUtf8(text) : QString());
	if (iconName && iconName[0] != '\0') {
		const QString iname = QString::fromUtf8(iconName);
		btn->setIcon(iname.startsWith(QLatin1Char(':'))
						 ? QIcon(iname)
						 : QIcon::fromTheme(iname));
	}
	if (parent)
		btn->setParent(static_cast<QWidget*>(parent));
	if (cb) {
		QObject::connect(btn, &QPushButton::clicked, [cb, ud]() { cb(ud); });
	}
	return btn;
}

int PluginManager::api_ui_button_set_enabled(void* mh, void* btn, int enabled)
{
	(void)mh;
	if (!btn)
		return -1;
	static_cast<QPushButton*>(btn)->setEnabled(enabled != 0);
	return 0;
}

int PluginManager::api_ui_button_set_text(void* mh, void* btn, const char* text)
{
	(void)mh;
	if (!btn)
		return -1;
	static_cast<QPushButton*>(btn)->setText(text ? QString::fromUtf8(text)
												 : QString());
	return 0;
}

void* PluginManager::api_ui_label_create(void* mh, void* parent,
										 const char* text)
{
	(void)mh;
	auto* lbl = new QLabel(text ? QString::fromUtf8(text) : QString());
	if (parent)
		lbl->setParent(static_cast<QWidget*>(parent));
	return lbl;
}

int PluginManager::api_ui_label_set_text(void* mh, void* label,
										 const char* text)
{
	(void)mh;
	if (!label)
		return -1;
	static_cast<QLabel*>(label)->setText(text ? QString::fromUtf8(text)
											  : QString());
	return 0;
}

void* PluginManager::api_ui_tree_create(void* mh, void* parent,
										const char** cols, int ncols,
										MMCOTreeSelectionCallback cb, void* ud)
{
	(void)mh;
	auto* tree = new QTreeWidget();
	tree->setRootIsDecorated(false);
	tree->setSortingEnabled(true);
	tree->setAlternatingRowColors(true);
	tree->setSelectionMode(QAbstractItemView::SingleSelection);

	if (parent)
		tree->setParent(static_cast<QWidget*>(parent));

	QStringList headers;
	for (int i = 0; i < ncols; ++i)
		headers << (cols[i] ? QString::fromUtf8(cols[i]) : QString());
	tree->setHeaderLabels(headers);

	// First column stretches
	if (ncols > 0) {
		tree->header()->setStretchLastSection(false);
		tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
		for (int i = 1; i < ncols; ++i)
			tree->header()->setSectionResizeMode(i,
												 QHeaderView::ResizeToContents);
	}

	if (cb) {
		QObject::connect(
			tree, &QTreeWidget::itemSelectionChanged, [tree, cb, ud]() {
				auto items = tree->selectedItems();
				int row = items.isEmpty()
							  ? -1
							  : tree->indexOfTopLevelItem(items.first());
				cb(ud, row);
			});
	}
	return tree;
}

int PluginManager::api_ui_tree_clear(void* mh, void* tree)
{
	(void)mh;
	if (!tree)
		return -1;
	static_cast<QTreeWidget*>(tree)->clear();
	return 0;
}

int PluginManager::api_ui_tree_add_row(void* mh, void* tree, const char** vals,
									   int ncols)
{
	(void)mh;
	if (!tree)
		return -1;
	auto* tw = static_cast<QTreeWidget*>(tree);
	auto* item = new QTreeWidgetItem(tw);
	for (int i = 0; i < ncols; ++i)
		item->setText(i, vals[i] ? QString::fromUtf8(vals[i]) : QString());
	return tw->indexOfTopLevelItem(item);
}

int PluginManager::api_ui_tree_selected_row(void* mh, void* tree)
{
	(void)mh;
	if (!tree)
		return -1;
	auto* tw = static_cast<QTreeWidget*>(tree);
	auto items = tw->selectedItems();
	if (items.isEmpty())
		return -1;
	return tw->indexOfTopLevelItem(items.first());
}

int PluginManager::api_ui_tree_set_row_data(void* mh, void* tree, int row,
											int64_t data)
{
	(void)mh;
	if (!tree)
		return -1;
	auto* tw = static_cast<QTreeWidget*>(tree);
	auto* item = tw->topLevelItem(row);
	if (!item)
		return -1;
	item->setData(0, Qt::UserRole, QVariant::fromValue(data));
	return 0;
}

int64_t PluginManager::api_ui_tree_get_row_data(void* mh, void* tree, int row)
{
	(void)mh;
	if (!tree)
		return 0;
	auto* tw = static_cast<QTreeWidget*>(tree);
	auto* item = tw->topLevelItem(row);
	if (!item)
		return 0;
	return item->data(0, Qt::UserRole).toLongLong();
}

int PluginManager::api_ui_tree_row_count(void* mh, void* tree)
{
	(void)mh;
	if (!tree)
		return 0;
	return static_cast<QTreeWidget*>(tree)->topLevelItemCount();
}

/* ── S15 — Launch Modifiers ───────────────────────────────────────── */

int PluginManager::api_launch_set_env(void* mh, const char* key,
									  const char* value)
{
	auto* r = rt(mh);
	if (!key || !value)
		return -1;
	r->manager->m_pendingLaunchEnv.insert(QString::fromUtf8(key),
										  QString::fromUtf8(value));
	return 0;
}

int PluginManager::api_launch_prepend_wrapper(void* mh, const char* wrapper_cmd)
{
	auto* r = rt(mh);
	if (!wrapper_cmd || wrapper_cmd[0] == '\0')
		return -1;
	QString cmd = QString::fromUtf8(wrapper_cmd);
	if (r->manager->m_pendingLaunchWrapper.isEmpty()) {
		r->manager->m_pendingLaunchWrapper = cmd;
	} else {
		r->manager->m_pendingLaunchWrapper =
			cmd + " " + r->manager->m_pendingLaunchWrapper;
	}
	return 0;
}

void PluginManager::clearPendingLaunchMods()
{
	m_pendingLaunchEnv.clear();
	m_pendingLaunchWrapper.clear();
}

/* ── S16 — Application Settings ───────────────────────────────────── */

const char* PluginManager::api_app_setting_get(void* mh, const char* key)
{
	auto* r = rt(mh);
	auto* app = r->manager->m_app;
	if (!app || !app->settings() || !key)
		return nullptr;

	QString qKey = QString::fromUtf8(key);
	if (!app->settings()->contains(qKey))
		return nullptr;

	QVariant val = app->settings()->get(qKey);
	if (!val.isValid())
		return nullptr;

	r->tempString = val.toString().toStdString();
	return r->tempString.c_str();
}

QMap<QString, QString> PluginManager::takePendingLaunchEnv()
{
	QMap<QString, QString> env;
	env.swap(m_pendingLaunchEnv);
	return env;
}

QString PluginManager::takePendingLaunchWrapper()
{
	QString w;
	w.swap(m_pendingLaunchWrapper);
	return w;
}

/* ── S18 — Plugin Icon Set (ABI 2+) ────────────────────────────────── */

const char* PluginManager::api_ui_plugin_icon(void* mh, const char* name)
{
	auto* r = rt(mh);
	if (!r || !name)
		return nullptr;

	auto& meta = r->manager->m_modules[r->moduleIndex];
	if (meta.iconSetResource.isEmpty())
		return nullptr;

	// Normalise the icon-set name: strip leading ':' or '/'.
	QString setName = meta.iconSetResource;
	while (setName.startsWith(QLatin1Char(':')) ||
		   setName.startsWith(QLatin1Char('/')))
		setName.remove(0, 1);
	// And the leading "plugins/" if the plugin already includes it.
	if (setName.startsWith(QLatin1String("plugins/")))
		setName.remove(0, 8);

	// Candidate paths to try, in order. We accept both common
	// extensions and the bare name so plugins can pass either.
	const QString rawName = QString::fromUtf8(name);
	QStringList candidates;
	candidates << QStringLiteral(":/plugins/%1/%2").arg(setName, rawName);
	if (!rawName.contains(QLatin1Char('.'))) {
		candidates
			<< QStringLiteral(":/plugins/%1/%2.svg").arg(setName, rawName)
			<< QStringLiteral(":/plugins/%1/%2.png").arg(setName, rawName);
	}

	for (const QString& path : candidates) {
		if (QFile::exists(path)) {
			r->tempString = path.toStdString();
			return r->tempString.c_str();
		}
	}

	qWarning().noquote() << "[Plugin:" << meta.name
						 << "] Icon not found:" << rawName << "(set:" << setName
						 << ")";
	return nullptr;
}

/* ── S17 — News API ────────────────────────────────────────────────── */

/*
 * Internal helper: rebuild m_newsCache from the MainWindow's NewsChecker
 * (feed index 0) and any extra feeds registered by plugins.
 * Called lazily on first access and after news_reload().
 */
void PluginManager::rebuildNewsCache()
{
	m_newsCache.clear();

	// Feed 0: default NewsChecker from MainWindow
	if (!m_app)
		return;

	auto* mw = m_app->mainWindow();
	if (mw) {
		auto* checker = mw->newsChecker();
		if (checker) {
			const auto entries = checker->getNewsEntries();
			for (const auto& e : entries) {
				NewsEntryCache c;
				c.feedIndex = 0;
				c.title = e->title;
				c.link = e->link;
				c.content = e->content;
				c.author = e->author;
				c.date = e->pubDate.toString(Qt::ISODate);
				m_newsCache.append(c);
			}
		}
	}

	// Feeds 1..N: extra feeds are appended by api_news_reload callbacks.
	// rebuildNewsCache only populates feed 0; extra feed entries persist
	// in m_newsCache between reloads (appended by the NetJob callbacks).
}

int PluginManager::api_news_get_entry_count(void* mh)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	r->manager->rebuildNewsCache();
	return r->manager->m_newsCache.size();
}

const char* PluginManager::api_news_get_entry_title(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	r->manager->rebuildNewsCache();
	if (index < 0 || index >= r->manager->m_newsCache.size())
		return nullptr;
	r->tempString = r->manager->m_newsCache[index].title.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_news_get_entry_link(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	r->manager->rebuildNewsCache();
	if (index < 0 || index >= r->manager->m_newsCache.size())
		return nullptr;
	r->tempString = r->manager->m_newsCache[index].link.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_news_get_entry_content(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	r->manager->rebuildNewsCache();
	if (index < 0 || index >= r->manager->m_newsCache.size())
		return nullptr;
	r->tempString = r->manager->m_newsCache[index].content.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_news_get_entry_author(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	r->manager->rebuildNewsCache();
	if (index < 0 || index >= r->manager->m_newsCache.size())
		return nullptr;
	r->tempString = r->manager->m_newsCache[index].author.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_news_get_entry_date(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	r->manager->rebuildNewsCache();
	if (index < 0 || index >= r->manager->m_newsCache.size())
		return nullptr;
	r->tempString = r->manager->m_newsCache[index].date.toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_news_get_entry_feed_index(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	r->manager->rebuildNewsCache();
	if (index < 0 || index >= r->manager->m_newsCache.size())
		return -1;
	return r->manager->m_newsCache[index].feedIndex;
}

int PluginManager::api_news_add_feed_url(void* mh, const char* url)
{
	auto* r = rt(mh);
	if (!r || !url)
		return -1;
	QString qUrl = QString::fromUtf8(url);
	if (qUrl.isEmpty())
		return -1;
	if (!r->manager->m_extraFeedUrls.contains(qUrl))
		r->manager->m_extraFeedUrls.append(qUrl);
	return 0;
}

int PluginManager::api_news_get_feed_count(void* mh)
{
	auto* r = rt(mh);
	if (!r)
		return 0;
	// +1 for the default feed
	return 1 + r->manager->m_extraFeedUrls.size();
}

const char* PluginManager::api_news_get_feed_url(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r || index < 0)
		return nullptr;
	if (index == 0) {
		// Default feed URL from BuildConfig
		r->tempString = BuildConfig.NEWS_RSS_URL.toStdString();
		return r->tempString.c_str();
	}
	int extraIdx = index - 1;
	if (extraIdx >= r->manager->m_extraFeedUrls.size())
		return nullptr;
	r->tempString = r->manager->m_extraFeedUrls[extraIdx].toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_news_reload(void* mh)
{
	auto* r = rt(mh);
	if (!r)
		return -1;

	auto* app = r->manager->m_app;
	if (!app)
		return -1;

	// Reload default feed via MainWindow's NewsChecker
	auto* mw = app->mainWindow();
	if (mw) {
		auto* checker = mw->newsChecker();
		if (checker)
			checker->reloadNews();
	}

	// Extra feeds: fetch via NetJob and append to cache
	// We use the existing http_get infrastructure for simplicity.
	// Each extra feed is fetched; on completion we parse and append.
	int feedIdx = 1;
	for (const QString& feedUrl : r->manager->m_extraFeedUrls) {
		const int capturedFeedIdx = feedIdx++;
		QByteArray* buf = new QByteArray();
		NetJob* job =
			new NetJob(QStringLiteral("NewsViewer extra feed"), app->network());
		job->addNetAction(Net::Download::makeByteArray(feedUrl, buf));

		QObject::connect(
			job, &NetJob::succeeded, r->manager,
			[mgr = r->manager, buf, capturedFeedIdx]() {
				QDomDocument doc;
				if (!doc.setContent(*buf)) {
					delete buf;
					return;
				}
				delete buf;

				QDomNodeList items = doc.elementsByTagName("item");
				for (int i = 0; i < items.length(); i++) {
					QDomElement el = items.at(i).toElement();
					auto childText = [&](const QString& tag,
										 const QString& def = {}) -> QString {
						auto nodes = el.elementsByTagName(tag);
						return nodes.count() > 0
								   ? nodes.at(0).toElement().text()
								   : def;
					};
					PluginManager::NewsEntryCache c;
					c.feedIndex = capturedFeedIdx;
					c.title = childText("title", QObject::tr("Untitled"));
					c.content = childText("description");
					c.link = childText("link");
					c.author = childText("dc:creator");
					QString dateStr = childText("pubDate");
					QDateTime dt = QDateTime::fromString(
						dateStr, "ddd, dd MMM yyyy hh:mm:ss");
					c.date = dt.isValid() ? dt.toString(Qt::ISODate) : dateStr;
					mgr->m_newsCache.append(c);
				}
				mgr->dispatchHook(MMCO_HOOK_NEWS_UPDATED);
			});

		QObject::connect(job, &NetJob::failed, r->manager,
						 [buf](const QString&) { delete buf; });

		job->start();
	}

	return 0;
}

/* ═════════════════════════════════════════════════════════════════════
 * S19 — System Tray  /  S20 — Main Window helpers
 * ═════════════════════════════════════════════════════════════════════
 *
 * All resources are tracked per-module so they can be torn down in
 * releaseTrayResourcesForModule() when a plugin is unloaded. We never
 * hand raw QObject pointers to plugins; everything is opaque.
 *
 * Icon resolution mirrors the existing UI builder: a logical name is
 * first looked up via QIcon::fromTheme(), then treated as a Qt
 * resource path. Empty / null inputs produce a null icon (Qt-safe).
 */

namespace
{
	QIcon mmco_resolve_icon(const char* name)
	{
		if (!name || !*name)
			return QIcon();
		QString s = QString::fromUtf8(name);
		QIcon themed = QIcon::fromTheme(s);
		if (!themed.isNull())
			return themed;
		return QIcon(s);
	}

	QSystemTrayIcon::MessageIcon mmco_message_icon(int icon_type)
	{
		switch (icon_type) {
			case 1:
				return QSystemTrayIcon::Information;
			case 2:
				return QSystemTrayIcon::Warning;
			case 3:
				return QSystemTrayIcon::Critical;
			default:
				return QSystemTrayIcon::NoIcon;
		}
	}
} // namespace

QWidget* PluginManager::resolveMainWindow()
{
	if (m_filteredMainWindow)
		return m_filteredMainWindow.data();

	for (auto* w : qApp->topLevelWidgets()) {
		if (w->objectName() == QStringLiteral("MainWindow")) {
			m_filteredMainWindow = w;
			return w;
		}
	}
	return nullptr;
}

void PluginManager::ensureCloseFilterInstalled()
{
	if (m_closeFilterInstalled)
		return;
	QWidget* mw = resolveMainWindow();
	if (!mw)
		return;
	mw->installEventFilter(this);
	m_closeFilterInstalled = true;
}

bool PluginManager::eventFilter(QObject* watched, QEvent* event)
{
	/* Only filter close events on the main window. */
	if (event && event->type() == QEvent::Close && watched &&
		watched == m_filteredMainWindow.data() && !m_closeFilters.isEmpty()) {
		bool swallow = false;
		/* Iterate over a copy: callbacks may install/remove filters. */
		const auto filters = m_closeFilters;
		for (const auto& f : filters) {
			if (!f.cb)
				continue;
			int rc = f.cb(f.user_data);
			if (rc != 0)
				swallow = true;
		}
		if (swallow) {
			auto* ce = static_cast<QCloseEvent*>(event);
			ce->ignore();
			/* Hide rather than close — mirrors what tray-aware apps do. */
			if (auto* mw = qobject_cast<QWidget*>(watched))
				mw->hide();
			return true;
		}
	}
	return QObject::eventFilter(watched, event);
}

void PluginManager::releaseTrayResourcesForModule(void* module_handle)
{
	/*
	 * Two cleanup modes:
	 *
	 *   • Normal runtime unload  → deleteLater() the QObjects so Qt
	 *     unwinds them properly on the next event-loop turn.
	 *
	 *   • Shutdown mode (m_shutdownDone is true *while we walk the
	 *     unload loop*)            → just hide() + sever signal
	 *     connections via the per-tray guard object, but DO NOT delete.
	 *     Calling deleteLater() during Application teardown trips Qt's
	 *     own signal/slot doubly-linked-list bookkeeping the same way
	 *     dlclose() would corrupt it (see the long comment in
	 *     shutdownAll()) — the OS reclaims everything at process exit
	 *     anyway.
	 *
	 * Also: when a single module owns both a QMenu *and* its child
	 * QActions, deleting the menu auto-deletes the actions. To avoid
	 * double-free we delete actions first **and let Qt sever the
	 * parent-child link** before the menu's own deleteLater runs.
	 */

	const bool shuttingDown = m_shutdownDone;

	/* Close filters — pure C-struct entries, safe to drop unconditionally. */
	for (int i = m_closeFilters.size() - 1; i >= 0; --i) {
		if (m_closeFilters[i].module_handle == module_handle)
			m_closeFilters.removeAt(i);
	}
	if (m_closeFilters.isEmpty() && m_closeFilterInstalled &&
		m_filteredMainWindow && !shuttingDown) {
		m_filteredMainWindow->removeEventFilter(this);
		m_closeFilterInstalled = false;
	}

	/* Tray icons — hide first so the platform plugin lets go of any
	 * embedded popup menu reference before we touch the QMenu. */
	for (int i = m_trayIcons.size() - 1; i >= 0; --i) {
		if (m_trayIcons[i].module_handle != module_handle)
			continue;
		auto* icon = m_trayIcons[i].icon;
		auto* guard = m_trayIcons[i].guard;
		if (icon) {
			/* Detach the context menu *before* hiding; some Qt
			 * platforms (XCB tray) re-enter the menu during hide
			 * otherwise. */
			icon->setContextMenu(nullptr);
			icon->hide();
			if (!shuttingDown)
				icon->deleteLater();
		}
		if (guard && !shuttingDown)
			guard->deleteLater();
		m_trayIcons.removeAt(i);
	}

	/* Tray-menu actions — only delete in normal mode, AND only if the
	 * action's parent menu does NOT also belong to this module (the
	 * QMenu's destructor will sweep its own children).  In shutdown
	 * mode we just forget about them; the process is going away. */
	if (!shuttingDown) {
		/* Gather the menu handles owned by this module so we can skip
		 * actions whose parent will be deleted anyway. */
		QSet<QObject*> ownedMenus;
		for (const auto& m : m_trayMenus) {
			if (m.module_handle == module_handle && m.menu)
				ownedMenus.insert(m.menu);
		}
		for (int i = m_trayActions.size() - 1; i >= 0; --i) {
			if (m_trayActions[i].module_handle != module_handle)
				continue;
			QAction* a = m_trayActions[i].action;
			if (a) {
				if (!ownedMenus.contains(a->parent()))
					a->deleteLater();
				/* else: parent menu will deleteLater itself below
				 * and Qt will free this action through QObject's
				 * normal parent-child cascade. */
			}
			m_trayActions.removeAt(i);
		}
	} else {
		/* Shutdown: just drop the records. */
		for (int i = m_trayActions.size() - 1; i >= 0; --i) {
			if (m_trayActions[i].module_handle == module_handle)
				m_trayActions.removeAt(i);
		}
	}

	/* Tray menus.
	 *
	 * Submenus are tracked in m_trayMenus too (added by
	 * api_tray_menu_add_submenu) but their parent is another QMenu in
	 * the same module. To avoid double-free we only call deleteLater
	 * on menus whose parent is NOT one of our own menus — Qt's
	 * parent-child cascade will sweep the rest. */
	if (!shuttingDown) {
		QSet<QObject*> ownedMenus;
		for (const auto& m : m_trayMenus) {
			if (m.module_handle == module_handle && m.menu)
				ownedMenus.insert(m.menu);
		}
		for (int i = m_trayMenus.size() - 1; i >= 0; --i) {
			if (m_trayMenus[i].module_handle != module_handle)
				continue;
			QMenu* menu = m_trayMenus[i].menu;
			if (menu && !ownedMenus.contains(menu->parent()))
				menu->deleteLater();
			m_trayMenus.removeAt(i);
		}
	} else {
		for (int i = m_trayMenus.size() - 1; i >= 0; --i) {
			if (m_trayMenus[i].module_handle == module_handle)
				m_trayMenus.removeAt(i);
		}
	}

	/* S23 — instance running-state callbacks owned by this module.
	 * Deleting each record's guard QObject severs the Qt connection
	 * to BaseInstance::runningStatusChanged automatically. In shutdown
	 * mode we leave the guard alone (same rationale as the tray
	 * section above) and just forget the record. */
	for (int i = m_instanceRunning.size() - 1; i >= 0; --i) {
		if (m_instanceRunning[i].module_handle != module_handle)
			continue;
		QObject* g = m_instanceRunning[i].guard;
		if (g && !shuttingDown)
			g->deleteLater();
		m_instanceRunning.removeAt(i);
	}
}

/* ── ABI 3 — Application signal bridges ──────────────────────────── */

void PluginManager::connectAppSignals()
{
	if (!m_app)
		return;

	/* globalSettingsAboutToOpen → MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN
	 *
	 * Replaces the legacy pattern of plugins doing
	 *   QObject::connect(APPLICATION,
	 *                    &Application::globalSettingsAboutToOpen,
	 *                    g_guard, []{ ... });
	 * which required the plugin to link against Application::staticMetaObject
	 * (i.e. against meshmc.lib / MeshMC_logic). The bridge re-publishes
	 * the signal as a hook so plugins reach it through the C ABI only.
	 *
	 * The connection is parented on `this` (PluginManager is a QObject),
	 * so Qt severs it automatically when PluginManager is destroyed. */
	QObject::connect(m_app, &Application::globalSettingsAboutToOpen, this,
					 [this]() {
						 this->dispatchHook(
							 MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN, nullptr);
					 });

	/* instanceSettingsPageCreated → MMCO_HOOK_INSTANCE_SETTINGS_PAGE_CREATED
	 *
	 * The signal carries (InstanceSettingsPage* page, InstancePtr inst).
	 * We can't expose the launcher types in the payload (would force a
	 * launcher include into plugin source), so we project both pointers
	 * through `void*` slots and let plugins qobject_cast<QWidget*> on
	 * page_handle if they need to interact with the widget.
	 *
	 * We also wire the page's own settingsLoaded / settingsAboutToApply
	 * signals to the matching ABI 3 hooks, so plugins can mirror values
	 * in/out of their custom widgets without needing the launcher
	 * InstanceSettingsPage type. The page itself owns the connection
	 * (anchored on the page) — when the dialog is destroyed the
	 * signal is severed and our hook dispatch stops automatically. */
	QObject::connect(
		m_app, &Application::instanceSettingsPageCreated, this,
		[this](InstanceSettingsPage* page, BaseInstance* inst) {
			MMCOInstanceSettingsPageEvent ev{};
			QByteArray idBytes;
			if (inst) {
				idBytes = inst->id().toUtf8();
				ev.instance_id = idBytes.constData();
				ev.instance_handle = inst;
			}
			ev.page_handle = page;
			this->dispatchHook(MMCO_HOOK_INSTANCE_SETTINGS_PAGE_CREATED, &ev);

			if (!page)
				return;
			/* Capture the raw BaseInstance pointer + a copy of its id
			 * so the lambdas can re-build the event when the page
			 * emits its loaded / about-to-apply edges. The pointer
			 * is owned by the host's InstanceList; the QObject::connect
			 * receiver-anchor (`page`) guarantees the lambda stops
			 * firing once the settings dialog is destroyed. */
			BaseInstance* capturedRaw = inst;
			QByteArray capturedId = idBytes;
			QObject::connect(
				page, &InstanceSettingsPage::settingsLoaded, page,
				[this, page, capturedRaw, capturedId]() {
					MMCOInstanceSettingsPageEvent ev2{};
					if (capturedRaw) {
						ev2.instance_id = capturedId.constData();
						ev2.instance_handle = capturedRaw;
					}
					ev2.page_handle = page;
					this->dispatchHook(
						MMCO_HOOK_INSTANCE_SETTINGS_PAGE_LOADED, &ev2);
				});
			QObject::connect(
				page, &InstanceSettingsPage::settingsAboutToApply, page,
				[this, page, capturedRaw, capturedId]() {
					MMCOInstanceSettingsPageEvent ev2{};
					if (capturedRaw) {
						ev2.instance_id = capturedId.constData();
						ev2.instance_handle = capturedRaw;
					}
					ev2.page_handle = page;
					this->dispatchHook(
						MMCO_HOOK_INSTANCE_SETTINGS_PAGE_APPLYING, &ev2);
				});
		});
}

/* ── S21 — Application Settings (write side, ABI 3+) ─────────────── */

int PluginManager::api_app_setting_set(void* mh, const char* key,
									   const char* value)
{
	auto* r = rt(mh);
	if (!r || !key)
		return -1;
	auto* app = r->manager->m_app;
	if (!app || !app->settings())
		return -1;
	const QString qKey = QString::fromUtf8(key);
	const QString qVal = value ? QString::fromUtf8(value) : QString();
	if (!app->settings()->contains(qKey))
		return -1;
	app->settings()->set(qKey, qVal);
	return 0;
}

int PluginManager::api_app_setting_register(void* mh, const char* key,
											const char* default_value)
{
	auto* r = rt(mh);
	if (!r || !key)
		return -1;
	auto* app = r->manager->m_app;
	if (!app || !app->settings())
		return -1;
	const QString qKey = QString::fromUtf8(key);
	if (app->settings()->contains(qKey))
		return 0; /* already registered — treat as success */
	const QString qDef =
		default_value ? QString::fromUtf8(default_value) : QString();
	app->settings()->registerSetting(qKey, qDef);
	return 0;
}

int PluginManager::api_app_setting_contains(void* mh, const char* key)
{
	auto* r = rt(mh);
	if (!r || !key)
		return 0;
	auto* app = r->manager->m_app;
	if (!app || !app->settings())
		return 0;
	return app->settings()->contains(QString::fromUtf8(key)) ? 1 : 0;
}

/* ── S22 — Themed icon resolution (ABI 3+) ───────────────────────── */

const char* PluginManager::api_ui_themed_icon(void* mh, const char* name)
{
	auto* r = rt(mh);
	if (!r || !name || !*name)
		return nullptr;
	auto* app = r->manager->m_app;
	if (!app)
		return nullptr;

	/* Application::getThemedIcon() returns a QIcon. We can't hand a
	 * QIcon to a C ABI; we instead return a string the existing
	 * icon-name parameters of the UI/tray/menu APIs already accept
	 * (XDG theme name or ":/..." Qt resource path).
	 *
	 * Strategy:
	 *   1. If the launcher's icon list owns an entry under this name,
	 *      return the themed resource path it resolves to.
	 *   2. Otherwise fall back to the bare name — QIcon::fromTheme()
	 *      inside the consumer API will pick it up via the XDG theme. */
	const QString qName = QString::fromUtf8(name);
	if (app->icons()) {
		const QIcon icon = app->icons()->getIcon(qName);
		if (!icon.isNull()) {
			/* QIcon doesn't expose the originating path, so we just
			 * return the logical name — the UI APIs already accept
			 * it and resolve via the same code path. */
			r->tempString = qName.toStdString();
			return r->tempString.c_str();
		}
	}
	r->tempString = qName.toStdString();
	return r->tempString.c_str();
}

/* ── S23 — Instance running-state signal bridge (ABI 3+) ─────────── */

int PluginManager::api_instance_running_register(void* mh,
												 const char* instance_id,
												 MMCOInstanceRunningCallback cb,
												 void* ud)
{
	auto* r = rt(mh);
	if (!r || !instance_id || !cb)
		return -1;
	auto* app = r->manager->m_app;
	if (!app || !app->instances())
		return -1;

	const QString qId = QString::fromUtf8(instance_id);
	auto inst = app->instances()->getInstanceById(qId);
	if (!inst)
		return -1;

	/* Replace any existing registration for this (module, instance)
	 * pair so we never have two callbacks firing for the same edge. */
	auto& vec = r->manager->m_instanceRunning;
	for (int i = vec.size() - 1; i >= 0; --i) {
		if (vec[i].module_handle == mh && vec[i].instanceId == qId) {
			if (vec[i].guard)
				vec[i].guard->deleteLater();
			vec.removeAt(i);
		}
	}

	auto* guard = new QObject();
	InstanceRunningRecord rec{mh, qId, cb, ud, guard};

	/* Capture by value: the bare BaseInstance pointer is what the Qt
	 * connection actually anchors on; instanceId is copied so we
	 * survive instance rename / re-bind. */
	const QByteArray idUtf8 = qId.toUtf8();
	QObject::connect(
		inst.get(), &BaseInstance::runningStatusChanged, guard,
		[cb, ud, idUtf8](bool running) {
			if (cb)
				cb(ud, idUtf8.constData(), running ? 1 : 0);
		});

	vec.append(rec);
	return 0;
}

int PluginManager::api_instance_running_unregister(void* mh,
												   const char* instance_id)
{
	auto* r = rt(mh);
	if (!r || !instance_id)
		return -1;
	const QString qId = QString::fromUtf8(instance_id);
	auto& vec = r->manager->m_instanceRunning;
	for (int i = vec.size() - 1; i >= 0; --i) {
		if (vec[i].module_handle == mh && vec[i].instanceId == qId) {
			if (vec[i].guard)
				vec[i].guard->deleteLater();
			vec.removeAt(i);
		}
	}
	return 0; /* idempotent */
}

/* ── S19 trampolines ─────────────────────────────────────────────── */

void* PluginManager::api_tray_create(void* mh, const char* icon_name,
									 const char* tooltip)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return nullptr;

	auto* tray = new QSystemTrayIcon();
	if (icon_name && *icon_name)
		tray->setIcon(mmco_resolve_icon(icon_name));
	if (tooltip)
		tray->setToolTip(QString::fromUtf8(tooltip));

	TrayRecord rec{mh, tray, new QObject()};
	r->manager->m_trayIcons.append(rec);
	return tray;
}

int PluginManager::api_tray_destroy(void* mh, void* tray_handle)
{
	auto* r = rt(mh);
	if (!r || !tray_handle)
		return -1;
	auto& vec = r->manager->m_trayIcons;
	for (int i = 0; i < vec.size(); ++i) {
		if (vec[i].icon == tray_handle && vec[i].module_handle == mh) {
			if (vec[i].icon) {
				vec[i].icon->hide();
				vec[i].icon->deleteLater();
			}
			if (vec[i].guard)
				vec[i].guard->deleteLater();
			vec.removeAt(i);
			return 0;
		}
	}
	return -1;
}

int PluginManager::api_tray_is_available(void* /*mh*/)
{
	return QSystemTrayIcon::isSystemTrayAvailable() ? 1 : 0;
}

int PluginManager::api_tray_set_icon(void* /*mh*/, void* tray_handle,
									 const char* icon_name)
{
	if (!tray_handle)
		return -1;
	static_cast<QSystemTrayIcon*>(tray_handle)
		->setIcon(mmco_resolve_icon(icon_name));
	return 0;
}

int PluginManager::api_tray_set_tooltip(void* /*mh*/, void* tray_handle,
										const char* tooltip)
{
	if (!tray_handle)
		return -1;
	static_cast<QSystemTrayIcon*>(tray_handle)
		->setToolTip(QString::fromUtf8(tooltip ? tooltip : ""));
	return 0;
}

int PluginManager::api_tray_set_visible(void* /*mh*/, void* tray_handle,
										int visible)
{
	if (!tray_handle)
		return -1;
	static_cast<QSystemTrayIcon*>(tray_handle)->setVisible(visible != 0);
	return 0;
}

int PluginManager::api_tray_show_message(void* mh, void* tray_handle,
										 const char* title, const char* message,
										 int icon_type, int msecs)
{
	if (msecs <= 0)
		msecs = 10000;
	const QString qtitle = QString::fromUtf8(title ? title : "");
	const QString qmsg = QString::fromUtf8(message ? message : "");
	const auto micon = mmco_message_icon(icon_type);

	if (tray_handle) {
		auto* tray = static_cast<QSystemTrayIcon*>(tray_handle);
		const bool wasVisible = tray->isVisible();
		if (!wasVisible)
			tray->show();
		tray->showMessage(qtitle, qmsg, micon, msecs);
		if (!wasVisible) {
			/* Restore prior state on the next event-loop turn so the
			 * notification has a chance to be raised. */
			QTimer::singleShot(msecs + 200, tray, [tray]() {
				if (tray)
					tray->hide();
			});
		}
		return 0;
	}

	/* Fire-and-forget mode — host owns a transient tray icon. */
	auto* r = rt(mh);
	if (!r)
		return -1;
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return -1;
	auto* tray = new QSystemTrayIcon();
	tray->show();
	tray->showMessage(qtitle, qmsg, micon, msecs);
	QTimer::singleShot(msecs + 500, tray, [tray]() {
		if (tray) {
			tray->hide();
			tray->deleteLater();
		}
	});
	return 0;
}

int PluginManager::api_tray_set_menu(void* /*mh*/, void* tray_handle,
									 void* menu_handle)
{
	if (!tray_handle)
		return -1;
	static_cast<QSystemTrayIcon*>(tray_handle)
		->setContextMenu(static_cast<QMenu*>(menu_handle));
	return 0;
}

int PluginManager::api_tray_set_activation_cb(void* mh, void* tray_handle,
											  MMCOTrayActivationCallback cb,
											  void* ud)
{
	auto* r = rt(mh);
	if (!r || !tray_handle)
		return -1;
	auto& vec = r->manager->m_trayIcons;
	for (auto& rec : vec) {
		if (rec.icon != tray_handle || rec.module_handle != mh)
			continue;
		/* Disconnect any previous connection by deleting & recreating
		 * the per-tray guard QObject. Qt severs every signal
		 * connection automatically. */
		if (rec.guard)
			rec.guard->deleteLater();
		rec.guard = new QObject();
		if (!cb)
			return 0;
		QObject::connect(rec.icon, &QSystemTrayIcon::activated, rec.guard,
						 [cb, ud](QSystemTrayIcon::ActivationReason reason) {
							 cb(ud, static_cast<int>(reason));
						 });
		return 0;
	}
	return -1;
}

void* PluginManager::api_tray_menu_create(void* mh)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	auto* menu = new QMenu();
	r->manager->m_trayMenus.append({mh, menu});
	return menu;
}

int PluginManager::api_tray_menu_destroy(void* mh, void* menu_handle)
{
	auto* r = rt(mh);
	if (!r || !menu_handle)
		return -1;
	auto& vec = r->manager->m_trayMenus;
	for (int i = 0; i < vec.size(); ++i) {
		if (vec[i].menu == menu_handle && vec[i].module_handle == mh) {
			/* Drop any actions registered against this menu first. */
			auto& acts = r->manager->m_trayActions;
			for (int j = acts.size() - 1; j >= 0; --j) {
				if (acts[j].action && acts[j].action->parent() ==
										  static_cast<QObject*>(menu_handle)) {
					acts[j].action->deleteLater();
					acts.removeAt(j);
				}
			}
			vec[i].menu->deleteLater();
			vec.removeAt(i);
			return 0;
		}
	}
	return -1;
}

int PluginManager::api_tray_menu_clear(void* /*mh*/, void* menu_handle)
{
	if (!menu_handle)
		return -1;
	static_cast<QMenu*>(menu_handle)->clear();
	return 0;
}

int PluginManager::api_tray_menu_add_separator(void* /*mh*/, void* menu_handle)
{
	if (!menu_handle)
		return -1;
	static_cast<QMenu*>(menu_handle)->addSeparator();
	return 0;
}

void* PluginManager::api_tray_menu_add_action(void* mh, void* menu_handle,
											  const char* label,
											  const char* icon_name,
											  MMCOMenuActionCallback cb,
											  void* ud)
{
	auto* r = rt(mh);
	if (!r || !menu_handle || !label)
		return nullptr;
	auto* menu = static_cast<QMenu*>(menu_handle);
	QAction* act = menu->addAction(QString::fromUtf8(label));
	if (icon_name && *icon_name)
		act->setIcon(mmco_resolve_icon(icon_name));
	if (cb) {
		QObject::connect(act, &QAction::triggered, act, [cb, ud]() { cb(ud); });
	}
	r->manager->m_trayActions.append({mh, act});
	return act;
}

int PluginManager::api_tray_menu_action_set_enabled(void* /*mh*/,
													void* action_handle,
													int enabled)
{
	if (!action_handle)
		return -1;
	static_cast<QAction*>(action_handle)->setEnabled(enabled != 0);
	return 0;
}

int PluginManager::api_tray_menu_action_set_text(void* /*mh*/,
												 void* action_handle,
												 const char* text)
{
	if (!action_handle)
		return -1;
	static_cast<QAction*>(action_handle)
		->setText(QString::fromUtf8(text ? text : ""));
	return 0;
}

void* PluginManager::api_tray_menu_add_submenu(void* mh, void* parent_menu,
											   const char* label,
											   const char* icon_name)
{
	auto* r = rt(mh);
	if (!r || !parent_menu || !label)
		return nullptr;
	auto* parent = static_cast<QMenu*>(parent_menu);
	auto* child = parent->addMenu(QString::fromUtf8(label));
	if (!child)
		return nullptr;
	if (icon_name && *icon_name)
		child->setIcon(mmco_resolve_icon(icon_name));
	/* Track in the per-module registry so shutdown / unload finds it.
	 * The QMenu is parented to `parent` so we don't deleteLater it
	 * during unload — the parent menu's cascade will. */
	r->manager->m_trayMenus.append({mh, child});
	return child;
}

/* ── S20 trampolines ─────────────────────────────────────────────── */

int PluginManager::api_main_window_install_close_filter(
	void* mh, MMCOMainWindowCloseCallback cb, void* user_data)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	auto* self = r->manager;

	if (!cb) {
		/* Clear all filters registered by this module. */
		for (int i = self->m_closeFilters.size() - 1; i >= 0; --i) {
			if (self->m_closeFilters[i].module_handle == mh)
				self->m_closeFilters.removeAt(i);
		}
		if (self->m_closeFilters.isEmpty() && self->m_closeFilterInstalled &&
			self->m_filteredMainWindow) {
			self->m_filteredMainWindow->removeEventFilter(self);
			self->m_closeFilterInstalled = false;
		}
		return 0;
	}

	if (!self->resolveMainWindow())
		return -1;

	self->m_closeFilters.append({mh, cb, user_data});
	self->ensureCloseFilterInstalled();
	return 0;
}

int PluginManager::api_main_window_show(void* mh)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	QWidget* mw = r->manager->resolveMainWindow();
	if (!mw)
		return -1;
	mw->show();
	mw->raise();
	mw->activateWindow();
	return 0;
}

int PluginManager::api_main_window_hide(void* mh)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	QWidget* mw = r->manager->resolveMainWindow();
	if (!mw)
		return -1;
	mw->hide();
	return 0;
}

int PluginManager::api_main_window_is_visible(void* mh)
{
	auto* r = rt(mh);
	if (!r)
		return 0;
	QWidget* mw = r->manager->resolveMainWindow();
	return (mw && mw->isVisible()) ? 1 : 0;
}

/* ── S24 — Per-instance settings (ABI 3+) ────────────────────────── */

namespace
{
/* Resolve an instance pointer by id without polluting the public
 * surface with another helper signature. Returns nullptr if the id
 * does not resolve or the host has no instance list yet. */
InstancePtr resolveInstance(Application* app, const char* instance_id)
{
	if (!app || !app->instances() || !instance_id)
		return {};
	return app->instances()->getInstanceById(QString::fromUtf8(instance_id));
}
} // namespace

const char* PluginManager::api_instance_setting_get(void* mh,
													const char* instance_id,
													const char* key)
{
	auto* r = rt(mh);
	if (!r || !key)
		return nullptr;
	auto inst = resolveInstance(r->manager->m_app, instance_id);
	if (!inst || !inst->settings())
		return nullptr;
	const QString qKey = QString::fromUtf8(key);
	if (!inst->settings()->contains(qKey))
		return nullptr;
	r->tempString = inst->settings()->get(qKey).toString().toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_instance_setting_set(void* mh, const char* instance_id,
											const char* key, const char* value)
{
	auto* r = rt(mh);
	if (!r || !key)
		return -1;
	auto inst = resolveInstance(r->manager->m_app, instance_id);
	if (!inst || !inst->settings())
		return -1;
	const QString qKey = QString::fromUtf8(key);
	const QString qVal = value ? QString::fromUtf8(value) : QString();
	if (!inst->settings()->contains(qKey))
		return -1;
	inst->settings()->set(qKey, qVal);
	return 0;
}

int PluginManager::api_instance_setting_register(void* mh,
												 const char* instance_id,
												 const char* key,
												 const char* default_value)
{
	auto* r = rt(mh);
	if (!r || !key)
		return -1;
	auto inst = resolveInstance(r->manager->m_app, instance_id);
	if (!inst || !inst->settings())
		return -1;
	const QString qKey = QString::fromUtf8(key);
	if (inst->settings()->contains(qKey))
		return 0;
	const QString qDef =
		default_value ? QString::fromUtf8(default_value) : QString();
	inst->settings()->registerSetting(qKey, qDef);
	return 0;
}

int PluginManager::api_instance_setting_register_override(void* mh,
														  const char* instance_id,
														  const char* key,
														  const char* gate_key)
{
	auto* r = rt(mh);
	if (!r || !key || !gate_key)
		return -1;
	auto inst = resolveInstance(r->manager->m_app, instance_id);
	auto* app = r->manager->m_app;
	if (!inst || !inst->settings() || !app || !app->settings())
		return -1;

	const QString qKey = QString::fromUtf8(key);
	const QString qGate = QString::fromUtf8(gate_key);

	/* Ensure the per-instance gate exists. The gate is a bool; we
	 * seed it to false (= use the global default) the first time we
	 * see this instance. */
	auto gate = inst->settings()->getSetting(qGate);
	if (!gate)
		gate = inst->settings()->registerSetting(qGate, false);
	if (!gate)
		return -1;

	/* Don't register the override twice: registerOverride() on the
	 * same key is a no-op-safe in SettingsObject, but checking
	 * contains() lets us short-circuit before we hit it. */
	if (inst->settings()->contains(qKey))
		return 0;
	auto original = app->settings()->getSetting(qKey);
	if (!original)
		return -1;
	inst->settings()->registerOverride(original, gate);
	return 0;
}

int PluginManager::api_instance_setting_reset(void* mh, const char* instance_id,
											  const char* key)
{
	auto* r = rt(mh);
	if (!r || !key)
		return -1;
	auto inst = resolveInstance(r->manager->m_app, instance_id);
	if (!inst || !inst->settings())
		return -1;
	inst->settings()->reset(QString::fromUtf8(key));
	return 0;
}

int PluginManager::api_instance_setting_contains(void* mh,
												 const char* instance_id,
												 const char* key)
{
	auto* r = rt(mh);
	if (!r || !key)
		return 0;
	auto inst = resolveInstance(r->manager->m_app, instance_id);
	if (!inst || !inst->settings())
		return 0;
	return inst->settings()->contains(QString::fromUtf8(key)) ? 1 : 0;
}

/* ── S25 — Account / skin / cape access (ABI 3+) ─────────────────── */

namespace
{
/* Resolve a MinecraftAccountPtr by the same string id S7's
 * account_get_profile_id() returns. Linear scan; the account list is
 * tiny (typically < 5 entries) so this is cheaper than maintaining a
 * cache that needs invalidation. */
MinecraftAccountPtr resolveAccount(Application* app, const char* account_id)
{
	if (!app || !account_id)
		return {};
	auto accounts = app->accounts();
	if (!accounts)
		return {};
	const QString qId = QString::fromUtf8(account_id);
	for (int i = 0; i < accounts->count(); ++i) {
		auto a = accounts->at(i);
		if (a && a->profileId() == qId)
			return a;
	}
	return {};
}
} // namespace

const char* PluginManager::api_account_get_id_by_index(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	auto* app = r->manager->m_app;
	if (!app || !app->accounts())
		return nullptr;
	if (index < 0 || index >= app->accounts()->count())
		return nullptr;
	auto a = app->accounts()->at(index);
	if (!a)
		return nullptr;
	r->tempString = a->profileId().toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_account_is_msa_by_id(void* mh, const char* account_id)
{
	auto* r = rt(mh);
	if (!r)
		return 0;
	auto a = resolveAccount(r->manager->m_app, account_id);
	return (a && a->isMSA()) ? 1 : 0;
}

const char* PluginManager::api_account_get_access_token(void* mh,
														const char* account_id)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a)
		return nullptr;
	r->tempString = a->accessToken().toStdString();
	return r->tempString.c_str();
}

const char*
PluginManager::api_account_get_current_cape_id(void* mh, const char* account_id)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a || !a->accountData())
		return nullptr;
	r->tempString =
		a->accountData()->minecraftProfile.currentCape.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_account_get_skin_variant(void* mh,
														const char* account_id)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a || !a->accountData())
		return nullptr;
	r->tempString =
		a->accountData()->minecraftProfile.skin.variant.toStdString();
	return r->tempString.c_str();
}

/* Skin / cape PNG blobs are returned by stashing a pointer-to-bytes
 * inside the per-module ModuleRuntime alongside the existing
 * tempString slot. We can't reuse tempString (it's a std::string and
 * mangles binary data); a separate QByteArray-shaped cache is needed.
 *
 * To avoid bloating ModuleRuntime with another field we re-use
 * tempString as raw bytes — std::string is byte-clean and its
 * c_str()/data() returns a valid pointer for the configured length.
 * The caller treats it as `void*` so the embedded NULs don't matter.
 * Same lifetime contract as every other getter on this struct: valid
 * until the next API call on the same module. */
int64_t PluginManager::api_account_get_skin_blob(void* mh,
												 const char* account_id,
												 const void** out_ptr)
{
	auto* r = rt(mh);
	if (!r || !out_ptr) {
		if (out_ptr)
			*out_ptr = nullptr;
		return -1;
	}
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a || !a->accountData()) {
		*out_ptr = nullptr;
		return -1;
	}
	const QByteArray& blob = a->accountData()->minecraftProfile.skin.data;
	r->tempString.assign(blob.constData(), blob.size());
	*out_ptr = r->tempString.data();
	return blob.size();
}

int PluginManager::api_account_cape_count(void* mh, const char* account_id)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a || !a->accountData())
		return -1;
	return a->accountData()->minecraftProfile.capes.size();
}

namespace
{
/* Index → cape pair on the active account, using deterministic
 * insertion order (QMap iterates sorted by key, which matches what
 * SkinManagerDialog used to do when it walked the QMap directly). */
const Cape* capeAt(MinecraftAccountPtr a, int index)
{
	if (!a || !a->accountData() || index < 0)
		return nullptr;
	const auto& capes = a->accountData()->minecraftProfile.capes;
	if (index >= capes.size())
		return nullptr;
	int i = 0;
	for (auto it = capes.cbegin(); it != capes.cend(); ++it, ++i) {
		if (i == index)
			return &it.value();
	}
	return nullptr;
}
} // namespace

const char* PluginManager::api_account_cape_get_id(void* mh,
												   const char* account_id,
												   int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	auto a = resolveAccount(r->manager->m_app, account_id);
	const Cape* c = capeAt(a, index);
	if (!c)
		return nullptr;
	r->tempString = c->id.toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_account_cape_get_alias(void* mh,
													  const char* account_id,
													  int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	auto a = resolveAccount(r->manager->m_app, account_id);
	const Cape* c = capeAt(a, index);
	if (!c)
		return nullptr;
	r->tempString = c->alias.toStdString();
	return r->tempString.c_str();
}

int64_t PluginManager::api_account_cape_get_blob(void* mh,
												 const char* account_id,
												 int index,
												 const void** out_ptr)
{
	auto* r = rt(mh);
	if (!r || !out_ptr) {
		if (out_ptr)
			*out_ptr = nullptr;
		return -1;
	}
	auto a = resolveAccount(r->manager->m_app, account_id);
	const Cape* c = capeAt(a, index);
	if (!c) {
		*out_ptr = nullptr;
		return -1;
	}
	r->tempString.assign(c->data.constData(), c->data.size());
	*out_ptr = r->tempString.data();
	return c->data.size();
}

int PluginManager::api_account_set_skin_variant(void* mh,
												const char* account_id,
												const char* variant)
{
	auto* r = rt(mh);
	if (!r || !variant)
		return -1;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a || !a->accountData())
		return -1;
	a->accountData()->minecraftProfile.skin.variant =
		QString::fromUtf8(variant);
	return 0;
}

int PluginManager::api_account_set_current_cape(void* mh,
												const char* account_id,
												const char* cape_id)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a || !a->accountData())
		return -1;
	a->accountData()->minecraftProfile.currentCape =
		cape_id ? QString::fromUtf8(cape_id) : QString();
	return 0;
}

int PluginManager::api_account_set_skin_blob(void* mh, const char* account_id,
											 const void* data, int64_t size)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a || !a->accountData())
		return -1;
	if (data && size > 0)
		a->accountData()->minecraftProfile.skin.data =
			QByteArray(static_cast<const char*>(data),
					   static_cast<int>(size));
	else
		a->accountData()->minecraftProfile.skin.data.clear();
	return 0;
}

/* ── S26 — Synchronous task helpers (ABI 3+) ─────────────────────── */

int PluginManager::api_account_skin_upload(void* mh, const char* account_id,
										   const void* png_bytes, int64_t size,
										   const char* variant)
{
	auto* r = rt(mh);
	if (!r || !png_bytes || size <= 0 || !variant)
		return -1;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a)
		return -1;

	const QByteArray bytes(static_cast<const char*>(png_bytes),
						   static_cast<int>(size));
	const QString variantStr =
		QString::fromUtf8(variant).trimmed().toUpper();
	const SkinUpload::Model model =
		variantStr == QLatin1String("SLIM") ||
				variantStr == QLatin1String("ALEX")
			? SkinUpload::ALEX
			: SkinUpload::STEVE;

	QWidget* parent = QApplication::activeWindow();
	auto task = shared_qobject_ptr<SkinUpload>(
		new SkinUpload(nullptr, a->accessToken(), bytes, model));
	ProgressDialog prog(parent);
	if (prog.execWithTask(task.get()) != QDialog::Accepted)
		return -1;
	return 0;
}

int PluginManager::api_account_skin_reset(void* mh, const char* account_id)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a)
		return -1;

	QWidget* parent = QApplication::activeWindow();
	auto task = shared_qobject_ptr<SkinDelete>(
		new SkinDelete(nullptr, a->accessToken()));
	ProgressDialog prog(parent);
	if (prog.execWithTask(task.get()) != QDialog::Accepted)
		return -1;
	return 0;
}

int PluginManager::api_account_cape_set(void* mh, const char* account_id,
										const char* cape_id)
{
	auto* r = rt(mh);
	if (!r)
		return -1;
	auto a = resolveAccount(r->manager->m_app, account_id);
	if (!a)
		return -1;

	const QString cape = cape_id ? QString::fromUtf8(cape_id) : QString();
	QWidget* parent = QApplication::activeWindow();
	auto task = shared_qobject_ptr<CapeChange>(
		new CapeChange(nullptr, a->accessToken(), cape));
	ProgressDialog prog(parent);
	if (prog.execWithTask(task.get()) != QDialog::Accepted)
		return -1;
	return 0;
}

/* ── S27 — Icon list enumeration (ABI 3+) ─────────────────────── */

int PluginManager::api_icon_list_count(void* mh)
{
	auto* r = rt(mh);
	if (!r)
		return 0;
	auto* app = r->manager->m_app;
	if (!app || !app->icons())
		return 0;
	return app->icons()->rowCount();
}

const char* PluginManager::api_icon_list_get_key(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	auto* app = r->manager->m_app;
	if (!app || !app->icons())
		return nullptr;
	auto* model = app->icons().get();
	if (index < 0 || index >= model->rowCount())
		return nullptr;
	QModelIndex idx = model->index(index, 0);
	r->tempString = model->data(idx, Qt::UserRole).toString().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_icon_list_get_name(void* mh, int index)
{
	auto* r = rt(mh);
	if (!r)
		return nullptr;
	auto* app = r->manager->m_app;
	if (!app || !app->icons())
		return nullptr;
	auto* model = app->icons().get();
	if (index < 0 || index >= model->rowCount())
		return nullptr;
	QModelIndex idx = model->index(index, 0);
	r->tempString = model->data(idx, Qt::DisplayRole).toString().toStdString();
	return r->tempString.c_str();
}

const char* PluginManager::api_icon_list_get_file_path(void* mh,
													   const char* icon_key)
{
	auto* r = rt(mh);
	if (!r || !icon_key)
		return nullptr;
	auto* app = r->manager->m_app;
	if (!app || !app->icons())
		return nullptr;
	const MMCIcon* ic = app->icons()->icon(QString::fromUtf8(icon_key));
	if (!ic)
		return nullptr;
	r->tempString = ic->getFilePath().toStdString();
	return r->tempString.c_str();
}

int PluginManager::api_icon_list_save_png(void* mh, const char* icon_key,
										  const char* dest_path)
{
	auto* r = rt(mh);
	if (!r || !icon_key || !dest_path)
		return -1;
	auto* app = r->manager->m_app;
	if (!app || !app->icons())
		return -1;
	app->icons()->saveIcon(QString::fromUtf8(icon_key),
						   QString::fromUtf8(dest_path), "PNG");
	return QFileInfo::exists(QString::fromUtf8(dest_path)) ? 0 : -1;
}

/* PluginPage MOC — required because PluginPage has Q_OBJECT */
#include "PluginManager.moc"
