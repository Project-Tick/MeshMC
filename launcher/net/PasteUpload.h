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
#include "tasks/Task.h"
#include <QNetworkReply>
#include <QBuffer>
#include <memory>

class PasteUpload : public Task
{
	Q_OBJECT
  public:
	PasteUpload(QWidget* window, QString text, QString key = "public");
	virtual ~PasteUpload();

	QString pasteLink()
	{
		return m_pasteLink;
	}
	QString pasteID()
	{
		return m_pasteID;
	}
	int maxSize()
	{
		// 2MB for paste.ee - public
		if (m_key == "public")
			return 1024 * 1024 * 2;
		// 12MB for paste.ee - with actual key
		return 1024 * 1024 * 12;
	}
	bool validateText();

  protected:
	virtual void executeTask();

  private:
	bool parseResult(QJsonDocument doc);
	QString m_error;
	QWidget* m_window;
	QString m_pasteID;
	QString m_pasteLink;
	QString m_key;
	QByteArray m_jsonContent;
	std::shared_ptr<QNetworkReply> m_reply;
  public slots:
	void downloadError(QNetworkReply::NetworkError);
	void downloadFinished();
};
