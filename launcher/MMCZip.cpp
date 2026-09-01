/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "MMCZip.h"
#include "FileSystem.h"

#include <archive.h>
#include <archive_entry.h>

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>

#include <memory>

// RAII helpers for libarchive
struct ArchiveReadDeleter {
	void operator()(struct archive* a) const
	{
		archive_read_free(a);
	} // NOLINT(readability-identifier-naming)
};
struct ArchiveWriteDeleter {
	void operator()(struct archive* a) const
	{
		archive_write_free(a);
	} // NOLINT(readability-identifier-naming)
};
using ArchiveReadPtr = std::unique_ptr<
	struct archive,
	ArchiveReadDeleter>; // NOLINT(readability-identifier-naming)
using ArchiveWritePtr = std::unique_ptr<
	struct archive,
	ArchiveWriteDeleter>; // NOLINT(readability-identifier-naming)

static ArchiveReadPtr openZipForReading(const QString& path)
{
	ArchiveReadPtr a(archive_read_new());
	archive_read_support_format_zip(a.get());
	if (archive_read_open_filename(a.get(), path.toUtf8().constData(), 10240) !=
		ARCHIVE_OK) {
		qWarning() << "Could not open archive:" << path
				   << archive_error_string(a.get());
		return nullptr;
	}
	return a;
}

static ArchiveWritePtr createZipForWriting(const QString& path)
{
	ArchiveWritePtr a(archive_write_new());
	archive_write_set_format_zip(a.get());
	if (archive_write_open_filename(a.get(), path.toUtf8().constData()) !=
		ARCHIVE_OK) {
		qWarning() << "Could not create archive:" << path
				   << archive_error_string(a.get());
		return nullptr;
	}
	return a;
}

static bool writeFileToArchive(
	struct archive* aw,
	const QString& entryName, // NOLINT(readability-identifier-naming)
	const QByteArray& data)
{
	struct archive_entry* entry = archive_entry_new();
	archive_entry_set_pathname(entry, entryName.toUtf8().constData());
	archive_entry_set_size(entry, data.size());
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0644);

	if (archive_write_header(aw, entry) != ARCHIVE_OK) {
		archive_entry_free(entry);
		return false;
	}
	if (data.size() > 0) {
		archive_write_data(aw, data.constData(), data.size());
	}
	archive_entry_free(entry);
	return true;
}

static bool writeDiskEntry(struct archive* ar, const QString& absFilePath,
						   QString* error = nullptr)
{
	// Ensure parent directory exists
	QFileInfo fi(absFilePath);
	QDir().mkpath(fi.absolutePath());

	if (absFilePath.endsWith('/')) {
		// Directory entry
		QDir().mkpath(absFilePath);
		return true;
	}

	QFile outFile(absFilePath);
	if (!outFile.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to open for writing:" << absFilePath;
		if (error) {
			*error = QStringLiteral("could not open '%1' for writing: %2")
						 .arg(absFilePath, outFile.errorString());
		}
		return false;
	}

	// Every bail-out below leaves a truncated file behind. A zero-byte native
	// library or mod jar is worse than none at all: the caller sees a failure,
	// but the file stays on disk and the next launch happily loads the corpse.
	// So clean up on every failure path.
	const auto fail = [&outFile, &absFilePath](QString* out,
											   const QString& why) {
		outFile.close();
		if (!QFile::remove(absFilePath)) {
			qWarning() << "Could not remove partial file" << absFilePath;
		}
		if (out) {
			*out = why;
		}
		return false;
	};

	const void* buff;
	size_t size;
	la_int64_t offset;
	int readRet;
	while ((readRet = archive_read_data_block(ar, &buff, &size, &offset)) ==
		   ARCHIVE_OK) {
		qint64 written = outFile.write(static_cast<const char*>(buff), size);
		if (written != static_cast<qint64>(size)) {
			qWarning() << "Write error for" << absFilePath << ": wrote"
					   << written << "of" << size << "bytes";
			return fail(error,
						QStringLiteral("write error for '%1': %2")
							.arg(absFilePath, outFile.errorString()));
		}
	}

	if (readRet != ARCHIVE_EOF) {
		const char* archiveError = archive_error_string(ar);
		qWarning() << "Archive read error while extracting" << absFilePath
				   << ":" << archiveError;
		return fail(error, QString::fromUtf8(archiveError
												 ? archiveError
												 : "unknown archive error"));
	}

	outFile.close();
	return true;
}

bool MMCZip::compressDir(QString zipFile, QString dir,
						 FilterFunction excludeFilter,
						 ProgressFunction progress)
{
	auto aw = createZipForWriting(zipFile);
	if (!aw)
		return false;

	QDir directory(dir);
	if (!directory.exists())
		return false;

	// Counting up front costs one directory walk with no file reads,
	// which is nothing next to reading and compressing everything we
	// just counted — and it is the difference between a percentage and
	// a bar that only sweeps. Only pay for it if someone is listening.
	qint64 total = 0;
	qint64 done = 0;
	if (progress) {
		QDirIterator counter(dir, QDir::Files | QDir::Hidden,
							 QDirIterator::Subdirectories);
		while (counter.hasNext()) {
			counter.next();
			if (excludeFilter &&
				excludeFilter(directory.relativeFilePath(counter.filePath())))
				continue;
			total++;
		}
		progress(0, total);
	}

	QDirIterator it(dir, QDir::Files | QDir::Hidden,
					QDirIterator::Subdirectories);
	bool success = true;
	while (it.hasNext()) {
		it.next();
		QString relPath = directory.relativeFilePath(it.filePath());
		if (excludeFilter && excludeFilter(relPath))
			continue;

		QFile file(it.filePath());
		if (!file.open(QIODevice::ReadOnly)) {
			success = false;
			break;
		}
		QByteArray data = file.readAll();
		file.close();

		if (!writeFileToArchive(aw.get(), relPath, data)) {
			success = false;
			break;
		}

		if (progress) {
			progress(++done, total);
		}
	}

	archive_write_close(aw.get());
	return success;
}

bool MMCZip::collectFileListRecursively(const QString& rootDir,
									   const QString& subDir,
									   QFileInfoList* files,
									   FilterFileFunction excludeFilter)
{
	QDir rootDirectory(rootDir);
	if (!rootDirectory.exists())
		return false;

	QDir directory = subDir.isNull() ? rootDirectory : QDir(subDir);
	if (!directory.exists())
		return false; // shouldn't ever happen

	// recurse into subdirectories first, so the list comes out in a
	// stable, depth-first order rather than in readdir order
	const QFileInfoList dirs = directory.entryInfoList(
		QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
	for (const auto& dir : dirs) {
		/* A directory the user unchecked is not walked at all: the
		 * filter already answers "no" for everything inside it, and
		 * descending into a blocked `.minecraft/logs` only to throw
		 * every entry away is the slow way of doing nothing. */
		if (excludeFilter && excludeFilter(dir))
			continue;
		if (!collectFileListRecursively(rootDir, dir.filePath(), files,
										excludeFilter))
			return false;
	}

	const QFileInfoList entries =
		directory.entryInfoList(QDir::Files | QDir::Hidden | QDir::System);
	for (const auto& entry : entries) {
		if (excludeFilter && excludeFilter(entry)) {
			qDebug() << "Skipping file"
					 << rootDirectory.relativeFilePath(
							entry.absoluteFilePath());
			continue;
		}
		files->append(entry);
	}
	return true;
}

MMCZip::ZipWriter::ZipWriter(QString path) : m_path(std::move(path)) {}

MMCZip::ZipWriter::~ZipWriter()
{
	(void)close();
}

bool MMCZip::ZipWriter::open()
{
	if (m_archive) {
		return true;
	}
	if (m_path.isEmpty()) {
		m_error = QStringLiteral("no output path was given");
		return false;
	}

	m_archive = archive_write_new();
	if (!m_archive) {
		m_error = QStringLiteral("could not allocate an archive writer");
		return false;
	}
	archive_write_set_format_zip(m_archive);
	/* Names go in as UTF-8 and are flagged as such, so that a pack with
	 * non-Latin file names survives the round trip through other tools. */
	if (archive_write_set_options(m_archive, "hdrcharset=UTF-8") !=
		ARCHIVE_OK) {
		qWarning() << "Could not set archive options for" << m_path << ":"
				   << archive_error_string(m_archive);
	}

	if (archive_write_open_filename(m_archive, m_path.toUtf8().constData()) !=
		ARCHIVE_OK) {
		m_error = QString::fromUtf8(archive_error_string(m_archive));
		qWarning() << "Could not create archive:" << m_path << m_error;
		archive_write_free(m_archive);
		m_archive = nullptr;
		return false;
	}
	return true;
}

bool MMCZip::ZipWriter::close()
{
	if (!m_archive) {
		return true;
	}
	bool success = true;
	if (archive_write_close(m_archive) != ARCHIVE_OK) {
		m_error = QString::fromUtf8(archive_error_string(m_archive));
		qWarning() << "Could not close archive" << m_path << m_error;
		success = false;
	}
	archive_write_free(m_archive);
	m_archive = nullptr;
	return success;
}

bool MMCZip::ZipWriter::addFile(const QString& entryName,
								const QByteArray& data)
{
	if (!m_archive) {
		m_error = QStringLiteral("archive is not open");
		return false;
	}
	if (!writeFileToArchive(m_archive, entryName, data)) {
		m_error = QString::fromUtf8(archive_error_string(m_archive));
		return false;
	}
	return true;
}

bool MMCZip::ZipWriter::addFile(const QString& sourcePath,
								const QString& entryName)
{
	if (!m_archive) {
		m_error = QStringLiteral("archive is not open");
		return false;
	}

	QFile file(sourcePath);
	if (!file.open(QIODevice::ReadOnly)) {
		m_error = QStringLiteral("could not read '%1': %2")
					  .arg(sourcePath, file.errorString());
		return false;
	}

	const QFileInfo info(sourcePath);
	const qint64 size = file.size();

	struct archive_entry* entry = archive_entry_new();
	archive_entry_set_pathname(entry, entryName.toUtf8().constData());
	archive_entry_set_size(entry, size);
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, info.isExecutable() ? 0755 : 0644);
	/* Kept so that an exported instance still shows when its files were
	 * last touched; a zip full of entries dated "now" makes every diff
	 * against the original look like a change. */
	const QDateTime modified = info.lastModified();
	if (modified.isValid()) {
		archive_entry_set_mtime(entry, modified.toSecsSinceEpoch(), 0);
	}

	if (archive_write_header(m_archive, entry) != ARCHIVE_OK) {
		m_error = QString::fromUtf8(archive_error_string(m_archive));
		archive_entry_free(entry);
		return false;
	}
	archive_entry_free(entry);

	/* Streamed rather than read whole: worlds and resource packs are
	 * routinely bigger than it is reasonable to hold in memory, and an
	 * export walks all of them. */
	QByteArray buffer(64 * 1024, Qt::Uninitialized);
	qint64 written = 0;
	while (written < size) {
		const qint64 read = file.read(buffer.data(), buffer.size());
		if (read < 0) {
			m_error = QStringLiteral("could not read '%1': %2")
						  .arg(sourcePath, file.errorString());
			return false;
		}
		if (read == 0) {
			/* The file shrank while we were reading it. The header
			 * already promised `size` bytes, so the entry cannot be
			 * completed - better to fail loudly than to write an
			 * archive libarchive itself considers malformed. */
			m_error = QStringLiteral("'%1' changed size while being read")
						  .arg(sourcePath);
			return false;
		}
		if (archive_write_data(m_archive, buffer.constData(), read) < 0) {
			m_error = QString::fromUtf8(archive_error_string(m_archive));
			return false;
		}
		written += read;
	}

	return true;
}

bool MMCZip::mergeZipFiles(const QString& intoPath, QFileInfo from,
						   QSet<QString>& contained,
						   const FilterFunction filter)
{
	// Read all entries from source zip
	auto ar = openZipForReading(from.filePath());
	if (!ar)
		return false;

	// Read existing entries from target if it exists, then append new ones
	// We build a list of entries to write
	struct EntryData {
		QString name;
		QByteArray data;
	};
	QList<EntryData> newEntries;

	struct archive_entry* entry;
	while (archive_read_next_header(ar.get(), &entry) == ARCHIVE_OK) {
		QString filename = QString::fromUtf8(archive_entry_pathname(entry));
		if (filter && !filter(filename)) {
			qDebug() << "Skipping file " << filename << " from "
					 << from.fileName() << " - filtered";
			archive_read_data_skip(ar.get());
			continue;
		}
		if (contained.contains(filename)) {
			qDebug() << "Skipping already contained file " << filename
					 << " from " << from.fileName();
			archive_read_data_skip(ar.get());
			continue;
		}
		contained.insert(filename);

		// Read data
		la_int64_t entrySize = archive_entry_size(entry);
		QByteArray data;
		if (entrySize > 0) {
			data.resize(entrySize);
			la_ssize_t bytesRead =
				archive_read_data(ar.get(), data.data(), entrySize);
			if (bytesRead < 0) {
				qCritical() << "Failed to read " << filename << " from "
							<< from.fileName();
				return false;
			}
			data.resize(bytesRead);
		}
		newEntries.append({filename, data});
	}

	// Now append to existing zip (or create new one)
	// libarchive doesn't support append, so we need to read existing + write
	// all
	QList<EntryData> existingEntries;
	if (QFile::exists(intoPath)) {
		auto existingAr = openZipForReading(intoPath);
		if (existingAr) {
			while (archive_read_next_header(existingAr.get(), &entry) ==
				   ARCHIVE_OK) {
				QString name = QString::fromUtf8(archive_entry_pathname(entry));
				la_int64_t sz = archive_entry_size(entry);
				QByteArray data;
				if (sz > 0) {
					data.resize(sz);
					archive_read_data(existingAr.get(), data.data(), sz);
				}
				existingEntries.append({name, data});
			}
		}
	}

	auto aw = createZipForWriting(intoPath);
	if (!aw)
		return false;

	// Write existing entries
	for (const auto& e : existingEntries) {
		if (!writeFileToArchive(aw.get(), e.name, e.data)) {
			qCritical() << "Failed to write existing entry " << e.name;
			return false;
		}
	}

	// Write new entries
	for (const auto& e : newEntries) {
		if (!writeFileToArchive(aw.get(), e.name, e.data)) {
			qCritical() << "Failed to write " << e.name << " into the jar";
			return false;
		}
	}

	archive_write_close(aw.get());
	return true;
}

bool MMCZip::createModdedJar(QString sourceJarPath, QString targetJarPath,
							 const QList<Mod>& mods)
{
	// Files already added to the jar. These files will be skipped.
	QSet<QString> addedFiles;

	// We collect all entries first, then write them all at once.
	struct EntryData {
		QString name;
		QByteArray data;
	};
	QList<EntryData> allEntries;

	// Modify the jar - process mods in reverse order
	QListIterator<Mod> i(mods);
	i.toBack();
	while (i.hasPrevious()) {
		const Mod& mod = i.previous();
		if (!mod.enabled())
			continue;

		if (mod.type() == Mod::MOD_ZIPFILE) {
			auto ar = openZipForReading(mod.filename().absoluteFilePath());
			if (!ar) {
				QFile::remove(targetJarPath);
				qCritical() << "Failed to add" << mod.filename().fileName()
							<< "to the jar.";
				return false;
			}
			struct archive_entry* entry;
			while (archive_read_next_header(ar.get(), &entry) == ARCHIVE_OK) {
				QString filename =
					QString::fromUtf8(archive_entry_pathname(entry));
				if (addedFiles.contains(filename)) {
					archive_read_data_skip(ar.get());
					continue;
				}
				addedFiles.insert(filename);
				la_int64_t sz = archive_entry_size(entry);
				QByteArray data;
				if (sz > 0) {
					data.resize(sz);
					archive_read_data(ar.get(), data.data(), sz);
				}
				allEntries.append({filename, data});
			}
		} else if (mod.type() == Mod::MOD_SINGLEFILE) {
			auto filename = mod.filename();
			QFile file(filename.absoluteFilePath());
			if (!file.open(QIODevice::ReadOnly)) {
				QFile::remove(targetJarPath);
				qCritical()
					<< "Failed to add" << filename.fileName() << "to the jar.";
				return false;
			}
			allEntries.append({filename.fileName(), file.readAll()});
			addedFiles.insert(filename.fileName());
		} else if (mod.type() == Mod::MOD_FOLDER) {
			auto filename = mod.filename();
			QDir modRoot(filename.absoluteFilePath());
			QDirIterator it(modRoot.absolutePath(), QDir::Files | QDir::Hidden,
							QDirIterator::Subdirectories);
			while (it.hasNext()) {
				it.next();
				QString relPath = modRoot.relativeFilePath(it.filePath());
				if (addedFiles.contains(relPath)) {
					continue;
				}
				QFile f(it.filePath());
				if (!f.open(QIODevice::ReadOnly)) {
					QFile::remove(targetJarPath);
					qCritical() << "Failed to add" << relPath << "of folder mod"
								<< filename.fileName() << "to the jar.";
					return false;
				}
				addedFiles.insert(relPath);
				allEntries.append({relPath, f.readAll()});
			}
			qDebug() << "Adding folder " << filename.fileName() << " from "
					 << filename.absoluteFilePath();
		} else {
			QFile::remove(targetJarPath);
			qCritical() << "Failed to add unknown mod type"
						<< mod.filename().fileName() << "to the jar.";
			return false;
		}
	}

	// Add source jar contents (skip META-INF and already added files)
	auto ar = openZipForReading(sourceJarPath);
	if (!ar) {
		QFile::remove(targetJarPath);
		qCritical() << "Failed to insert minecraft.jar contents.";
		return false;
	}
	struct archive_entry* entry;
	while (archive_read_next_header(ar.get(), &entry) == ARCHIVE_OK) {
		QString filename = QString::fromUtf8(archive_entry_pathname(entry));
		if (filename.contains("META-INF")) {
			archive_read_data_skip(ar.get());
			continue;
		}
		if (addedFiles.contains(filename)) {
			archive_read_data_skip(ar.get());
			continue;
		}
		la_int64_t sz = archive_entry_size(entry);
		QByteArray data;
		if (sz > 0) {
			data.resize(sz);
			archive_read_data(ar.get(), data.data(), sz);
		}
		allEntries.append({filename, data});
	}

	// Write the final jar
	auto aw = createZipForWriting(targetJarPath);
	if (!aw) {
		QFile::remove(targetJarPath);
		qCritical() << "Failed to open the minecraft.jar for modding";
		return false;
	}
	for (const auto& e : allEntries) {
		if (!writeFileToArchive(aw.get(), e.name, e.data)) {
			archive_write_close(aw.get());
			(void)QFile::remove(targetJarPath);
			qCritical() << "Failed to finalize minecraft.jar!";
			return false;
		}
	}
	archive_write_close(aw.get());
	return true;
}

QString MMCZip::findFolderOfFileInZip(const QString& zipPath,
									  const QString& what, const QString& root)
{
	auto entries = listEntries(zipPath, root, QDir::Files);
	for (const auto& fileName : entries) {
		if (fileName == what)
			return root;
	}
	auto dirs = listEntries(zipPath, root, QDir::Dirs);
	for (const auto& dirName : dirs) {
		QString result = findFolderOfFileInZip(zipPath, what, root + dirName);
		if (!result.isEmpty())
			return result;
	}
	return QString();
}

bool MMCZip::findFilesInZip(const QString& zipPath, const QString& what,
							QStringList& result, const QString& root)
{
	auto entries = listEntries(zipPath, root, QDir::Files);
	for (const auto& fileName : entries) {
		if (fileName == what) {
			result.append(root);
			return true;
		}
	}
	auto dirs = listEntries(zipPath, root, QDir::Dirs);
	for (const auto& dirName : dirs) {
		(void)findFilesInZip(zipPath, what, result, root + dirName);
	}
	return !result.isEmpty();
}

nonstd::optional<QStringList> MMCZip::extractSubDir(const QString& zipPath,
													const QString& subdir,
													const QString& target)
{
	return MMCZip::extractSubDir(zipPath, subdir, target, ExtractReporting{});
}

nonstd::optional<QStringList>
MMCZip::extractSubDir(const QString& zipPath, const QString& subdir,
					  const QString& target,
					  const ExtractReporting& reporting)
{
	QDir directory(target);
	QStringList extracted;

	qDebug() << "Extracting subdir" << subdir << "from" << zipPath << "to"
			 << target;

	/* One extra pass over the entry list, with no file data read, is
	 * what turns the progress bar from a sweeping animation into a
	 * percentage. Only paid for when somebody is watching. */
	qint64 total = 0;
	qint64 done = 0;
	if (reporting.progress) {
		for (const QString& name : listEntries(zipPath)) {
			if (name.startsWith(subdir))
				total++;
		}
		reporting.progress(0, total);
	}

	auto ar = openZipForReading(zipPath);
	if (!ar) {
		// check if this is a minimum size empty zip file...
		QFileInfo fileInfo(zipPath);
		if (fileInfo.size() == 22) {
			return extracted;
		}
		qWarning() << "Failed to open archive:" << zipPath;
		return nonstd::nullopt;
	}

	struct archive_entry* entry;
	bool hasEntries = false;
	int readRet;
	while ((readRet = archive_read_next_header(ar.get(), &entry)) ==
		   ARCHIVE_OK) {
		/* Between entries rather than inside one: a half-written file is
		 * worse than one more file, and the wait is bounded by the
		 * largest entry in the archive. */
		if (reporting.isCancelled && reporting.isCancelled()) {
			qDebug() << "Extraction of" << zipPath << "cancelled";
			for (const auto& f : extracted)
				(void)QFile::remove(f);
			return nonstd::nullopt;
		}

		hasEntries = true;
		QString name = QString::fromUtf8(archive_entry_pathname(entry));
		if (!name.startsWith(subdir)) {
			archive_read_data_skip(ar.get());
			continue;
		}
		QString relName = name.mid(subdir.size());
		QString absFilePath = directory.absoluteFilePath(relName);
		if (relName.isEmpty()) {
			absFilePath += "/";
		}

		if (reporting.entryStarted) {
			reporting.entryStarted(relName);
		}

		if (!writeDiskEntry(ar.get(), absFilePath)) {
			qWarning() << "Failed to extract file" << name << "to"
					   << absFilePath;
			// Clean up extracted files
			for (const auto& f : extracted)
				(void)QFile::remove(f);
			return nonstd::nullopt;
		}
		extracted.append(absFilePath);
		if (reporting.progress) {
			reporting.progress(++done, total);
		}
		qDebug() << "Extracted file" << relName;
	}

	if (readRet != ARCHIVE_EOF) {
		qWarning() << "Archive read failed for:" << zipPath
				   << "Error:" << archive_error_string(ar.get());
		for (const auto& f : extracted)
			(void)QFile::remove(f);
		return nonstd::nullopt;
	}

	if (!hasEntries) {
		qDebug() << "Extracting empty archives seems odd...";
	}
	return extracted;
}

bool MMCZip::extractRelFile(const QString& zipPath, const QString& file,
							const QString& target, QString* error)
{
	auto ar = openZipForReading(zipPath);
	if (!ar) {
		if (error) {
			*error = QStringLiteral("could not open archive '%1'").arg(zipPath);
		}
		return false;
	}

	struct archive_entry* entry;
	while (archive_read_next_header(ar.get(), &entry) == ARCHIVE_OK) {
		QString name = QString::fromUtf8(archive_entry_pathname(entry));
		if (name == file) {
			return writeDiskEntry(ar.get(), target, error);
		}
		archive_read_data_skip(ar.get());
	}
	if (error) {
		*error = QStringLiteral("'%1' not found in archive '%2'")
					 .arg(file, zipPath);
	}
	return false;
}

nonstd::optional<QStringList> MMCZip::extractDir(QString fileCompressed,
												 QString dir)
{
	return MMCZip::extractSubDir(fileCompressed, "", dir);
}

nonstd::optional<QStringList> MMCZip::extractDir(QString fileCompressed,
												 QString subdir, QString dir)
{
	return MMCZip::extractSubDir(fileCompressed, subdir, dir);
}

bool MMCZip::extractFile(QString fileCompressed, QString file, QString target)
{
	// check if this is a minimum size empty zip file...
	QFileInfo fileInfo(fileCompressed);
	if (fileInfo.size() == 22) {
		return true;
	}
	return MMCZip::extractRelFile(fileCompressed, file, target);
}

QByteArray MMCZip::readFileFromZip(const QString& zipPath,
								   const QString& entryName)
{
	auto ar = openZipForReading(zipPath);
	if (!ar)
		return {};

	struct archive_entry* entry;
	while (archive_read_next_header(ar.get(), &entry) == ARCHIVE_OK) {
		QString name = QString::fromUtf8(archive_entry_pathname(entry));
		if (name == entryName) {
			la_int64_t sz = archive_entry_size(entry);
			if (sz <= 0) {
				// Size may be unknown; read in chunks
				QByteArray result;
				char buf[8192];
				la_ssize_t r;
				while ((r = archive_read_data(ar.get(), buf, sizeof(buf))) > 0)
					result.append(buf, r);
				return result;
			}
			QByteArray data(sz, Qt::Uninitialized);
			archive_read_data(ar.get(), data.data(), sz);
			return data;
		}
		archive_read_data_skip(ar.get());
	}
	return {};
}

bool MMCZip::entryExists(const QString& zipPath, const QString& entryName)
{
	auto ar = openZipForReading(zipPath);
	if (!ar)
		return false;

	struct archive_entry* entry;
	while (archive_read_next_header(ar.get(), &entry) == ARCHIVE_OK) {
		QString name = QString::fromUtf8(archive_entry_pathname(entry));
		if (name == entryName || name == entryName + "/")
			return true;
		archive_read_data_skip(ar.get());
	}
	return false;
}

QStringList MMCZip::listEntries(const QString& zipPath)
{
	QStringList result;
	auto ar = openZipForReading(zipPath);
	if (!ar)
		return result;

	struct archive_entry* entry;
	while (archive_read_next_header(ar.get(), &entry) == ARCHIVE_OK) {
		result.append(QString::fromUtf8(archive_entry_pathname(entry)));
		archive_read_data_skip(ar.get());
	}
	return result;
}

QStringList MMCZip::listEntries(const QString& zipPath, const QString& dirPath,
								QDir::Filters type)
{
	QStringList result;
	auto ar = openZipForReading(zipPath);
	if (!ar)
		return result;

	QSet<QString> seen;
	struct archive_entry* entry;
	while (archive_read_next_header(ar.get(), &entry) == ARCHIVE_OK) {
		QString name = QString::fromUtf8(archive_entry_pathname(entry));

		// Must start with dirPath
		if (!name.startsWith(dirPath)) {
			archive_read_data_skip(ar.get());
			continue;
		}

		QString relative = name.mid(dirPath.size());
		// Remove leading slashes
		while (relative.startsWith('/'))
			relative = relative.mid(1);

		if (relative.isEmpty()) {
			archive_read_data_skip(ar.get());
			continue;
		}

		int slashIdx = relative.indexOf('/');
		if (slashIdx < 0) {
			// It's a file directly in dirPath
			if (type & QDir::Files) {
				if (!seen.contains(relative)) {
					seen.insert(relative);
					result.append(relative);
				}
			}
		} else {
			// It's inside a subdirectory
			if (type & QDir::Dirs) {
				QString dirName =
					relative.left(slashIdx + 1); // include trailing /
				if (!seen.contains(dirName)) {
					seen.insert(dirName);
					result.append(dirName);
				}
			}
		}
		archive_read_data_skip(ar.get());
	}
	return result;
}

QDateTime MMCZip::getEntryModTime(const QString& zipPath,
								  const QString& entryName)
{
	auto ar = openZipForReading(zipPath);
	if (!ar)
		return {};

	struct archive_entry* entry;
	while (archive_read_next_header(ar.get(), &entry) == ARCHIVE_OK) {
		QString name = QString::fromUtf8(archive_entry_pathname(entry));
		if (name == entryName) {
			time_t mtime = archive_entry_mtime(entry);
			return QDateTime::fromSecsSinceEpoch(mtime);
		}
		archive_read_data_skip(ar.get());
	}
	return {};
}
