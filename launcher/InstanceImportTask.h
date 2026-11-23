/* Copyright 2013-2021 MultiMC Contributors
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

#include "InstanceTask.h"
#include "net/NetJob.h"
#include <QUrl>
#include <QFuture>
#include <atomic>
#include <QFutureWatcher>
#include "settings/SettingsObject.h"
#include "QObjectPtr.h"

#include <nonstd/optional>

class QuaZip;
class QDir;
namespace Flame
{
    class FileResolvingTask;
    struct Manifest;
}

class InstanceImportTask : public InstanceTask
{
    Q_OBJECT
public:
    struct PackSourceHint {
        QString provider;
        QString packId;
        QString packSlug;
        QString packName;
        QString versionId;
        QString versionLabel;
        QString iconUrl;
        QString sourceUrl;
    };
    struct UpdateTarget {
        QString instanceId;
        QString versionId;
        QString versionLabel;
    };

    explicit InstanceImportTask(const QUrl sourceUrl);
    void setBrowserDownload(int projectId, int fileId, const QString& fileName)
    {
        m_browserProject = projectId;
        m_browserFile = fileId;
        m_browserFileName = fileName;
    }
    void setTrustedSource(bool trusted) { m_trustedSource = trusted; }
    void setPackSourceHint(const PackSourceHint& hint) { m_packHint = hint; }
    void setUpdateTarget(const UpdateTarget& target) {
        m_overrideInstance = target.instanceId;
        m_packHint.versionId = target.versionId;
        m_packHint.versionLabel = target.versionLabel;
    }
    ~InstanceImportTask() override;

    // the downloads can stop: the modpack's and the mods some formats download afterwards; the
    // extraction can't (GH-5177)
    bool canAbort() const override { return true; }
    bool abort() override;

protected:
    //! Entry point for tasks.
    virtual void executeTask() override;

private:
    void recordPackSource(SettingsObject& settings, const QString& provider,
                          const QString& name, const QString& version);
    void finishPackImport();
    bool confirmPackSources(const QDir& gameDir, QStringList externalFiles);
    void processZipPack();
    void processMultiMC();
    void processTechnic();
    void processFlame();
    void configureFlameInstance(Flame::Manifest& pack);
    void downloadFlameFiles();
    void processModrinth();

private slots:
    void downloadSucceeded();
    void downloadFailed(QString reason);
    void downloadProgressChanged(qint64 current, qint64 total);
    void extractFinished();
    void extractAborted();

private: /* data */
    NetJob::Ptr m_filesNetJob;
    shared_qobject_ptr<Flame::FileResolvingTask> m_modIdResolver;
    PackSourceHint m_packHint;
    bool m_trustedSource = false;
    int m_browserProject = 0;
    int m_browserFile = 0;
    QString m_browserFileName;
    QUrl m_sourceUrl;
    QString m_archivePath;
    bool m_downloadRequired = false;
    // a download failed because the user stopped it
    bool m_stopped = false;
    std::atomic<bool> m_cancelExtraction{false};
    std::unique_ptr<QuaZip> m_packZip;
    QFuture<nonstd::optional<QStringList>> m_extractFuture;
    QFutureWatcher<nonstd::optional<QStringList>> m_extractFutureWatcher;
    enum class ModpackType{
        Unknown,
        MultiMC,
        Technic,
        Flame,
        Modrinth,
    } m_modpackType = ModpackType::Unknown;
};
