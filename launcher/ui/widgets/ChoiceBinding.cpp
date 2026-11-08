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

#include "ChoiceBinding.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QLineEdit>

ChoiceBinding::ChoiceBinding(const QString& settingId, QWidget* parent)
	: QWidget(parent)
{
	setObjectName(QStringLiteral("kcfg_") + settingId);
	hide();
}

void ChoiceBinding::addChoice(QAbstractButton* button, const QString& value)
{
	m_entries.append({button, value, nullptr});
	connect(button, &QAbstractButton::toggled, this,
			&ChoiceBinding::choiceChanged);
}

void ChoiceBinding::addTextChoice(QAbstractButton* button, QLineEdit* edit)
{
	m_entries.append({button, QString(), edit});
	connect(button, &QAbstractButton::toggled, this,
			&ChoiceBinding::choiceChanged);
	connect(edit, &QLineEdit::textChanged, this, [this] {
		for (const Entry& entry : std::as_const(m_entries)) {
			if (entry.edit && entry.button->isChecked()) {
				emit choiceChanged();
			}
		}
	});
}

void ChoiceBinding::setComboBox(QComboBox* combo)
{
	m_combo = combo;
	connect(combo, &QComboBox::currentIndexChanged, this,
			&ChoiceBinding::choiceChanged);
}

QString ChoiceBinding::choice() const
{
	if (m_combo) {
		return m_combo->currentData().toString();
	}
	for (const Entry& entry : m_entries) {
		if (entry.button->isChecked()) {
			return entry.edit ? entry.edit->text() : entry.value;
		}
	}
	return QString();
}

void ChoiceBinding::setChoice(const QString& value)
{
	if (m_combo) {
		const int index = m_combo->findData(value);
		if (index >= 0) {
			m_combo->setCurrentIndex(index);
		}
		return;
	}
	for (const Entry& entry : std::as_const(m_entries)) {
		if (!entry.edit && entry.value == value) {
			entry.button->setChecked(true);
			return;
		}
	}
	for (const Entry& entry : std::as_const(m_entries)) {
		if (entry.edit) {
			entry.button->setChecked(true);
			entry.edit->setText(value);
			return;
		}
	}
}
