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

#include "WideBar.h"
#include <QToolButton>
#include <QMenu>

class ActionButton : public QToolButton
{
	Q_OBJECT
  public:
	ActionButton(QAction* action, QWidget* parent = 0)
		: QToolButton(parent), m_action(action)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		connect(action, &QAction::changed, this, &ActionButton::actionChanged);
		connect(this, &ActionButton::clicked, action, &QAction::trigger);
		actionChanged();
	};
  private slots:
	void actionChanged()
	{
		setEnabled(m_action->isEnabled());
		setChecked(m_action->isChecked());
		setCheckable(m_action->isCheckable());
		setText(m_action->text());
		setIcon(m_action->icon());
		setToolTip(m_action->toolTip());
		setHidden(!m_action->isVisible());
		setFocusPolicy(Qt::NoFocus);
	}

  private:
	QAction* m_action;
};

WideBar::WideBar(const QString& title, QWidget* parent)
	: QToolBar(title, parent)
{
	setFloatable(false);
	setMovable(false);
}

WideBar::WideBar(QWidget* parent) : QToolBar(parent)
{
	setFloatable(false);
	setMovable(false);
}

struct WideBar::BarEntry {
	enum Type { None, Action, Separator, Spacer } type = None;
	QAction* qAction = nullptr;
	QAction* wideAction = nullptr;
};

WideBar::~WideBar()
{
	for (auto* iter : m_entries) {
		delete iter;
	}
}

void WideBar::addAction(QAction* action)
{
	auto entry = new BarEntry();
	entry->qAction = addWidget(new ActionButton(action, this));
	entry->wideAction = action;
	entry->type = BarEntry::Action;
	m_entries.push_back(entry);
}

void WideBar::addSeparator()
{
	auto entry = new BarEntry();
	entry->qAction = QToolBar::addSeparator();
	entry->type = BarEntry::Separator;
	m_entries.push_back(entry);
}

void WideBar::insertSpacer(QAction* action)
{
	auto iter = std::find_if(
		m_entries.begin(), m_entries.end(),
		[action](BarEntry* entry) { return entry->wideAction == action; });
	if (iter == m_entries.end()) {
		return;
	}
	QWidget* spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto entry = new BarEntry();
	entry->qAction = insertWidget((*iter)->qAction, spacer);
	entry->type = BarEntry::Spacer;
	m_entries.insert(iter, entry);
}

QMenu* WideBar::createContextMenu(QWidget* parent, const QString& title)
{
	QMenu* contextMenu = new QMenu(title, parent);
	for (auto& item : m_entries) {
		switch (item->type) {
			default:
			case BarEntry::None:
				break;
			case BarEntry::Separator:
			case BarEntry::Spacer:
				contextMenu->addSeparator();
				break;
			case BarEntry::Action:
				contextMenu->addAction(item->wideAction);
				break;
		}
	}
	return contextMenu;
}

#include "WideBar.moc"
