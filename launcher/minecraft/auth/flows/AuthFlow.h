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
 */

#pragma once

#include <QObject>
#include <QList>
#include <QVector>
#include <QSet>
#include <QNetworkReply>
#include <QImage>

#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AccountTask.h"
#include "minecraft/auth/AuthStep.h"

class AuthFlow : public AccountTask
{
	Q_OBJECT

  public:
	explicit AuthFlow(AccountData* data, QObject* parent = 0);

	Katabasis::Validity validity()
	{
		return m_data->validity_;
	};

	QString getStateMessage() const override;

	void executeTask() override;

  signals:
	// No extra signals needed - authorizeWithBrowser is on AccountTask

  private slots:
	void stepFinished(AccountTaskState resultingState, QString message);

  protected:
	void succeed();
	void nextStep();

  protected:
	QList<AuthStep::Ptr> m_steps;
	AuthStep::Ptr m_currentStep;
};
