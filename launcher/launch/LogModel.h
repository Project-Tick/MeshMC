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

#include <QAbstractListModel>
#include <QString>
#include "MessageLevel.h"

class LogModel : public QAbstractListModel
{
	Q_OBJECT
  public:
	explicit LogModel(QObject* parent = 0);

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role) const;

	void append(MessageLevel::Enum, QString line);
	void clear();

	void suspend(bool suspend);
	bool suspended();

	QString toPlainText();

	int getMaxLines();
	void setMaxLines(int maxLines);
	void setStopOnOverflow(bool stop);
	void setOverflowMessage(const QString& overflowMessage);

	void setLineWrap(bool state);
	bool wrapLines() const;

	enum Roles { LevelRole = Qt::UserRole };

  private /* types */:
	struct entry {
		MessageLevel::Enum level;
		QString line;
	};

  private: /* data */
	QVector<entry> m_content;
	int m_maxLines = 1000;
	// first line in the circular buffer
	int m_firstLine = 0;
	// number of lines occupied in the circular buffer
	int m_numLines = 0;
	bool m_stopOnOverflow = false;
	QString m_overflowMessage = "OVERFLOW";
	bool m_suspended = false;
	bool m_lineWrap = true;

  private:
	Q_DISABLE_COPY(LogModel)
};
