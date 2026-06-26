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
 */

#pragma once

#include <QDialog>

class QLabel;
class QTableWidget;
class QPushButton;
class QDialogButtonBox;

/**
 * \brief Shows the runtime feature flags known to the launcher.
 *
 * Lists every flag from the loaded Unleash document with its toggle state and
 * its effective (per-installation) state, the endpoint it came from, and when
 * it was last refreshed. A Refresh button re-fetches the document.
 */
class FeatureFlagsDialog : public QDialog
{
	Q_OBJECT
  public:
	explicit FeatureFlagsDialog(QWidget* parent = nullptr);
	~FeatureFlagsDialog() override = default;

  private slots:
	void reloadTable();
	void requestRefresh();

  private:
	QLabel* m_statusLabel = nullptr;
	QLabel* m_endpointLabel = nullptr;
	QTableWidget* m_table = nullptr;
	QPushButton* m_refreshButton = nullptr;
	QDialogButtonBox* m_buttons = nullptr;
};
