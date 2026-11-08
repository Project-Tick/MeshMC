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

#include "OverrideGroup.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>

#include "settings/SettingsObject.h"

OverrideGroup::OverrideGroup(SettingsObject* settings, QGroupBox* box,
							 const QString& gateId)
	: OverrideGroup(
		  settings, gateId, [box] { return box->isChecked(); },
		  [box](bool on) { box->setChecked(on); })
{
}

OverrideGroup::OverrideGroup(SettingsObject* settings, const QString& gateId,
							 std::function<bool()> isOn,
							 std::function<void(bool)> setOn)
	: m_settings(settings), m_gateId(gateId), m_isOn(std::move(isOn)),
	  m_setOn(std::move(setOn))
{
}

OverrideGroup& OverrideGroup::bind(const QString& id, QWidget* widget)
{
	if (auto* check = qobject_cast<QCheckBox*>(widget)) {
		return bind(
			id, [check] { return check->isChecked(); },
			[check](const QVariant& v) { check->setChecked(v.toBool()); });
	}
	if (auto* spin = qobject_cast<QSpinBox*>(widget)) {
		return bind(
			id, [spin] { return spin->value(); },
			[spin](const QVariant& v) { spin->setValue(v.toInt()); });
	}
	if (auto* line = qobject_cast<QLineEdit*>(widget)) {
		return bind(
			id, [line] { return line->text(); },
			[line](const QVariant& v) { line->setText(v.toString()); });
	}
	if (auto* text = qobject_cast<QPlainTextEdit*>(widget)) {
		return bind(
			id, [text] { return text->toPlainText(); },
			[text](const QVariant& v) { text->setPlainText(v.toString()); });
	}
	if (auto* combo = qobject_cast<QComboBox*>(widget)) {
		return bind(
			id, [combo] { return combo->currentData().toString(); },
			[combo](const QVariant& v) {
				// a value that is not in the list leaves the choice as it is
				const int index = combo->findData(v.toString());
				if (index >= 0) {
					combo->setCurrentIndex(index);
				}
			});
	}
	qWarning() << "OverrideGroup: no way to show the setting" << id << "in a"
			   << (widget ? widget->metaObject()->className() : "null widget");
	return *this;
}

OverrideGroup& OverrideGroup::bind(const QString& id, Reader read,
								   Writer write)
{
	m_bindings.append({id, std::move(read), std::move(write), nullptr});
	return *this;
}

OverrideGroup& OverrideGroup::legacyGate(const QString& id)
{
	m_legacyGates.append(id);
	return *this;
}

OverrideGroup&
OverrideGroup::transformOnApply(const QString& id,
								std::function<QVariant(QVariant)> f)
{
	for (auto& binding : m_bindings) {
		if (binding.id == id) {
			binding.transform = std::move(f);
			return *this;
		}
	}
	qWarning() << "OverrideGroup: nothing is bound to" << id;
	return *this;
}

bool OverrideGroup::isOn() const
{
	return m_isOn();
}

void OverrideGroup::load()
{
	bool on = m_settings->get(m_gateId).toBool();
	for (const auto& legacy : m_legacyGates) {
		on = on || m_settings->get(legacy).toBool();
	}
	m_setOn(on);
	for (const auto& binding : m_bindings) {
		binding.write(m_settings->get(binding.id));
	}
}

void OverrideGroup::apply()
{
	const bool on = m_isOn();
	m_settings->set(m_gateId, on);
	for (const auto& binding : m_bindings) {
		if (on) {
			QVariant value = binding.read();
			if (binding.transform) {
				value = binding.transform(value);
			}
			m_settings->set(binding.id, value);
		} else {
			m_settings->reset(binding.id);
		}
	}
	for (const auto& legacy : m_legacyGates) {
		m_settings->reset(legacy);
	}
}
