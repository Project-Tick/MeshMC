/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "GameOptions.h"
#include "FileSystem.h"
#include <QDebug>
#include <QSaveFile>

namespace
{
	bool load(const QString& path, std::vector<GameOptionItem>& contents,
			  int& version)
	{
		contents.clear();
		QFile file(path);
		if (!file.open(QFile::ReadOnly)) {
			qWarning() << "Failed to read options file.";
			return false;
		}
		version = 0;
		while (!file.atEnd()) {
			auto line = file.readLine();
			if (line.endsWith('\n')) {
				line.chop(1);
			}
			auto separatorIndex = line.indexOf(':');
			if (separatorIndex == -1) {
				continue;
			}
			auto key = QString::fromUtf8(line.data(), separatorIndex);
			auto value = QString::fromUtf8(line.data() + separatorIndex + 1,
										   line.size() - 1 - separatorIndex);
			qDebug() << "!!" << key << "!!";
			if (key == "version") {
				version = value.toInt();
				continue;
			}
			contents.emplace_back(GameOptionItem{key, value});
		}
		qDebug() << "Loaded" << path << "with version:" << version;
		return true;
	}
	bool save(const QString& path, std::vector<GameOptionItem>& mapping,
			  int version)
	{
		QSaveFile out(path);
		if (!out.open(QIODevice::WriteOnly)) {
			return false;
		}
		if (version != 0) {
			QString versionLine = QString("version:%1\n").arg(version);
			out.write(versionLine.toUtf8());
		}
		auto iter = mapping.begin();
		while (iter != mapping.end()) {
			out.write(iter->key.toUtf8());
			out.write(":");
			out.write(iter->value.toUtf8());
			out.write("\n");
			iter++;
		}
		return out.commit();
	}
} // namespace

GameOptions::GameOptions(const QString& path) : path(path)
{
	reload();
}

QVariant GameOptions::headerData(int section, Qt::Orientation orientation,
								 int role) const
{
	if (role != Qt::DisplayRole) {
		return QAbstractListModel::headerData(section, orientation, role);
	}
	switch (section) {
		case 0:
			return tr("Key");
		case 1:
			return tr("Value");
		default:
			return QVariant();
	}
}

QVariant GameOptions::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= int(contents.size()))
		return QVariant();

	switch (role) {
		case Qt::DisplayRole:
			if (column == 0) {
				return contents[row].key;
			} else {
				return contents[row].value;
			}
		default:
			return QVariant();
	}
	return QVariant();
}

int GameOptions::rowCount(const QModelIndex&) const
{
	return contents.size();
}

int GameOptions::columnCount(const QModelIndex&) const
{
	return 2;
}

bool GameOptions::isLoaded() const
{
	return loaded;
}

bool GameOptions::reload()
{
	beginResetModel();
	loaded = load(path, contents, version);
	endResetModel();
	return loaded;
}

bool GameOptions::save()
{
	return ::save(path, contents, version);
}
