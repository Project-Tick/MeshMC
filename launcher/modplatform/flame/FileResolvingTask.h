/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "PackManifest.h"

namespace Flame
{
	class FileResolvingTask : public Task
	{
		Q_OBJECT
	  public:
		explicit FileResolvingTask(
			shared_qobject_ptr<QNetworkAccessManager> network,
			Flame::Manifest& toProcess);
		virtual ~FileResolvingTask() {};

		const Flame::Manifest& getResults() const
		{
			return m_toProcess;
		}

        bool canAbort() const override { return true; }
        bool abort() override { return m_dljob && m_dljob->abort(); }

	  protected:
		virtual void executeTask() override;

	  protected slots:
		void netJobFinished();

	  private: /* data */
		shared_qobject_ptr<QNetworkAccessManager> m_network;
		Flame::Manifest m_toProcess;
		QVector<QByteArray> results;
		NetJob::Ptr m_dljob;
	};
} // namespace Flame
