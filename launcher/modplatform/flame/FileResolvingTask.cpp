/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "FileResolvingTask.h"
#include "Json.h"
#include "FlameApi.h"

Flame::FileResolvingTask::FileResolvingTask(
	shared_qobject_ptr<QNetworkAccessManager> network,
	Flame::Manifest& toProcess)
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
	for (auto& file : m_toProcess.files) {
		auto projectIdStr = QString::number(file.projectId);
		auto fileIdStr = QString::number(file.fileId);
        auto dl = Net::Download::makeByteArray(FlameApi::fileUrl(projectIdStr, fileIdStr), &results[index]);
		m_dljob->addNetAction(dl);
		index++;
	}
	connect(m_dljob.get(), &NetJob::finished, this,
			&Flame::FileResolvingTask::netJobFinished);
    connect(m_dljob.get(), &Task::progress, this, &Task::setProgress);
    propagateStepsFrom(m_dljob.get());
	m_dljob->start();
}

void Flame::FileResolvingTask::netJobFinished()
{
    if (m_dljob->wasAborted())
    {
        emitAborted();
        return;
    }
	int index = 0;
	int unresolved = 0;
	for (auto& bytes : results) {
		auto& out = m_toProcess.files[index];
		try {
			if (!out.parseFromBytes(bytes)) {
				unresolved++;
				qWarning() << "Resolving of" << out.projectId << out.fileId
						   << "failed: mod may have restricted downloads";
			}
		} catch (const JSONValidationError& e) {
			unresolved++;
			qCritical() << "Resolving of" << out.projectId << out.fileId
						<< "failed because of a parsing error:";
			qCritical() << e.cause();
			qCritical() << "JSON:";
			qCritical() << bytes;
		}
		index++;
	}
	if (unresolved > 0) {
		qWarning() << unresolved
				   << "mod(s) could not be resolved (restricted downloads). "
					  "They will be skipped.";
	}
	emitSucceeded();
}
