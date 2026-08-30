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

#include <QDialog>
#include <QStringList>

namespace Ui
{
	class UntrustedModsDialog;
}

/* Consent prompt for installing code the launcher cannot vouch for.
 *
 * A modpack may name mod downloads on any host it likes, and it may
 * simply carry jars inside the archive. Either way the game will load
 * them as code, with the user's permissions. When the pack did not come
 * from one of the catalogue browsers, that is a decision only the user
 * can make, so it is put to them.
 *
 * This is a dialog of its own rather than a message box with the list
 * hidden behind "Show Details" for one reason: the list is the
 * information. A prompt whose evidence is one click away, and whose
 * accept button can be hit before the text has been read, measures
 * whether the user can dismiss a dialog rather than whether they
 * consent. So the files are visible, and accepting takes a deliberate
 * second action - ticking a box that does not even become available for
 * the first few seconds.
 */
class UntrustedModsDialog : public QDialog
{
	Q_OBJECT
  public:
	/* @p paths are the files in question, as instance-relative paths. */
	explicit UntrustedModsDialog(const QStringList& paths,
								 QWidget* parent = nullptr);
	~UntrustedModsDialog() override;

  private:
	Ui::UntrustedModsDialog* m_ui;
};
