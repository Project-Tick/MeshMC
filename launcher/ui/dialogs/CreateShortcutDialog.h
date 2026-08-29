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
#include <QString>

class MinecraftInstance;

namespace Ui
{
	class CreateShortcutDialog;
}

/**
 * Asks what a shortcut to one instance should look like, then hands the
 * answer to ShortcutUtils.
 *
 * Nothing is written while the dialog is open: the caller runs exec() and,
 * if that comes back accepted, calls createShortcut().
 */
class CreateShortcutDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit CreateShortcutDialog(MinecraftInstance* instance,
								  QWidget* parent = nullptr);
	~CreateShortcutDialog() override;

	/// Write the shortcut the user just described.
	void createShortcut();

  private slots:
	void on_iconButton_clicked();
	void on_overrideAccountCheckbox_toggled(bool checked);
	void on_targetCheckbox_toggled(bool checked);
	void on_worldTarget_toggled(bool checked);
	void on_serverTarget_toggled(bool checked);
	void on_worldSelectionBox_currentIndexChanged(int);
	void on_serverAddressBox_textChanged(const QString&);

  private:
	/** Rebuilds the suggested name and decides whether the chosen
	 *  target is complete enough to accept. */
	void refresh();

	Ui::CreateShortcutDialog* ui;
	MinecraftInstance* m_instance;
	QString m_iconKey;

	/** Whether this instance's Minecraft can be told to open a world
	 *  straight from the command line. Without it the target section is
	 *  server-only. */
	bool m_canJoinWorld = false;
};
