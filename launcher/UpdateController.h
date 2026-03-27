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

#include <QString>
#include <QList>
#include <updater/GoUpdate.h>

class QWidget;

class UpdateController
{
public:
    UpdateController(QWidget * parent, const QString &root, const QString updateFilesDir, GoUpdate::OperationList operations);
    void installUpdates();

private:
    void fail();
    bool rollback();

private:
    QString m_root;
    QString m_updateFilesDir;
    GoUpdate::OperationList m_operations;
    QWidget * m_parent;

    struct BackupEntry
    {
        // path where we got the new file from
        QString update;
        // path of what is being actually updated
        QString original;
        // path where the backup of the updated file was placed
        QString backup;
    };
    QList <BackupEntry> m_replace_backups;
    QList <BackupEntry> m_delete_backups;
    enum Failure
    {
        Replace,
        Delete,
        Start,
        Nothing
    } m_failedOperationType = Nothing;
    QString m_failedFile;
};
