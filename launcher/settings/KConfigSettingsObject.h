/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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
#include <memory>

#include "settings/SettingsObject.h"

class KCoreConfigSkeleton;
#include <KSharedConfig>

/*!
 * \brief A settings object that stores its settings in a KConfig file.
 *
 * This is what meshmc.cfg and every instance.cfg are. The file is a plain
 * KConfig INI file with the settings at the top level, outside of any
 * [section], and every value is stored as a string (the value is written
 * through QVariant::toString(), as it always was).
 *
 * Settings are looked up by their string ID, which is what the rest of the
 * program, and the plugins, do. For the settings dialog the same settings are
 * also available as a KConfigSkeleton, see skeleton(): that is what lets
 * KConfigDialog and KConfigDialogManager bind a widget named "kcfg_<ID>" to
 * a setting, with Apply, Defaults and the "modified" state handled by KDE
 * Frameworks.
 *
 * Details that differ from what older versions wrote, and are handled here:
 *  - Older versions escaped '#' in values as "\#". KConfig does not know that
 *    escape, so a file that still contains it is rewritten once when it is
 *    opened (the original is kept as "<file>.pre-kconfig").
 *  - Prism Launcher keeps the settings of an instance in a [General] section.
 *    Those are moved to the top level when the file is opened, so such an
 *    instance.cfg can be imported.
 *
 * The path is made absolute right away. KConfig would otherwise take a
 * relative file name to be relative to the user's configuration directory
 * and not to the working directory.
 *
 * Two objects for the same file share one KConfig, and so what has been
 * written but not saved yet.
 */
class KConfigSettingsObject : public SettingsObject
{
	Q_OBJECT
  public:
	explicit KConfigSettingsObject(const QString& path,
								   QObject* parent = nullptr);
	~KConfigSettingsObject() override;

	/*!
	 * \brief Gets the absolute path to the config file.
	 */
	virtual QString filePath() const
	{
		return m_filePath;
	}

	bool reload() override;

	void suspendSave() override;
	void resumeSave() override;

	/*!
	 * \brief The registered settings as a KConfigSkeleton.
	 *
	 * Every plain setting becomes an item named after its ID, of the type of
	 * its default value (bool, integer, floating point, or text for anything
	 * else). Settings that are only an override of, or a passthrough to,
	 * another setting have no item of their own.
	 *
	 * Built on the first call, from the settings registered up to then, and
	 * loaded from the file. Saving the skeleton writes the file and is
	 * reported through SettingChanged() for every setting that changed, the
	 * same way a set() is.
	 *
	 * Owned by this object.
	 */
	KCoreConfigSkeleton* skeleton();

	/*!
	 * \brief Use @p skeleton, which kconfig_compiler made, as the skeleton().
	 *
	 * Its items must be named after the settings' IDs (that is what the
	 * generated class does) and it must have been made with config(). It
	 * replaces the one that skeleton() would build from the registered
	 * settings, so that the typed accessors of the generated class and the
	 * settings dialog see the same values, and a set() is seen by both.
	 * Call it once, after the settings are registered and before skeleton().
	 *
	 * The settings object owns it.
	 */
	void adoptSkeleton(std::unique_ptr<KCoreConfigSkeleton> skeleton);

	/*!
	 * \brief The KConfig the settings are in, for a skeleton to be made with.
	 */
	KSharedConfig::Ptr config() const;

  protected slots:
	virtual void changeSetting(const Setting& setting, QVariant value) override;
	virtual void resetSetting(const Setting& setting) override;

  protected:
	virtual QVariant retrieveValue(const Setting& setting) override;
	void doSave();

  private:
	void open();
	void save();
	void removeKey(const QString& key);
	void skeletonSaved();
	void refreshItem(const QString& id);

  private:
	struct Private;
	std::unique_ptr<Private> d;
	QString m_filePath;
};
