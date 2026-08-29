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

#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

/* Extra data roles the project list models expose for the delegate below.
 *
 * Qt::UserRole itself is already taken: both search models return the
 * platform's project id there, and existing code reads it. These start
 * one past it.
 *
 * Qt::DisplayRole is deliberately not reused for the title. The default
 * delegate paints DisplayRole, so leaving it as the plain name keeps the
 * list readable if a view is ever shown without this delegate attached. */
namespace ProjectItemRole
{
	enum Role {
		/* QString - project name, drawn large on the first line. */
		Title = Qt::UserRole + 1,
		/* QString - short summary, wrapped over at most two lines. */
		Description,
		/* bool - already present in the target folder. Such rows are
		 * dimmed and tagged, because installing them again is a no-op. */
		Installed,
	};
}

/* Draws one search result: optional checkbox, icon, title, description.
 *
 * Rows are sized by the view's iconSize rather than by this delegate, so
 * a view using it should set an icon size big enough for two lines of
 * description (48x48 is what the download dialog uses). The description
 * silently drops to a single elided line when the row is too short for
 * two, so a smaller icon size degrades gracefully instead of overflowing.
 *
 * The checkbox is painted only when the model supplies
 * Qt::CheckStateRole. Clicks inside it are swallowed and reported via
 * checkboxClicked() rather than going through setData(), so toggling a
 * row never disturbs the current selection or triggers the view's
 * double-click handler. */
class ProjectItemDelegate final : public QStyledItemDelegate
{
	Q_OBJECT

  public:
	explicit ProjectItemDelegate(QWidget* parent = nullptr);

	void paint(QPainter* painter, const QStyleOptionViewItem& option,
			   const QModelIndex& index) const override;

	bool editorEvent(QEvent* event, QAbstractItemModel* model,
					 const QStyleOptionViewItem& option,
					 const QModelIndex& index) override;

  signals:
	void checkboxClicked(const QModelIndex& index);

  private:
	/* Where the checkbox goes: hard left, vertically centred. */
	QStyleOptionViewItem checkboxOption(const QStyleOptionViewItem& option,
										const QStyle* style) const;
};
