/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * BackupPlugin — MMCO entry point for the BackupSystem plugin.
 *
 * All Qt and MeshMC types come through the SDK header. The plugin
 * does not directly #include any Qt or MeshMC headers.
 */

#include "plugin/sdk/mmco_sdk.h"
#include "BackupPage.h"
#include "BackupManager.h"

MMCO_DEFINE_MODULE(
	"BackupSystem", "1.1.0", "Project Tick",
	"Instance backup snapshots — create, restore, export, import",
	"GPL-3.0-or-later");

static MMCOContext* g_ctx = nullptr;
static constexpr const char SETTING_KEY[] =
	"plugin.backup_system.BackupBeforeLaunch";
static QCheckBox* g_backupCheckbox = nullptr;
static QObject* g_guard = nullptr;

static bool is_enabled()
{
	if (!g_ctx)
		return false;
	if (!g_ctx->app_setting_contains(g_ctx->module_handle, SETTING_KEY))
		return false;
	const char* v = g_ctx->app_setting_get(g_ctx->module_handle, SETTING_KEY);
	if (!v)
		return false;
	QString s = QString::fromUtf8(v).trimmed().toLower();
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

	auto* groupBox = new QGroupBox(QObject::tr("Backup System"));
	groupBox->setObjectName(QStringLiteral("backupFeaturesGroupBox"));
	auto* groupLayout = new QVBoxLayout(groupBox);

	g_backupCheckbox = new QCheckBox(
		QObject::tr("Automatically backup instances before launch"), groupBox);
	g_backupCheckbox->setObjectName(QStringLiteral("backupBeforeLaunchCheck"));
	g_backupCheckbox->setToolTip(
		QObject::tr("Creates a snapshot of each instance before launching.\n"
					"Backups are saved in the instance's .backups folder\n"
					"with the label \"pre-launch\"."));
	groupLayout->addWidget(g_backupCheckbox);

	int spacerIdx = layout->count() - 1;
	layout->insertWidget(spacerIdx, groupBox);

	g_backupCheckbox->setChecked(is_enabled());

	QObject::connect(g_backupCheckbox, &QCheckBox::toggled, g_guard,
					 [](bool checked) {
						 if (g_ctx)
							 g_ctx->app_setting_set(g_ctx->module_handle,
													SETTING_KEY,
													checked ? "1" : "0");
					 });
}

static int on_app_initialized(void* /*mh*/, uint32_t /*hook_id*/,
							  void* /*payload*/, void* /*user_data*/)
{
	g_guard = new QObject();
	return 0;
}

/* MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN — replaces the legacy
 * direct connect to Application::globalSettingsAboutToOpen. */
static int on_global_settings_about_to_open(void*, uint32_t, void*, void*)
{
	g_backupCheckbox = nullptr;
	QTimer::singleShot(0, qApp, injectCheckboxIntoMeshMCPage);
	return 0;
}

static int on_pre_launch(void* /*mh*/, uint32_t /*hook_id*/, void* payload,
						 void* /*ud*/)
{
	if (!g_ctx || !payload)
		return 0;

	if (!is_enabled())
		return 0;

	auto* info = static_cast<MMCOInstanceInfo*>(payload);
	if (!info->instance_id || !info->instance_path)
		return 0;

	MMCO_LOG(g_ctx, "Pre-launch backup triggered for instance.");

	BackupManager mgr(QString::fromUtf8(info->instance_id),
					  QString::fromUtf8(info->instance_path), g_ctx);
	auto entry = mgr.createBackup("pre-launch");
	if (entry.fullPath.isEmpty()) {
		MMCO_WARN(g_ctx, "Pre-launch backup failed.");
	} else {
		MMCO_LOG(g_ctx, "Pre-launch backup created successfully.");
	}
	return 0;
}

static int on_instance_pages(void* /*mh*/, uint32_t /*hook_id*/, void* payload,
							 void* /*ud*/)
{
	auto* evt = static_cast<MMCOInstancePagesEvent*>(payload);
	if (!evt || !evt->page_list_handle || !evt->instance_id)
		return 0;

	auto* pages = static_cast<QList<BasePage*>*>(evt->page_list_handle);
	const QString instId = QString::fromUtf8(evt->instance_id);
	const QString instRoot =
		evt->instance_path ? QString::fromUtf8(evt->instance_path) : QString();
	pages->append(new BackupPage(instId, instRoot, g_ctx));
	return 0;
}

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;

	MMCO_LOG(ctx, "BackupSystem initializing...");

	ensureSettingRegistered();

	ctx->hook_register(ctx->module_handle, MMCO_HOOK_APP_INITIALIZED,
					   on_app_initialized, nullptr);
	ctx->hook_register(ctx->module_handle,
					   MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN,
					   on_global_settings_about_to_open, nullptr);

	int rc = ctx->hook_register(ctx->module_handle, MMCO_HOOK_UI_INSTANCE_PAGES,
								on_instance_pages, nullptr);
	if (rc != 0) {
		MMCO_ERR(ctx, "Failed to register INSTANCE_PAGES hook");
		return rc;
	}

	rc = ctx->hook_register(ctx->module_handle, MMCO_HOOK_INSTANCE_PRE_LAUNCH,
							on_pre_launch, nullptr);
	if (rc != 0) {
		MMCO_ERR(ctx, "Failed to register INSTANCE_PRE_LAUNCH hook");
		return rc;
	}

	ctx->ui_register_instance_action(
		ctx->module_handle, "View Backups",
		"View and manage backups for this instance.", "backup",
		"backup-system");

	MMCO_LOG(ctx, "BackupSystem initialized successfully.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx) {
		MMCO_LOG(g_ctx, "BackupSystem unloading.");
	}
	g_ctx = nullptr;
	g_backupCheckbox = nullptr;
	g_guard = nullptr;
}

} /* extern "C" */
