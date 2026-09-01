/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QByteArray>
#include <QDir>
#include <QFileInfoList>
#include <QFuture>
#include <QFutureWatcher>
#include <QList>
#include <QMap>
#include <QString>

#include <atomic>

#include "MMCZip.h"
#include "QObjectPtr.h"
#include "tasks/Task.h"

class MinecraftInstance;

namespace Net
{
	class JsonPost;
}

/* What the export dialog collected. Passed as one struct because these
 * are the dialog's fields, and a seven-argument constructor is a place
 * for two of them to be swapped without the compiler noticing. */
struct FlamePackExportOptions {
	QString name;
	QString version;
	QString author;
	bool optionalFiles = true;
	MinecraftInstance* instance = nullptr;
	QString output;
	MMCZip::FilterFileFunction filter;
	/* MiB, or 0 for "do not state a requirement". */
	int recommendedRAM = 0;
};

/*
 * Writing an instance out as a CurseForge pack.
 *
 * The shape is the same as the Modrinth exporter's - a manifest plus an
 * `overrides/` folder, where anything the manifest can name is left out
 * of the archive and fetched on install - but the way a file gets named
 * is not, and that difference drives the whole task.
 *
 * Modrinth indexes files by digest, so a mod can be identified with the
 * SHA-1 we already have. CurseForge indexes them by FlameFingerprint and
 * by nothing else, and it answers questions about fingerprints only in
 * bulk, over POST. So this task hashes candidates on a worker thread,
 * asks about all of the unknown ones at once, and then has to ask a
 * second question - `/v1/mods` - because a fingerprint match names a
 * project by number and `modlist.html` has to print its title, its slug
 * and its authors.
 *
 * The second question is asked for every named file rather than only for
 * the ones the sidecars could not describe: this launcher's sidecars
 * record a project id and a slug but never an author list, so treating a
 * locally resolved file as fully known would produce a mod list where
 * some entries say who wrote them and others do not, decided by which
 * mods happened to be installed through the launcher.
 *
 * Only `mods/` and `resourcepacks/` are named. Those are the folders a
 * CurseForge pack carries as project references; everything else in an
 * instance travels in `overrides/`, which is always correct if larger.
 */
class FlamePackExportTask : public Task
{
	Q_OBJECT
  public:
	explicit FlamePackExportTask(FlamePackExportOptions options,
								 QObject* parent = nullptr);

	bool canAbort() const override
	{
		return true;
	}

  public slots:
	bool abort() override;

  protected:
	void executeTask() override;

  private:
	/* A file the manifest can name. `addonId` / `fileId` are what
	 * CurseForge calls the project and the file; the rest is only for
	 * the mod list. */
	struct ResolvedFile {
		int addonId = 0;
		int fileId = 0;
		/* Whether the file is switched on, which decides `required`. A
		 * disabled mod in an optional-files export is offered rather
		 * than imposed. */
		bool enabled = true;
		/* Resource packs are named in the manifest like everything else
		 * but are left out of the mod list, which is a list of mods. */
		bool isMod = true;

		QString name;
		QString slug;
		QString authors;
	};

	/* A file we have fingerprinted but cannot name yet. */
	struct PendingFile {
		QString path; /* relative to the game directory */
		quint32 fingerprint = 0;
		bool enabled = true;
		bool isMod = true;
	};

	struct ScanResult {
		bool ok = false;
		QFileInfoList files;
		QMap<QString, ResolvedFile> resolved;
		QList<PendingFile> pending;
	};

	/* Runs on a worker thread: walks the game directory, fingerprints
	 * the candidates and reads their sidecars. */
	ScanResult scanFiles() const;
	void onScanFinished();

	/* The bulk fingerprint question. Skipped when the sidecars already
	 * accounted for everything. */
	void matchFingerprints();
	void onFingerprintsMatched();

	/* Titles, slugs and authors for everything that ended up named. */
	void lookUpProjects();
	void onProjectsLookedUp();

	void buildZip();
	QByteArray generateManifest() const;
	QByteArray generateModList() const;

	/* Neither question is worth failing an export over, so a failure
	 * carries on to the next step instead of stopping.
	 *
	 * A fingerprint service that is down means files ship inside the
	 * pack rather than being referenced by it; a missing project title
	 * means a mod list entry falls back to what the sidecar knew. Both
	 * produce a worse pack, and both are better than no pack. Two
	 * handlers because they resume in different places. */
	void onFingerprintMatchFailed(const QString& reason);
	void onProjectLookupFailed(const QString& reason);

	const FlamePackExportOptions m_options;
	const QDir m_gameRoot;

	QFileInfoList m_files;
	QMap<QString, ResolvedFile> m_resolved;
	QList<PendingFile> m_pending;

	shared_qobject_ptr<Net::JsonPost> m_request;
	Task::Ptr m_zipTask;

	QFuture<ScanResult> m_scanFuture;
	QFutureWatcher<ScanResult> m_scanWatcher;

	/* Latched by abort(). Read by the scan between files, and by every
	 * handler that might otherwise report a second verdict after the
	 * aborted one. */
	std::atomic<bool> m_aborted{false};
};
