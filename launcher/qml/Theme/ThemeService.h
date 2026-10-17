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

#include <QObject>
#include <QString>
#include <qqmlintegration.h>

#include <memory>

#include "theme/SystemPalette.h"
#include "theme/ThemePalette.h"

class KirigamiColors;

/*
 * The C++ half of MeshMC.Theme: owns which palette is active and follows the
 * OS when asked to.
 *
 * Registered as this module's QML singleton because Theme.qml -- the
 * documented public surface every other QML file binds to -- needs exactly
 * one instance to forward palette/dark/mode to. Consumers are expected to go
 * through Theme.*, not this type; it is deliberately absent from the
 * contract, ThemePalette's colours pass through untouched.
 *
 * One `changed()` signal covers palette, dark and mode's effect together, so
 * a binding can never observe, say, the new `dark` with the previous
 * `palette` still attached mid-switch.
 *
 * The "system" scheme is the desktop's own colours (ThemePalette::
 * fromSystem()), read from Kirigami's platform theme on a Plasma session
 * (KirigamiColors) and from QPalette everywhere else, and re-read live when
 * either reports a change. It decides light or dark by itself, so `mode`
 * only applies to the four named schemes.
 */
class ThemeService : public QObject
{
	Q_OBJECT
	QML_ELEMENT
	QML_SINGLETON

	Q_PROPERTY(ThemePalette palette READ palette NOTIFY changed)
	Q_PROPERTY(bool dark READ dark NOTIFY changed)
	Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY changed)
	/* "system", "amethyst", "ember", "diamond" or "grass" -- see
	 * ThemePalette::Scheme and ThemePalette::fromSystem(). */
	Q_PROPERTY(QString scheme READ scheme WRITE setScheme NOTIFY changed)
	/* The "system" scheme's palette as it stands, whichever scheme is
	 * active: the picker shows it live on its System card. */
	Q_PROPERTY(ThemePalette systemPalette READ systemPalette NOTIFY systemPaletteChanged)

  public:
	explicit ThemeService(QObject* parent = nullptr);
	~ThemeService() override;

	ThemePalette palette() const { return m_palette; }
	bool dark() const { return m_dark; }
	QString mode() const { return m_mode; }

	/* Accepts "system", "dark" or "light"; anything else is rejected with a
	 * warning and leaves the current mode untouched. */
	void setMode(const QString& mode);

	QString scheme() const;
	/* Unknown names are rejected with a warning, like setMode(). */
	void setScheme(const QString& scheme);

	ThemePalette systemPalette() const { return m_systemPalette; }

	/* @p scheme's palette in the mode it would show in, for a picker that
	 * shows every scheme side by side without switching to it. */
	Q_INVOKABLE ThemePalette previewPalette(const QString& scheme) const;

  signals:
	void changed();
	void systemPaletteChanged();

  protected:
	/* QEvent::ApplicationPaletteChange: the OS switched light/dark or its
	 * accent colour. Installed on the application, so it sees that event
	 * the moment Qt has the new palette. */
	bool eventFilter(QObject* watched, QEvent* event) override;

  private:
	/* Re-reads the desktop's colours; announces systemPaletteChanged, and
	 * the active theme's change where it follows the system, only when
	 * something actually moved. */
	void readSystemColors();
	/* Light or dark for the named schemes, by `mode`. */
	bool modeIsDark() const;
	/* Works out palette and dark from the scheme, mode and system colours,
	 * and announces them -- always when @p announce, since a new mode or
	 * scheme is news even when it happens to look the same; otherwise only
	 * on a real change. */
	void resolve(bool announce);

	/* Dark by default: the named schemes are designed dark-first. "system"
	 * follows the OS instead, and is one setting away. */
	QString m_mode = QStringLiteral("dark");
	/* The "system" scheme is the app's default (Application.cpp registers
	 * the "UiPalette" setting as "system"); this only matters before
	 * Main.qml's own Component.onCompleted applies the persisted setting,
	 * or for a consumer that never does, e.g. a unit test constructing
	 * ThemeService directly. */
	bool m_followSystem = true;
	// The named scheme shown whenever m_followSystem is off.
	ThemePalette::Scheme m_scheme = ThemePalette::Scheme::Grass;

	std::unique_ptr<KirigamiColors> m_kirigami;
	bool m_systemDark = false;
	ThemePalette m_systemPalette;

	bool m_dark = true;
	ThemePalette m_palette;
};
