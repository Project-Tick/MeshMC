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

#include "FileResolvingTask.h"
#include "Json.h"

namespace {
    const char * metabase = "https://cursemeta.dries007.net";
}

Flame::FileResolvingTask::FileResolvingTask(shared_qobject_ptr<QNetworkAccessManager> network, Flame::Manifest& toProcess)
    : m_network(network), m_toProcess(toProcess)
{
}

void Flame::FileResolvingTask::executeTask()
{
    setStatus(tr("Resolving mod IDs..."));
    setProgress(0, m_toProcess.files.size());
    m_dljob = new NetJob("Mod id resolver", m_network);
    results.resize(m_toProcess.files.size());
    int index = 0;
    for(auto & file: m_toProcess.files)
    {
        auto projectIdStr = QString::number(file.projectId);
        auto fileIdStr = QString::number(file.fileId);
        QString metaurl = QString("%1/%2/%3.json").arg(metabase, projectIdStr, fileIdStr);
        auto dl = Net::Download::makeByteArray(QUrl(metaurl), &results[index]);
        m_dljob->addNetAction(dl);
        index ++;
    }
    connect(m_dljob.get(), &NetJob::finished, this, &Flame::FileResolvingTask::netJobFinished);
    m_dljob->start();
}

void Flame::FileResolvingTask::netJobFinished()
{
    bool failed = false;
    int index = 0;
    for(auto & bytes: results)
    {
        auto & out = m_toProcess.files[index];
        try
        {
            failed &= (!out.parseFromBytes(bytes));
        }
        catch (const JSONValidationError &e)
        {

            qCritical() << "Resolving of" << out.projectId << out.fileId << "failed because of a parsing error:";
            qCritical() << e.cause();
            qCritical() << "JSON:";
            qCritical() << bytes;
            failed = true;
        }
        index++;
    }
    if(!failed)
    {
        emitSucceeded();
    }
    else
    {
        emitFailed(tr("Some mod ID resolving tasks failed."));
    }
}
