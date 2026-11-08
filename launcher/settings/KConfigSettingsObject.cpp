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

#include "KConfigSettingsObject.h"
#include "OverrideSetting.h"
#include "PassthroughSetting.h"
#include "Setting.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSaveFile>

#include <KConfig>
#include <KConfigGroup>
#include <KCoreConfigSkeleton>
#include <KSharedConfig>

#include <deque>

namespace
{
	// The settings live outside of any section; that is the "default group" of
	// a KConfig.
	QString topLevelGroup()
	{
		return QString();
	}

	// Prism Launcher writes the settings of an instance under [General].
	QString foreignGroup()
	{
		return QStringLiteral("General");
	}

	/*!
	 * Older versions wrote '#' in a value as "\#" and treated an unescaped '#'
	 * as the start of a comment. KConfig has no "\#" escape: it would hand the
	 * backslash back as part of the value (and warn about an invalid escape
	 * sequence).
	 *
	 * Rewrites such a file once, turning every "\#" into "#". The scan goes
	 * through the escapes pair by pair, so "\\#" (a backslash followed by a
	 * '#') is left alone, which is also how KConfig itself writes that. A file
	 * KConfig wrote therefore never changes, which makes this safe to run on
	 * every load.
	 *
	 * The original is kept next to the file as "<file>.pre-kconfig", unless
	 * such a backup is already there.
	 *
	 * \return true if the file was rewritten.
	 */
	bool migrateLegacyEscapes(const QString& path)
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			return false;
		}
		const QByteArray in = file.readAll();
		file.close();

		if (!in.contains("\\#")) {
			return false;
		}

		QByteArray out;
		out.reserve(in.size());
		for (int i = 0; i < in.size(); ++i) {
			const char c = in.at(i);
			if (c == '\\' && i + 1 < in.size()) {
				if (in.at(i + 1) == '#') {
					out.append('#');
				} else {
					out.append(c);
					out.append(in.at(i + 1));
				}
				++i;
				continue;
			}
			out.append(c);
		}
		if (out == in) {
			return false;
		}

		const QString backup = path + QStringLiteral(".pre-kconfig");
		if (!QFile::exists(backup) && !QFile::copy(path, backup)) {
			qWarning() << "Could not back up" << path << "to" << backup
					   << "; leaving the file as it is.";
			return false;
		}

		QSaveFile target(path);
		if (!target.open(QIODevice::WriteOnly) ||
			target.write(out) != out.size() || !target.commit()) {
			qWarning() << "Could not rewrite" << path;
			return false;
		}
		qDebug() << "Converted the '#' escapes in" << path;
		return true;
	}

	/*!
	 * A value that is still under an older name moves to the current one; the
	 * skeleton knows only one key per item. The older names are dropped.
	 */
	void moveOlderNames(KConfigGroup& top, const Setting& setting)
	{
		const QStringList keys = setting.configKeys();
		const QString key = keys.first();
		if (!top.hasKey(key)) {
			for (const auto& alias : keys.mid(1)) {
				if (top.hasKey(alias)) {
					top.writeEntry(key, top.readEntry(alias, QString()));
					break;
				}
			}
		}
		for (const auto& alias : keys.mid(1)) {
			top.deleteEntry(alias);
		}
	}

	//! A setting that has a value of its own, and not just a view of another.
	bool isPlain(const Setting& setting)
	{
		return !dynamic_cast<const OverrideSetting*>(&setting) &&
			   !dynamic_cast<const PassthroughSetting*>(&setting);
	}
} // namespace

struct KConfigSettingsObject::Private {
	KSharedConfig::Ptr config;

	// The skeleton's items keep their value in a variable of the caller. These
	// are those variables; a deque, because they must not move.
	std::unique_ptr<KCoreConfigSkeleton> skeleton;
	std::deque<bool> bools;
	std::deque<qint32> ints;
	std::deque<qint64> longs;
	std::deque<double> doubles;
	std::deque<QString> strings;

	// What each setting of the skeleton was the last time anybody looked, to
	// find out what saving the skeleton changed.
	QList<std::shared_ptr<Setting>> plain;
	QHash<QString, QVariant> known;
};

KConfigSettingsObject::KConfigSettingsObject(const QString& path,
											 QObject* parent)
	: SettingsObject(parent), d(new Private)
{
	// KConfig reads a relative name as relative to the user's configuration
	// directory, not the working directory.
	m_filePath = QFileInfo(path).absoluteFilePath();
	open();
}

KConfigSettingsObject::~KConfigSettingsObject() {}

void KConfigSettingsObject::open()
{
	migrateLegacyEscapes(m_filePath);
	// SimpleConfig: this one file, without kdeglobals or any system-wide
	// configuration layered underneath.
	d->config = KSharedConfig::openConfig(m_filePath, KConfig::SimpleConfig);

	// Prism Launcher's layout: move what is in [General] to the top level,
	// unless the top level already has it.
	KConfigGroup foreign = d->config->group(foreignGroup());
	if (foreign.exists()) {
		KConfigGroup top = d->config->group(topLevelGroup());
		for (const QString& key : foreign.keyList()) {
			if (!top.hasKey(key)) {
				top.writeEntry(key, foreign.readEntry(key, QString()));
			}
		}
		foreign.deleteGroup();
	}
}

bool KConfigSettingsObject::reload()
{
	if (!QFileInfo::exists(m_filePath)) {
		return false;
	}
	migrateLegacyEscapes(m_filePath);
	d->config->reparseConfiguration();
	if (d->skeleton) {
		d->skeleton->load();
	}
	return SettingsObject::reload();
}

void KConfigSettingsObject::suspendSave()
{
	m_suspendSave = true;
}

void KConfigSettingsObject::resumeSave()
{
	m_suspendSave = false;
	if (m_doSave) {
		save();
	}
}

void KConfigSettingsObject::removeKey(const QString& key)
{
	d->config->group(topLevelGroup()).deleteEntry(key);
}

void KConfigSettingsObject::changeSetting(const Setting& setting,
										  QVariant value)
{
	if (contains(setting.id())) {
		auto list = setting.configKeys();
		// valid value -> set the main config, remove all the sysnonyms
		if (value.isValid()) {
			const QString mainKey = list.takeFirst();
			d->config->group(topLevelGroup())
				.writeEntry(mainKey, value.toString());
			for (const auto& key : list)
				removeKey(key);
			d->known.insert(setting.id(), QVariant(value.toString()));
		}
		// invalid -> remove all (just like resetSetting)
		else {
			for (const auto& key : list)
				removeKey(key);
			d->known.insert(setting.id(), QVariant());
		}
		refreshItem(setting.id());
		doSave();
	}
}

void KConfigSettingsObject::refreshItem(const QString& id)
{
	// The skeleton keeps what it read in the variables of its items. A set()
	// goes to the file around it, so the item has to read it again, or whoever
	// asks the item (the typed accessors, the settings dialog) gets what the
	// setting was.
	if (!d->skeleton) {
		return;
	}
	if (auto* item = d->skeleton->findItem(id)) {
		item->readConfig(d->config.data());
	}
}

void KConfigSettingsObject::doSave()
{
	if (m_suspendSave) {
		m_doSave = true;
	} else {
		save();
	}
}

void KConfigSettingsObject::save()
{
	// KConfig does not create the folder the file is going to be in.
	QDir().mkpath(QFileInfo(m_filePath).absolutePath());
	if (!d->config->sync()) {
		qWarning() << "Could not write" << m_filePath;
		return;
	}
	m_doSave = false;
}

void KConfigSettingsObject::resetSetting(const Setting& setting)
{
	// if we have the setting, remove all the synonyms. ALL OF THEM
	if (contains(setting.id())) {
		for (const auto& key : setting.configKeys())
			removeKey(key);
		d->known.insert(setting.id(), QVariant());
		refreshItem(setting.id());
		doSave();
	}
}

QVariant KConfigSettingsObject::retrieveValue(const Setting& setting)
{
	// if we have the setting, return value of the first matching synonym
	if (contains(setting.id())) {
		const KConfigGroup group = d->config->group(topLevelGroup());
		for (const auto& key : setting.configKeys()) {
			if (group.hasKey(key)) {
				return QVariant(group.readEntry(key, QString()));
			}
		}
	}
	return QVariant();
}

KCoreConfigSkeleton* KConfigSettingsObject::skeleton()
{
	if (d->skeleton) {
		return d->skeleton.get();
	}

	d->skeleton = std::make_unique<KCoreConfigSkeleton>(d->config);
	KConfigGroup top = d->config->group(topLevelGroup());

	for (const auto& setting : allSettings()) {
		if (!isPlain(*setting)) {
			continue;
		}
		const QString id = setting->id();
		const QString key = setting->configKeys().first();
		moveOlderNames(top, *setting);

		const QVariant def = setting->defValue();
		switch (def.metaType().id()) {
			case QMetaType::Bool: {
				auto& ref = d->bools.emplace_back(def.toBool());
				d->skeleton->addItem(
					new KCoreConfigSkeleton::ItemBool(topLevelGroup(), key, ref,
												  def.toBool()),
					id);
				break;
			}
			case QMetaType::Int:
			case QMetaType::Short:
			case QMetaType::UShort:
			case QMetaType::Char:
			case QMetaType::SChar:
			case QMetaType::UChar: {
				auto& ref = d->ints.emplace_back(def.toInt());
				d->skeleton->addItem(
					new KCoreConfigSkeleton::ItemInt(topLevelGroup(), key, ref,
												 def.toInt()),
					id);
				break;
			}
			case QMetaType::UInt:
			case QMetaType::Long:
			case QMetaType::ULong:
			case QMetaType::LongLong:
			case QMetaType::ULongLong: {
				auto& ref = d->longs.emplace_back(def.toLongLong());
				d->skeleton->addItem(
					new KCoreConfigSkeleton::ItemLongLong(topLevelGroup(), key, ref,
													  def.toLongLong()),
					id);
				break;
			}
			case QMetaType::Double:
			case QMetaType::Float: {
				auto& ref = d->doubles.emplace_back(def.toDouble());
				d->skeleton->addItem(
					new KCoreConfigSkeleton::ItemDouble(topLevelGroup(), key, ref,
													def.toDouble()),
					id);
				break;
			}
			default: {
				auto& ref = d->strings.emplace_back(def.toString());
				d->skeleton->addItem(
					new KCoreConfigSkeleton::ItemString(topLevelGroup(), key, ref,
													def.toString()),
					id);
				break;
			}
		}

		d->plain.append(setting);
		d->known.insert(id, retrieveValue(*setting));
	}

	d->skeleton->load();
	connect(d->skeleton.get(), &KCoreConfigSkeleton::configChanged, this,
			&KConfigSettingsObject::skeletonSaved);
	return d->skeleton.get();
}

void KConfigSettingsObject::adoptSkeleton(
	std::unique_ptr<KCoreConfigSkeleton> skeleton)
{
	if (d->skeleton) {
		qCritical() << "The skeleton of" << m_filePath
					<< "is there already; the one that was given is dropped.";
		return;
	}
	d->skeleton = std::move(skeleton);
	KConfigGroup top = d->config->group(topLevelGroup());

	for (const auto& setting : allSettings()) {
		if (!isPlain(*setting)) {
			continue;
		}
		if (!d->skeleton->findItem(setting->id())) {
			// no item, so the dialog cannot bind it, but a get() still works
			qWarning() << "The skeleton has no item for the setting"
					   << setting->id();
			continue;
		}
		moveOlderNames(top, *setting);
		d->plain.append(setting);
		d->known.insert(setting->id(), retrieveValue(*setting));
	}

	d->skeleton->load();
	connect(d->skeleton.get(), &KCoreConfigSkeleton::configChanged, this,
			&KConfigSettingsObject::skeletonSaved);
}

KSharedConfig::Ptr KConfigSettingsObject::config() const
{
	return d->config;
}

void KConfigSettingsObject::skeletonSaved()
{
	// The skeleton wrote the file. Tell everybody who listens to a setting
	// about the ones whose value is not what it was.
	for (const auto& setting : d->plain) {
		const QVariant now = retrieveValue(*setting);
		if (d->known.value(setting->id()) != now) {
			d->known.insert(setting->id(), now);
			emit setting->SettingChanged(*setting, now);
		}
	}
}
