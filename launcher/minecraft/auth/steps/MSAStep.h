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
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"

#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>

class MSAStep : public AuthStep
{
	Q_OBJECT
  public:
	enum Action { Refresh, Login };

  public:
	explicit MSAStep(AccountData* data, Action action);
	virtual ~MSAStep() noexcept;

	void perform() override;
	void rehydrate() override;

	QString describe() override;

  private slots:
	void onGranted();
	void onRequestFailed(QAbstractOAuth::Error error);
	void onOpenBrowser(const QUrl& url);

  private:
	QOAuth2AuthorizationCodeFlow* m_oauth2 = nullptr;
	QOAuthHttpServerReplyHandler* m_replyHandler = nullptr;
	Action m_action;
};
