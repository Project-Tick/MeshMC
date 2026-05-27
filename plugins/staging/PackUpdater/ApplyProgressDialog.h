/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ApplyProgressDialog — modal driver for "Apply Update".
 *
 * Walks the user through six steps in order:
 *
 *   1. Confirm plan        — show file + component diff, get OK.
 *   2. Backup              — S10 zip the whole instance.
 *   3. Download manifest   — fetch new mrpack zip (UpdateApplier).
 *   4. Apply files         — Add / Replace / Remove via S05+S09.
 *   5. Apply overrides     — extract `overrides/` from the mrpack
 *                            zip into the instance .minecraft/.
 *   6. Apply components    — instance_component_set_version per
 *                            uid in the plan.
 *   7. Update metadata     — bump PackVersionId / Label /
 *                            InstalledAt on the instance.cfg keys.
 *
 * Anything past step 1 that fails surfaces a dialog with
 * "Restore from backup?" so the user isn't stranded in a
 * half-applied state.
 *
 * The dialog owns the apply state machine — there's no separate
 * task object. Worker threads aren't involved; we lean on Qt's
 * event loop and step through asynchronously after each HTTP /
 * download response. That keeps the cancel button responsive
 * without dragging in QThread / QtConcurrent.
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"
#include "PackMetadata.h"
#include "UpdateApplier.h"

class QLabel;
class QProgressBar;
class QPushButton;
class QTextEdit;

class ApplyProgressDialog : public QDialog
{
	Q_OBJECT
  public:
	ApplyProgressDialog(const QString& instanceId, const QString& instanceRoot,
						const pack_updater::PackRecord& installed,
						const QUrl& newPackUrl, const QString& newVersionId,
						const QString& newVersionLabel, MMCOContext* ctx,
						QWidget* parent = nullptr);

  private:
	enum Step {
		StepIdle,
		StepDownload,
		StepDiff,
		StepConfirm,
		StepBackup,
		StepFiles,
		StepOverrides,
		StepComponents,
		StepMetadata,
		StepDone,
		StepFailed,
	};

	void buildUi();
	void advance();
	void setStatus(const QString& text);
	void setProgress(int pct);

	void doDownload();
	void onManifestReady(pack_updater::ParsedManifest m);
	void doDiff();
	void doConfirm();
	void doBackup();
	void doFiles();
	void onFilesDone(bool ok, const QString& failMsg);
	void doOverrides();
	void doComponents();
	void doMetadata();

	void fail(const QString& msg);
	void offerRestore();

	QString m_instanceId;
	QString m_instanceRoot;
	pack_updater::PackRecord m_installed;
	QUrl m_newPackUrl;
	QString m_newVersionId;
	QString m_newVersionLabel;
	MMCOContext* m_ctx = nullptr;

	QString m_scratchDir;
	QString m_backupPath;
	pack_updater::ParsedManifest m_manifest;
	pack_updater::UpdatePlan m_plan;
	Step m_step = StepIdle;

	QLabel* m_titleLabel = nullptr;
	QLabel* m_statusLabel = nullptr;
	QProgressBar* m_progress = nullptr;
	QTextEdit* m_planView = nullptr;
	QPushButton* m_primaryBtn = nullptr;
	QPushButton* m_cancelBtn = nullptr;
};
