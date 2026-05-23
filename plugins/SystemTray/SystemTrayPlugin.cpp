/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 *  SystemTray  —  MeshMC MMCO plugin
 *
 *  Adds a persistent QSystemTrayIcon for the launcher with:
 *    • Show / Hide / Quit actions
 *    • A dynamically rebuilt "Launch instance…" submenu
 *      (up to MAX_INSTANCE_ENTRIES recent instances)
 *    • Optional "minimize to tray" close-event filter
 *
 *  Everything is funnelled through the S19/S20 API surfaces exposed by
 *  PluginManager, so this plugin compiles cleanly against the public
 *  SDK and does not poke at MainWindow internals.
 *
 *  Settings (all booleans, stored under the plugin's namespace):
 *    enabled           — master switch                          (default 1)
 *    minimize_to_tray  — swallow window close, hide instead     (default 0)
 *    show_notifications — show a transient hint on first hide   (default 1)
 */

#include "plugin/sdk/mmco_sdk.h"

/* ── dependencies ─────────────────────────────────────────────────── *
 *
 * SystemTray depends on DesktopNotifier ≥ 1.0.0.
 *
 * Rationale: SystemTray itself only provides the persistent tray icon
 * and its right-click menu — the actual desktop-notification dispatch
 * (instance launched, news updated, …) lives in DesktopNotifier. By
 * declaring a hard dependency we guarantee:
 *
 *   1. DesktopNotifier is loaded *before* SystemTray (the resolver
 *      runs a Kahn topological sort over the dep graph).
 *   2. If the user removes / disables DesktopNotifier, SystemTray
 *      refuses to load with a clear "Required dependency missing"
 *      reason instead of silently dropping notifications.
 *
 * The dependency is "hard" (optional=0). To make it soft, flip
 * optional to 1 — the resolver will then load SystemTray even when
 * DesktopNotifier is absent, but the load order guarantee still holds
 * whenever both are present. */
static const MMCODependency k_systemTrayDeps[] = {
	{"DesktopNotifier", "1.0.0", 0},
};

MMCO_DEFINE_MODULE_EX(
	"SystemTray", "1.0.0", "Project Tick",
	"System tray icon with quick-launch menu and minimize-to-tray support",
	"Apache-2.0",
	/* code_link        */ nullptr,
	/* icon_set         */ nullptr,
	/* dependencies     */ k_systemTrayDeps,
	/* dependency_count */ 1u,
	/* signing_key_id   */ nullptr);

/* ── module-local state ───────────────────────────────────────────── */

static MMCOContext* g_ctx = nullptr;
static void* g_tray = nullptr;		 /* QSystemTrayIcon*           */
static void* g_menu = nullptr;		 /* QMenu*                     */
static void* g_launchMenu = nullptr; /* QMenu* (submenu)        */
static void* g_showAction = nullptr;
static void* g_hideAction = nullptr;
static void* g_quitAction = nullptr;
static QObject* g_guard = nullptr; /* anchor for our Qt connections */
static QCheckBox* g_enabledCheckbox = nullptr;

/* The launcher-wide setting key we mirror our "enabled" plugin-local
 * setting onto. Lives in APPLICATION->settings() so it's visible in
 * Settings → MeshMC and can be toggled by the user without poking at
 * raw config files. The plugin still treats its own plugin-namespaced
 * "enabled" setting as the runtime source of truth — the global key
 * just drives the UI checkbox and is mirrored back into the plugin
 * namespace whenever the user flips it. */
static constexpr const char SETTING_GLOBAL_ENABLED[] =
	"plugin.system_tray.Enabled";

static constexpr int MAX_INSTANCE_ENTRIES = 8;

/* Stable copies of instance IDs (the API guarantees the returned C
 * string only until the next call on the same module — we have to copy
 * before stashing for callbacks). */
struct InstanceEntry {
	std::string id;
	std::string name;
};
static QVector<InstanceEntry> g_launchEntries;

/* Per-action user-data wrapper passed to the C-style callback. We allocate
 * one per entry and free them all when the submenu is rebuilt. */
struct LaunchUserData {
	int entryIndex;
};
static QVector<LaunchUserData*> g_launchUserData;

/* ── settings helpers ─────────────────────────────────────────────── */

static bool settingBool(const char* key, bool fallback)
{
	if (!g_ctx)
		return fallback;
	const char* v = g_ctx->setting_get(g_ctx->module_handle, key);
	if (!v)
		return fallback;
	QString s = QString::fromUtf8(v).trimmed().toLower();
	if (s.isEmpty())
		return fallback;
	return s == "1" || s == "true" || s == "yes" || s == "on";
}

static void settingSetBool(const char* key, bool value)
{
	if (!g_ctx)
		return;
	g_ctx->setting_set(g_ctx->module_handle, key, value ? "1" : "0");
}

/* ── action callbacks (C-linkage style) ───────────────────────────── */

static void on_show_clicked(void* /*ud*/)
{
	if (g_ctx)
		g_ctx->main_window_show(g_ctx->module_handle);
}

static void on_hide_clicked(void* /*ud*/)
{
	if (g_ctx)
		g_ctx->main_window_hide(g_ctx->module_handle);
}

static void on_quit_clicked(void* /*ud*/)
{
	/* QCoreApplication::quit() is the cleanest path — it tears down the
	 * event loop which in turn unwinds MeshMC's Application shutdown,
	 * giving PluginManager a chance to mmco_unload() us properly. */
	QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
}

static void on_launch_entry(void* ud)
{
	if (!g_ctx || !ud)
		return;
	auto* data = static_cast<LaunchUserData*>(ud);
	if (data->entryIndex < 0 || data->entryIndex >= g_launchEntries.size())
		return;
	const std::string& id = g_launchEntries[data->entryIndex].id;
	g_ctx->instance_launch(g_ctx->module_handle, id.c_str(), /*online=*/1);
}

/* ── tray activation: left-click toggles the main window ──────────── */

static void on_tray_activated(void* /*ud*/, int reason)
{
	/* reason: 1=Trigger (single), 2=DoubleClick, 3=MiddleClick, 4=Context.
	 * On X11 single-click is the natural "show window" gesture; on
	 * Windows/macOS it is double-click. We respond to both. */
	if (reason != 1 && reason != 2)
		return;
	if (!g_ctx)
		return;
	if (g_ctx->main_window_is_visible(g_ctx->module_handle))
		g_ctx->main_window_hide(g_ctx->module_handle);
	else
		g_ctx->main_window_show(g_ctx->module_handle);
}

/* ── close-filter: swallow QCloseEvent when minimize_to_tray is on ── */

static bool g_hintShown = false;

static int on_main_window_close(void* /*ud*/)
{
	if (!g_ctx)
		return 0;
	if (!settingBool("minimize_to_tray", false))
		return 0;
	/* Show a one-time hint so the user knows the launcher is still
	 * running in the tray. */
	if (!g_hintShown && settingBool("show_notifications", true)) {
		g_hintShown = true;
		g_ctx->tray_show_message(g_ctx->module_handle, g_tray,
								 "MeshMC is still running",
								 "Use the tray icon to bring the window back "
								 "or quit the launcher.",
								 1 /* Info */, 6000);
	}
	return 1; /* swallow → host will hide() the main window */
}

/* ── launch submenu rebuilding ────────────────────────────────────── */

static void rebuild_launch_submenu()
{
	if (!g_ctx || !g_launchMenu)
		return;

	/* Free old per-entry user-data and clear the menu. */
	for (auto* ud : g_launchUserData)
		delete ud;
	g_launchUserData.clear();
	g_launchEntries.clear();
	g_ctx->tray_menu_clear(g_ctx->module_handle, g_launchMenu);

	int total = g_ctx->instance_count(g_ctx->module_handle);
	int shown = 0;
	for (int i = 0; i < total && shown < MAX_INSTANCE_ENTRIES; ++i) {
		const char* id = g_ctx->instance_get_id(g_ctx->module_handle, i);
		if (!id)
			continue;
		std::string idCopy = id;
		const char* name =
			g_ctx->instance_get_name(g_ctx->module_handle, idCopy.c_str());
		std::string nameCopy = name ? name : idCopy;

		InstanceEntry e;
		e.id = idCopy;
		e.name = nameCopy;
		g_launchEntries.push_back(e);

		auto* ud = new LaunchUserData{shown};
		g_launchUserData.push_back(ud);

		g_ctx->tray_menu_add_action(g_ctx->module_handle, g_launchMenu,
									nameCopy.c_str(), /*icon=*/nullptr,
									on_launch_entry, ud);
		++shown;
	}

	if (shown == 0) {
		/* Add a disabled placeholder so the submenu is never empty. */
		void* placeholder = g_ctx->tray_menu_add_action(
			g_ctx->module_handle, g_launchMenu, "(no instances)", nullptr,
			nullptr, nullptr);
		if (placeholder)
			g_ctx->tray_menu_action_set_enabled(g_ctx->module_handle,
												placeholder, 0);
	}
}

/* ── Settings UI injection ────────────────────────────────────────── */

/* Walk qApp->allWidgets() for the MeshMCPage (the first tab on the
 * global Settings dialog). Same pattern BackupSystem and GitVersioning
 * use — the page is rebuilt every time the dialog opens, so we have
 * to re-find it and re-inject after every globalSettingsAboutToOpen.
 *
 * The injected checkbox is wired to APPLICATION->settings()
 * "plugin.system_tray.Enabled". Flipping it doesn't tear down or
 * re-initialise the plugin live (Qt has no graceful way to reverse
 * mmco_init mid-session) — instead we explain that the change takes
 * effect after restart, and on the next launcher startup mmco_init
 * sees the new value and either skips itself or comes up normally. */
static void injectCheckboxIntoMeshMCPage()
{
	QWidget* meshMCPage = nullptr;
	for (auto* w : qApp->allWidgets()) {
		if (w->objectName() == QStringLiteral("MeshMCPage")) {
			meshMCPage = w;
			break;
		}
	}
	if (!meshMCPage)
		return;

	auto* layout =
		meshMCPage->findChild<QVBoxLayout*>(QStringLiteral("verticalLayout_9"));
	if (!layout)
		return;

	auto* groupBox = new QGroupBox(QObject::tr("System Tray"));
	groupBox->setObjectName(QStringLiteral("systemTrayGroupBox"));
	auto* gl = new QVBoxLayout(groupBox);

	g_enabledCheckbox = new QCheckBox(
		QObject::tr("Show MeshMC system tray icon (Restart required.)"),
		groupBox);
	g_enabledCheckbox->setObjectName(QStringLiteral("systemTrayEnabledCheck"));
	g_enabledCheckbox->setToolTip(QObject::tr(
		"When on, MeshMC keeps a persistent tray icon with quick-launch "
		"shortcuts and an optional minimise-to-tray close handler.\n\n"
		"Toggle takes effect after restarting MeshMC."));
	gl->addWidget(g_enabledCheckbox);

	int spacerIdx = layout->count() - 1;
	layout->insertWidget(spacerIdx, groupBox);

	bool current = false;
	if (g_ctx) {
		const char* v = g_ctx->app_setting_get(g_ctx->module_handle,
											   SETTING_GLOBAL_ENABLED);
		if (v) {
			QString s = QString::fromUtf8(v).trimmed().toLower();
			current = s == QLatin1String("1") || s == QLatin1String("true") ||
					  s == QLatin1String("yes") || s == QLatin1String("on");
		}
	}
	g_enabledCheckbox->setChecked(current);

	QObject::connect(
		g_enabledCheckbox, &QCheckBox::toggled, g_guard, [](bool checked) {
			if (!g_ctx)
				return;
			g_ctx->app_setting_set(g_ctx->module_handle, SETTING_GLOBAL_ENABLED,
								   checked ? "1" : "0");
			settingSetBool("enabled", checked);
		});
}

/* Hook handler for MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN — replaces
 * the legacy direct connect to Application::globalSettingsAboutToOpen. */
static int on_global_settings_about_to_open(void*, uint32_t, void*, void*)
{
	g_enabledCheckbox = nullptr;
	QTimer::singleShot(0, qApp, injectCheckboxIntoMeshMCPage);
	return 0;
}

static int on_app_initialized(void*, uint32_t, void*, void*)
{
	/* Re-injection is now triggered via the hook above; this handler
	 * stays around as a placeholder so we can wire it up next to the
	 * other APP_INITIALIZED-dependent state if needed. */
	return 0;
}

/* ── hooks ────────────────────────────────────────────────────────── */

static int on_ui_main_ready(void* /*mh*/, uint32_t /*hook_id*/,
							void* /*payload*/, void* /*ud*/)
{
	if (!g_ctx)
		return 0;

	/* Install the close-event filter unconditionally — the callback
	 * itself short-circuits when the setting is off. This way the
	 * setting can be toggled at runtime without re-registering. */
	g_ctx->main_window_install_close_filter(g_ctx->module_handle,
											on_main_window_close, nullptr);

	/* Refresh the submenu now that the UI is up — instance list is ready. */
	rebuild_launch_submenu();
	return 0;
}

static int on_instance_created(void* /*mh*/, uint32_t /*hook_id*/,
							   void* /*payload*/, void* /*ud*/)
{
	rebuild_launch_submenu();
	return 0;
}

static int on_instance_removed(void* /*mh*/, uint32_t /*hook_id*/,
							   void* /*payload*/, void* /*ud*/)
{
	rebuild_launch_submenu();
	return 0;
}

/* ── lifecycle ────────────────────────────────────────────────────── */

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;
	MMCO_LOG(ctx, "SystemTray initializing...");

	/* Lifetime anchor for our Qt connections (settings-page injection,
	 * checkbox toggled signal). We intentionally never delete this;
	 * Qt may still have queued events targeting it at shutdown. */
	g_guard = new QObject();

	/* Mirror the plugin-local "enabled" key onto a launcher-wide
	 * setting so the user can toggle it from Settings → MeshMC. The
	 * global setting wins on conflict — every plugin-local read
	 * delegates here first. */
	if (!ctx->app_setting_contains(ctx->module_handle,
								   SETTING_GLOBAL_ENABLED)) {
		/* First run — seed from the plugin-local value (if any), otherwise
		 * default ON. */
		ctx->app_setting_register(ctx->module_handle, SETTING_GLOBAL_ENABLED,
								  settingBool("enabled", true) ? "1" : "0");
	}
	bool globalEnabled = false;
	{
		const char* v =
			ctx->app_setting_get(ctx->module_handle, SETTING_GLOBAL_ENABLED);
		if (v) {
			QString s = QString::fromUtf8(v).trimmed().toLower();
			globalEnabled =
				s == QLatin1String("1") || s == QLatin1String("true") ||
				s == QLatin1String("yes") || s == QLatin1String("on");
		}
	}
	/* Re-sync the plugin-local copy so existing call-sites see the
	 * canonical answer. */
	settingSetBool("enabled", globalEnabled);
	if (!globalEnabled) {
		MMCO_LOG(ctx, "SystemTray: disabled via global setting; idle "
					  "(re-enable from Settings → MeshMC).");
		/* Still wire up the hook so we can inject the checkbox — the
		 * user needs a way to flip it back on. */
		ctx->hook_register(ctx->module_handle, MMCO_HOOK_APP_INITIALIZED,
						   on_app_initialized, nullptr);
		ctx->hook_register(ctx->module_handle,
						   MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN,
						   on_global_settings_about_to_open, nullptr);
		return 0;
	}

	if (!ctx->tray_is_available(ctx->module_handle)) {
		MMCO_WARN(ctx, "SystemTray: host has no system tray available; idle.");
		return 0;
	}

	/* Default setting values (only set when missing — so we don't overwrite
	 * a user choice on re-init). */
	if (!ctx->setting_get(ctx->module_handle, "enabled"))
		settingSetBool("enabled", true);
	/* If the user installs SystemTray they almost always want the
	 * close-to-tray behaviour — otherwise the plugin is just a quit
	 * button.  Default to ON; users who want vanilla "close == quit"
	 * can flip the setting in the config file. */
	if (!ctx->setting_get(ctx->module_handle, "minimize_to_tray"))
		settingSetBool("minimize_to_tray", true);
	if (!ctx->setting_get(ctx->module_handle, "show_notifications"))
		settingSetBool("show_notifications", true);

	/* Resolve the launcher's own logo through the SDK — the icon is
	 * baked into MeshMC's resource bundle at ":/org.projecttick.MeshMC.svg"
	 * and is loaded directly. We try a few fallbacks so the tray still
	 * gets a sensible icon on stripped/older builds. */
	const char* iconCandidates[] = {
		":/org.projecttick.MeshMC.svg",			 /* primary — MeshMC logo  */
		":/multimc/scalable/instances/logo.svg", /* instance default */
		"meshmc",								 /* themed name (XDG)      */
		"applications-games",					 /* last-ditch fallback    */
	};
	g_tray = nullptr;
	for (const char* name : iconCandidates) {
		g_tray = ctx->tray_create(ctx->module_handle, name, "MeshMC");
		if (g_tray)
			break;
	}
	if (!g_tray) {
		MMCO_ERR(ctx, "SystemTray: tray_create() failed.");
		return 0;
	}

	/* Build the menu.
	 *
	 * Layout (top → bottom, the way most launchers do it):
	 *   Open MeshMC               ← primary action, picks Show or Hide
	 *   Hide window
	 *   ─────────────────────
	 *   Launch instance ▸
	 *       <Instance 1>
	 *       <Instance 2>
	 *       …
	 *   ─────────────────────
	 *   Quit MeshMC
	 *
	 * The instance list is its own submenu so refreshing it on
	 * INSTANCE_CREATED/REMOVED never touches Show/Hide/Quit — and so
	 * Wayland's StatusNotifierItem implementation doesn't have to
	 * cope with a long flat menu of unknown length. */
	g_menu = ctx->tray_menu_create(ctx->module_handle);

	g_showAction =
		ctx->tray_menu_add_action(ctx->module_handle, g_menu, "Open MeshMC",
								  nullptr, on_show_clicked, nullptr);
	g_hideAction =
		ctx->tray_menu_add_action(ctx->module_handle, g_menu, "Hide window",
								  nullptr, on_hide_clicked, nullptr);
	ctx->tray_menu_add_separator(ctx->module_handle, g_menu);

	g_launchMenu = ctx->tray_menu_add_submenu(ctx->module_handle, g_menu,
											  "Launch instance", nullptr);

	ctx->tray_menu_add_separator(ctx->module_handle, g_menu);
	g_quitAction =
		ctx->tray_menu_add_action(ctx->module_handle, g_menu, "Quit MeshMC",
								  nullptr, on_quit_clicked, nullptr);

	ctx->tray_set_menu(ctx->module_handle, g_tray, g_menu);
	ctx->tray_set_activation_cb(ctx->module_handle, g_tray, on_tray_activated,
								nullptr);
	ctx->tray_set_visible(ctx->module_handle, g_tray, 1);

	/* Hooks. */
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_APP_INITIALIZED,
					   on_app_initialized, nullptr);
	ctx->hook_register(ctx->module_handle,
					   MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN,
					   on_global_settings_about_to_open, nullptr);
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_UI_MAIN_READY,
					   on_ui_main_ready, nullptr);
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_INSTANCE_CREATED,
					   on_instance_created, nullptr);
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_INSTANCE_REMOVED,
					   on_instance_removed, nullptr);

	MMCO_LOG(ctx, "SystemTray initialized.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx)
		MMCO_LOG(g_ctx, "SystemTray unloading.");

	for (auto* ud : g_launchUserData)
		delete ud;
	g_launchUserData.clear();
	g_launchEntries.clear();

	/* PluginManager will sweep up the tray/menu/actions/close-filter on
	 * its own — see releaseTrayResourcesForModule(). We just drop our
	 * raw handles so we never touch them again. */
	g_tray = nullptr;
	g_menu = nullptr;
	g_launchMenu = nullptr;
	g_showAction = nullptr;
	g_hideAction = nullptr;
	g_quitAction = nullptr;
	g_ctx = nullptr;
}

} /* extern "C" */
