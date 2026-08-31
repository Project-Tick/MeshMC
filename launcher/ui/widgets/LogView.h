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
#include <QPlainTextEdit>
#include <QAbstractItemView>

class QAbstractItemModel;

class LogView : public QPlainTextEdit
{
	Q_OBJECT
  public:
	explicit LogView(QWidget* parent = nullptr);
	virtual ~LogView();

	virtual void setModel(QAbstractItemModel* model);
	QAbstractItemModel* model() const;

  public slots:
	void setWordWrap(bool wrapping);
	void findNext(const QString& what, bool reverse);
	void scrollToBottom();

  protected slots:
	void repopulate();
	// note: this supports only appending
	void rowsInserted(const QModelIndex& parent, int first, int last);
	void rowsAboutToBeInserted(const QModelIndex& parent, int first, int last);
	// note: this supports only removing from front
	void rowsRemoved(const QModelIndex& parent, int first, int last);
	void modelDestroyed(QObject* model);

  protected:
	QAbstractItemModel* m_model = nullptr;
	QTextCharFormat* m_defaultFormat = nullptr;
	bool m_scroll = false;
	bool m_scrolling = false;
};
