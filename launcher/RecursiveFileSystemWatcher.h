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

#pragma once

#include <QFileSystemWatcher>
#include <QDir>
#include "pathmatcher/IPathMatcher.h"

class RecursiveFileSystemWatcher : public QObject
{
	Q_OBJECT
  public:
	RecursiveFileSystemWatcher(QObject* parent);

	void setRootDir(const QDir& root);
	QDir rootDir() const
	{
		return m_root;
	}

	// WARNING: setting this to true may be bad for performance
	void setWatchFiles(const bool watchFiles);
	bool watchFiles() const
	{
		return m_watchFiles;
	}

	void setMatcher(IPathMatcher::Ptr matcher)
	{
		m_matcher = matcher;
	}

	QStringList files() const
	{
		return m_files;
	}

  signals:
	void filesChanged();
	void fileChanged(const QString& path);

  public slots:
	void enable();
	void disable();

  private:
	QDir m_root;
	bool m_watchFiles = false;
	bool m_isEnabled = false;
	IPathMatcher::Ptr m_matcher;

	QFileSystemWatcher* m_watcher;

	QStringList m_files;
	void setFiles(const QStringList& files);

	void addFilesToWatcherRecursive(const QDir& dir);
	QStringList scanRecursive(const QDir& dir);

  private slots:
	void fileChange(const QString& path);
	void directoryChange(const QString& path);
};
