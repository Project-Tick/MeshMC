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
#include <QStringList>
#include <QVariant>

#include <functional>

class QGroupBox;
class QWidget;
class SettingsObject;

/*!
 * A group of widgets that show settings of an instance, and that the user
 * may switch on to override what the global settings say.
 *
 * The settings of an instance are an OverrideSetting of a global one, and the
 * gate that decides which of the two is read is a setting of the instance
 * too, "OverrideMemory" for example. The page of an instance shows that as a
 * group box that can be checked. KConfigDialogManager has no such thing, so
 * this is the part of the page that stays by hand, once, instead of once for
 * every group:
 *
 *  - load() sets the gate from its setting, and the widgets from the
 *    settings. With the gate off the settings give the global values, so the
 *    widgets show what the instance would use.
 *  - apply() writes the gate, and, with it on, the values; with it off it
 *    resets them, so the instance follows the global settings again.
 *
 * A widget is bound by the ID of its setting. A check box, a spin box, a line
 * edit, a plain text edit and a combo box (by its item data) are known;
 * anything else takes its own read and write functions.
 */
class OverrideGroup
{
  public:
	using Reader = std::function<QVariant()>;
	using Writer = std::function<void(const QVariant&)>;

	//! The gate is a group box that can be checked.
	OverrideGroup(SettingsObject* settings, QGroupBox* box,
				  const QString& gateId);

	//! The gate is whatever @p isOn and @p setOn say.
	OverrideGroup(SettingsObject* settings, const QString& gateId,
				  std::function<bool()> isOn, std::function<void(bool)> setOn);

	OverrideGroup& bind(const QString& id, QWidget* widget);
	OverrideGroup& bind(const QString& id, Reader read, Writer write);

	/*!
	 * Earlier versions had one gate for two groups ("OverrideJava"). The group
	 * is on if one of these is, and apply() drops them.
	 */
	OverrideGroup& legacyGate(const QString& id);

	//! A value is not written as it is shown, but as this makes of it.
	OverrideGroup& transformOnApply(const QString& id,
									std::function<QVariant(QVariant)> f);

	void load();
	void apply();

	bool isOn() const;

  private:
	struct Binding {
		QString id;
		Reader read;
		Writer write;
		std::function<QVariant(QVariant)> transform;
	};

	SettingsObject* m_settings;
	QString m_gateId;
	std::function<bool()> m_isOn;
	std::function<void(bool)> m_setOn;
	QList<Binding> m_bindings;
	QStringList m_legacyGates;
};
