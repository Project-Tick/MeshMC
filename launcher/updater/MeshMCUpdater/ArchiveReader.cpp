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

#include "ArchiveReader.h"

#include "UpdaterUtil.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <archive.h>
#include <archive_entry.h>

namespace ArchiveReader
{

	namespace
	{

		constexpr size_t kReadBlockSize = 64 * 1024;

		QString lastError(archive* handle)
		{
			const char* message = archive_error_string(handle);
			return message != nullptr
					   ? QString::fromLocal8Bit(message)
					   : QStringLiteral("unknown libarchive error");
		}

		//! Entry name, preferring the UTF-8 field the container may carry.
		QString entryName(archive_entry* entry)
		{
			if (const char* utf8 = archive_entry_pathname_utf8(entry))
				return QString::fromUtf8(utf8);
			if (const char* local = archive_entry_pathname(entry))
				return QString::fromLocal8Bit(local);
			return QString();
		}

		/*!
		 * Resolve \a name against \a destDir, refusing anything that escapes
		 * it.
		 *
		 * Covers "../.." components, absolute paths and, on Windows,
		 * drive-qualified names -- none of which libarchive rejects on its own
		 * by default.
		 */
		bool resolveEntryPath(const QDir& destDir, const QString& name,
							  QString& resolved)
		{
			if (name.isEmpty())
				return false;

			const QString normalised = QDir::fromNativeSeparators(name);
			if (QDir::isAbsolutePath(normalised))
				return false;

			const QString base = QDir::cleanPath(destDir.absolutePath());
			const QString candidate =
				QDir::cleanPath(base + QLatin1Char('/') + normalised);

			if (candidate != base &&
				!candidate.startsWith(base + QLatin1Char('/')))
				return false;

			resolved = candidate;
			return true;
		}

	} // namespace

	Result extract(const QString& archivePath, const QString& destDir)
	{
		Result result;

		if (!UpdaterUtil::ensureDirectory(destDir)) {
			result.error = QStringLiteral("cannot create %1")
							   .arg(QDir::toNativeSeparators(destDir));
			return result;
		}

		archive* reader = archive_read_new();
		archive_read_support_format_all(reader);
		archive_read_support_filter_all(reader);

		// Open by wide path on Windows: the staging area lives under the user's
		// profile, which routinely contains characters the local 8-bit codec
		// cannot represent.
#ifdef Q_OS_WIN
		const std::wstring nativePath =
			QDir::toNativeSeparators(archivePath).toStdWString();
		const int openStatus = archive_read_open_filename_w(
			reader, nativePath.c_str(), kReadBlockSize);
#else
		const int openStatus = archive_read_open_filename(
			reader, archivePath.toLocal8Bit().constData(), kReadBlockSize);
#endif
		if (openStatus != ARCHIVE_OK) {
			result.error = QStringLiteral("cannot open archive: %1")
							   .arg(lastError(reader));
			archive_read_free(reader);
			return result;
		}

		archive* writer = archive_write_disk_new();
		archive_write_disk_set_options(
			writer, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
						ARCHIVE_EXTRACT_SECURE_NODOTDOT |
						ARCHIVE_EXTRACT_SECURE_SYMLINKS);
		archive_write_disk_set_standard_lookup(writer);

		const QDir base(destDir);
		archive_entry* entry = nullptr;
		int status = ARCHIVE_OK;

		while ((status = archive_read_next_header(reader, &entry)) ==
			   ARCHIVE_OK) {
			const QString name = entryName(entry);
			QString target;
			if (!resolveEntryPath(base, name, target)) {
				result.error =
					QStringLiteral("archive entry \"%1\" points outside the "
								   "extraction directory")
						.arg(name);
				break;
			}

			archive_entry_set_pathname_utf8(entry, target.toUtf8().constData());

			// archive_write_disk does not reliably create missing parents.
			if (!UpdaterUtil::ensureDirectory(
					QFileInfo(target).absolutePath())) {
				result.error = QStringLiteral("cannot create directory %1")
								   .arg(QFileInfo(target).absolutePath());
				break;
			}

			const int headerStatus = archive_write_header(writer, entry);
			if (headerStatus < ARCHIVE_WARN) {
				result.error = QStringLiteral("cannot write %1: %2")
								   .arg(name, lastError(writer));
				break;
			}
			if (headerStatus != ARCHIVE_OK)
				qWarning() << "ArchiveReader:" << name << ":"
						   << lastError(writer);

			if (archive_entry_size(entry) > 0) {
				const void* buffer = nullptr;
				size_t size = 0;
				la_int64_t offset = 0;
				int blockStatus = ARCHIVE_OK;
				while ((blockStatus = archive_read_data_block(
							reader, &buffer, &size, &offset)) == ARCHIVE_OK) {
					if (archive_write_data_block(writer, buffer, size, offset) <
						ARCHIVE_WARN) {
						result.error = QStringLiteral("cannot write %1: %2")
										   .arg(name, lastError(writer));
						blockStatus = ARCHIVE_FATAL;
						break;
					}
					result.byteCount += static_cast<qint64>(size);
				}
				if (blockStatus != ARCHIVE_EOF) {
					if (result.error.isEmpty())
						result.error = QStringLiteral("cannot read %1: %2")
										   .arg(name, lastError(reader));
					break;
				}
			}

			archive_write_finish_entry(writer);
			if (!archive_entry_filetype(entry) ||
				archive_entry_filetype(entry) == AE_IFREG)
				++result.fileCount;
		}

		if (result.error.isEmpty() && status != ARCHIVE_EOF &&
			status != ARCHIVE_OK)
			result.error = QStringLiteral("archive ended unexpectedly: %1")
							   .arg(lastError(reader));

		archive_read_close(reader);
		archive_read_free(reader);
		archive_write_close(writer);
		archive_write_free(writer);

		result.ok = result.error.isEmpty();
		return result;
	}

	QString descendIntoSingleRoot(const QString& dir)
	{
		const QDir base(dir);
		const QStringList directories =
			base.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
		const QStringList files = base.entryList(
			QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);

		if (directories.size() == 1 && files.isEmpty())
			return base.absoluteFilePath(directories.first());
		return base.absolutePath();
	}

} // namespace ArchiveReader
