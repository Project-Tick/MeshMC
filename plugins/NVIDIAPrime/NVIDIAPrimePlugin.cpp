/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: MIT
 *
 * NVIDIAPrimePlugin — MMCO entry point for the NVIDIA Prime plugin.
 *
 * Forces Minecraft to use the NVIDIA discrete GPU on Optimus laptops.
 *
 * Behaviour:
 *   - Flatpak:      prepends `prime-run` as a wrapper command.
 *   - Non-Flatpak:  injects environment variables:
 *       __NV_PRIME_RENDER_OFFLOAD=1
 *       __VK_LAYER_NV_optimus=NVIDIA_only
 *       __GLX_VENDOR_LIBRARY_NAME=nvidia
 *
 * The checkbox is injected into the existing Minecraft settings page
 * at runtime.  The setting is stored as plugin.nvidia_prime.enabled.
 */

#include "plugin/sdk/mmco_sdk.h"

MMCO_DEFINE_MODULE("NVIDIA Prime Module", "1.0.0", "Project Tick",
				   "Discrete GPU offload via NVIDIA Prime Render Offload",
				   "MIT");

static MMCOContext* g_ctx = nullptr;
static constexpr const char SETTING_KEY[] = "plugin.nvidia_prime.enabled";
static QCheckBox* g_primeCheckbox =
	nullptr; /* raw ptr — widget owned by dialog */
static QObject* g_guard = nullptr;

static bool is_flatpak()
{
	return QFile::exists("/.flatpak-info");
}

static bool is_enabled()
{
	if (!g_ctx)
		return false;
	if (!g_ctx->app_setting_contains(g_ctx->module_handle, SETTING_KEY))
		return false;
	const char* v = g_ctx->app_setting_get(g_ctx->module_handle, SETTING_KEY);
	if (!v)
		return false;
	const QString s = QString::fromUtf8(v).trimmed().toLower();
	return s == QLatin1String("1") || s == QLatin1String("true") ||
		   s == QLatin1String("yes") || s == QLatin1String("on");
}

static void ensureSettingRegistered()
{
	if (!g_ctx)
		return;
	if (!g_ctx->app_setting_contains(g_ctx->module_handle, SETTING_KEY))
		g_ctx->app_setting_register(g_ctx->module_handle, SETTING_KEY, "0");
}

static void injectCheckboxIntoMinecraftPage()
{
	/* Find the MinecraftPage widget (objectName set by .ui file) */
	QWidget* mcPage = nullptr;
	for (auto* w : qApp->allWidgets()) {
		if (w->objectName() == QStringLiteral("MinecraftPage")) {
			mcPage = w;
			break;
		}
	}
	if (!mcPage)
		return;

	/* Find verticalLayout_3 inside the minecraftTab */
	auto* layout =
		mcPage->findChild<QVBoxLayout*>(QStringLiteral("verticalLayout_3"));
	if (!layout)
		return;

	/* Build the Performance group box */
	auto* groupBox = new QGroupBox(QObject::tr("Performance"));
	groupBox->setObjectName(QStringLiteral("nvidiaPrimeGroupBox"));
	auto* groupLayout = new QVBoxLayout(groupBox);

	g_primeCheckbox = new QCheckBox(
		QObject::tr("Use discrete GPU (NVIDIA Prime Render Offload)"),
		groupBox);
	g_primeCheckbox->setObjectName(QStringLiteral("useNVIDIAPrimeCheck"));
	g_primeCheckbox->setToolTip(
		QObject::tr("Forces Minecraft to use the NVIDIA discrete GPU on "
					"Optimus laptops.\nUses prime-run on Flatpak, or sets "
					"environment variables directly otherwise."));
	groupLayout->addWidget(g_primeCheckbox);

	/* Insert before the spacer (last item in the layout) */
	int spacerIdx = layout->count() - 1;
	layout->insertWidget(spacerIdx, groupBox);

	/* Load current setting */
	g_primeCheckbox->setChecked(is_enabled());

	/* Save immediately when toggled — avoids relying on a
	 * close-dialog hook that would fire after the checkbox (and the
	 * page widget) is already destroyed. */
	QObject::connect(
		g_primeCheckbox, &QCheckBox::toggled, g_guard, [](bool checked) {
			if (!g_ctx)
				return;
			g_ctx->app_setting_set(g_ctx->module_handle, SETTING_KEY,
								   checked ? "1" : "0");
		});
}

/* Hook handler for MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN — the C-ABI
 * replacement for the legacy
 *   QObject::connect(APPLICATION,
 *                    &Application::globalSettingsAboutToOpen, ...)
 * direct connection.  The hook fires before the dialog is built;
 * QTimer::singleShot(0, ...) defers the widget walk to the next
 * event-loop turn so the MinecraftPage already exists. */
static int on_global_settings_about_to_open(void*, uint32_t, void*, void*)
{
	g_primeCheckbox = nullptr;
	QTimer::singleShot(0, qApp, injectCheckboxIntoMinecraftPage);
	return 0;
}

static int on_app_initialized(void* /*mh*/, uint32_t /*hook_id*/,
							  void* /*payload*/, void* /*user_data*/)
{
	char buf[256];
	snprintf(
		buf, sizeof(buf), "NVIDIA Prime Render Offload is %s (Flatpak: %s)",
		is_enabled() ? "ENABLED" : "disabled", is_flatpak() ? "yes" : "no");
	MMCO_LOG(g_ctx, buf);

	/* g_guard receives the QCheckBox::toggled connection later, when
	 * the settings dialog opens.  Deleting (well, nulling) the guard
	 * in mmco_unload() severs every connection that has been anchored
	 * on it, so nothing dangles into unloaded .so memory. */
	g_guard = new QObject();
	return 0;
}

static int on_instance_pre_launch(void* mh, uint32_t /*hook_id*/, void* payload,
								  void* /*user_data*/)
{
	if (!is_enabled())
		return 0;

	auto* info = static_cast<MMCOInstanceInfo*>(payload);

	if (is_flatpak()) {
		g_ctx->launch_prepend_wrapper(mh, "prime-run");

		char buf[512];
		snprintf(buf, sizeof(buf),
				 "Instance '%s': prime-run wrapper will be used (Flatpak)",
				 info->instance_name ? info->instance_name : "?");
		MMCO_LOG(g_ctx, buf);
	} else {
		g_ctx->launch_set_env(mh, "__NV_PRIME_RENDER_OFFLOAD", "1");
		g_ctx->launch_set_env(mh, "__VK_LAYER_NV_optimus", "NVIDIA_only");
		g_ctx->launch_set_env(mh, "__GLX_VENDOR_LIBRARY_NAME", "nvidia");

		char buf[512];
		snprintf(buf, sizeof(buf),
				 "Instance '%s': NVIDIA offload env vars injected",
				 info->instance_name ? info->instance_name : "?");
		MMCO_LOG(g_ctx, buf);
	}

	return 0;
}

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;
	MMCO_LOG(ctx, "NVIDIA Prime plugin initializing...");

	ensureSettingRegistered();

	ctx->hook_register(ctx->module_handle, MMCO_HOOK_APP_INITIALIZED,
					   on_app_initialized, nullptr);

	ctx->hook_register(ctx->module_handle,
					   MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN,
					   on_global_settings_about_to_open, nullptr);

	ctx->hook_register(ctx->module_handle, MMCO_HOOK_INSTANCE_PRE_LAUNCH,
					   on_instance_pre_launch, nullptr);

	MMCO_LOG(ctx, "NVIDIA Prime plugin initialized.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx) {
		MMCO_LOG(g_ctx, "NVIDIA Prime plugin unloading.");
	}
	g_ctx = nullptr;
	g_primeCheckbox = nullptr;
	/* g_guard is intentionally NOT deleted here.
	 * Deleting a QObject during Application teardown triggers
	 * Qt signal/slot doubly-linked-list surgery which can detect
	 * heap corruption caused by teardown ordering.
	 * Since dlclose() is skipped at shutdown, the code and data
	 * remain mapped — the OS reclaims everything at process exit. */
	g_guard = nullptr;
}

} /* extern "C" */
