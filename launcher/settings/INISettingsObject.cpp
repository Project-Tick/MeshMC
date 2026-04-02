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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "INISettingsObject.h"
#include "Setting.h"

INISettingsObject::INISettingsObject(const QString& path, QObject* parent)
	: SettingsObject(parent)
{
	m_filePath = path;
	m_ini.loadFile(path);
}

void INISettingsObject::setFilePath(const QString& filePath)
{
	m_filePath = filePath;
}

bool INISettingsObject::reload()
{
	return m_ini.loadFile(m_filePath) && SettingsObject::reload();
}

void INISettingsObject::suspendSave()
{
	m_suspendSave = true;
}

void INISettingsObject::resumeSave()
{
	m_suspendSave = false;
	if (m_doSave) {
		m_ini.saveFile(m_filePath);
	}
}

void INISettingsObject::changeSetting(const Setting& setting, QVariant value)
{
	if (contains(setting.id())) {
		// valid value -> set the main config, remove all the sysnonyms
		if (value.isValid()) {
			auto list = setting.configKeys();
			m_ini.set(list.takeFirst(), value);
			for (auto iter : list)
				m_ini.remove(iter);
		}
		// invalid -> remove all (just like resetSetting)
		else {
			for (auto iter : setting.configKeys())
				m_ini.remove(iter);
		}
		doSave();
	}
}

void INISettingsObject::doSave()
{
	if (m_suspendSave) {
		m_doSave = true;
	} else {
		m_ini.saveFile(m_filePath);
	}
}

void INISettingsObject::resetSetting(const Setting& setting)
{
	// if we have the setting, remove all the synonyms. ALL OF THEM
	if (contains(setting.id())) {
		for (auto iter : setting.configKeys())
			m_ini.remove(iter);
		doSave();
	}
}

QVariant INISettingsObject::retrieveValue(const Setting& setting)
{
	// if we have the setting, return value of the first matching synonym
	if (contains(setting.id())) {
		for (auto iter : setting.configKeys()) {
			if (m_ini.contains(iter))
				return m_ini[iter];
		}
	}
	return QVariant();
}
