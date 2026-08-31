/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 * Copyright 2020-2021 Petr Mrazek <peterix@gmail.com>
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

#include "FTBPackManifest.h"

#include "InstanceTask.h"
#include "net/NetJob.h"

namespace ModpacksCH
{

	class PackInstallTask : public InstanceTask
	{
		Q_OBJECT

	  public:
		explicit PackInstallTask(Modpack pack, QString version);
		virtual ~PackInstallTask() {}

		bool canAbort() const override
		{
			/* Two abortable stretches, with a gap between them: the
			 * pack's own downloads while `abortable` is set, and the
			 * optional game-file download the base class runs once the
			 * instance is built. Answering "always" would light up a
			 * button that abort() then refuses to act on. */
			return abortable || InstanceTask::canAbort();
		}
		bool abort() override;

	  protected:
		virtual void executeTask() override;

	  private slots:
		void onDownloadSucceeded();
		void onDownloadFailed(QString reason);

	  private:
		void downloadPack();
		void install();

	  private:
		bool abortable = false;

		NetJob::Ptr jobPtr;
		QByteArray response;

		Modpack m_pack;
		QString m_version_name;
		Version m_version;

		QMap<QString, QString> filesToCopy;
	};

} // namespace ModpacksCH
