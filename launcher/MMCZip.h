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

#include <QString>
#include <QFileInfo>
#include <QSet>
#include "minecraft/mod/Mod.h"
#include <functional>

#include <JlCompress.h>
#include <nonstd/optional>

namespace MMCZip
{
    using FilterFunction = std::function<bool(const QString &)>;

    /**
     * Merge two zip files, using a filter function
     */
    bool mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained,
                                            const FilterFunction filter = nullptr);

    /**
     * take a source jar, add mods to it, resulting in target jar
     */
    bool createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod>& mods);

    /**
     * Find a single file in archive by file name (not path)
     *
     * \return the path prefix where the file is
     */
    QString findFolderOfFileInZip(QuaZip * zip, const QString & what, const QString &root = QString(""));

    /**
     * Find a multiple files of the same name in archive by file name
     * If a file is found in a path, no deeper paths are searched
     *
     * \return true if anything was found
     */
    bool findFilesInZip(QuaZip * zip, const QString & what, QStringList & result, const QString &root = QString());

    /**
     * Extract a subdirectory from an archive
     */

    /**
     * Compress a directory, using a filter function to exclude entries
     */
    bool compressDir(QString zipFile, QString dir, FilterFunction excludeFilter);

    nonstd::optional<QStringList> extractSubDir(QuaZip *zip, const QString & subdir, const QString &target);

    bool extractRelFile(QuaZip *zip, const QString & file, const QString &target);

    /**
     * Extract a whole archive.
     *
     * \param fileCompressed The name of the archive.
     * \param dir The directory to extract to, the current directory if left empty.
     * \return The list of the full paths of the files extracted, empty on failure.
     */
    nonstd::optional<QStringList> extractDir(QString fileCompressed, QString dir);

    /**
     * Extract a subdirectory from an archive
     *
     * \param fileCompressed The name of the archive.
     * \param subdir The directory within the archive to extract
     * \param dir The directory to extract to, the current directory if left empty.
     * \return The list of the full paths of the files extracted, empty on failure.
     */
    nonstd::optional<QStringList> extractDir(QString fileCompressed, QString subdir, QString dir);

    /**
     * Extract a single file from an archive into a directory
     *
     * \param fileCompressed The name of the archive.
     * \param file The file within the archive to extract
     * \param dir The directory to extract to, the current directory if left empty.
     * \return true for success or false for failure
     */
    bool extractFile(QString fileCompressed, QString file, QString dir);

}
