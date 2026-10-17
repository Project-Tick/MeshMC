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

#include "qml/Theme/KirigamiColors.h"

#include <QStringList>

#include <Kirigami/Platform/PlatformPluginFactory>

namespace
{
	bool isPlasmaSession()
	{
		const QStringList desktops = qEnvironmentVariable("XDG_CURRENT_DESKTOP")
										 .split(QLatin1Char(':'), Qt::SkipEmptyParts);
		return desktops.contains(QStringLiteral("KDE"), Qt::CaseInsensitive) ||
			   qEnvironmentVariable("KDE_FULL_SESSION") == QLatin1String("true");
	}
} // namespace

QString KirigamiColors::desktopPlugin()
{
	const QString plugin = QStringLiteral("org.kde.desktop");
	if (!isPlasmaSession())
		return QString();
	// Cached by Kirigami per name, found or not, so asking twice is free.
	return Kirigami::Platform::PlatformPluginFactory::findPlugin(plugin) ? plugin : QString();
}

std::unique_ptr<KirigamiColors> KirigamiColors::forDesktop()
{
	const QString plugin = desktopPlugin();
	if (plugin.isEmpty())
		return nullptr;
	auto* factory = Kirigami::Platform::PlatformPluginFactory::findPlugin(plugin);
	auto colors = std::make_unique<KirigamiColors>(
		[factory](PlatformTheme::ColorSet set, QObject* parent) -> PlatformTheme* {
			PlatformTheme* theme = factory->createPlatformTheme(parent);
			if (theme) {
				theme->setInherit(false);
				theme->setColorSet(set);
			}
			return theme;
		});
	if (!colors->isValid())
		return nullptr;
	return colors;
}

KirigamiColors::KirigamiColors(const ThemeFactory& factory, QObject* parent)
	: QObject(parent)
{
	const auto make = [this, &factory](PlatformTheme::ColorSet set) {
		// One holder per set: the object a Kirigami.Theme attached property
		// would sit on in QML, so no two themes share one.
		PlatformTheme* theme = factory(set, new QObject(this));
		if (theme)
			connect(theme, &PlatformTheme::colorsChanged, this,
					&KirigamiColors::scheduleChanged);
		return theme;
	};
	m_window = make(PlatformTheme::Window);
	m_view = make(PlatformTheme::View);
	m_selection = make(PlatformTheme::Selection);
	m_tooltip = make(PlatformTheme::Tooltip);
}

bool KirigamiColors::isValid() const
{
	return m_window && m_view && m_selection && m_tooltip;
}

SystemColors KirigamiColors::colors() const
{
	SystemColors s;
	if (!isValid())
		return s;

	s.window = m_window->backgroundColor();
	s.text = m_window->textColor();
	s.disabledText = m_window->disabledTextColor();
	// Plasma's accent colour is its highlight; there is no separate role.
	s.accent = m_window->highlightColor();
	s.accentLabel = m_window->highlightedTextColor();
	s.positive = m_window->positiveTextColor();
	s.neutral = m_window->neutralTextColor();
	s.negative = m_window->negativeTextColor();
	// What Kirigami.InlineMessage's Information type is drawn in.
	s.info = m_window->activeTextColor();

	s.view = m_view->backgroundColor();
	// The colour KDE's own text fields give their placeholder.
	s.placeholderText = m_view->disabledTextColor();
	s.focus = m_view->focusColor();

	s.highlight = m_selection->backgroundColor();
	s.highlightedText = m_selection->textColor();

	s.tooltip = m_tooltip->backgroundColor();
	s.tooltipText = m_tooltip->textColor();
	return s;
}

void KirigamiColors::scheduleChanged()
{
	/* A new colour scheme reaches each set's theme in turn; announcing after
	 * the event loop comes back means it is announced once, whole, rather
	 * than once per set with the others still on the old scheme. */
	if (m_changePending)
		return;
	m_changePending = true;
	QMetaObject::invokeMethod(
		this,
		[this]() {
			m_changePending = false;
			emit changed();
		},
		Qt::QueuedConnection);
}
