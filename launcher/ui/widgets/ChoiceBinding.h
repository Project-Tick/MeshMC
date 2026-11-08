/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QList>
#include <QString>
#include <QWidget>

class QAbstractButton;
class QComboBox;
class QLineEdit;

/*!
 * Lets a choice of radio buttons, or the item data of a combo box, be a
 * setting that KConfigDialogManager binds, by being the widget named
 * "kcfg_<ID>". (KConfigDialogManager binds a combo box by its index or its
 * shown text, and neither is what a setting such as the ID of a provider
 * holds.)
 *
 * The setting is a text: each button stands for one value, and a button may
 * instead stand for whatever a line edit holds (a "custom" entry next to the
 * fixed ones). The widget is only the glue: it is hidden, never put in a
 * layout, and made a child of the page that has the buttons.
 *
 * With a combo box, choice() is the item data of the current item as a text,
 * and setChoice() selects the item that has it.
 *
 * choice() is the value of the checked button, or an empty text when none is.
 * setChoice() checks the button of that value; a value that no fixed button
 * stands for goes to the button of the line edit, if there is one, and is put
 * in the line edit.
 */
class ChoiceBinding : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString choice READ choice WRITE setChoice NOTIFY choiceChanged
				   USER true)
  public:
	explicit ChoiceBinding(const QString& settingId, QWidget* parent);

	//! \p button stands for \p value.
	void addChoice(QAbstractButton* button, const QString& value);
	//! \p button stands for the text of \p edit.
	void addTextChoice(QAbstractButton* button, QLineEdit* edit);

	//! The item data of \p combo is the value.
	void setComboBox(QComboBox* combo);

	QString choice() const;
	void setChoice(const QString& value);

  signals:
	void choiceChanged();

  private:
	struct Entry {
		QAbstractButton* button;
		QString value;
		QLineEdit* edit; // when not null, the value is its text
	};
	QList<Entry> m_entries;
	QComboBox* m_combo = nullptr;
};
