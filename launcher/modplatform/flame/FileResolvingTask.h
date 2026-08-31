/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
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
