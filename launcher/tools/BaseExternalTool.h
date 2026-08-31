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
