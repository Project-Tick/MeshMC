/* Copyright 2013-2021 MultiMC Contributors
 * Copyright 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "BaseInstance.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include "settings/INISettingsObject.h"
#include "settings/Setting.h"
#include "settings/OverrideSetting.h"

#include "FileSystem.h"
#include "Commandline.h"
#include "BuildConfig.h"

BaseInstance::BaseInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
    : QObject()
{
    m_settings = settings;
    m_rootDir = rootDir;

    m_settings->registerSetting("PackProvider", "");
    m_settings->registerSetting("PackId", "");
    m_settings->registerSetting("PackSlug", "");
    m_settings->registerSetting("PackVersionId", "");
    m_settings->registerSetting("PackVersionLabel", "");
    m_settings->registerSetting("PackIconUrl", "");
    m_settings->registerSetting("PackSourceUrl", "");
    m_settings->registerSetting("PackInstalledAt", "");
    m_settings->registerSetting("PackManifestSha512", "");
    m_settings->registerSetting("PackName", "");
    m_settings->registerSetting("PackUpdateUrl", "");
    m_settings->registerSetting("name", "Unnamed Instance");
    m_settings->registerSetting("iconKey", "default");
    m_settings->registerSetting("notes", "");
    m_settings->registerSetting("lastLaunchTime", 0);
    m_settings->registerSetting("totalTimePlayed", 0);
    m_settings->registerSetting("lastTimePlayed", 0);

    // Custom Commands
    auto commandSetting = m_settings->registerSetting({"OverrideCommands","OverrideLaunchCmd"}, false);
    m_settings->registerOverride(globalSettings->getSetting("PreLaunchCommand"), commandSetting);
    m_settings->registerOverride(globalSettings->getSetting("WrapperCommand"), commandSetting);
    m_settings->registerOverride(globalSettings->getSetting("PostExitCommand"), commandSetting);

    // Console
    auto consoleSetting = m_settings->registerSetting("OverrideConsole", false);
    m_settings->registerOverride(globalSettings->getSetting("ShowConsole"), consoleSetting);
    m_settings->registerOverride(globalSettings->getSetting("AutoCloseConsole"), consoleSetting);
    m_settings->registerOverride(globalSettings->getSetting("ShowConsoleOnError"), consoleSetting);
    m_settings->registerOverride(globalSettings->getSetting("LogPrePostOutput"), consoleSetting);

    m_settings->registerPassthrough(globalSettings->getSetting("ConsoleMaxLines"), nullptr);
    m_settings->registerPassthrough(globalSettings->getSetting("ConsoleOverflowStop"), nullptr);

    // Managed Packs
    m_settings->registerSetting("ManagedPack", false);
    m_settings->registerSetting("ManagedPackType", "");
    m_settings->registerSetting("ManagedPackID", "");
    m_settings->registerSetting("ManagedPackName", "");
    m_settings->registerSetting("ManagedPackVersionID", "");
    m_settings->registerSetting("ManagedPackVersionName", "");
}

QString BaseInstance::getPreLaunchCommand()
{
    return settings()->get("PreLaunchCommand").toString();
}

QString BaseInstance::getWrapperCommand()
{
    return settings()->get("WrapperCommand").toString();
}

QString BaseInstance::getPostExitCommand()
{
    return settings()->get("PostExitCommand").toString();
}

bool BaseInstance::isManagedPack()
{
    return settings()->get("ManagedPack").toBool();
}

QString BaseInstance::getManagedPackType()
{
    return settings()->get("ManagedPackType").toString();
}

QString BaseInstance::getManagedPackID()
{
    return settings()->get("ManagedPackID").toString();
}

QString BaseInstance::getManagedPackName()
{
    return settings()->get("ManagedPackName").toString();
}

QString BaseInstance::getManagedPackVersionID()
{
    return settings()->get("ManagedPackVersionID").toString();
}

QString BaseInstance::getManagedPackVersionName()
{
    return settings()->get("ManagedPackVersionName").toString();
}

void BaseInstance::setManagedPack(const QString& type, const QString& id, const QString& name, const QString& versionId, const QString& version)
{
    settings()->set("ManagedPack", true);
    settings()->set("ManagedPackType", type);
    settings()->set("ManagedPackID", id);
    settings()->set("ManagedPackName", name);
    settings()->set("ManagedPackVersionID", versionId);
    settings()->set("ManagedPackVersionName", version);
}

int BaseInstance::getConsoleMaxLines() const
{
    auto lineSetting = settings()->getSetting("ConsoleMaxLines");
    bool conversionOk = false;
    int maxLines = lineSetting->get().toInt(&conversionOk);
    if(!conversionOk)
    {
        maxLines = lineSetting->defValue().toInt();
        qWarning() << "ConsoleMaxLines has nonsensical value, defaulting to" << maxLines;
    }
    return maxLines;
}

bool BaseInstance::shouldStopOnConsoleOverflow() const
{
    return settings()->get("ConsoleOverflowStop").toBool();
}

void BaseInstance::iconUpdated(QString key)
{
    if(iconKey() == key)
    {
        emit propertiesChanged(this);
    }
}

void BaseInstance::invalidate()
{
    changeStatus(Status::Gone);
    qDebug() << "Instance" << id() << "has been invalidated.";
}

void BaseInstance::changeStatus(BaseInstance::Status newStatus)
{
    Status status = currentStatus();
    if(status != newStatus)
    {
        m_status = newStatus;
        emit statusChanged(status, newStatus);
    }
}

BaseInstance::Status BaseInstance::currentStatus() const
{
    return m_status;
}

QString BaseInstance::id() const
{
    return QFileInfo(instanceRoot()).fileName();
}

void BaseInstance::setApplyingPackUpdate(bool applying)
{
    if (m_applyingPackUpdate == applying)
        return;
    m_applyingPackUpdate = applying;
    emit propertiesChanged(this);
}

bool BaseInstance::isRunning() const
{
    return m_isRunning;
}

void BaseInstance::setRunning(bool running)
{
    if(running == m_isRunning)
        return;

    m_isRunning = running;

    if(!m_settings->get("RecordGameTime").toBool())
    {
        emit runningStatusChanged(running);
        return;
    }

    if(running)
    {
        m_timeStarted.start();
        m_countingTime = true;
    }
    else
    {
        recordTimePlayed();
    }

    emit runningStatusChanged(running);
}

void BaseInstance::recordTimePlayed()
{
    if(!m_countingTime)
        return;

    settings()->set("totalTimePlayed", qint64(totalTimePlayed()));
    settings()->set("lastTimePlayed", qint64(lastTimePlayed()));
    m_countingTime = false;

    emit propertiesChanged(this);
}

int64_t BaseInstance::totalTimePlayed() const
{
    // clock changes used to be able to save a negative total
    qint64 current = qMax<qint64>(0, settings()->get("totalTimePlayed").toLongLong());
    if(m_countingTime)
    {
        return current + m_timeStarted.elapsed() / 1000;
    }
    return current;
}

int64_t BaseInstance::lastTimePlayed() const
{
    if(m_countingTime)
    {
        return m_timeStarted.elapsed() / 1000;
    }
    return settings()->get("lastTimePlayed").toLongLong();
}

void BaseInstance::resetTimePlayed()
{
    settings()->reset("totalTimePlayed");
    settings()->reset("lastTimePlayed");
}

QString BaseInstance::instanceType() const
{
    return m_settings->get("InstanceType").toString();
}

QString BaseInstance::instanceRoot() const
{
    return m_rootDir;
}

SettingsObjectPtr BaseInstance::settings() const
{
    return m_settings;
}

bool BaseInstance::canLaunch() const
{
    return !hasVersionBroken() && !isRunning() && !isApplyingPackUpdate();
}

bool BaseInstance::reloadSettings()
{
    return m_settings->reload();
}

qint64 BaseInstance::lastLaunch() const
{
    return m_settings->get("lastLaunchTime").value<qint64>();
}

void BaseInstance::setLastLaunch(qint64 val)
{
    //FIXME: if no change, do not set. setting involves saving a file.
    m_settings->set("lastLaunchTime", val);
    emit propertiesChanged(this);
}

void BaseInstance::setNotes(QString val)
{
    //FIXME: if no change, do not set. setting involves saving a file.
    m_settings->set("notes", val);
}

QString BaseInstance::notes() const
{
    return m_settings->get("notes").toString();
}

void BaseInstance::setIconKey(QString val)
{
    //FIXME: if no change, do not set. setting involves saving a file.
    m_settings->set("iconKey", val);
    emit propertiesChanged(this);
}

QString BaseInstance::iconKey() const
{
    return m_settings->get("iconKey").toString();
}

void BaseInstance::setName(QString val)
{
    //FIXME: if no change, do not set. setting involves saving a file.
    m_settings->set("name", val);
    emit propertiesChanged(this);
}

QString BaseInstance::name() const
{
    return m_settings->get("name").toString();
}

QString BaseInstance::windowTitle() const
{
    return BuildConfig.LAUNCHER_NAME + ": " + name().replace(QRegExp("[ \n\r\t]+"), " ");
}

QString BaseInstance::instanceTitle() const
{
    return name().replace(QRegExp("[ \n\r\t]+"), " ");
}

// FIXME: why is this here? move it to MinecraftInstance!!!
QStringList BaseInstance::extraArguments() const
{
    return Commandline::splitArgs(settings()->get("JvmArgs").toString());
}

shared_qobject_ptr<LaunchTask> BaseInstance::getLaunchTask()
{
    return m_launchProcess;
}

QString BaseInstance::managedPackProvider() const
{
	return m_settings->get("PackProvider").toString().trimmed();
}

QString BaseInstance::managedPackId() const
{
	return m_settings->get("PackId").toString().trimmed();
}

QString BaseInstance::managedPackSlug() const
{
	return m_settings->get("PackSlug").toString().trimmed();
}

QString BaseInstance::managedPackName() const
{
	const QString recorded = m_settings->get("PackName").toString().trimmed();
	if (!recorded.isEmpty()) {
		return recorded;
	}
	/* Older imports never recorded the title. The slug is the next best
	 * thing - it is derived from the title and is at least stable - and
	 * failing that the instance name, which the user chose and will
	 * recognise even if it is not what the catalogue calls the pack. */
	const QString slug = managedPackSlug();
	if (!slug.isEmpty()) {
		return slug;
	}
	return name();
}

QString BaseInstance::managedPackVersionId() const
{
	return m_settings->get("PackVersionId").toString().trimmed();
}

QString BaseInstance::managedPackVersionName() const
{
	return m_settings->get("PackVersionLabel").toString().trimmed();
}

QString BaseInstance::managedPackSourceUrl() const
{
	return m_settings->get("PackSourceUrl").toString().trimmed();
}

bool BaseInstance::isManagedPack() const
{
	return !managedPackProvider().isEmpty();
}

bool BaseInstance::hasManagedPackId() const
{
	/* An id without a provider does not say which API to ask, and a
	 * provider without an id gives every endpoint nothing to key on, so
	 * a catalogue lookup needs both. */
	return !managedPackProvider().isEmpty() && !managedPackId().isEmpty();
}

QString BaseInstance::managedPackUpdateUrl() const
{
	return m_settings->get("PackUpdateUrl").toString().trimmed();
}

void BaseInstance::setManagedPackUpdateUrl(const QString& url)
{
	const QString trimmed = url.trimmed();
	if (managedPackUpdateUrl() == trimmed) {
		// Writing a setting writes a file; this one is fed by a
		// textChanged signal, so it would otherwise write on every
		// keystroke that does not change the trimmed value.
		return;
	}
	m_settings->set("PackUpdateUrl", trimmed);
}
