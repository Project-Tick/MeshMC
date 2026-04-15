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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>

#include "AuthFlow.h"
#include "katabasis/Globals.h"

#include <Application.h>

AuthFlow::AuthFlow(AccountData* data, QObject* parent)
	: AccountTask(data, parent)
{
}

void AuthFlow::succeed()
{
	m_data->validity_ = Katabasis::Validity::Certain;
	changeState(AccountTaskState::STATE_SUCCEEDED,
				tr("Finished all authentication steps"));
}

void AuthFlow::executeTask()
{
	if (m_currentStep) {
		return;
	}
	changeState(AccountTaskState::STATE_WORKING, tr("Initializing"));
	nextStep();
}

void AuthFlow::nextStep()
{
	if (m_steps.size() == 0) {
		// we got to the end without an incident... assume this is all.
		m_currentStep.reset();
		succeed();
		return;
	}
	m_currentStep = m_steps.front();
	qDebug() << "AuthFlow:" << m_currentStep->describe();
	m_steps.pop_front();
	connect(m_currentStep.get(), &AuthStep::finished, this,
			&AuthFlow::stepFinished);
	connect(m_currentStep.get(), &AuthStep::authorizeWithBrowser, this,
			&AuthFlow::authorizeWithBrowser);

	m_currentStep->perform();
}

QString AuthFlow::getStateMessage() const
{
	switch (m_taskState) {
		case AccountTaskState::STATE_WORKING: {
			if (m_currentStep) {
				return m_currentStep->describe();
			} else {
				return tr("Working...");
			}
		}
		default: {
			return AccountTask::getStateMessage();
		}
	}
}

void AuthFlow::stepFinished(AccountTaskState resultingState, QString message)
{
	if (changeState(resultingState, message)) {
		nextStep();
	}
}
