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
