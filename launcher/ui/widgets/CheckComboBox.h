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

#include <QComboBox>
#include <QString>
#include <QStringList>

/* A combo box whose entries are ticked rather than picked.
 *
 * Needed for filters where several values make sense at once - the
 * Minecraft version filter lets a search cover 1.20.1 and 1.20.4
 * together - without giving a whole column of the panel to a list of
 * checkboxes as long as the game's version history.
 *
 * It behaves like the reference launcher's equivalent: the popup stays
 * open while entries are ticked, the closed box shows the ticked
 * entries joined by a separator, or a stand-in text when none are, and
 * the wheel does not change the selection.
 *
 * Entries are plain strings supplied by the owner rather than a model
 * this box adapts. The reference launcher wraps a live model in a proxy
 * that adds check state, because its version list is a model chain; the
 * one thing this is used for here is a list of version names, and
 * setItems() keeps whatever was ticked when the list is refilled, which
 * is what a list arriving from the network needs. */
class CheckComboBox final : public QComboBox
{
	Q_OBJECT

  public:
	explicit CheckComboBox(QWidget* parent = nullptr);

	/* Shown when nothing is ticked, in place of the joined entries. */
	QString defaultText() const
	{
		return m_defaultText;
	}
	void setDefaultText(const QString& text);

	/* Placed between ticked entries in the closed box. Defaults to
	 * ", ". */
	QString separator() const
	{
		return m_separator;
	}
	void setSeparator(const QString& separator);

	/* Replaces the entries, keeping the ticks on any that are still
	 * there. Entries the user had ticked and that are gone from the new
	 * list are dropped, which is the honest answer: the filter can no
	 * longer express them. */
	void setItems(const QStringList& items);

	QStringList checkedItems() const;
	/* Ticks exactly these, unticking everything else. Names that are
	 * not among the entries are ignored. */
	void setCheckedItems(const QStringList& items);

  signals:
	/* Emitted once per tick, and by setItems() when refilling changed
	 * which entries are ticked. */
	void checkedItemsChanged(const QStringList& items);

  protected:
	/* The closed box shows the ticked entries, not the current one. */
	void paintEvent(QPaintEvent* event) override;
	/* Kept open while entries are being ticked. */
	void hidePopup() override;
	bool eventFilter(QObject* watched, QEvent* event) override;

  private:
	void toggleItem(int index);

  private:
	QString m_defaultText;
	QString m_separator;
	/* Set while a press is landing on an entry of the open popup, so
	 * that the click that ticks it does not also close the popup. */
	bool m_pressOnItem = false;
};
