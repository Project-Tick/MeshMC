/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * PackPortalPlugin — MMCO entry point.
 *
 * The plugin only owns the EXPORT side of the round-trip. Importing
 * mrpack / CurseForge zip / MultiMC raw zip is already handled by the
 * launcher's built-in `InstanceImportTask` (reachable from
 * Add Instance → Import from zip), which sniffs the archive's manifest
 * and instantiates the right creation task. Adding a second "Import
 * pack" entry-point would just duplicate that flow with a worse UX
 * (no progress bar, no metadata side-bar, no integration with the
 * New Instance dialog's group selector).
 *
 * Wiring:
 *   • UI_INSTANCE_PAGES — per-instance "Pack Export" page (advanced
 *                         options, format toggles, summary banner).
 *   • UI_CONTEXT_MENU   — per-instance "Export as pack…" entry.
 *   • UI_MAIN_READY     — override the MainWindow's actionExportInstance
 *                         QAction so the launcher-wide "Export Instance"
 *                         toolbar button / File menu entry opens our
 *                         format-aware dialog instead of the stock
 *                         single-format ExportInstanceDialog.
 *
 * The actionExportInstance override pattern matches SkinManager's
 * actionUploadSkin pattern: grab the QAction by objectName off the
 * MAIN_READY payload's main_window handle, disconnect every existing
 * slot, connect ours. The launcher's MainWindow is built exactly once
 * per session, so a single MAIN_READY rewire suffices (unlike the
 * AccountListPage case which Qt rebuilds on every settings open).
 */

#include "plugin/sdk/mmco_sdk.h"
#include "ExportEngine.h"
#include "formats/MrPackFormat.h"
#include "formats/CurseForgeFormat.h"
#include "formats/MultiMCFormat.h"

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QMainWindow>
#include <QPointer>

MMCO_DEFINE_MODULE(
	"PackPortal", "1.0.0", "Project Tick",
	"Import / export CurseForge zip, Modrinth .mrpack and MultiMC raw "
	"instance archives",
	"GPL-3.0-or-later");

static MMCOContext* g_ctx = nullptr;
static QObject* g_guard = nullptr;

/* The id of the currently-selected instance, as recorded by MainWindow
 * into the global `SelectedInstance` setting. Used by the actionExportInstance
 * override and the context-menu import callbacks. */
static QString currentSelectedInstanceId()
{
	if (!g_ctx)
		return {};
	const char* selected =
		g_ctx->app_setting_get(g_ctx->module_handle, "SelectedInstance");
	return selected ? QString::fromUtf8(selected) : QString();
}

/* Modal "Export <instance> as a pack" dialog used by the actionExportInstance
 * override and the per-instance context-menu action.
 *
 * The per-instance BasePage exposes the same controls (and is the
 * place to go for unattended export from automation), but the dialog
 * version is what the launcher's main "Export Instance" toolbar
 * button now opens — replacing the stock ExportInstanceDialog which
 * only handles the MultiMC raw zip format. */
static void runExportDialogFor(const QString& instanceId)
{
	QWidget* parent = QApplication::activeWindow();
	if (instanceId.isEmpty()) {
		QMessageBox::information(
			parent, QObject::tr("PackPortal"),
			QObject::tr("Pick an instance in the launcher first, then "
						"click Export Instance again."));
		return;
	}
	if (!g_ctx) {
		QMessageBox::warning(parent, QObject::tr("PackPortal"),
							 QObject::tr("Plugin context not ready."));
		return;
	}
	const QByteArray idUtf8 = instanceId.toUtf8();
	const char* instNameC =
		g_ctx->instance_get_name(g_ctx->module_handle, idUtf8.constData());
	if (!instNameC) {
		QMessageBox::warning(parent, QObject::tr("PackPortal"),
							 QObject::tr("Instance no longer exists."));
		return;
	}
	const QString instName = QString::fromUtf8(instNameC);

	QDialog dlg(parent);
	dlg.setWindowTitle(QObject::tr("Export Instance as Pack"));
	dlg.setMinimumWidth(520);

	auto* root = new QVBoxLayout(&dlg);

	auto* form = new QFormLayout();
	auto* fmtCombo = new QComboBox(&dlg);
	fmtCombo->addItem(QObject::tr("Modrinth (.mrpack)"),
					  int(pack::Format::MrPack));
	fmtCombo->addItem(QObject::tr("CurseForge (.zip)"),
					  int(pack::Format::CurseForge));
	fmtCombo->addItem(QObject::tr("MultiMC/MeshMC raw (.zip)"),
					  int(pack::Format::MultiMC));
	form->addRow(QObject::tr("Format:"), fmtCombo);

	auto* nameEdit = new QLineEdit(instName, &dlg);
	auto* versionEdit = new QLineEdit(QStringLiteral("1.0.0"), &dlg);
	auto* authorEdit = new QLineEdit(&dlg);
	auto* summaryEdit = new QLineEdit(&dlg);
	form->addRow(QObject::tr("Name:"), nameEdit);
	form->addRow(QObject::tr("Version:"), versionEdit);
	form->addRow(QObject::tr("Author:"), authorEdit);
	form->addRow(QObject::tr("Summary:"), summaryEdit);
	root->addLayout(form);

	auto* chkSkipWorlds = new QCheckBox(QObject::tr("Skip world saves"), &dlg);
	chkSkipWorlds->setChecked(true);
	auto* chkSkipVolatile = new QCheckBox(
		QObject::tr("Skip logs / crash reports / screenshots"), &dlg);
	chkSkipVolatile->setChecked(true);
	auto* chkEmbedAll = new QCheckBox(
		QObject::tr("Embed every mod (no external download references)"),
		&dlg);
	root->addWidget(chkSkipWorlds);
	root->addWidget(chkSkipVolatile);
	root->addWidget(chkEmbedAll);

	auto* resultLabel = new QLabel(&dlg);
	resultLabel->setWordWrap(true);
	root->addWidget(resultLabel);

	auto* btnRow = new QHBoxLayout();
	btnRow->addStretch();
	auto* exportBtn = new QPushButton(QObject::tr("Export…"), &dlg);
	auto* closeBtn = new QPushButton(QObject::tr("Close"), &dlg);
	btnRow->addWidget(exportBtn);
	btnRow->addWidget(closeBtn);
	root->addLayout(btnRow);

	QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
	QObject::connect(exportBtn, &QPushButton::clicked, &dlg, [&]() {
		auto fmt = pack::Format(fmtCombo->currentData().toInt());

		pack::ExportEngine::Options opts;
		opts.format = fmt;
		opts.name = nameEdit->text();
		opts.version = versionEdit->text();
		opts.author = authorEdit->text();
		opts.summary = summaryEdit->text();
		opts.skipWorlds = chkSkipWorlds->isChecked();
		opts.skipVolatile = chkSkipVolatile->isChecked();
		opts.embedAllMods = chkEmbedAll->isChecked();

		pack::PackInfo previewInfo;
		previewInfo.name = opts.name;
		previewInfo.version = opts.version;
		QString suggested;
		QString filter;
		switch (fmt) {
			case pack::Format::MrPack:
				suggested = pack::mrpack::suggestedFileName(previewInfo);
				filter = QObject::tr("Modrinth packs (*.mrpack)");
				break;
			case pack::Format::CurseForge:
				suggested = pack::curseforge::suggestedFileName(previewInfo);
				filter = QObject::tr("Pack archives (*.zip)");
				break;
			case pack::Format::MultiMC:
				suggested = pack::multimc::suggestedFileName(previewInfo);
				filter = QObject::tr("Pack archives (*.zip)");
				break;
		}

		QString out = QFileDialog::getSaveFileName(
			&dlg, QObject::tr("Export pack"),
			QDir::home().filePath(suggested), filter);
		if (out.isEmpty())
			return;

		exportBtn->setEnabled(false);
		closeBtn->setEnabled(false);
		resultLabel->setText(QObject::tr("Exporting… this can take a "
										 "minute for large instances."));
		QApplication::processEvents();

		pack::ExportEngine eng(g_ctx);
		auto r = eng.exportInstance(instanceId, opts, out);

		if (!r.ok) {
			resultLabel->setText(
				QObject::tr(
					"<b style='color:#cc6666'>Export failed:</b> %1")
					.arg(r.errorMsg.toHtmlEscaped()));
		} else {
			QString warnHtml;
			if (!r.warnings.isEmpty()) {
				warnHtml = QObject::tr("<br><br><b>Warnings:</b><ul>");
				for (const auto& w : r.warnings)
					warnHtml += QStringLiteral("<li>%1</li>")
									.arg(w.toHtmlEscaped());
				warnHtml += QStringLiteral("</ul>");
			}
			resultLabel->setText(
				QObject::tr("<b style='color:#66aa66'>Exported:</b> %1<br>"
							"Embedded %2 files, referenced %3 external "
							"files.%4")
					.arg(r.outputPath.toHtmlEscaped())
					.arg(r.filesEmbedded)
					.arg(r.filesReferenced)
					.arg(warnHtml));
		}
		exportBtn->setEnabled(true);
		closeBtn->setEnabled(true);
	});

	dlg.exec();
}

/* MAIN_READY: hijack the launcher's built-in "Export Instance" QAction
 * so File → Export Instance and the matching toolbar button open our
 * multi-format dialog instead of the stock ExportInstanceDialog
 * (which only knows the MultiMC raw zip format).
 *
 * The pattern mirrors SkinManager's handling of actionUploadSkin:
 * find the QAction by objectName under the MainWindow handle the
 * MAIN_READY payload provides, disconnect every existing slot, and
 * connect our own. MainWindow lives for the entire application
 * session, so a single rewire here is sufficient — no need to
 * re-hook on settings open. */
static int on_ui_main_ready(void*, uint32_t, void* payload, void*)
{
	auto* p = static_cast<MMCOUiMainReadyPayload*>(payload);
	QMainWindow* mainWindow = nullptr;
	if (p && p->main_window)
		mainWindow = static_cast<QMainWindow*>(p->main_window);

	/* Fallback for older hosts that don't fill in main_window. */
	if (!mainWindow) {
		for (auto* w : qApp->allWidgets()) {
			if (auto* mw = qobject_cast<QMainWindow*>(w);
				mw && mw->objectName() == QStringLiteral("MainWindow")) {
				mainWindow = mw;
				break;
			}
		}
	}
	if (!mainWindow) {
		MMCO_WARN(g_ctx,
				  "PackPortal: MainWindow not reachable; "
				  "actionExportInstance override skipped.");
		return 0;
	}

	auto* action = mainWindow->findChild<QAction*>(
		QStringLiteral("actionExportInstance"));
	if (!action) {
		MMCO_WARN(g_ctx,
				  "PackPortal: actionExportInstance not found on "
				  "MainWindow — host may have renamed it. Skipping "
				  "override.");
		return 0;
	}

	/* Sever every existing connection (including MainWindow's own
	 * auto-wired on_actionExportInstance_triggered slot) and replace
	 * with our handler. The QObject we anchor to is g_guard so the
	 * lambda's lifetime is tied to the plugin's, not to the action's
	 * (the action outlives mmco_unload, the guard does not). */
	action->disconnect();
	QObject::connect(action, &QAction::triggered, g_guard,
					 []() { runExportDialogFor(currentSelectedInstanceId()); });

	MMCO_LOG(g_ctx,
			 "PackPortal: actionExportInstance hijacked — multi-format "
			 "export dialog now active.");
	return 0;
}

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;
	MMCO_LOG(ctx, "PackPortal initialising…");

	/* Anchor object for our long-lived Qt signal/slot connections.
	 * Mirrors the NVIDIAPrime / SkinManager idiom — we intentionally
	 * leak this on unload (just drop the pointer) because Qt may still
	 * have queued events targeting it. */
	g_guard = new QObject();

	// The plugin's ONLY UI hook is the MAIN_READY one — it replaces
	// the launcher's built-in actionExportInstance handler with our
	// multi-format dialog. We deliberately do NOT:
	//   • register a per-instance BasePage     (would inject a
	//                                            "Pack Export" tab
	//                                            into every instance's
	//                                            Settings panel)
	//   • register a per-instance toolbar action via
	//     ui_register_instance_action          (would add an extra
	//                                            "Pack Export" row to
	//                                            the right-side toolbar)
	//   • register a context-menu entry        (duplicate of the
	//                                            existing Export
	//                                            button)
	//   • register an "Import Pack" action     (covered by the stock
	//                                            Add Instance →
	//                                            Import from zip
	//                                            flow, which already
	//                                            recognises mrpack /
	//                                            CurseForge / MultiMC
	//                                            archives).
	// The principle is "override the existing button, don't add new
	// surface". Users keep their existing muscle memory; we just
	// upgrade the dialog that pops open.
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_UI_MAIN_READY,
					   on_ui_main_ready, nullptr);

	MMCO_LOG(ctx, "PackPortal ready.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx)
		MMCO_LOG(g_ctx, "PackPortal unloading.");
	g_ctx = nullptr;
	/* g_guard intentionally leaked — see comment in mmco_init. */
	g_guard = nullptr;
}

} /* extern "C" */
