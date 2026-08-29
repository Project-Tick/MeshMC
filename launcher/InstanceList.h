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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QObject>
#include <QAbstractListModel>
#include <QSet>
#include <QList>

#include "BaseInstance.h"

#include "QObjectPtr.h"

class QFileSystemWatcher;
class InstanceTask;
using InstanceId = QString;
using GroupId = QString;
using InstanceLocator = std::pair<InstancePtr, int>;

enum class InstCreateError {
	NoCreateError = 0,
	NoSuchVersion,
	UnknownCreateError,
	InstExists,
	CantCreateDir
};

enum class GroupsState { NotLoaded, Steady, Dirty };

/// One shortcut that went to the trash along with its instance, and
/// where it went, so that both halves can be put back.
struct TrashedShortcut {
	ShortcutData shortcut;
	QString trashPath;
};

/// Everything needed to undo one trashInstance() call.
struct TrashedInstance {
	InstanceId id;
	/// Display name as it was, for the offer to restore it. The instance's
	/// own config file is in the trash, so this cannot be looked up later.
	QString name;
	/// The folder it came from, and where it goes back to.
	QString path;
	QString trashPath;
	/// The group it was in, which is not recorded inside the folder.
	GroupId group;
	QList<TrashedShortcut> shortcuts;
};

class InstanceList : public QAbstractListModel
{
	Q_OBJECT

  public:
	explicit InstanceList(SettingsObjectPtr settings, const QString& instDir,
						  QObject* parent = 0);
	virtual ~InstanceList();

  public:
	QModelIndex index(int row, int column = 0,
					  const QModelIndex& parent = QModelIndex()) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;

	bool setData(const QModelIndex& index, const QVariant& value,
				 int role) override;

	enum AdditionalRoles {
		GroupRole = Qt::UserRole,
		InstancePointerRole = 0x34B1CB48, ///< Return pointer to real instance
		InstanceIDRole = 0x34B1CB49		  ///< Return id if the instance
	};
	/*!
	 * \brief Error codes returned by functions in the InstanceList class.
	 * NoError Indicates that no error occurred.
	 * UnknownError indicates that an unspecified error occurred.
	 */
	enum InstListError { NoError = 0, UnknownError };

	InstancePtr at(int i) const
	{
		return m_instances.at(i);
	}

	int count() const
	{
		return m_instances.count();
	}

	InstListError loadList();
	void saveNow();

	InstancePtr getInstanceById(QString id) const;
	QModelIndex getInstanceIndexById(const QString& id) const;
	QStringList getGroups();
	bool isGroupCollapsed(const QString& groupName);

	GroupId getInstanceGroup(const InstanceId& id) const;
	void setInstanceGroup(const InstanceId& id, const GroupId& name);

	void deleteGroup(const GroupId& name);

	/// Delete an instance and its shortcuts for good.
	void deleteInstance(const InstanceId& id);

	/**
	 * Move an instance and its shortcuts to the platform's trash.
	 *
	 * Returns false if the platform has no usable trash, or if the move
	 * failed; in both cases nothing has been removed and the caller
	 * should offer deleteInstance() instead.
	 */
	bool trashInstance(const InstanceId& id);

	/// Whether there is anything left to undo.
	bool trashedSomething() const;

	/**
	 * Display name of the instance undoTrashInstance() would put back, so
	 * that the offer to restore it can say which one it means. Empty when
	 * there is nothing to restore.
	 *
	 * Read from the record rather than from disk: the instance's own
	 * config file is in the trash by now.
	 */
	QString lastTrashedName() const;

	/**
	 * Put the most recently trashed instance back, shortcuts included.
	 *
	 * Returns true when there was nothing to do. Returns false if a part
	 * could not be restored; the entry is then kept only when the
	 * instance folder itself is still in the trash, so that a second
	 * attempt can retry it.
	 */
	bool undoTrashInstance();

	// Wrap an instance creation task in some more task machinery and make it
	// ready to be used
	Task* wrapInstanceTask(InstanceTask* task);

	/**
	 * Create a new empty staging area for instance creation and @return a
	 * path/key top commit it later. Used by instance manipulation tasks.
	 */
	QString getStagedInstancePath();

	/**
	 * Commit the staging area given by @keyPath to the provider - used when
	 * creation succeeds. Used by instance manipulation tasks.
	 */
	bool commitStagedInstance(const QString& keyPath,
							  const QString& instanceName,
							  const QString& groupName);

	/**
	 * Destroy a previously created staging area given by @keyPath - used when
	 * creation fails. Used by instance manipulation tasks.
	 */
	bool destroyStagingPath(const QString& keyPath);

	int getTotalPlayTime();

	Qt::DropActions supportedDragActions() const override;

	Qt::DropActions supportedDropActions() const override;

	bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row,
						 int column, const QModelIndex& parent) const override;

	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row,
					  int column, const QModelIndex& parent) override;

	QStringList mimeTypes() const override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;

  signals:
	void dataIsInvalid();
	void instancesChanged();
	void instanceSelectRequest(QString instanceId);
	void groupsChanged(QSet<QString> groups);

  public slots:
	void on_InstFolderChanged(const Setting& setting, QVariant value);
	void on_GroupStateChanged(const QString& group, bool collapsed);

  private slots:
	void propertiesChanged(BaseInstance* inst);
	void providerUpdated();
	void instanceDirContentsChanged(const QString& path);

  private:
	int getInstIndex(BaseInstance* inst) const;
	void updateTotalPlayTime();
	void suspendWatch();
	void resumeWatch();
	void add(const QList<InstancePtr>& list);
	void loadGroupList();
	void saveGroupList();
	QList<InstanceId> discoverInstances();
	InstancePtr loadInstance(const InstanceId& id);

  private:
	int m_watchLevel = 0;
	int totalPlayTime = 0;
	bool m_dirty = false;
	QList<InstancePtr> m_instances;
	QSet<QString> m_groupNameCache;

	SettingsObjectPtr m_globalSettings;
	QString m_instDir;
	QFileSystemWatcher* m_watcher;
	// FIXME: this is so inefficient that looking at it is almost painful.
	QSet<QString> m_collapsedGroups;
	QMap<InstanceId, GroupId> m_instanceGroupIndex;
	QSet<InstanceId> instanceSet;
	/// Trashed instances, most recent last, for undoTrashInstance().
	QList<TrashedInstance> m_trashHistory;
	bool m_groupsLoaded = false;
	bool m_instancesProbed = false;
};
