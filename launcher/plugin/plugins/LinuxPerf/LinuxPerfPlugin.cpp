/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: Apache-2.0
 */

#include "plugin/sdk/mmco_cxx_sdk.hpp"
#include "vendor/gamemode_client.h"
#include <QFileInfo>
#include <QStandardPaths>

MMCO_DEFINE_MODULE("Linux Performance Tools", "1.2.0", "Project Tick",
				   "MangoHud FPS overlay and GameMode performance integration "
				   "for Minecraft on Linux",
				   "Apache-2.0");

static constexpr const char SETTING_MANGOHUD[] =
	"plugin.linuxperf.mangohud.enabled";
static constexpr const char SETTING_GAMEMODE[] =
	"plugin.linuxperf.gamemode.enabled";
static constexpr const char SETTING_INSTANCE_OVERRIDE[] =
	"plugin.linuxperf.override";

static MMCOContext* g_ctx = nullptr;
static QObject* g_guard = nullptr;

static QCheckBox* g_mangoCheckbox = nullptr;
static QCheckBox* g_gamemodeCheckbox = nullptr;

static bool is_flatpak()
{
	return QFile::exists(QStringLiteral("/.flatpak-info"));
}

static QString flatpak_mangohud_executable()
{
	const QStringList candidates = {
		QStringLiteral("/usr/lib/extensions/vulkan/MangoHud/bin/mangohud"),
		QStringLiteral(
			"/usr/lib/extensions/vulkan/"
			"org.freedesktop.Platform.VulkanLayer.MangoHud/bin/mangohud")};

	for (const auto& candidate : candidates) {
		QFileInfo info(candidate);
		if (info.exists() && info.isExecutable())
			return candidate;
	}

	return {};
}

static QString mangohud_executable()
{
	if (is_flatpak()) {
		auto flatpakExecutable = flatpak_mangohud_executable();
		if (!flatpakExecutable.isEmpty())
			return flatpakExecutable;
	}

	return QStandardPaths::findExecutable(QStringLiteral("mangohud"));
}

static QString gamemoderun_executable()
{
	return QStandardPaths::findExecutable(QStringLiteral("gamemoderun"));
}

static bool mangohud_available()
{
	return !mangohud_executable().isEmpty();
}

static bool gamemoderun_available()
{
	return !gamemoderun_executable().isEmpty();
}

static QString mangohud_missing_tooltip()
{
	if (is_flatpak()) {
		return QObject::tr(
			"MangoHud is not mounted inside this Flatpak sandbox.\n"
			"Install the org.freedesktop.Platform.VulkanLayer.MangoHud branch\n"
			"that matches the base runtime and restart MeshMC. A mismatched\n"
			"branch will not appear under /usr/lib/extensions/vulkan.");
	}

	return QObject::tr(
		"MangoHud is not installed or not found on PATH.\n"
		"Install it (package: mangohud) and reopen this dialog.");
}

static QString gamemode_missing_tooltip()
{
	if (is_flatpak()) {
		return QObject::tr(
			"GameMode is not available inside this Flatpak sandbox.\n"
			"Bundle the GameMode client tools in the Flatpak and allow access "
			"to\n"
			"com.feralinteractive.GameMode on the session bus.");
	}

	return QObject::tr(
		"gamemoderun is not installed or not found on PATH.\n"
		"Install GameMode and restart MeshMC to enable this option.");
}

static QString gamemode_status_text()
{
	int status = gamemode_query_status();
	QString gmBin = gamemoderun_executable();
	if (status >= 0) {
		return QObject::
			tr("GameMode daemon: running — will activate at game launch  (%1)")
				.arg(gmBin);
	}

	if (is_flatpak()) {
		return QObject::tr("GameMode daemon: not reachable from sandbox — add "
						   "session bus access to\n"
						   "com.feralinteractive.GameMode and ensure the host "
						   "gamemoded service is running");
	}

	return QObject::tr(
		"GameMode daemon: not responding — run \"systemctl --user enable --now "
		"gamemoded\" or check gamemode install");
}

static bool ctxBoolApp(const char* key)
{
	if (!g_ctx)
		return false;
	if (!g_ctx->app_setting_contains(g_ctx->module_handle, key))
		return false;
	const char* v = g_ctx->app_setting_get(g_ctx->module_handle, key);
	if (!v)
		return false;
	QString s = QString::fromUtf8(v).trimmed().toLower();
	return s == QLatin1String("1") || s == QLatin1String("true") ||
		   s == QLatin1String("yes") || s == QLatin1String("on");
}

static bool ctxBoolInstance(const char* instanceId, const char* key)
{
	if (!g_ctx || !instanceId)
		return false;
	if (!g_ctx->instance_setting_contains(g_ctx->module_handle, instanceId,
										  key))
		return false;
	const char* v =
		g_ctx->instance_setting_get(g_ctx->module_handle, instanceId, key);
	if (!v)
		return false;
	QString s = QString::fromUtf8(v).trimmed().toLower();
	return s == QLatin1String("1") || s == QLatin1String("true") ||
		   s == QLatin1String("yes") || s == QLatin1String("on");
}

static void ensureSettingsRegistered()
{
	if (!g_ctx)
		return;
	auto ensureKey = [&](const char* key) {
		if (!g_ctx->app_setting_contains(g_ctx->module_handle, key))
			g_ctx->app_setting_register(g_ctx->module_handle, key, "0");
	};
	ensureKey(SETTING_MANGOHUD);
	ensureKey(SETTING_GAMEMODE);
}

static bool isMangohudEnabled()
{
	return ctxBoolApp(SETTING_MANGOHUD);
}

static bool isGamemodeEnabled()
{
	return ctxBoolApp(SETTING_GAMEMODE);
}

static void ensureInstanceSettingsRegistered(const char* instanceId)
{
	if (!g_ctx || !instanceId)
		return;

	ensureSettingsRegistered();

	if (!g_ctx->instance_setting_contains(g_ctx->module_handle, instanceId,
										  SETTING_INSTANCE_OVERRIDE)) {
		g_ctx->instance_setting_register(g_ctx->module_handle, instanceId,
										 SETTING_INSTANCE_OVERRIDE, "0");
	}
	g_ctx->instance_setting_register_override(g_ctx->module_handle, instanceId,
											  SETTING_MANGOHUD,
											  SETTING_INSTANCE_OVERRIDE);
	g_ctx->instance_setting_register_override(g_ctx->module_handle, instanceId,
											  SETTING_GAMEMODE,
											  SETTING_INSTANCE_OVERRIDE);
}

static bool isMangohudEnabledForInstance(const char* instanceId)
{
	if (!instanceId)
		return isMangohudEnabled();
	ensureInstanceSettingsRegistered(instanceId);
	return ctxBoolInstance(instanceId, SETTING_MANGOHUD);
}

static bool isGamemodeEnabledForInstance(const char* instanceId)
{
	if (!instanceId)
		return isGamemodeEnabled();
	ensureInstanceSettingsRegistered(instanceId);
	return ctxBoolInstance(instanceId, SETTING_GAMEMODE);
}

static void injectCheckboxesIntoMinecraftPage()
{
	/* Locate the MinecraftPage widget by objectName (set in the .ui file). */
	QWidget* mcPage = nullptr;
	for (auto* w : qApp->allWidgets()) {
		if (w->objectName() == QStringLiteral("MinecraftPage")) {
			mcPage = w;
			break;
		}
	}
	if (!mcPage)
		return;

	/* Find the vertical layout that hosts the Minecraft tab content. */
	auto* layout =
		mcPage->findChild<QVBoxLayout*>(QStringLiteral("verticalLayout_3"));
	if (!layout)
		return;

	/* Skip injection if our group box is already present (re-open guard). */
	if (mcPage->findChild<QGroupBox*>(QStringLiteral("linuxPerfGroupBox")))
		return;

	auto* groupBox = new QGroupBox(QObject::tr("Linux Performance Tools"));
	groupBox->setObjectName(QStringLiteral("linuxPerfGroupBox"));
	auto* groupLayout = new QVBoxLayout(groupBox);

	g_mangoCheckbox = new QCheckBox(
		QObject::tr("Enable MangoHud overlay (FPS / GPU / CPU metrics)"),
		groupBox);
	g_mangoCheckbox->setObjectName(QStringLiteral("linuxPerfMangoHudCheck"));
	const bool mangoAvail = mangohud_available();
	if (mangoAvail) {
		QString mangoBin = mangohud_executable();
		g_mangoCheckbox->setToolTip(
			QObject::tr(
				"Injects the MangoHud overlay into Minecraft via the mangohud "
				"wrapper.\n"
				"Displays real-time FPS, frame timing, GPU and CPU metrics.\n"
				"Works with both OpenGL and Vulkan (LWJGL2 / LWJGL3).\n\n"
				"Found: %1")
				.arg(mangoBin));
	} else {
		g_mangoCheckbox->setToolTip(mangohud_missing_tooltip());
	}
	g_mangoCheckbox->setEnabled(mangoAvail);
	g_mangoCheckbox->setChecked(isMangohudEnabled() && mangoAvail);
	groupLayout->addWidget(g_mangoCheckbox);

	g_gamemodeCheckbox = new QCheckBox(
		QObject::tr(
			"Enable GameMode (CPU / scheduler performance optimisations)"),
		groupBox);
	g_gamemodeCheckbox->setObjectName(QStringLiteral("linuxPerfGameModeCheck"));
	g_gamemodeCheckbox->setToolTip(QObject::tr(
		"Launches Minecraft via gamemoderun so that Feral Interactive's\n"
		"GameMode daemon can apply CPU governor and scheduler optimisations\n"
		"for the duration of the game session.\n"
		"Requires GameMode to be installed (package: gamemode)."));
	const bool gmAvail = gamemoderun_available();
	g_gamemodeCheckbox->setEnabled(gmAvail);
	if (!gmAvail)
		g_gamemodeCheckbox->setToolTip(gamemode_missing_tooltip());
	g_gamemodeCheckbox->setChecked(isGamemodeEnabled() && gmAvail);
	groupLayout->addWidget(g_gamemodeCheckbox);

	if (gmAvail) {
		/* gamemode_query_status() returns:
		 *   0  = daemon running, no game registered
		 *   1  = daemon running, some game registered
		 *   2  = daemon running, this process registered
		 *  -1  = daemon not reachable (not started or libgamemode unavailable)
		 *
		 * We show DAEMON reachability — not whether a game is currently
		 * using GameMode (which would always be 0 / "inactive" pre-launch
		 * and mislead the user into thinking the feature is broken). */
		auto* statusLabel = new QLabel(gamemode_status_text(), groupBox);
		statusLabel->setObjectName(QStringLiteral("linuxPerfGameModeStatus"));
		statusLabel->setWordWrap(true);
		QFont f = statusLabel->font();
		f.setPointSizeF(f.pointSizeF() * 0.85);
		statusLabel->setFont(f);
		groupLayout->addWidget(statusLabel);
	}

	int spacerIdx = layout->count() - 1;
	layout->insertWidget(spacerIdx, groupBox);

	QObject::connect(
		g_mangoCheckbox, &QCheckBox::toggled, g_guard, [](bool checked) {
			if (g_ctx)
				g_ctx->app_setting_set(g_ctx->module_handle, SETTING_MANGOHUD,
									   checked ? "1" : "0");
		});
	QObject::connect(
		g_gamemodeCheckbox, &QCheckBox::toggled, g_guard, [](bool checked) {
			if (g_ctx)
				g_ctx->app_setting_set(g_ctx->module_handle, SETTING_GAMEMODE,
									   checked ? "1" : "0");
		});
}

/* Per-page widget bag, keyed by the page QWidget* so we can refresh
 * it when the page emits its loaded / about-to-apply edges via the
 * matching ABI 3 hooks. */
struct InstancePageWidgets {
	QGroupBox* groupBox;
	QCheckBox* mango;
	QCheckBox* gm;
	QByteArray instanceId; /* copied — survives signal storm */
};
static QHash<QWidget*, InstancePageWidgets*> g_pageWidgets;

static void syncInstanceWidgets(InstancePageWidgets* w)
{
	if (!w || !g_ctx)
		return;
	const char* id = w->instanceId.constData();
	const bool overrideEnabled = ctxBoolInstance(id, SETTING_INSTANCE_OVERRIDE);
	w->groupBox->setChecked(overrideEnabled);
	w->mango->setChecked(isMangohudEnabledForInstance(id) &&
						 w->mango->isEnabled());
	w->gm->setChecked(isGamemodeEnabledForInstance(id) && w->gm->isEnabled());
}

static void applyInstanceWidgets(InstancePageWidgets* w)
{
	if (!w || !g_ctx)
		return;
	const char* id = w->instanceId.constData();
	ensureInstanceSettingsRegistered(id);
	const bool overrideEnabled = w->groupBox->isChecked();
	g_ctx->instance_setting_set(g_ctx->module_handle, id,
								SETTING_INSTANCE_OVERRIDE,
								overrideEnabled ? "1" : "0");
	if (overrideEnabled) {
		g_ctx->instance_setting_set(g_ctx->module_handle, id, SETTING_MANGOHUD,
									w->mango->isChecked() ? "1" : "0");
		g_ctx->instance_setting_set(g_ctx->module_handle, id, SETTING_GAMEMODE,
									w->gm->isChecked() ? "1" : "0");
	} else {
		g_ctx->instance_setting_reset(g_ctx->module_handle, id,
									  SETTING_MANGOHUD);
		g_ctx->instance_setting_reset(g_ctx->module_handle, id,
									  SETTING_GAMEMODE);
	}
}

static void injectCheckboxesIntoInstanceSettingsPage(QWidget* page,
													 const char* instanceId)
{
	if (!page || !instanceId)
		return;

	ensureInstanceSettingsRegistered(instanceId);

	auto* layout =
		page->findChild<QVBoxLayout*>(QStringLiteral("verticalLayout_8"));
	if (!layout)
		return;

	if (page->findChild<QGroupBox*>(
			QStringLiteral("linuxPerfInstanceGroupBox")))
		return;

	auto* groupBox = new QGroupBox(
		QObject::tr("Override global Linux performance tools settings"), page);
	groupBox->setObjectName(QStringLiteral("linuxPerfInstanceGroupBox"));
	groupBox->setCheckable(true);

	auto* groupLayout = new QVBoxLayout(groupBox);

	auto* mangoCheckbox = new QCheckBox(
		QObject::tr("Enable MangoHud overlay (FPS / GPU / CPU metrics)"),
		groupBox);
	mangoCheckbox->setObjectName(
		QStringLiteral("linuxPerfInstanceMangoHudCheck"));
	const bool mangoAvail = mangohud_available();
	if (mangoAvail) {
		QString mangoBin = mangohud_executable();
		mangoCheckbox->setToolTip(
			QObject::tr(
				"Injects the MangoHud overlay into Minecraft via the mangohud "
				"wrapper.\n"
				"Displays real-time FPS, frame timing, GPU and CPU metrics.\n"
				"Works with both OpenGL and Vulkan (LWJGL2 / LWJGL3).\n\n"
				"Found: %1")
				.arg(mangoBin));
	} else {
		mangoCheckbox->setToolTip(mangohud_missing_tooltip());
	}
	mangoCheckbox->setEnabled(mangoAvail);
	groupLayout->addWidget(mangoCheckbox);

	auto* gamemodeCheckbox = new QCheckBox(
		QObject::tr(
			"Enable GameMode (CPU / scheduler performance optimisations)"),
		groupBox);
	gamemodeCheckbox->setObjectName(
		QStringLiteral("linuxPerfInstanceGameModeCheck"));
	gamemodeCheckbox->setToolTip(QObject::tr(
		"Launches Minecraft via gamemoderun so that Feral Interactive's\n"
		"GameMode daemon can apply CPU governor and scheduler optimisations\n"
		"for the duration of the game session.\n"
		"Requires GameMode to be installed (package: gamemode)."));
	const bool gmAvail = gamemoderun_available();
	gamemodeCheckbox->setEnabled(gmAvail);
	if (!gmAvail) {
		gamemodeCheckbox->setToolTip(gamemode_missing_tooltip());
	}
	groupLayout->addWidget(gamemodeCheckbox);

	if (gmAvail) {
		auto* statusLabel = new QLabel(gamemode_status_text(), groupBox);
		statusLabel->setObjectName(
			QStringLiteral("linuxPerfInstanceGameModeStatus"));
		statusLabel->setWordWrap(true);
		QFont f = statusLabel->font();
		f.setPointSizeF(f.pointSizeF() * 0.85);
		statusLabel->setFont(f);
		groupLayout->addWidget(statusLabel);
	}

	int spacerIdx = layout->count() - 1;
	layout->insertWidget(spacerIdx, groupBox);

	/* Stash the widget bag so the LOADED / APPLYING hook callbacks can
	 * find it again when the page emits its lifecycle signals. The
	 * bag is freed when the page is destroyed. */
	auto* bag = new InstancePageWidgets;
	bag->groupBox = groupBox;
	bag->mango = mangoCheckbox;
	bag->gm = gamemodeCheckbox;
	bag->instanceId = QByteArray(instanceId);
	g_pageWidgets.insert(page, bag);

	QObject::connect(page, &QObject::destroyed, qApp, [page]() {
		auto it = g_pageWidgets.find(page);
		if (it != g_pageWidgets.end()) {
			delete it.value();
			g_pageWidgets.erase(it);
		}
	});

	syncInstanceWidgets(bag);
}

static int on_app_initialized(void* /*mh*/, uint32_t /*hook_id*/,
							  void* /*payload*/, void* /*user_data*/)
{
	char buf[256];
	snprintf(buf, sizeof(buf),
			 "LinuxPerf: MangoHud available=%s  GameMode available=%s  "
			 "MangoHud enabled=%s  GameMode enabled=%s",
			 mangohud_available() ? "yes" : "no",
			 gamemoderun_available() ? "yes" : "no",
			 isMangohudEnabled() ? "yes" : "no",
			 isGamemodeEnabled() ? "yes" : "no");
	MMCO_LOG(g_ctx, buf);

	/* Settings dialog lifecycle is now driven via ABI 3 hooks; see
	 * the handlers below. */
	return 0;
}

/* MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN — replaces the legacy
 * direct connect to Application::globalSettingsAboutToOpen. */
static int on_global_settings_about_to_open(void*, uint32_t, void*, void*)
{
	g_mangoCheckbox = nullptr;
	g_gamemodeCheckbox = nullptr;
	QTimer::singleShot(0, qApp, injectCheckboxesIntoMinecraftPage);
	return 0;
}

/* MMCO_HOOK_INSTANCE_SETTINGS_PAGE_CREATED — replaces direct
 * Application::instanceSettingsPageCreated. The page_handle is an
 * opaque QWidget*; we cast it (a Qt operation, allowed) and inject
 * our group box. */
static int on_instance_settings_page_created(void*, uint32_t, void* payload,
											 void*)
{
	auto* evt = static_cast<MMCOInstanceSettingsPageEvent*>(payload);
	if (!evt || !evt->page_handle || !evt->instance_id)
		return 0;
	injectCheckboxesIntoInstanceSettingsPage(
		static_cast<QWidget*>(evt->page_handle), evt->instance_id);
	return 0;
}

/* MMCO_HOOK_INSTANCE_SETTINGS_PAGE_LOADED — page just refreshed its
 * values from the backing store; mirror them into our checkboxes. */
static int on_instance_settings_page_loaded(void*, uint32_t, void* payload,
											void*)
{
	auto* evt = static_cast<MMCOInstanceSettingsPageEvent*>(payload);
	if (!evt || !evt->page_handle)
		return 0;
	auto it = g_pageWidgets.find(static_cast<QWidget*>(evt->page_handle));
	if (it != g_pageWidgets.end())
		syncInstanceWidgets(it.value());
	return 0;
}

/* MMCO_HOOK_INSTANCE_SETTINGS_PAGE_APPLYING — page is about to commit
 * its values; push our widgets back into the instance settings. */
static int on_instance_settings_page_applying(void*, uint32_t, void* payload,
											  void*)
{
	auto* evt = static_cast<MMCOInstanceSettingsPageEvent*>(payload);
	if (!evt || !evt->page_handle)
		return 0;
	auto it = g_pageWidgets.find(static_cast<QWidget*>(evt->page_handle));
	if (it != g_pageWidgets.end())
		applyInstanceWidgets(it.value());
	return 0;
}

static int on_instance_pre_launch(void* mh, uint32_t /*hook_id*/, void* payload,
								  void* /*user_data*/)
{
	auto* info = static_cast<MMCOInstanceInfo*>(payload);
	const char* iname = info->instance_name ? info->instance_name : "?";
	const char* iid = info ? info->instance_id : nullptr;

	/*
	 * Wrapper command build order (each call *prepends* to the chain):
	 *
	 *   Step 1 - mangohud:   pending = "mangohud"
	 *   Step 2 - gamemoderun: pending = "gamemoderun mangohud"
	 *
	 * Final command: gamemoderun mangohud java [args]
	 *
	 * gamemoderun wraps the entire chain and requests GameMode for the child;
	 * mangohud hooks into the JVM's graphics APIs via LD_PRELOAD.
	 */
	bool mangoEnabled =
		isMangohudEnabledForInstance(iid) && mangohud_available();
	bool gamemodeEnabled =
		isGamemodeEnabledForInstance(iid) && gamemoderun_available();

	if (mangoEnabled) {
		QByteArray mangohudWrapper = mangohud_executable().toUtf8();

		/* Use the wrapper in both native and Flatpak environments. In the
		 * Flatpak build, prefer the mounted extension path directly instead of
		 * relying on PATH propagation. */
		g_ctx->launch_prepend_wrapper(mh, mangohudWrapper.constData());

		/* MANGOHUD=1  — enables the Vulkan implicit layer (all MC ≥ 1.17) */
		g_ctx->launch_set_env(mh, "MANGOHUD", "1");

		/* MANGOHUD_DLSYM=1 — hook dlsym so MangoHud intercepts OpenGL
		 * functions loaded dynamically by LWJGL (critical for Java/OpenGL) */
		g_ctx->launch_set_env(mh, "MANGOHUD_DLSYM", "1");

		/* MANGOHUD_CONFIGFILE — MangoHud auto-detects config by process name.
		 * Since Minecraft runs as "java", it can't find "java.conf" and
		 * may silently use no HUD.  Point it at the standard MangoHud.conf
		 * so the overlay always appears with sensible defaults. */
		QByteArray mangoCfg =
			(QDir::homePath() +
			 QStringLiteral("/.config/MangoHud/MangoHud.conf"))
				.toUtf8();
		g_ctx->launch_set_env(mh, "MANGOHUD_CONFIGFILE", mangoCfg.constData());

		char buf[512];
		snprintf(buf, sizeof(buf),
				 "LinuxPerf: instance '%s': mangohud wrapper '%s' + env vars "
				 "applied%s",
				 iname, mangohudWrapper.constData(),
				 is_flatpak() ? " (Flatpak)" : "");
		MMCO_LOG(g_ctx, buf);
	}

	if (gamemodeEnabled) {
		QByteArray gamemodeWrapper = gamemoderun_executable().toUtf8();

		/* Prepend gamemoderun so it wraps the entire command (including
		 * mangohud if both are enabled), ensuring GameMode activates for the
		 * child PID. */
		g_ctx->launch_prepend_wrapper(mh, gamemodeWrapper.constData());

		char buf[512];
		snprintf(buf, sizeof(buf),
				 "LinuxPerf: instance '%s': gamemoderun wrapper '%s' applied",
				 iname, gamemodeWrapper.constData());
		MMCO_LOG(g_ctx, buf);
	}

	return 0; /* Never cancel the launch */
}

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;
	MMCO_LOG(ctx, "LinuxPerf plugin initializing...");

	ensureSettingsRegistered();
	g_guard = new QObject();

	ctx->hook_register(ctx->module_handle, MMCO_HOOK_APP_INITIALIZED,
					   on_app_initialized, nullptr);
	ctx->hook_register(ctx->module_handle,
					   MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN,
					   on_global_settings_about_to_open, nullptr);
	ctx->hook_register(ctx->module_handle,
					   MMCO_HOOK_INSTANCE_SETTINGS_PAGE_CREATED,
					   on_instance_settings_page_created, nullptr);
	ctx->hook_register(ctx->module_handle,
					   MMCO_HOOK_INSTANCE_SETTINGS_PAGE_LOADED,
					   on_instance_settings_page_loaded, nullptr);
	ctx->hook_register(ctx->module_handle,
					   MMCO_HOOK_INSTANCE_SETTINGS_PAGE_APPLYING,
					   on_instance_settings_page_applying, nullptr);
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_INSTANCE_PRE_LAUNCH,
					   on_instance_pre_launch, nullptr);

	MMCO_LOG(ctx, "LinuxPerf plugin initialized.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx)
		MMCO_LOG(g_ctx, "LinuxPerf plugin unloading.");
	g_ctx = nullptr;
	g_mangoCheckbox = nullptr;
	g_gamemodeCheckbox = nullptr;
	for (auto* w : g_pageWidgets)
		delete w;
	g_pageWidgets.clear();
	/* g_guard intentionally not deleted — see NVIDIAPrime note */
	g_guard = nullptr;
}

} /* extern "C" */
