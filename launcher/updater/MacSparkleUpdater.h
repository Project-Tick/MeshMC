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

#include "ExternalUpdater.h"

#include <QSet>
#include <QString>

class MacSparkleUpdater : public ExternalUpdater
{
	Q_OBJECT

  public:
	MacSparkleUpdater();
	~MacSparkleUpdater() override;

	void checkForUpdates() override;

	bool getAutomaticallyChecksForUpdates() override;
	double getUpdateCheckInterval() override;
	bool getBetaAllowed() override;

	void setAutomaticallyChecksForUpdates(bool check) override;
	void setUpdateCheckInterval(double seconds) override;
	void setBetaAllowed(bool allowed) override;

	QSet<QString> getAllowedChannels();
	void setAllowedChannel(const QString& channel);
	void setAllowedChannels(const QSet<QString>& channels);

	//! Back to release-only.
	void clearAllowedChannels();

  private:
	class Private;
	Private* priv;
};
