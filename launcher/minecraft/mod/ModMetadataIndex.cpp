/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#include "ModMetadataIndex.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSaveFile>

#include "FileSystem.h"

ModMetadataIndex::ModMetadataIndex(const QDir& folder)
	: m_folder(folder), m_indexDir(folder.absoluteFilePath(".index"))
{
}

QString ModMetadataIndex::canonicalFileName(const QString& fileName)
{
	if (fileName.endsWith(QStringLiteral(".disabled"))) {
		return fileName.left(fileName.size() - 9);
	}
	return fileName;
}

QString ModMetadataIndex::normalizeName(const QString& name)
{
	QString n = name.toLower().trimmed();
	n.remove(QRegularExpression(QStringLiteral("\\s*\\([^)]*\\)\\s*")));
	n.remove(QRegularExpression(QStringLiteral("[^a-z0-9 ]")));
	n = n.simplified();
	return n;
}

QString ModMetadataIndex::sidecarPath(const QString& fileName) const
{
	return m_indexDir.absoluteFilePath(canonicalFileName(fileName) +
									   QStringLiteral(".json"));
}

ModMetadataIndex::Entry ModMetadataIndex::parseJson(const QByteArray& bytes)
{
	Entry e;
	QJsonParseError perr{};
	QJsonDocument doc = QJsonDocument::fromJson(bytes, &perr);
	if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
		return e;
	}
	const QJsonObject o = doc.object();
	e.fileName = o.value(QStringLiteral("fileName")).toString();
	e.platform = o.value(QStringLiteral("platform")).toString();
	e.projectId = o.value(QStringLiteral("projectId")).toString();
	e.versionId = o.value(QStringLiteral("versionId")).toString();
	e.name = o.value(QStringLiteral("name")).toString();
	e.slug = o.value(QStringLiteral("slug")).toString();
	e.downloadUrl = o.value(QStringLiteral("downloadUrl")).toString();
	e.sha1 = o.value(QStringLiteral("sha1")).toString().toLower();
	e.fileSize =
		static_cast<qint64>(o.value(QStringLiteral("fileSize")).toDouble(0));
	e.isDependency = o.value(QStringLiteral("isDependency")).toBool(false);
	const QString iso = o.value(QStringLiteral("installedAt")).toString();
	if (!iso.isEmpty()) {
		e.installedAt = QDateTime::fromString(iso, Qt::ISODate);
	}
	return e;
}

QByteArray ModMetadataIndex::serializeJson(const Entry& entry)
{
	QJsonObject o;
	o.insert(QStringLiteral("schema"), 1);
	o.insert(QStringLiteral("fileName"), entry.fileName);
	o.insert(QStringLiteral("platform"), entry.platform);
	o.insert(QStringLiteral("projectId"), entry.projectId);
	o.insert(QStringLiteral("versionId"), entry.versionId);
	o.insert(QStringLiteral("name"), entry.name);
	o.insert(QStringLiteral("slug"), entry.slug);
	o.insert(QStringLiteral("downloadUrl"), entry.downloadUrl);
	o.insert(QStringLiteral("sha1"), entry.sha1.toLower());
	o.insert(QStringLiteral("fileSize"), static_cast<double>(entry.fileSize));
	o.insert(QStringLiteral("isDependency"), entry.isDependency);
	const QDateTime ts = entry.installedAt.isValid()
							 ? entry.installedAt
							 : QDateTime::currentDateTimeUtc();
	o.insert(QStringLiteral("installedAt"), ts.toString(Qt::ISODate));
	return QJsonDocument(o).toJson(QJsonDocument::Indented);
}

void ModMetadataIndex::load()
{
	QMutexLocker lock(&m_mutex);
	m_entries.clear();

	if (!m_indexDir.exists()) {
		/* Nothing to load yet — that is fine for a fresh folder. */
		return;
	}

	const QStringList jsonFiles = m_indexDir.entryList(
		QStringList{QStringLiteral("*.json")}, QDir::Files | QDir::Readable);

	for (const QString& name : jsonFiles) {
		const QString abs = m_indexDir.absoluteFilePath(name);
		QFile f(abs);
		if (!f.open(QIODevice::ReadOnly)) {
			qWarning() << "ModMetadataIndex: cannot read sidecar" << abs;
			continue;
		}
		const QByteArray bytes = f.readAll();
		f.close();

		Entry e = parseJson(bytes);
		if (!e.isValid()) {
			qWarning() << "ModMetadataIndex: discarding malformed sidecar"
					   << abs;
			QFile::remove(abs);
			continue;
		}

		/* Prune sidecars whose target file has been removed externally. */
		const QString canon = canonicalFileName(e.fileName);
		const bool exists =
			m_folder.exists(canon) ||
			m_folder.exists(canon + QStringLiteral(".disabled"));
		if (!exists) {
			qDebug() << "ModMetadataIndex: pruning orphaned sidecar" << abs;
			QFile::remove(abs);
			continue;
		}

		m_entries.insert(canon, e);
	}
}

ModMetadataIndex::Entry ModMetadataIndex::get(const QString& fileName) const
{
	QMutexLocker lock(&m_mutex);
	return m_entries.value(canonicalFileName(fileName));
}

bool ModMetadataIndex::contains(const QString& fileName) const
{
	QMutexLocker lock(&m_mutex);
	return m_entries.contains(canonicalFileName(fileName));
}

QList<ModMetadataIndex::Entry> ModMetadataIndex::all() const
{
	QMutexLocker lock(&m_mutex);
	return m_entries.values();
}

ModMetadataIndex::Entry
ModMetadataIndex::findByPlatformProject(const QString& platform,
										const QString& projectId) const
{
	if (platform.isEmpty() || projectId.isEmpty()) {
		return {};
	}
	QMutexLocker lock(&m_mutex);
	for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
		const Entry& e = it.value();
		if (e.platform.compare(platform, Qt::CaseInsensitive) == 0 &&
			e.projectId == projectId) {
			return e;
		}
	}
	return {};
}

ModMetadataIndex::Entry
ModMetadataIndex::findByNormalizedName(const QString& normalizedName) const
{
	if (normalizedName.isEmpty()) {
		return {};
	}
	QMutexLocker lock(&m_mutex);
	for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
		const Entry& e = it.value();
		if (normalizeName(e.name) == normalizedName) {
			return e;
		}
	}
	return {};
}

ModMetadataIndex::Entry ModMetadataIndex::findBySha1(const QString& sha1) const
{
	if (sha1.isEmpty()) {
		return {};
	}
	const QString needle = sha1.toLower();
	QMutexLocker lock(&m_mutex);
	for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
		const Entry& e = it.value();
		if (e.sha1 == needle) {
			return e;
		}
	}
	return {};
}

bool ModMetadataIndex::put(const Entry& entry)
{
	if (!entry.isValid()) {
		qWarning() << "ModMetadataIndex::put refused invalid entry";
		return false;
	}

	FS::ensureFolderPathExists(m_indexDir.absolutePath());

	const QByteArray payload = serializeJson(entry);
	const QString path = sidecarPath(entry.fileName);

	QSaveFile f(path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		qWarning() << "ModMetadataIndex::put cannot open" << path
				   << "for writing";
		return false;
	}
	if (f.write(payload) != payload.size()) {
		f.cancelWriting();
		qWarning() << "ModMetadataIndex::put short write to" << path;
		return false;
	}
	if (!f.commit()) {
		qWarning() << "ModMetadataIndex::put failed to commit" << path;
		return false;
	}

	{
		QMutexLocker lock(&m_mutex);
		m_entries.insert(canonicalFileName(entry.fileName), entry);
	}
	return true;
}

bool ModMetadataIndex::remove(const QString& fileName)
{
	const QString canon = canonicalFileName(fileName);
	bool diskRemoved = false;
	const QString path = sidecarPath(fileName);
	if (QFile::exists(path)) {
		diskRemoved = QFile::remove(path);
		if (!diskRemoved) {
			qWarning() << "ModMetadataIndex::remove failed to delete" << path;
		}
	}
	{
		QMutexLocker lock(&m_mutex);
		m_entries.remove(canon);
	}
	return diskRemoved;
}

void ModMetadataIndex::rename(const QString& oldFileName,
							  const QString& newFileName)
{
	const QString oldCanon = canonicalFileName(oldFileName);
	const QString newCanon = canonicalFileName(newFileName);
	if (oldCanon == newCanon) {
		return; /* Toggling enabled/disabled keeps canonical name identical. */
	}

	Entry e;
	{
		QMutexLocker lock(&m_mutex);
		if (!m_entries.contains(oldCanon)) {
			return;
		}
		e = m_entries.take(oldCanon);
	}

	const QString oldPath = sidecarPath(oldFileName);
	if (QFile::exists(oldPath)) {
		QFile::remove(oldPath);
	}

	e.fileName = newCanon;
	put(e);
}
