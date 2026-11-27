/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "InstanceShortcuts.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QDebug>

namespace
{
QString registryPath(const QString& root)
{
    const auto key = QCryptographicHash::hash(QDir::cleanPath(root).toUtf8(), QCryptographicHash::Sha256).toHex();
    return QDir::current().absoluteFilePath("instance-shortcuts/" + QString::fromLatin1(key) + ".json");
}

QJsonObject readRegistry(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
}

QByteArray fileHash(const QString& path)
{
    const QFileInfo info(path);
    if (!info.isFile() || info.isSymLink())
        return {};

    QFile file(path);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    return file.open(QIODevice::ReadOnly) && hash.addData(&file) ? hash.result() : QByteArray();
}

QByteArray fingerprint(const QString& path)
{
    const QFileInfo info(path);
    if (!info.isDir())
        return fileHash(path).toHex();
    if (info.isSymLink() || !path.endsWith(".app", Qt::CaseInsensitive))
        return {};

    QStringList files;
    const QDir bundle(path);
    QDirIterator items(path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                       QDirIterator::Subdirectories);
    while (items.hasNext()) {
        const auto entry = items.next();
        const auto entryInfo = items.fileInfo();
        if (entryInfo.isSymLink() || (!entryInfo.isFile() && !entryInfo.isDir()))
            return {};
        files.append(bundle.relativeFilePath(entry));
    }
    files.sort();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (const auto& name : files) {
        hash.addData(name.toUtf8());
        hash.addData("\0", 1);
        const auto file = bundle.filePath(name);
        if (QFileInfo(file).isDir()) {
            hash.addData("D", 1);
        } else {
            const auto digest = fileHash(file);
            if (digest.isEmpty())
                return {};
            hash.addData("F", 1);
            hash.addData(digest);
        }
    }
    return hash.result().toHex();
}

QStringList ownedPaths(const QString& registry)
{
    const auto entries = readRegistry(registry);
    QStringList result;
    for (auto entry = entries.constBegin(); entry != entries.constEnd(); ++entry) {
        const auto expected = entry.value().toString().toLatin1();
        if (!expected.isEmpty() && fingerprint(entry.key()) == expected)
            result.append(entry.key());
    }
    return result;
}
}

bool InstanceShortcuts::record(const QString& instanceRoot, const QString& shortcutPath)
{
    const auto registry = registryPath(instanceRoot);
    const auto path = QFileInfo(shortcutPath).absoluteFilePath();
    const auto digest = fingerprint(path);
    if (digest.isEmpty() || !QDir().mkpath(QFileInfo(registry).absolutePath()))
        return false;

    auto entries = readRegistry(registry);
    entries.insert(path, QString::fromLatin1(digest));
    const auto data = QJsonDocument(entries).toJson(QJsonDocument::Compact);
    QSaveFile file(registry);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
}

int InstanceShortcuts::countOwned(const QString& instanceRoot)
{
    return ownedPaths(registryPath(instanceRoot)).size();
}

void InstanceShortcuts::removeOwned(const QString& instanceRoot)
{
    const auto registry = registryPath(instanceRoot);
    for (const auto& path : ownedPaths(registry)) {
        const auto removed = QFileInfo(path).isDir() ? QDir(path).removeRecursively() : QFile::remove(path);
        if (!removed)
            qWarning() << "Could not remove instance shortcut" << path;
    }
    QFile::remove(registry);
}
