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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 */// Licensed under the Apache-2.0 license. See README.md for details.

#include "ProgressWidget.h"
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QEventLoop>

#include "tasks/Task.h"

ProgressWidget::ProgressWidget(QWidget* parent) : QWidget(parent)
{
	m_label = new QLabel(this);
	m_label->setWordWrap(true);
	m_bar = new QProgressBar(this);
	m_bar->setMinimum(0);
	m_bar->setMaximum(100);
	QVBoxLayout* layout = new QVBoxLayout(this);
	/* No margins of its own: this is often squeezed into a narrow strip
	 * between other widgets. */
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_label);
	layout->addWidget(m_bar);
	layout->addStretch();
	setLayout(layout);
}

void ProgressWidget::hideIfInactive(bool hide)
{
	m_hideIfInactive = hide;
	updateVisibility();
}

void ProgressWidget::progressFormat(const QString& format)
{
	m_progressFormat = format;
	if (format.isEmpty()) {
		/* No "45%" on top of the bar. Wanted where the bar is only a
		 * hint that something is happening. */
		m_bar->setTextVisible(false);
	} else {
		m_bar->setFormat(format);
		m_bar->setTextVisible(true);
	}
}

void ProgressWidget::watch(Task* task)
{
	if (m_watched) {
		disconnect(m_watched.data(), nullptr, this, nullptr);
	}

	m_watched = task;
	if (!m_watched) {
		updateVisibility();
		return;
	}

	connect(m_watched.data(), &Task::finished, this,
			&ProgressWidget::handleTaskFinish);
	connect(m_watched.data(), &Task::status, this,
			&ProgressWidget::handleTaskStatus);
	connect(m_watched.data(), &Task::progress, this,
			&ProgressWidget::handleTaskProgress);
	connect(m_watched.data(), &Task::destroyed, this,
			&ProgressWidget::taskDestroyed);

	/* Reset, or the bar would briefly show the previous task's tail. */
	m_bar->setMaximum(100);
	m_bar->setValue(0);
	updateVisibility();
}

void ProgressWidget::start(std::shared_ptr<Task> task)
{
	m_task = task;
	watch(m_task.get());
	if (m_task && !m_task->isRunning()) {
		QMetaObject::invokeMethod(m_task.get(), "start", Qt::QueuedConnection);
	}
}

void ProgressWidget::updateVisibility()
{
	if (!m_hideIfInactive) {
		show();
		return;
	}
	setVisible(m_watched && m_watched->isRunning());
}

bool ProgressWidget::exec(std::shared_ptr<Task> task)
{
	QEventLoop loop;
	connect(task.get(), &Task::finished, &loop, &QEventLoop::quit);
	start(task);
	if (task->isRunning()) {
		loop.exec();
	}
	return task->wasSuccessful();
}

void ProgressWidget::handleTaskFinish()
{
	if (m_watched && !m_watched->wasSuccessful()) {
		m_label->setText(m_watched->failReason());
	}
	updateVisibility();
}
void ProgressWidget::handleTaskStatus(const QString& status)
{
	m_label->setText(status);
}
void ProgressWidget::handleTaskProgress(qint64 current, qint64 total)
{
	m_bar->setMaximum(total);
	m_bar->setValue(current);
	updateVisibility();
}
void ProgressWidget::taskDestroyed()
{
	m_watched = nullptr;
	m_task = nullptr;
	updateVisibility();
}
