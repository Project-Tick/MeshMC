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

class XboxAuthorizationStep : public AuthStep
{
	Q_OBJECT

  public:
	explicit XboxAuthorizationStep(AccountData* data, Katabasis::Token* token,
								   QString relyingParty,
								   QString authorizationKind);
	virtual ~XboxAuthorizationStep() noexcept;

	void perform() override;
	void rehydrate() override;

	QString describe() override;

  private:
	bool processSTSError(QNetworkReply::NetworkError error, QByteArray data,
						 QList<QNetworkReply::RawHeaderPair> headers);

  private slots:
	void onRequestDone(QNetworkReply::NetworkError, QByteArray,
					   QList<QNetworkReply::RawHeaderPair>);

  private:
	Katabasis::Token* m_token;
	QString m_relyingParty;
	QString m_authorizationKind;
};
