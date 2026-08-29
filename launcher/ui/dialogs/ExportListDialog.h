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
#include <QList>
#include <QString>

#include "modplatform/ContentListExport.h"

class QCheckBox;
class QComboBox;
class QGroupBox;
class QPlainTextEdit;
class QPushButton;
class QTextBrowser;
class QTextEdit;

/*
 * Turns what is in a content folder into a list to paste or save.
 *
 * The built-in formats cover the places such a list usually ends up - a
 * forum post, a README, a spreadsheet - and each of them can be taken as
 * the starting point for a line template when none of them is quite
 * right. The rendered text is always shown, and for the two marked-up
 * formats it is also shown as it will look.
 *
 * The dialog is built here rather than in a .ui file, which is how the
 * other dialogs added with the download screen are put together.
 */
class ExportListDialog final : public QDialog
{
	Q_OBJECT

  public:
	/* `name` names what is being exported and is offered as the file
	 * name; it usually is the instance's name. */
	ExportListDialog(QString name, QList<ContentListExport::Item> items,
					 QWidget* parent = nullptr);

	void done(int result) override;

  private:
	void buildUi();
	void formatChanged(int index);

	/* Re-renders from the current format and options. Cheap enough to
	 * run on every keystroke in the template box. */
	void regenerate();

	void insertPlaceholder(ContentListExport::Field field);

	/* Custom mode swaps the option checkboxes for buttons that insert
	 * the matching placeholder, since a template says for itself which
	 * fields it wants and in what order. */
	void setCustomMode(bool custom);

	ContentListExport::Fields selectedFields() const;

	QList<ContentListExport::Item> m_items;
	QString m_name;
	ContentListExport::Format m_format = ContentListExport::Format::Html;

	/* Set once the user has changed the template themselves, after which
	 * switching format no longer overwrites what they wrote. */
	bool m_templateEdited = false;

	QComboBox* m_formatBox = nullptr;
	QGroupBox* m_templateGroup = nullptr;
	QTextEdit* m_templateText = nullptr;
	QCheckBox* m_versionCheck = nullptr;
	QCheckBox* m_authorsCheck = nullptr;
	QCheckBox* m_urlCheck = nullptr;
	QCheckBox* m_fileNameCheck = nullptr;
	QPushButton* m_versionButton = nullptr;
	QPushButton* m_authorsButton = nullptr;
	QPushButton* m_urlButton = nullptr;
	QPushButton* m_fileNameButton = nullptr;
	QPlainTextEdit* m_finalText = nullptr;
	QTextBrowser* m_resultText = nullptr;
};
