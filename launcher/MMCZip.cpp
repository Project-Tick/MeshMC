/* Copyright 2013-2021 MultiMC Contributors
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

#include <quazip.h>
#include <quazipdir.h>
#include <quazipfile.h>
#include <JlCompress.h>
#include "MMCZip.h"
#include "FileSystem.h"

#include <QDebug>

namespace
{
bool compressFile(QuaZip &zip, const QFileInfo &file, const QString &name)
{
    if (!file.isSymLink())
        return JlCompress::compressFile(&zip, file.absoluteFilePath(), name);

    // Keep the contents of a linked file in the archive, as the old fork did.
    QFile source(file.absoluteFilePath());
    QuaZipFile entry(&zip);
    if (!source.open(QIODevice::ReadOnly) ||
        !entry.open(QIODevice::WriteOnly, QuaZipNewInfo(name, file.absoluteFilePath())))
        return false;
    const bool copied = JlCompress::copyData(source, entry);
    entry.close();
    return copied && entry.getZipError() == UNZ_OK;
}

bool compressSubDir(QuaZip &zip, const QDir &root, const QDir &directory,
                    QSet<QString> &added, const QString &prefix, const MMCZip::ArchiveFilter &exclude)
{
    const QString relative = root.relativeFilePath(directory.absolutePath());
    if (relative != "." && (!exclude || !exclude(relative)))
    {
        QuaZipFile entry(&zip);
        if (!entry.open(QIODevice::WriteOnly, QuaZipNewInfo(prefix + relative + '/', directory.absolutePath()),
                        nullptr, 0, 0))
            return false;
        entry.close();
        if (entry.getZipError() != UNZ_OK)
            return false;
    }

    for (const auto &child : directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden))
    {
        if (!compressSubDir(zip, root, QDir(child.absoluteFilePath()), added, prefix, exclude))
            return false;
    }
    for (const auto &file : directory.entryInfoList(QDir::Files | QDir::Hidden))
    {
        const QString path = file.absoluteFilePath();
        const QString name = root.relativeFilePath(path);
        if (path == zip.getZipName() || (exclude && exclude(name)))
            continue;
        if (!compressFile(zip, file, prefix + name))
            return false;
        added.insert(prefix + name);
    }
    return true;
}
}

bool MMCZip::compressDir(const QString &output, const QString &directory,
                         const QString &prefix, const ArchiveFilter &exclude)
{
    const QFileInfo archive(output);
    const QDir root(directory);
    if (!root.exists() || !QDir().mkpath(archive.absolutePath()))
        return false;

    QuaZip zip(archive.absoluteFilePath());
    if (!zip.open(QuaZip::mdCreate))
        return false;
    QSet<QString> added;
    const bool complete = compressSubDir(zip, root, root, added, prefix.isEmpty() ? prefix : prefix + '/', exclude);
    zip.close();
    if (complete && zip.getZipError() == UNZ_OK)
        return true;
    QFile::remove(archive.absoluteFilePath());
    return false;
}

// ours
bool MMCZip::mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained, const ArchiveFilter &filter)
{
    QuaZip modZip(from.filePath());
    modZip.open(QuaZip::mdUnzip);

    QuaZipFile fileInsideMod(&modZip);
    QuaZipFile zipOutFile(into);
    for (bool more = modZip.goToFirstFile(); more; more = modZip.goToNextFile())
    {
        QString filename = modZip.getCurrentFileName();
        if (filter && !filter(filename))
        {
            qDebug() << "Skipping file " << filename << " from "
                        << from.fileName() << " - filtered";
            continue;
        }
        if (contained.contains(filename))
        {
            qDebug() << "Skipping already contained file " << filename << " from "
                        << from.fileName();
            continue;
        }
        contained.insert(filename);

        if (!fileInsideMod.open(QIODevice::ReadOnly))
        {
            qCritical() << "Failed to open " << filename << " from " << from.fileName();
            return false;
        }

        QuaZipNewInfo info_out(fileInsideMod.getActualFileName());

        if (!zipOutFile.open(QIODevice::WriteOnly, info_out))
        {
            qCritical() << "Failed to open " << filename << " in the jar";
            fileInsideMod.close();
            return false;
        }
        if (!JlCompress::copyData(fileInsideMod, zipOutFile))
        {
            zipOutFile.close();
            fileInsideMod.close();
            qCritical() << "Failed to copy data of " << filename << " into the jar";
            return false;
        }
        zipOutFile.close();
        fileInsideMod.close();
    }
    return true;
}

// ours
bool MMCZip::createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod>& mods)
{
    QuaZip zipOut(targetJarPath);
    if (!zipOut.open(QuaZip::mdCreate))
    {
        QFile::remove(targetJarPath);
        qCritical() << "Failed to open the minecraft.jar for modding";
        return false;
    }
    // Files already added to the jar.
    // These files will be skipped.
    QSet<QString> addedFiles;

    // Modify the jar
    QListIterator<Mod> i(mods);
    i.toBack();
    while (i.hasPrevious())
    {
        const Mod &mod = i.previous();
        // do not merge disabled mods.
        if (!mod.enabled())
            continue;
        if (mod.type() == Mod::MOD_ZIPFILE)
        {
            if (!mergeZipFiles(&zipOut, mod.filename(), addedFiles))
            {
                zipOut.close();
                QFile::remove(targetJarPath);
                qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
                return false;
            }
        }
        else if (mod.type() == Mod::MOD_SINGLEFILE)
        {
            // FIXME: buggy - does not work with addedFiles
            auto filename = mod.filename();
            if (!compressFile(zipOut, filename, filename.fileName()))
            {
                zipOut.close();
                QFile::remove(targetJarPath);
                qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
                return false;
            }
            addedFiles.insert(filename.fileName());
        }
        else if (mod.type() == Mod::MOD_FOLDER)
        {
            // FIXME: buggy - does not work with addedFiles
            auto filename = mod.filename();
            QString what_to_zip = filename.absoluteFilePath();
            QDir dir(what_to_zip);
            dir.cdUp();
            QString parent_dir = dir.absolutePath();
            if (!compressSubDir(zipOut, QDir(parent_dir), QDir(what_to_zip), addedFiles, {}, {}))
            {
                zipOut.close();
                QFile::remove(targetJarPath);
                qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
                return false;
            }
            qDebug() << "Adding folder " << filename.fileName() << " from "
                        << filename.absoluteFilePath();
        }
        else
        {
            // Make sure we do not continue launching when something is missing or undefined...
            zipOut.close();
            QFile::remove(targetJarPath);
            qCritical() << "Failed to add unknown mod type" << mod.filename().fileName() << "to the jar.";
            return false;
        }
    }

    if (!mergeZipFiles(&zipOut, QFileInfo(sourceJarPath), addedFiles, [](const QString key){return !key.contains("META-INF");}))
    {
        zipOut.close();
        QFile::remove(targetJarPath);
        qCritical() << "Failed to insert minecraft.jar contents.";
        return false;
    }

    // Recompress the jar
    zipOut.close();
    if (zipOut.getZipError() != 0)
    {
        QFile::remove(targetJarPath);
        qCritical() << "Failed to finalize minecraft.jar!";
        return false;
    }
    return true;
}

// ours
QString MMCZip::findFolderOfFileInZip(QuaZip * zip, const QString & what, const QString &root)
{
    QuaZipDir rootDir(zip, root);
    for(auto fileName: rootDir.entryList(QDir::Files))
    {
        if(fileName == what)
            return root;
    }
    for(auto fileName: rootDir.entryList(QDir::Dirs))
    {
        QString result = findFolderOfFileInZip(zip, what, root + fileName);
        if(!result.isEmpty())
        {
            return result;
        }
    }
    return QString();
}

// ours
bool MMCZip::findFilesInZip(QuaZip * zip, const QString & what, QStringList & result, const QString &root)
{
    QuaZipDir rootDir(zip, root);
    for(auto fileName: rootDir.entryList(QDir::Files))
    {
        if(fileName == what)
        {
            result.append(root);
            return true;
        }
    }
    for(auto fileName: rootDir.entryList(QDir::Dirs))
    {
        findFilesInZip(zip, what, result, root + fileName);
    }
    return !result.isEmpty();
}


// ours
nonstd::optional<QStringList> MMCZip::extractSubDir(QuaZip *zip, const QString & subdir, const QString &target)
{
    return extractSubDirWithProgress(zip, subdir, target, {});
}

nonstd::optional<QStringList> MMCZip::extractSubDirWithProgress(
    QuaZip* zip, const QString& subdir, const QString& target,
    const std::function<bool(int, int, const QString&)>& progress)
{
    QDir directory(target);
    QStringList extracted;

    qDebug() << "Extracting subdir" << subdir << "from" << zip->getZipName() << "to" << target;
    auto numEntries = zip->getEntriesCount();
    if(numEntries < 0) {
        qWarning() << "Failed to enumerate files in archive";
        return nonstd::nullopt;
    }
    else if(numEntries == 0) {
        qDebug() << "Extracting empty archives seems odd...";
        return extracted;
    }
    else if (!zip->goToFirstFile())
    {
        qWarning() << "Failed to seek to first file in zip";
        return nonstd::nullopt;
    }

    int processed = 0;
    do
    {
        QString name = zip->getCurrentFileName();
        if (progress && !progress(processed++, numEntries, name))
            return nonstd::nullopt;
        if(!name.startsWith(subdir))
        {
            continue;
        }
        name.remove(0, subdir.size());
        // archives come from outside: an entry like '../../file' must not land outside the target
        auto inside = FS::PathInside(directory.absolutePath(), name);
        if(!inside)
        {
            qWarning() << "Refusing to extract" << zip->getZipName() << "- the entry" << name << "points outside of" << target;
            JlCompress::removeFile(extracted);
            return nonstd::nullopt;
        }
        QString absFilePath = *inside;
        // folders keep their slash, so JlCompress makes them a folder
        if(name.isEmpty() || name.endsWith('/'))
        {
            absFilePath += "/";
        }
        if (!extractRelFile(zip, {}, absFilePath))
        {
            qWarning() << "Failed to extract file" << name << "to" << absFilePath;
            JlCompress::removeFile(extracted);
            return nonstd::nullopt;
        }
        extracted.append(absFilePath);
        qDebug() << "Extracted file" << name;
    } while (zip->goToNextFile());
    if (progress && !progress(numEntries, numEntries, QString()))
        return nonstd::nullopt;
    return extracted;
}

// ours
nonstd::optional<QStringList> MMCZip::extractSubDirLimited(QuaZip *zip, const QString & subdir, const QString &target, qint64 maxBytes)
{
    QDir directory(target);
    QStringList extracted;
    auto fail = [&extracted]() {
        JlCompress::removeFile(extracted);
        return nonstd::nullopt;
    };

    qDebug() << "Extracting subdir" << subdir << "from" << zip->getZipName() << "to" << target << "- at most" << maxBytes << "bytes";
    if(!zip->goToFirstFile())
    {
        // an empty archive has nothing to extract
        return zip->getZipError() == UNZ_OK ? nonstd::optional<QStringList>(extracted) : fail();
    }

    qint64 written = 0;
    do
    {
        QString name = zip->getCurrentFileName();
        if(!name.startsWith(subdir))
        {
            continue;
        }
        name.remove(0, subdir.size());
        auto inside = FS::PathInside(directory.absolutePath(), name);
        if(!inside)
        {
            qWarning() << "Refusing to extract" << zip->getZipName() << "- the entry" << name << "points outside of" << target;
            return fail();
        }
        if(name.isEmpty() || name.endsWith('/'))
        {
            if(!directory.mkpath(*inside))
            {
                qWarning() << "Failed to create folder" << *inside;
                return fail();
            }
            continue;
        }
        if(!directory.mkpath(QFileInfo(*inside).absolutePath()))
        {
            qWarning() << "Failed to create folder for" << *inside;
            return fail();
        }

        QuaZipFile in(zip);
        if(!in.open(QIODevice::ReadOnly))
        {
            qWarning() << "Failed to read" << name << "from" << zip->getZipName();
            return fail();
        }
        QFile out(*inside);
        if(!out.open(QIODevice::WriteOnly))
        {
            qWarning() << "Failed to write" << *inside;
            return fail();
        }
        extracted.append(*inside);
        char buffer[64 * 1024];
        qint64 read;
        bool tooMuch = false, writeFailed = false;
        while((read = in.read(buffer, sizeof(buffer))) > 0)
        {
            written += read;
            tooMuch = written > maxBytes;
            writeFailed = !tooMuch && out.write(buffer, read) != read;
            if(tooMuch || writeFailed)
            {
                break;
            }
        }
        // closed before anything is removed: Windows can't delete open files
        out.close();
        in.close();
        if(tooMuch)
        {
            qWarning() << "Refusing to extract" << zip->getZipName() << "- it holds more than" << maxBytes << "bytes";
            return fail();
        }
        if(writeFailed)
        {
            qWarning() << "Failed to write" << *inside;
            return fail();
        }
        // closing checks the entry's checksum
        if(read < 0 || in.getZipError() != UNZ_OK)
        {
            qWarning() << "Failed to read" << name << "from" << zip->getZipName();
            return fail();
        }
    } while(zip->goToNextFile());
    return extracted;
}

bool MMCZip::extractRelFile(QuaZip *zip, const QString &file, const QString &target)
{
    if (!zip || zip->getMode() != QuaZip::mdUnzip || (!file.isEmpty() && !zip->setCurrentFile(file)))
        return false;
    QuaZipFile entry(zip);
    if (!entry.open(QIODevice::ReadOnly))
        return false;
    if (target.endsWith('/'))
        return QDir().mkpath(target);
    if (!QDir().mkpath(QFileInfo(target).absolutePath()))
        return false;

    // Keep archive entries as regular files, including entries marked as symlinks.
    QFile output(target);
    if (!output.open(QIODevice::WriteOnly))
        return false;
    const bool copied = JlCompress::copyData(entry, output);
    output.close();
    entry.close();
    if (copied && entry.getZipError() == UNZ_OK && output.error() == QFile::NoError)
        return true;
    output.remove();
    return false;
}

// ours
nonstd::optional<QStringList> MMCZip::extractDir(QString fileCompressed, QString dir)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip))
    {
        // check if this is a minimum size empty zip file...
        QFileInfo fileInfo(fileCompressed);
        if(fileInfo.size() == 22) {
            return QStringList();
        }
        qWarning() << "Could not open archive for unzipping:" << fileCompressed << "Error:" << zip.getZipError();;
        return nonstd::nullopt;
    }
    return MMCZip::extractSubDir(&zip, "", dir);
}

// ours
nonstd::optional<QStringList> MMCZip::extractDir(QString fileCompressed, QString subdir, QString dir)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip))
    {
        // check if this is a minimum size empty zip file...
        QFileInfo fileInfo(fileCompressed);
        if(fileInfo.size() == 22) {
            return QStringList();
        }
        qWarning() << "Could not open archive for unzipping:" << fileCompressed << "Error:" << zip.getZipError();;
        return nonstd::nullopt;
    }
    return MMCZip::extractSubDir(&zip, subdir, dir);
}

// ours
bool MMCZip::extractFile(QString fileCompressed, QString file, QString target)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip))
    {
        // check if this is a minimum size empty zip file...
        QFileInfo fileInfo(fileCompressed);
        if(fileInfo.size() == 22) {
            return true;
        }
        qWarning() << "Could not open archive for unzipping:" << fileCompressed << "Error:" << zip.getZipError();
        return false;
    }
    return MMCZip::extractRelFile(&zip, file, target);
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
		if (dir.isSymLink() || (excludeFilter && excludeFilter(dir)))
			continue;
		if (!collectFileListRecursively(rootDir, dir.filePath(), files,
										excludeFilter))
			return false;
	}

	const QFileInfoList entries =
		directory.entryInfoList(QDir::Files | QDir::Hidden);
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
