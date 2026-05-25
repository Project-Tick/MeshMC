/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ApplyProgressDialog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>

ApplyProgressDialog::ApplyProgressDialog(
	const QString& instanceId, const QString& instanceRoot,
	const pack_updater::PackRecord& installed, const QUrl& newPackUrl,
	const QString& newVersionId, const QString& newVersionLabel,
	MMCOContext* ctx, QWidget* parent)
	: QDialog(parent), m_instanceId(instanceId), m_instanceRoot(instanceRoot),
	  m_installed(installed), m_newPackUrl(newPackUrl),
	  m_newVersionId(newVersionId), m_newVersionLabel(newVersionLabel),
	  m_ctx(ctx)
{
	setWindowTitle(tr("Apply Pack Update"));
	setMinimumSize(620, 480);
	setModal(true);
	buildUi();

	/* Scratch dir lives under the system temp area — survives the
	 * dialog instance but cleans up on next reboot. The pack id +
	 * a uuid component prevent collisions when the user applies
	 * updates across multiple instances back to back. */
	m_scratchDir = QStandardPaths::writableLocation(
					   QStandardPaths::TempLocation) +
				   QStringLiteral("/packupdater/") +
				   QUuid::createUuid().toString(QUuid::WithoutBraces);

	/* Kick off the download as soon as the dialog is visible —
	 * single shot so the layout has a tick to settle. */
	QTimer::singleShot(0, this, [this]() {
		m_step = StepDownload;
		doDownload();
	});
}

void ApplyProgressDialog::buildUi()
{
	auto* outer = new QVBoxLayout(this);

	m_titleLabel = new QLabel(this);
	auto f = m_titleLabel->font();
	f.setBold(true);
	f.setPointSize(f.pointSize() + 2);
	m_titleLabel->setFont(f);
	m_titleLabel->setText(tr("Updating %1 → %2")
							  .arg(m_installed.installedVersionLabel.isEmpty()
									   ? QStringLiteral("?")
									   : m_installed.installedVersionLabel,
								   m_newVersionLabel));
	outer->addWidget(m_titleLabel);

	m_statusLabel = new QLabel(tr("Preparing…"), this);
	m_statusLabel->setWordWrap(true);
	outer->addWidget(m_statusLabel);

	m_progress = new QProgressBar(this);
	m_progress->setRange(0, 100);
	m_progress->setValue(0);
	outer->addWidget(m_progress);

	m_planView = new QTextEdit(this);
	m_planView->setReadOnly(true);
	m_planView->setVisible(false);
	outer->addWidget(m_planView, 1);

	auto* btnRow = new QHBoxLayout();
	btnRow->addStretch(1);
	m_primaryBtn = new QPushButton(tr("Apply"), this);
	m_primaryBtn->setEnabled(false);
	m_cancelBtn = new QPushButton(tr("Cancel"), this);
	connect(m_primaryBtn, &QPushButton::clicked, this,
			&ApplyProgressDialog::advance);
	connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
	btnRow->addWidget(m_primaryBtn);
	btnRow->addWidget(m_cancelBtn);
	outer->addLayout(btnRow);
}

void ApplyProgressDialog::setStatus(const QString& text)
{
	m_statusLabel->setText(text);
}

void ApplyProgressDialog::setProgress(int pct)
{
	m_progress->setValue(pct);
}

void ApplyProgressDialog::advance()
{
	switch (m_step) {
		case StepConfirm:
			m_step = StepBackup;
			doBackup();
			break;
		case StepDone:
			accept();
			break;
		case StepFailed:
			offerRestore();
			break;
		default:
			break;
	}
}

/* ─── Step 1: download ──────────────────────────────────────────── */
void ApplyProgressDialog::doDownload()
{
	setStatus(tr("Downloading new pack manifest…"));
	setProgress(5);
	pack_updater::fetchAndParseManifest(
		m_ctx, m_installed.provider, m_newPackUrl, m_scratchDir,
		[this](pack_updater::ParsedManifest m) { onManifestReady(m); });
}

void ApplyProgressDialog::onManifestReady(pack_updater::ParsedManifest m)
{
	if (m.versionId.isEmpty() && m.files.isEmpty()) {
		fail(tr("Could not download or parse the new pack."));
		return;
	}
	m_manifest = std::move(m);
	setProgress(20);
	m_step = StepDiff;
	doDiff();
}

/* ─── Step 2: diff + confirm ────────────────────────────────────── */
void ApplyProgressDialog::doDiff()
{
	setStatus(tr("Comparing installed mods against the new pack…"));
	m_plan = pack_updater::diffAgainstInstance(
		m_ctx, m_instanceId, m_instanceRoot, m_installed, m_manifest);
	if (!m_plan.ok) {
		fail(m_plan.errorMessage.isEmpty()
				 ? tr("Could not build update plan.")
				 : m_plan.errorMessage);
		return;
	}
	setProgress(30);
	m_step = StepConfirm;
	doConfirm();
}

void ApplyProgressDialog::doConfirm()
{
	/* Render the plan as a human summary. Keep it text so users
	 * can copy-paste it into bug reports / discord. */
	QStringList lines;
	int adds = 0, replaces = 0, removes = 0;
	for (const auto& a : m_plan.files) {
		switch (a.kind) {
			case pack_updater::FileAction::Add:
				adds++;
				lines << QStringLiteral("  + %1").arg(a.instanceRelativePath);
				break;
			case pack_updater::FileAction::Replace:
				replaces++;
				lines << QStringLiteral("  ~ %1").arg(a.instanceRelativePath);
				break;
			case pack_updater::FileAction::Remove:
				removes++;
				lines << QStringLiteral("  − %1").arg(a.instanceRelativePath);
				break;
		}
	}
	QString summary;
	summary += tr("Files:  %1 added, %2 replaced, %3 removed\n")
				   .arg(adds)
				   .arg(replaces)
				   .arg(removes);
	if (m_plan.hasComponentChanges()) {
		summary += tr("\nLoader / Minecraft version changes:\n");
		for (const auto& c : m_plan.components) {
			summary += QStringLiteral("  %1 → %2\n").arg(c.uid, c.newVersion);
		}
		summary += tr("\nYou'll be asked once more before these are "
					  "written.\n");
	}
	summary += tr("\nA full backup will be taken before any changes.\n\n");
	summary += tr("Per-file detail:\n");
	summary += lines.join('\n');

	m_planView->setPlainText(summary);
	m_planView->setVisible(true);

	if (!m_plan.hasFileChanges() && !m_plan.hasComponentChanges()) {
		setStatus(tr("Nothing to apply — the instance already matches "
					 "the new pack."));
		m_primaryBtn->setText(tr("Close"));
		m_primaryBtn->setEnabled(true);
		m_step = StepDone;
		return;
	}

	setStatus(tr("Review the plan, then click Apply to proceed."));
	m_primaryBtn->setText(tr("Apply"));
	m_primaryBtn->setEnabled(true);
}

/* ─── Step 3: backup ────────────────────────────────────────────── */
void ApplyProgressDialog::doBackup()
{
	setStatus(tr("Backing up instance…"));
	setProgress(40);
	m_primaryBtn->setEnabled(false);

	const QString stamp = QDateTime::currentDateTimeUtc().toString(
		QStringLiteral("yyyyMMdd-HHmmss"));
	const QString backupDir =
		m_instanceRoot + QStringLiteral("/.backups");
	QDir().mkpath(backupDir);
	m_backupPath = QStringLiteral("%1/%2-pre-pack-update.zip")
					   .arg(backupDir, stamp);

	if (!m_ctx || !m_ctx->zip_compress_dir) {
		fail(tr("This launcher build is missing zip compress support."));
		return;
	}
	const QByteArray zipUtf8 = m_backupPath.toUtf8();
	const QByteArray dirUtf8 = m_instanceRoot.toUtf8();
	if (m_ctx->zip_compress_dir(m_ctx->module_handle, zipUtf8.constData(),
								dirUtf8.constData()) != 0) {
		fail(tr("Backup failed (zip_compress_dir returned an error)."));
		return;
	}
	setProgress(55);
	m_step = StepFiles;
	doFiles();
}

/* ─── Step 4: apply file changes ────────────────────────────────── */
namespace
{
	/* Map the on-disk folder name (what we record in sidecars
	 * and what the manifest stores in `path`) to the S05 type
	 * selector. They're close but not identical — `mods/` is
	 * S05's "loader", `coremods/` is "core", the others match. */
	const char* s05TypeForFolder(const QString& folder)
	{
		if (folder == QLatin1String("mods"))
			return "loader";
		if (folder == QLatin1String("coremods"))
			return "core";
		if (folder == QLatin1String("resourcepacks"))
			return "resourcepack";
		if (folder == QLatin1String("shaderpacks"))
			return "shaderpack";
		if (folder == QLatin1String("texturepacks"))
			return "texturepack";
		return nullptr;
	}

	/* Find the S05 index of a mod by file name. Walks
	 * mod_get_filename across [0, mod_count). Returns -1 when no
	 * match. The S05 string accessors return tempString pointers
	 * that get stomped on the next API call, so we copy
	 * immediately. */
	int findModIndex(MMCOContext* ctx, const QByteArray& idUtf8,
					 const char* type, const QString& fileName)
	{
		if (!ctx->mod_count || !ctx->mod_get_filename)
			return -1;
		const int n =
			ctx->mod_count(ctx->module_handle, idUtf8.constData(), type);
		for (int i = 0; i < n; ++i) {
			const char* fn = ctx->mod_get_filename(
				ctx->module_handle, idUtf8.constData(), type, i);
			if (!fn)
				continue;
			if (QString::fromUtf8(fn) == fileName)
				return i;
		}
		return -1;
	}
} /* namespace */

void ApplyProgressDialog::doFiles()
{
	setStatus(tr("Applying mod / pack file changes…"));
	setProgress(60);

	if (!m_ctx || !m_ctx->mod_remove || !m_ctx->mod_install ||
		!m_ctx->mod_refresh || !m_ctx->mod_count ||
		!m_ctx->mod_get_filename) {
		fail(tr("Mod-management API unavailable on this launcher."));
		return;
	}

	const QByteArray idUtf8 = m_instanceId.toUtf8();

	/* Removes first — frees disk + sidecars before we drop in
	 * the new versions. Each remove call needs the *current*
	 * index, so we walk filenames each time. (Indices shift after
	 * every successful remove, but findModIndex re-walks, so we
	 * don't have to track that.) Failures here are non-fatal: we
	 * log and continue; the user has a backup. */
	int removed = 0, missed = 0;
	for (const auto& a : m_plan.files) {
		if (a.kind != pack_updater::FileAction::Remove)
			continue;
		const char* type = s05TypeForFolder(a.folder);
		if (!type)
			continue;
		const int idx = findModIndex(m_ctx, idUtf8, type, a.fileName);
		if (idx < 0) {
			/* The mod model didn't know about this file. Fall
			 * back to deleting the jar straight off disk — this
			 * is how manually-dropped mods get cleaned up on
			 * update (the launcher's mod model only indexes files
			 * it imported, so anything else is invisible to
			 * mod_remove). */
			const QString diskPath = m_instanceRoot +
									 QStringLiteral("/.minecraft/") +
									 a.folder + QStringLiteral("/") +
									 a.fileName;
			if (QFile::remove(diskPath)) {
				removed++;
				MMCO_LOG(m_ctx,
						 QString("PackUpdater: removed orphan %1/%2")
							 .arg(a.folder, a.fileName)
							 .toUtf8()
							 .constData());
			} else {
				missed++;
				MMCO_LOG(m_ctx,
						 QString("PackUpdater: could not remove %1/%2 "
								 "(not found on disk?)")
							 .arg(a.folder, a.fileName)
							 .toUtf8()
							 .constData());
			}
			continue;
		}
		if (m_ctx->mod_remove(m_ctx->module_handle, idUtf8.constData(),
							  type, idx) == 0) {
			removed++;
		} else {
			missed++;
		}
	}
	MMCO_LOG(m_ctx, QString("PackUpdater: remove pass: %1 removed, "
							"%2 skipped/failed")
						.arg(removed)
						.arg(missed)
						.toUtf8()
						.constData());

	/* Add / Replace: download each file into a scratch dir, then
	 * hand the disk path to mod_install which copies it into the
	 * right folder. Replace = Remove old + install new; we do the
	 * remove inline here so we don't need a separate pass.
	 *
	 * We track pending count on a dynamic property so the C-ABI
	 * HTTP thunks can decrement it without threading state
	 * through the thunk struct. */
	const QString stagingDir = m_scratchDir + QStringLiteral("/staging");
	QDir().mkpath(stagingDir);

	int pending = 0;
	for (const auto& a : m_plan.files) {
		if (a.kind == pack_updater::FileAction::Remove)
			continue;
		if (!a.downloadUrl.isValid())
			continue;
		pending++;
	}
	setProperty("packupd_pending", pending);
	setProperty("packupd_failed", 0);

	if (pending == 0) {
		onFilesDone(true, {});
		return;
	}

	struct DlState {
		QPointer<ApplyProgressDialog> dlg;
		QString stagedPath;
		QString fileName;
		QString folder;
		bool isReplace; /* true → remove the old one before install */
	};

	for (const auto& a : m_plan.files) {
		if (a.kind == pack_updater::FileAction::Remove)
			continue;
		if (!a.downloadUrl.isValid())
			continue;

		const QString stagedPath =
			stagingDir + QStringLiteral("/") + a.fileName;

		auto* state = new DlState{this, stagedPath, a.fileName, a.folder,
								  a.kind == pack_updater::FileAction::Replace};

		const QByteArray urlUtf8 = a.downloadUrl.toString().toUtf8();
		int rc = m_ctx->http_get(
			m_ctx->module_handle, urlUtf8.constData(),
			[](void* ud, int status, const void* body, size_t size) {
				std::unique_ptr<DlState> s(static_cast<DlState*>(ud));
				if (!s->dlg)
					return;

				bool ok = (status >= 200 && status < 300);
				if (ok) {
					QFile out(s->stagedPath);
					if (out.open(QIODevice::WriteOnly |
								 QIODevice::Truncate)) {
						out.write(static_cast<const char*>(body),
								  qint64(size));
						out.close();
					} else {
						ok = false;
					}
				}

				if (ok) {
					/* Install through S05 so the launcher's mod
					 * model picks it up and the sidecar can be
					 * (re)written by the launcher's normal install
					 * flow. */
					MMCOContext* ctx = s->dlg->m_ctx;
					const QByteArray idUtf8 =
						s->dlg->m_instanceId.toUtf8();
					const char* type = s05TypeForFolder(s->folder);
					if (type) {
						if (s->isReplace) {
							const int idx = findModIndex(ctx, idUtf8, type,
														 s->fileName);
							if (idx >= 0) {
								ctx->mod_remove(ctx->module_handle,
												idUtf8.constData(), type,
												idx);
							}
						}
						const QByteArray pathUtf8 =
							s->stagedPath.toUtf8();
						if (ctx->mod_install(ctx->module_handle,
											 idUtf8.constData(), type,
											 pathUtf8.constData()) != 0)
							ok = false;
					} else {
						ok = false;
					}
					QFile::remove(s->stagedPath);
				}

				int p = s->dlg->property("packupd_pending").toInt() - 1;
				int f = s->dlg->property("packupd_failed").toInt() +
						(ok ? 0 : 1);
				s->dlg->setProperty("packupd_pending", p);
				s->dlg->setProperty("packupd_failed", f);
				if (p <= 0) {
					s->dlg->onFilesDone(
						f == 0,
						f == 0 ? QString()
							   : QObject::tr("%1 file(s) failed to install.")
									 .arg(f));
				}
			},
			state);
		if (rc != 0) {
			delete state;
			setProperty("packupd_pending",
						property("packupd_pending").toInt() - 1);
			setProperty("packupd_failed",
						property("packupd_failed").toInt() + 1);
		}
	}

	/* If queueing failed for everything, bail immediately
	 * (otherwise the HTTP callbacks will eventually drain the
	 * pending count). */
	if (property("packupd_pending").toInt() <= 0 &&
		property("packupd_failed").toInt() > 0) {
		onFilesDone(false, tr("Could not queue downloads."));
	}
}

void ApplyProgressDialog::onFilesDone(bool ok, const QString& failMsg)
{
	if (!ok) {
		fail(failMsg);
		return;
	}
	/* Refresh the mod lists so the launcher's in-memory model
	 * picks up the new files. */
	for (const char* folder : {"loader", "core", "resourcepack",
							   "shaderpack", "texturepack"}) {
		const QByteArray idUtf8 = m_instanceId.toUtf8();
		(void) m_ctx->mod_refresh(m_ctx->module_handle, idUtf8.constData(),
								  folder);
	}
	setProgress(75);
	m_step = StepOverrides;
	doOverrides();
}

/* ─── Step 5: overrides ─────────────────────────────────────────── */
void ApplyProgressDialog::doOverrides()
{
	setStatus(tr("Extracting overrides (configs, scripts)…"));
	setProgress(80);

	if (!m_manifest.downloadedZipPath.isEmpty() && m_ctx &&
		m_ctx->zip_extract) {
		/* Drop the entire mrpack zip into a scratch tree so we can
		 * pick the overrides/ subtree out without re-implementing
		 * a zip reader. */
		const QString outDir =
			m_scratchDir + QStringLiteral("/extracted");
		QDir().mkpath(outDir);
		const QByteArray zipUtf8 = m_manifest.downloadedZipPath.toUtf8();
		const QByteArray outUtf8 = outDir.toUtf8();
		if (m_ctx->zip_extract(m_ctx->module_handle, zipUtf8.constData(),
							   outUtf8.constData()) == 0) {
			const QString src = outDir + QStringLiteral("/overrides");
			const QString dst =
				m_instanceRoot + QStringLiteral("/.minecraft");
			if (QFileInfo::exists(src)) {
				/* QDir doesn't recursively copy out of the box;
				 * we walk and copy. It's not big — overrides are
				 * normally configs and scripts. */
				QDir s(src);
				const auto entries = s.entryInfoList(
					QDir::AllEntries | QDir::NoDotAndDotDot |
					QDir::Hidden);
				std::function<bool(const QFileInfo&, const QString&)> copy;
				copy = [&copy](const QFileInfo& fi, const QString& tgt) {
					if (fi.isDir()) {
						QDir().mkpath(tgt);
						for (const auto& sub :
							 QDir(fi.absoluteFilePath())
								 .entryInfoList(QDir::AllEntries |
												QDir::NoDotAndDotDot |
												QDir::Hidden)) {
							if (!copy(sub,
									  tgt + QStringLiteral("/") +
										  sub.fileName()))
								return false;
						}
						return true;
					}
					QFile::remove(tgt);
					return QFile::copy(fi.absoluteFilePath(), tgt);
				};
				for (const auto& fi : entries)
					copy(fi, dst + QStringLiteral("/") + fi.fileName());
			}
		}
	}

	setProgress(88);
	m_step = StepComponents;
	doComponents();
}

/* ─── Step 6: components (loader / MC version) ──────────────────── */
void ApplyProgressDialog::doComponents()
{
	if (!m_plan.hasComponentChanges()) {
		m_step = StepMetadata;
		doMetadata();
		return;
	}

	/* Per the agreed flow: confirm component changes once more
	 * before writing them, because a wrong loader bump can break
	 * worlds. */
	QString lines;
	for (const auto& c : m_plan.components) {
		lines += QStringLiteral("  %1 → %2\n").arg(c.uid, c.newVersion);
	}
	const auto choice = QMessageBox::question(
		this, tr("Confirm loader / Minecraft changes"),
		tr("The new pack wants these component versions:\n\n%1\n"
		   "Apply now? You already have a backup.")
			.arg(lines),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	if (choice == QMessageBox::Yes && m_ctx &&
		m_ctx->instance_component_set_version) {
		const QByteArray idUtf8 = m_instanceId.toUtf8();
		for (const auto& c : m_plan.components) {
			const QByteArray uidUtf8 = c.uid.toUtf8();
			const QByteArray verUtf8 = c.newVersion.toUtf8();
			(void) m_ctx->instance_component_set_version(
				m_ctx->module_handle, idUtf8.constData(),
				uidUtf8.constData(), verUtf8.constData());
		}
	}
	setProgress(94);
	m_step = StepMetadata;
	doMetadata();
}

/* ─── Step 7: metadata bump ─────────────────────────────────────── */
void ApplyProgressDialog::doMetadata()
{
	setStatus(tr("Saving pack metadata…"));
	pack_updater::PackRecord rec = m_installed;
	rec.installedVersionId = m_newVersionId;
	rec.installedVersionLabel = m_newVersionLabel;
	rec.installedAtIso8601 =
		QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
	pack_updater::save(m_ctx, m_instanceId, rec);

	/* Clear the badge — we're up to date now. */
	if (m_ctx && m_ctx->instance_set_update_available) {
		const QByteArray idUtf8 = m_instanceId.toUtf8();
		m_ctx->instance_set_update_available(m_ctx->module_handle,
											 idUtf8.constData(), 0);
	}

	setProgress(100);
	setStatus(tr("<b style='color:#66aa66'>Update applied.</b> "
				 "Backup kept at <code>%1</code>.")
				  .arg(m_backupPath.toHtmlEscaped()));
	m_primaryBtn->setText(tr("Close"));
	m_primaryBtn->setEnabled(true);
	m_cancelBtn->setVisible(false);
	m_step = StepDone;
}

/* ─── Failure path ──────────────────────────────────────────────── */
void ApplyProgressDialog::fail(const QString& msg)
{
	m_step = StepFailed;
	setStatus(tr("<b style='color:#cc6666'>Failed:</b> %1").arg(msg));
	m_primaryBtn->setText(tr("Restore from backup"));
	m_primaryBtn->setEnabled(!m_backupPath.isEmpty() &&
							 QFile::exists(m_backupPath));
	m_cancelBtn->setText(tr("Close"));
}

void ApplyProgressDialog::offerRestore()
{
	if (m_backupPath.isEmpty() || !QFile::exists(m_backupPath)) {
		QMessageBox::warning(this, tr("No backup"),
							 tr("There's no backup to restore from."));
		return;
	}
	const auto choice = QMessageBox::question(
		this, tr("Restore from backup"),
		tr("Replace the current instance contents with the pre-update "
		   "backup at:\n\n%1\n\nThis cannot be undone.")
			.arg(m_backupPath),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (choice != QMessageBox::Yes)
		return;

	if (!m_ctx || !m_ctx->zip_extract) {
		QMessageBox::warning(this, tr("No extractor"),
							 tr("zip_extract API unavailable."));
		return;
	}
	/* Drop everything under the instance root and re-explode the
	 * backup zip into it. We deliberately keep this destructive
	 * step explicit — the user already confirmed. */
	QDir(m_instanceRoot).removeRecursively();
	QDir().mkpath(m_instanceRoot);

	const QByteArray zipUtf8 = m_backupPath.toUtf8();
	const QByteArray outUtf8 = m_instanceRoot.toUtf8();
	if (m_ctx->zip_extract(m_ctx->module_handle, zipUtf8.constData(),
						   outUtf8.constData()) != 0) {
		QMessageBox::critical(this, tr("Restore failed"),
							  tr("zip_extract returned an error. The "
								 "backup zip is at:\n%1")
								  .arg(m_backupPath));
		return;
	}
	QMessageBox::information(this, tr("Restored"),
							 tr("Instance restored from backup."));
	accept();
}
