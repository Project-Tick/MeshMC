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

#include "qml/ColorSchemes.h"

#include <QGuiApplication>
#include <QStringList>
#include <QStyleHints>

#include <KColorSchemeManager>

bool ColorSchemes::isPlasmaSession()
{
	const QStringList desktops = qEnvironmentVariable("XDG_CURRENT_DESKTOP")
									 .split(QLatin1Char(':'), Qt::SkipEmptyParts);
	return desktops.contains(QLatin1String("KDE"), Qt::CaseInsensitive) ||
		   qEnvironmentVariable("KDE_FULL_SESSION") == QLatin1String("true");
}

ColorSchemes::ColorSchemes(QObject* parent) : QObject(parent)
{
	if (isPlasmaSession()) {
		return;
	}
	m_manager = KColorSchemeManager::instance();
	// Kept in the launcher's own settings instead; see the class comment.
	m_manager->setAutosaveChanges(false);
	m_chosen = m_manager->activeSchemeId();

	/* The manager follows the system's light or dark switch by itself too,
	 * but runs into the check followSystem() describes and drops to the
	 * platform palette; this runs after its handler and puts Breeze back. */
	connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
			[this]() {
				if (m_chosen.isEmpty()) {
					followSystem();
				}
			});
}

QAbstractItemModel* ColorSchemes::model() const
{
	return m_manager ? m_manager->model() : nullptr;
}

void ColorSchemes::choose(const QString& id)
{
	if (!m_manager) {
		return;
	}
	if (id.isEmpty()) {
		followSystem();
	} else {
		m_manager->activateSchemeId(id);
	}
	// An unknown id falls back to following the system, as an empty one.
	const QString active = m_manager->activeSchemeId();
	if (active != m_chosen) {
		m_chosen = active;
		emit chosenChanged();
	}
}

void ColorSchemes::followSystem()
{
	/* KColorSchemeManager picks Breeze Light or Dark for "the system" only
	 * while the application carries no scheme path -- a check meant for
	 * platform themes that bring their own -- and every scheme it applies
	 * leaves one there, its own included. Without clearing it, asking for
	 * the system scheme resets to the bare platform palette instead. */
	qApp->setProperty("KDE_COLOR_SCHEME_PATH", QVariant());
	m_manager->activateSchemeId(QString());
}
