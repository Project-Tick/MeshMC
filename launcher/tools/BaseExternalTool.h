/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QObject>
#include <BaseInstance.h>

class BaseInstance;
class SettingsObject;
class QProcess;

class BaseExternalTool : public QObject
{
	Q_OBJECT
  public:
	explicit BaseExternalTool(SettingsObjectPtr settings, InstancePtr instance,
							  QObject* parent = 0);
	virtual ~BaseExternalTool();

  protected:
	InstancePtr m_instance;
	SettingsObjectPtr globalSettings;
};

class BaseDetachedTool : public BaseExternalTool
{
	Q_OBJECT
  public:
	explicit BaseDetachedTool(SettingsObjectPtr settings, InstancePtr instance,
							  QObject* parent = 0);

  public slots:
	void run();

  protected:
	virtual void runImpl() = 0;
};

class BaseExternalToolFactory
{
  public:
	virtual ~BaseExternalToolFactory();

	virtual QString name() const = 0;

	virtual void registerSettings(SettingsObjectPtr settings) = 0;

	virtual BaseExternalTool* createTool(InstancePtr instance,
										 QObject* parent = 0) = 0;

	virtual bool check(QString* error) = 0;
	virtual bool check(const QString& path, QString* error) = 0;

  protected:
	SettingsObjectPtr globalSettings;
};

class BaseDetachedToolFactory : public BaseExternalToolFactory
{
  public:
	virtual BaseDetachedTool* createDetachedTool(InstancePtr instance,
												 QObject* parent = 0);
};
