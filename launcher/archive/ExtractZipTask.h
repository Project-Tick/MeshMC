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

#include <QDir>
#include <QFuture>
#include <QFutureWatcher>
#include <QString>
#include <QStringList>

#include <atomic>
#include <optional>

#include "tasks/Task.h"

namespace MMCZip
{
	/*
	 * Unpacking an archive, off the UI thread, with a progress bar and a
	 * way out.
	 *
	 * The modpack importer used to hand MMCZip::extractSubDir() to
	 * QtConcurrent::run() and watch the future: the work did happen in
	 * the background, but nothing came back until it was over. A
	 * multi-gigabyte pack looked like a frozen "Extracting modpack"
	 * label with a bar that never moved, and pressing the dialog's abort
	 * button did nothing at all, because there was nothing to press it
	 * against.
	 *
	 * `subdirectory` is a prefix inside the archive; only entries under
	 * it are written out, with the prefix removed. Empty means the whole
	 * archive - which is what the formats that ship their files at the
	 * root need.
	 */
	class ExtractZipTask : public Task
	{
		Q_OBJECT
	  public:
		ExtractZipTask(QString input, QDir outputDir,
					   QString subdirectory = QString(),
					   QObject* parent = nullptr);
		~ExtractZipTask() override = default;

		/* The absolute paths of everything that was written. Only
		 * meaningful after the task succeeded. */
		QStringList extractedFiles() const
		{
			return m_extracted;
		}

		bool canAbort() const override
		{
			return true;
		}
		bool abort() override;

	  protected:
		void executeTask() override;

	  private:
		/* Empty on success, the reason on failure. */
		using ZipResult = std::optional<QString>;

		ZipResult extractZip();
		void finish();

		/* See ExportToZipTask: the unpacking runs on a worker thread,
		 * and task state may only be touched on the thread the task
		 * lives on. */
		void reportStatus(const QString& status);
		void reportProgress(qint64 current, qint64 total);

		QString m_input;
		QDir m_outputDir;
		QString m_subdirectory;
		QStringList m_extracted;

		/* See ExportToZipTask: the future from QtConcurrent::run() cannot
		 * be cancelled, so the flag has to be ours. */
		std::atomic<bool> m_cancelled{false};

		QFuture<ZipResult> m_future;
		QFutureWatcher<ZipResult> m_watcher;
	};
} // namespace MMCZip
