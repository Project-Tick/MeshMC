/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * GitVersioningPlugin — MMCO entry point.
 *
 * Hooks:
 *   APP_INITIALIZED      — register a settings checkbox + verify git
 *                          is reachable.
 *   UI_INSTANCE_PAGES    — inject a "Version History" page per instance.
 *   INSTANCE_PRE_LAUNCH  — auto-snapshot the instance if the user has
 *                          opted in.
 *   INSTANCE_REMOVED     — never delete .history; we keep it around so
 *                          users can recover a deleted instance from
 *                          backup. We just emit a log line.
 */

#include "plugin/sdk/mmco_sdk.h"
#include "GitRepo.h"
#include "GitVersioningPage.h"

MMCO_DEFINE_MODULE("GitVersioning", "1.0.0", "Project Tick",
				   "Track instance changes as Git commits — snapshot, restore, "
				   "per-file history, tags.",
				   "GPL-3.0-or-later");

static MMCOContext* g_ctx = nullptr;
static constexpr const char SETTING_AUTO_SNAPSHOT[] =
	"plugin.git_versioning.AutoSnapshotBeforeLaunch";
static QObject* g_guard = nullptr;
static QCheckBox* g_checkbox = nullptr;
static bool g_gitAvailable = false;

static bool autoSnapshotEnabled()
{
	if (!g_ctx)
		return false;
	if (!g_ctx->app_setting_contains(g_ctx->module_handle,
									 SETTING_AUTO_SNAPSHOT))
		return false;
	const char* v =
		g_ctx->app_setting_get(g_ctx->module_handle, SETTING_AUTO_SNAPSHOT);
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
	if (!g_ctx->app_setting_contains(g_ctx->module_handle,
									 SETTING_AUTO_SNAPSHOT))
		g_ctx->app_setting_register(g_ctx->module_handle, SETTING_AUTO_SNAPSHOT,
									"0");
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

	auto* groupBox = new QGroupBox(QObject::tr("Git Versioning"));
	groupBox->setObjectName(QStringLiteral("gitVersioningGroupBox"));
	auto* gl = new QVBoxLayout(groupBox);

	g_checkbox = new QCheckBox(
		QObject::tr("Auto-snapshot instance state before every launch"),
		groupBox);
	g_checkbox->setObjectName(QStringLiteral("gitAutoSnapshotCheck"));
	g_checkbox->setToolTip(
		QObject::tr("Commit any pending changes to the instance's Git history "
					"right before the JVM starts. The history lives in the "
					"instance's .history/ directory."));
	gl->addWidget(g_checkbox);

	if (!g_gitAvailable) {
		auto* warn = new QLabel(QObject::tr(
			"<i>git is not installed — version history disabled.</i>"));
		warn->setStyleSheet(QStringLiteral("color: #cc6666;"));
		gl->addWidget(warn);
		g_checkbox->setEnabled(false);
	}

	int spacerIdx = layout->count() - 1;
	layout->insertWidget(spacerIdx, groupBox);

	g_checkbox->setChecked(autoSnapshotEnabled());

	QObject::connect(g_checkbox, &QCheckBox::toggled, g_guard,
					 [](bool checked) {
						 if (g_ctx)
							 g_ctx->app_setting_set(g_ctx->module_handle,
													SETTING_AUTO_SNAPSHOT,
													checked ? "1" : "0");
					 });
}

static int on_app_initialized(void*, uint32_t, void*, void*)
{
	g_gitAvailable = GitRepo::gitAvailable();
	if (g_gitAvailable && g_ctx) {
		QByteArray msg = "git detected: " + GitRepo::gitVersion().toUtf8();
		MMCO_LOG(g_ctx, msg.constData());
	} else if (g_ctx) {
		MMCO_WARN(g_ctx, "system git not found — GitVersioning will run in "
						 "read-only mode (instance page still visible but "
						 "every operation will fail gracefully).");
	}

	g_guard = new QObject();
	return 0;
}

/* MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN handler — replaces the
 * legacy direct connect to Application::globalSettingsAboutToOpen. */
static int on_global_settings_about_to_open(void*, uint32_t, void*, void*)
{
	g_checkbox = nullptr;
	QTimer::singleShot(0, qApp, injectCheckboxIntoMeshMCPage);
	return 0;
}

static int on_pre_launch(void*, uint32_t, void* payload, void*)
{
	if (!g_ctx || !payload || !g_gitAvailable || !autoSnapshotEnabled())
		return 0;

	auto* info = static_cast<MMCOInstanceInfo*>(payload);
	if (!info->instance_id || !info->instance_path)
		return 0;

	GitRepo repo(QString::fromUtf8(info->instance_id),
				 QString::fromUtf8(info->instance_path));
	if (!repo.isInitialized()) {
		QString err;
		if (!repo.initialize(&err)) {
			QByteArray msg = "GitVersioning: init failed: " + err.toUtf8();
			MMCO_WARN(g_ctx, msg.constData());
			return 0;
		}
	}

	QString err;
	QString sha = repo.commit(
		QStringLiteral("Pre-launch snapshot — %1")
			.arg(QDateTime::currentDateTime().toString(Qt::ISODate)),
		/*isPreLaunch=*/true, &err);
	if (sha.isEmpty() && !err.isEmpty()) {
		QByteArray msg =
			"GitVersioning: pre-launch commit failed: " + err.toUtf8();
		MMCO_WARN(g_ctx, msg.constData());
	} else if (!sha.isEmpty()) {
		QByteArray msg = "GitVersioning: snapshot " + sha.toUtf8();
		MMCO_LOG(g_ctx, msg.constData());
	}
	return 0;
}

static int on_instance_pages(void*, uint32_t, void* payload, void*)
{
	auto* evt = static_cast<MMCOInstancePagesEvent*>(payload);
	if (!evt || !evt->page_list_handle || !evt->instance_id)
		return 0;

	auto* pages = static_cast<QList<BasePage*>*>(evt->page_list_handle);

	const QString instId = QString::fromUtf8(evt->instance_id);
	const QString instRoot =
		evt->instance_path ? QString::fromUtf8(evt->instance_path) : QString();
	pages->append(new GitVersioningPage(instId, instRoot));
	return 0;
}

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;
	MMCO_LOG(ctx, "GitVersioning initialising…");

	ensureSettingRegistered();

	ctx->hook_register(ctx->module_handle, MMCO_HOOK_APP_INITIALIZED,
					   on_app_initialized, nullptr);
	ctx->hook_register(ctx->module_handle,
					   MMCO_HOOK_GLOBAL_SETTINGS_ABOUT_TO_OPEN,
					   on_global_settings_about_to_open, nullptr);
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_INSTANCE_PRE_LAUNCH,
					   on_pre_launch, nullptr);
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_UI_INSTANCE_PAGES,
					   on_instance_pages, nullptr);

	ctx->ui_register_instance_action(
		ctx->module_handle, "Version History",
		"View and manage the instance's snapshot history", "version-control",
		"git-versioning");

	MMCO_LOG(ctx, "GitVersioning ready.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx)
		MMCO_LOG(g_ctx, "GitVersioning unloading.");
	g_ctx = nullptr;
	g_checkbox = nullptr;
	g_guard = nullptr;
}

} /* extern "C" */
