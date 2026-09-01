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
#include <QPointer>
#include <QString>

#include <atomic>

#include "MMCZip.h"
#include "QObjectPtr.h"
/* Included rather than forward declared: QPointer<NetJob> below wants the
 * complete type, and DependencyResolver - which holds the same list of
 * in-flight jobs - settled the same way. */
#include "net/NetJob.h"
#include "tasks/Task.h"

class MinecraftInstance;

/*
 * Writing an instance out as a Modrinth pack (`.mrpack`).
 *
 * An mrpack is a manifest plus an `overrides/` folder: every file the
 * manifest can name by URL is *not* in the archive, and gets downloaded
 * on install; everything else is carried along. So the interesting part
 * of this task is deciding which files can be named.
 *
 * That question is answered from the instance's own sidecar index
 * (`mods/.index/*.pw.toml`, see ModMetadataIndex) rather than by asking
 * the platform about every file: the launcher recorded where each
 * managed file came from when it installed it, which is both faster and
 * honest about files it never installed. Files with no usable record are
 * looked up once by hash - that is the only way back for a mod the user
 * dropped in by hand - and whatever is still unresolved after that ships
 * in `overrides/`, which is always correct if larger.
 *
 * Only URLs on the hosts the mrpack format allows are named in the
 * manifest - see ModrinthApi::mrpackHosts() - since those are exactly
 * the ones our own importer will fetch an mrpack's files from; exporting
 * a URL we would refuse to install is worse than shipping the file.
 */
class ModrinthPackExportTask : public Task
{
	Q_OBJECT
  public:
	ModrinthPackExportTask(QString name, QString version, QString summary,
						   bool optionalFiles, MinecraftInstance* instance,
						   QString output,
						   MMCZip::FilterFileFunction filter,
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
	/* A file the manifest can name: it exists on a host we trust, and we
	 * know enough about it to let an installer verify what it got. */
	struct ResolvedFile {
		QString url;
		QString sha1;
		QString sha512;
		qint64 size = 0;
		/* "client" / "server" / "both" / empty, as the sidecar recorded
		 * it. Decides the manifest's `env` block. */
		QString side;
	};

	/* A file we have hashed but cannot name yet. */
	struct PendingFile {
		QString path; /* relative to the game directory */
		QString sha1;
		QString sha512;
		qint64 size = 0;
	};

	struct ScanResult {
		bool ok = false;
		QFileInfoList files;
		QMap<QString, ResolvedFile> resolved;
		QList<PendingFile> pending;
	};

	/* Runs on a worker thread: walks the game directory, hashes the
	 * candidate archives and reads their sidecars. Hashing an instance's
	 * mods is seconds of pure I/O, which is not something to do on the
	 * thread that draws the progress bar. */
	ScanResult scanFiles() const;
	void onScanFinished();

	/* One lookup per file we could not resolve locally. Failures are not
	 * errors: a mod that is not on Modrinth simply travels inside the
	 * pack. */
	void lookUpPendingFiles();

	/* Fold one lookup's answer into m_resolved, when it really named the
	 * file we asked about. Kept apart from the bookkeeping below because
	 * an answer that turns out to be useless still counts as answered. */
	void recordLookupResult(const PendingFile& pending,
							const QByteArray& bytes);
	void onOneLookupDone();

	void buildZip();
	QByteArray generateIndex() const;

	const QString m_name;
	const QString m_version;
	const QString m_summary;
	const bool m_optionalFiles;
	MinecraftInstance* m_instance;
	const QDir m_gameRoot;
	const QString m_output;
	const MMCZip::FilterFileFunction m_filter;

	QFileInfoList m_files;
	QMap<QString, ResolvedFile> m_resolved;
	QList<PendingFile> m_pending;

	int m_lookupsOutstanding = 0;
	int m_lookupsDone = 0;
	int m_lookupsTotal = 0;
	QList<QPointer<NetJob>> m_activeJobs;

	Task::Ptr m_zipTask;

	QFuture<ScanResult> m_scanFuture;
	QFutureWatcher<ScanResult> m_scanWatcher;

	/* Latched by abort(). Read by the scan between files, and by every
	 * handler that might otherwise report a second verdict after the
	 * aborted one. */
	std::atomic<bool> m_aborted{false};
};
