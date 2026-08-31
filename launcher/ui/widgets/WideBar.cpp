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

  public slots:
	void actionChanged()
	{
		setEnabled(m_action->isEnabled());
		setChecked(m_action->isChecked());
		setCheckable(m_action->isCheckable());
		setText(m_action->text());
		setIcon(m_action->icon());
		setToolTip(m_action->toolTip());
		setHidden(!m_action->isVisible());

		/* An action that carries a menu gets a button split in two: the
		 * near side does what the action does, the arrow opens the menu.
		 * Without this the menu is unreachable - the button knows
		 * nothing about it and a plain click just triggers the action,
		 * which is how a submenu full of entries can end up invisible. */
		if (menu() != m_action->menu()) {
			setMenu(m_action->menu());
		}
		setPopupMode(m_action->menu() ? QToolButton::MenuButtonPopup
									  : QToolButton::DelayedPopup);

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

void WideBar::refreshActions()
{
	for (auto* entry : m_entries) {
		if (entry->type != BarEntry::Action || !entry->qAction) {
			continue;
		}
		if (auto* button =
				qobject_cast<ActionButton*>(widgetForAction(entry->qAction))) {
			button->actionChanged();
		}
	}
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
