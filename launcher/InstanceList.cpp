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

#include "QtContainerCompat.h"
#include <QDir>
#include <QDirIterator>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QSet>
#include <QFile>
#include <QThread>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QTimer>
#include <QDebug>
#include <QFileSystemWatcher>
#include <QUuid>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeData>

#include "InstanceList.h"
#include "modplatform/PackContents.h"
#include "minecraft/mod/ModMetadataIndex.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INIFile.h"
#include <QTemporaryDir>
#include <QDirIterator>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include "BaseInstance.h"
#include "InstanceTask.h"
#include "settings/INISettingsObject.h"
#include "minecraft/legacy/LegacyInstance.h"
#include "NullInstance.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "FileSystem.h"
#include "ExponentialSeries.h"
#include "WatchLock.h"

const static int GROUP_FILE_FORMAT_VERSION = 1;

InstanceList::InstanceList(SettingsObjectPtr settings, const QString & instDir, QObject *parent)
    : QAbstractListModel(parent), m_globalSettings(settings)
{
    resumeWatch();
    connect(this, &InstanceList::instancesChanged, this, &InstanceList::providerUpdated);
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &InstanceList::instanceDirContentsChanged);
    refreshInstanceDirs(instDir);
}

InstanceList::~InstanceList()
{
}

Qt::DropActions InstanceList::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions InstanceList::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool InstanceList::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    if(data && data->hasFormat("application/x-instanceid")) {
        return true;
    }
    return false;
}

bool InstanceList::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if(data && data->hasFormat("application/x-instanceid")) {
        return true;
    }
    return false;
}

QStringList InstanceList::mimeTypes() const
{
    auto types = QAbstractListModel::mimeTypes();
    types.push_back("application/x-instanceid");
    return types;
}

QMimeData * InstanceList::mimeData(const QModelIndexList& indexes) const
{
    auto mimeData = QAbstractListModel::mimeData(indexes);
    if(indexes.size() == 1) {
        auto instanceId = data(indexes[0], InstanceIDRole).toString();
        mimeData->setData("application/x-instanceid", instanceId.toUtf8());
    }
    return mimeData;
}


int InstanceList::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_instances.count();
}

QModelIndex InstanceList::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (row < 0 || row >= m_instances.size())
        return QModelIndex();
    return createIndex(row, column, (void *)m_instances.at(row).get());
}

QVariant InstanceList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }
    BaseInstance *pdata = static_cast<BaseInstance *>(index.internalPointer());
    switch (role)
    {
    case InstancePointerRole:
    {
        QVariant v = QVariant::fromValue((void *)pdata);
        return v;
    }
    case InstanceIDRole:
    {
        return pdata->id();
    }
    case Qt::EditRole:
    case Qt::DisplayRole:
    {
        return pdata->name();
    }
    case Qt::AccessibleTextRole:
    {
        return tr("%1 Instance").arg(pdata->name());
    }
    case Qt::ToolTipRole:
    {
        return pdata->instanceRoot();
    }
    case Qt::DecorationRole:
    {
        return pdata->iconKey();
    }
    // HACK: see InstanceView.h in gui!
    case GroupRole:
    {
        return getInstanceGroup(pdata->id());
    }
    default:
        break;
    }
    return QVariant();
}

bool InstanceList::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
    {
        return false;
    }
    if(role != Qt::EditRole)
    {
        return false;
    }
    BaseInstance *pdata = static_cast<BaseInstance *>(index.internalPointer());
    auto newName = value.toString();
    if(pdata->name() == newName)
    {
        return true;
    }
    pdata->setName(newName);
    return true;
}

Qt::ItemFlags InstanceList::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f;
    if (index.isValid())
    {
        f |= (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    }
    return f;
}

GroupId InstanceList::getInstanceGroup(const InstanceId& id) const
{
    auto inst = getInstanceById(id);
    if(!inst)
    {
        return GroupId();
    }
    auto iter = m_instanceGroupIndex.find(inst->id());
    if(iter != m_instanceGroupIndex.end())
    {
        return *iter;
    }
    return GroupId();
}

void InstanceList::setInstanceGroup(const InstanceId& id, const GroupId& name)
{
    auto inst = getInstanceById(id);
    if(!inst)
    {
        qDebug() << "Attempt to set a null instance's group";
        return;
    }

    bool changed = false;
    auto iter = m_instanceGroupIndex.find(inst->id());
    if(iter != m_instanceGroupIndex.end())
    {
        if(*iter != name)
        {
            *iter = name;
            changed = true;
        }
    }
    else
    {
        changed = true;
        m_instanceGroupIndex[id] = name;
    }

    if(changed)
    {
        m_groupNameCache.insert(name);
        auto idx = getInstIndex(inst.get());
        emit dataChanged(index(idx), index(idx), {GroupRole});
        saveGroupList();
    }
}

QStringList InstanceList::getGroups()
{
    return m_groupNameCache.values();
}

void InstanceList::deleteGroup(const QString& name)
{
    bool removed = false;
    qDebug() << "Delete group" << name;
    for(auto & instance: m_instances)
    {
        const auto & instID = instance->id();
        auto instGroupName = getInstanceGroup(instID);
        if(instGroupName == name)
        {
            m_instanceGroupIndex.remove(instID);
            qDebug() << "Remove" << instID << "from group" << name;
            removed = true;
            auto idx = getInstIndex(instance.get());
            if(idx > 0)
            {
                emit dataChanged(index(idx), index(idx), {GroupRole});
            }
        }
    }
    if(removed)
    {
        saveGroupList();
    }
}

bool InstanceList::isGroupCollapsed(const QString& group)
{
    return m_collapsedGroups.contains(group);
}

void InstanceList::deleteInstance(const InstanceId& id)
{
    auto inst = getInstanceById(id);
    if(!inst)
    {
        qDebug() << "Cannot delete instance" << id << ". No such instance is present (deleted externally?).";
        return;
    }

    if (inst->isApplyingPackUpdate())
        return;

    if(m_instanceGroupIndex.remove(id))
    {
        saveGroupList();
    }

    qDebug() << "Will delete instance" << id;
    if(!FS::deletePath(inst->instanceRoot()))
    {
        qWarning() << "Deletion of instance" << id << "has not been completely successful ...";
        return;
    }

    qDebug() << "Instance" << id << "has been deleted by the launcher.";
}

static QMap<InstanceId, InstanceLocator> getIdMapping(const QList<InstancePtr> &list)
{
    QMap<InstanceId, InstanceLocator> out;
    int i = 0;
    for(auto & item: list)
    {
        auto id = item->id();
        if(out.contains(id))
        {
            qWarning() << "Duplicate ID" << id << "in instance list";
        }
        out[id] = std::make_pair(item, i);
        i++;
    }
    return out;
}

QList< InstanceId > InstanceList::discoverInstances()
{
    m_instancePaths.clear();
    QList<InstanceId> out;
    for (const auto& root : m_instDirs) {
        QDirIterator iter(root, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden);
        while (iter.hasNext()) {
            const auto path = iter.next();
            const auto id = iter.fileName();
            if (id == QStringLiteral("_LAUNCHER_TEMP") || m_instancePaths.contains(id)
                || !QFileInfo(FS::PathCombine(path, "instance.cfg")).isFile()) {
                continue;
            }
            const QFileInfo info(path);
            if (info.isSymLink() && m_instDirs.contains(QFileInfo(info.symLinkTarget()).canonicalPath()))
                continue;

            m_instancePaths.insert(id, path);
            out.append(id);
        }
    }
    return out;
}

InstanceList::InstListError InstanceList::loadList()
{
    // the group file can change outside too (edited by hand, GH-1824): read it again. Nothing
    // gets lost: group changes made here are saved right away.
    loadGroupList();

    auto existingIds = getIdMapping(m_instances);

    QStringList replacements;
    QList<InstancePtr> newList;
    const auto discovered = discoverInstances();
    for (const auto& id : discovered) {
        auto existing = existingIds.find(id);
        if (existing == existingIds.end()) {
            auto instance = loadInstance(id);
            if (instance)
                newList.append(instance);
        } else if (existing.value().first->instanceRoot() == m_instancePaths.value(id)) {
            existingIds.erase(existing);
        } else {
            replacements.append(id);
        }
    }

    // TODO: looks like a general algorithm with a few specifics inserted. Do something about it.
    if(!existingIds.isEmpty())
    {
        // get the list of removed instances and sort it by their original index, from last to first
        auto deadList = existingIds.values();
        auto orderSortPredicate = [](const InstanceLocator & a, const InstanceLocator & b) -> bool
        {
            return a.second > b.second;
        };
        std::sort(deadList.begin(), deadList.end(), orderSortPredicate);
        // remove the contiguous ranges of rows
        int front_bookmark = -1;
        int back_bookmark = -1;
        int currentItem = -1;
        auto removeNow = [&]()
        {
            beginRemoveRows(QModelIndex(), front_bookmark, back_bookmark);
            m_instances.erase(m_instances.begin() + front_bookmark, m_instances.begin() + back_bookmark + 1);
            endRemoveRows();
            front_bookmark = -1;
            back_bookmark = currentItem;
        };
        for(auto & removedItem: deadList)
        {
            auto instPtr = removedItem.first;
            instPtr->invalidate();
            currentItem = removedItem.second;
            if(back_bookmark == -1)
            {
                // no bookmark yet
                back_bookmark = currentItem;
            }
            else if(currentItem == front_bookmark - 1)
            {
                // part of contiguous sequence, continue
            }
            else
            {
                // seam between previous and current item
                removeNow();
            }
            front_bookmark = currentItem;
        }
        if(back_bookmark != -1)
        {
            removeNow();
        }
    }
    for (const auto& id : replacements) {
        auto instance = loadInstance(id);
        if (instance)
            newList.append(instance);
    }
    // the instances kept may be in other groups now
    if(!m_instances.isEmpty())
    {
        emit dataChanged(index(0), index(m_instances.size() - 1), {GroupRole});
    }
    if(newList.size())
    {
        add(newList);
    }
    m_dirty = false;
    updateTotalPlayTime();
    return NoError;
}

void InstanceList::updateTotalPlayTime()
{
    totalPlayTime = 0;
    for(auto const& itr : m_instances)
    {
        totalPlayTime += itr.get()->totalTimePlayed();
    }
}

void InstanceList::saveNow()
{
    for(auto & item: m_instances)
    {
        item->saveNow();
    }
}

void InstanceList::add(const QList<InstancePtr> &t)
{
    beginInsertRows(QModelIndex(), m_instances.count(), m_instances.count() + t.size() - 1);
    m_instances.append(t);
    for(auto & ptr : t)
    {
        connect(ptr.get(), &BaseInstance::propertiesChanged, this, &InstanceList::propertiesChanged);
    }
    endInsertRows();
}

void InstanceList::resumeWatch()
{
    if(m_watchLevel > 0)
    {
        qWarning() << "Bad suspend level resume in instance list";
        return;
    }
    m_watchLevel++;
    if(m_watchLevel > 0 && m_dirty)
    {
        loadList();
    }
}

void InstanceList::suspendWatch()
{
    m_watchLevel --;
}

void InstanceList::providerUpdated()
{
    m_dirty = true;
    if(m_watchLevel == 1)
    {
        loadList();
    }
}

InstancePtr InstanceList::getInstanceById(QString instId) const
{
    if(instId.isEmpty())
        return InstancePtr();
    for(auto & inst: m_instances)
    {
        if (inst->id() == instId)
        {
            return inst;
        }
    }
    return InstancePtr();
}

QModelIndex InstanceList::getInstanceIndexById(const QString &id) const
{
    return index(getInstIndex(getInstanceById(id).get()));
}

int InstanceList::getInstIndex(BaseInstance *inst) const
{
    int count = m_instances.count();
    for (int i = 0; i < count; i++)
    {
        if (inst == m_instances[i].get())
        {
            return i;
        }
    }
    return -1;
}

void InstanceList::propertiesChanged(BaseInstance *inst)
{
    int i = getInstIndex(inst);
    if (i != -1)
    {
        emit dataChanged(index(i), index(i));
        updateTotalPlayTime();
    }
}

InstancePtr InstanceList::loadInstance(const InstanceId& id)
{
    if(!m_groupsLoaded)
    {
        loadGroupList();
    }

    const auto instanceRoot = m_instancePaths.value(id);
    auto instanceSettings = std::make_shared<INISettingsObject>(FS::PathCombine(instanceRoot, "instance.cfg"));
    InstancePtr inst;

    instanceSettings->registerSetting("InstanceType", "Legacy");

    QString inst_type = instanceSettings->get("InstanceType").toString();

    if (inst_type == "OneSix" || inst_type == "Nostalgia")
    {
        inst.reset(new MinecraftInstance(m_globalSettings, instanceSettings, instanceRoot));
    }
    else if (inst_type == "Legacy")
    {
        inst.reset(new LegacyInstance(m_globalSettings, instanceSettings, instanceRoot));
    }
    else
    {
        inst.reset(new NullInstance(m_globalSettings, instanceSettings, instanceRoot));
    }
    qDebug() << "Loaded instance " << inst->name() << " from " << inst->instanceRoot();
    return inst;
}

void InstanceList::saveGroupList()
{
    qDebug() << "Will save group list now.";
    const auto groupFileName = QDir::current().absoluteFilePath("instgroups.json");
    QMap<QString, QSet<QString>> reverseGroupMap;
    for (auto iter = m_instanceGroupIndex.begin(); iter != m_instanceGroupIndex.end(); iter++)
    {
        QString id = iter.key();
        QString group = iter.value();
        if (group.isEmpty())
            continue;
        if (!reverseGroupMap.count(group))
        {
            QSet<QString> set;
            set.insert(id);
            reverseGroupMap[group] = set;
        }
        else
        {
            QSet<QString> &set = reverseGroupMap[group];
            set.insert(id);
        }
    }
    QJsonObject toplevel;
    toplevel.insert("formatVersion", QJsonValue(QString("1")));
    QJsonObject groupsArr;
    for (auto iter = reverseGroupMap.begin(); iter != reverseGroupMap.end(); iter++)
    {
        auto list = iter.value();
        auto name = iter.key();
        QJsonObject groupObj;
        QJsonArray instanceArr;
        groupObj.insert("hidden", QJsonValue(m_collapsedGroups.contains(name)));
        for (auto item : list)
        {
            instanceArr.append(QJsonValue(item));
        }
        groupObj.insert("instances", instanceArr);
        groupsArr.insert(name, groupObj);
    }
    toplevel.insert("groups", groupsArr);
    QJsonDocument doc(toplevel);
    try
    {
        FS::write(groupFileName, doc.toJson());
        qDebug() << "Group list saved.";
    }
    catch (const FS::FileSystemException &e)
    {
        qCritical() << "Failed to write instance group file :" << e.cause();
    }
}

void InstanceList::loadGroupList()
{
    qDebug() << "Will load group list now.";
    const auto groupFileName = QDir::current().absoluteFilePath("instgroups.json");
    auto source = groupFileName;
    if (!QFileInfo::exists(source) && !m_instDir.isEmpty())
        source = QDir(m_instDir).absoluteFilePath("instgroups.json");

    // Read the legacy primary-folder file until it has been migrated.
    if (!QFileInfo::exists(source)) {
        m_instanceGroupIndex.clear();
        m_groupNameCache.clear();
        m_collapsedGroups.clear();
        m_groupsLoaded = true;
        return;
    }

    QByteArray jsonData;
    try
    {
        jsonData = FS::read(source);
    }
    catch (const FS::FileSystemException &e)
    {
        qCritical() << "Failed to read instance group file :" << e.cause();
        return;
    }

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &error);

    // if the json was bad, fail
    if (error.error != QJsonParseError::NoError)
    {
        qCritical() << QString("Failed to parse instance group file: %1 at offset %2")
                            .arg(error.errorString(), QString::number(error.offset))
                            .toUtf8();
        return;
    }

    // if the root of the json wasn't an object, fail
    if (!jsonDoc.isObject())
    {
        qWarning() << "Invalid group file. Root entry should be an object.";
        return;
    }

    QJsonObject rootObj = jsonDoc.object();

    // Make sure the format version matches, otherwise fail.
    if (rootObj.value("formatVersion").toVariant().toInt() != GROUP_FILE_FORMAT_VERSION)
        return;

    // Get the groups. if it's not an object, fail
    if (!rootObj.value("groups").isObject())
    {
        qWarning() << "Invalid group list JSON: 'groups' should be an object.";
        return;
    }

    QSet<QString> groupSet;
    m_instanceGroupIndex.clear();
    m_collapsedGroups.clear();

    // Iterate through all the groups.
    QJsonObject groupMapping = rootObj.value("groups").toObject();
    for (QJsonObject::iterator iter = groupMapping.begin(); iter != groupMapping.end(); iter++)
    {
        QString groupName = iter.key();

        // If not an object, complain and skip to the next one.
        if (!iter.value().isObject())
        {
            qWarning() << QString("Group '%1' in the group list should be an object.").arg(groupName).toUtf8();
            continue;
        }

        QJsonObject groupObj = iter.value().toObject();
        if (!groupObj.value("instances").isArray())
        {
            qWarning() << QString("Group '%1' in the group list is invalid. It should contain an array called 'instances'.").arg(groupName).toUtf8();
            continue;
        }

        // keep a list/set of groups for choosing
        groupSet.insert(groupName);

        auto hidden = groupObj.value("hidden").toBool(false);
        if(hidden) {
            m_collapsedGroups.insert(groupName);
        }

        // Iterate through the list of instances in the group.
        QJsonArray instancesArray = groupObj.value("instances").toArray();

        for (QJsonArray::iterator iter2 = instancesArray.begin(); iter2 != instancesArray.end(); iter2++)
        {
            m_instanceGroupIndex[(*iter2).toString()] = groupName;
        }
    }
    m_groupsLoaded = true;
    m_groupNameCache = groupSet;
    qDebug() << "Group list loaded.";
    if (source != groupFileName)
        saveGroupList();
}

void InstanceList::instanceDirContentsChanged(const QString& path)
{
    Q_UNUSED(path);
    emit instancesChanged();
}

QStringList InstanceList::decodeInstanceDirList(const QVariant& value)
{
    QStringList dirs;
    const auto array = QJsonDocument::fromJson(value.toString().toUtf8()).array();
    for (const auto& item : array) {
        if (item.isString() && !item.toString().isEmpty())
            dirs.append(item.toString());
    }
    return dirs;
}

QString InstanceList::encodeInstanceDirList(const QStringList& dirs)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(dirs)).toJson(QJsonDocument::Compact));
}

void InstanceList::refreshInstanceDirs(const QString& primary)
{
    QString primaryDir;
    if (!primary.isEmpty() && QDir().mkpath(primary))
        primaryDir = QDir(primary).canonicalPath();

    QStringList dirs;
    if (!primaryDir.isEmpty())
        dirs.append(primaryDir);
    for (const auto& additional : decodeInstanceDirList(m_globalSettings->get("AdditionalInstanceDirs"))) {
        const auto dir = QDir(additional).canonicalPath();
        if (!dir.isEmpty() && !dirs.contains(dir))
            dirs.append(dir);
    }
    if (m_instDir == primaryDir && m_instDirs == dirs)
        return;

    const auto watched = m_watcher->directories();
    if (!watched.isEmpty())
        m_watcher->removePaths(watched);
    if (!dirs.isEmpty())
        m_watcher->addPaths(dirs);

    m_instDir = primaryDir;
    m_instDirs = dirs;
}

void InstanceList::on_InstFolderChanged(const Setting&, QVariant)
{
    refreshInstanceDirs(m_globalSettings->get("InstanceDir").toString());
    emit instancesChanged();
}

void InstanceList::on_GroupStateChanged(const QString& group, bool collapsed)
{
    qDebug() << "Group" << group << (collapsed ? "collapsed" : "expanded");
    if(collapsed) {
        m_collapsedGroups.insert(group);
    } else {
        m_collapsedGroups.remove(group);
    }
    saveGroupList();
}

namespace
{
struct PackFileChange
{
    QString source;
    QString target;
    QString backup;
    bool placed = false;
};

bool safePackTarget(const QString& root, const QString& path)
{
    const QDir directory(root);
    const auto relative = directory.relativeFilePath(path);
    if (relative.isEmpty() || QDir::isAbsolutePath(relative) || relative.contains(':') ||
        relative == "." || relative == ".." || relative.startsWith("../"))
        return false;
    auto probe = directory.absolutePath();
    for (const auto& part : relative.split('/')) {
        probe = FS::PathCombine(probe, part);
        if (QFileInfo(probe).isSymLink())
            return false;
    }
    return true;
}

QString applyPackUpdate(const QString& root, const QString& gameRoot, const QString& staging)
{
    const QDir stagedGame(FS::PathCombine(staging, ".minecraft"));
    const QDir targetGame(gameRoot);
    QStringList installed;
    QStringList incoming;
    if (!PackContents::read(staging, incoming))
        return QObject::tr("The selected archive could not be installed as a managed modpack.");
    PackContents::read(root, installed);

    INIFile oldSettings;
    INIFile newSettings;
    const auto stagedConfig = FS::PathCombine(staging, "instance.cfg");
    if (!oldSettings.loadFile(FS::PathCombine(root, "instance.cfg")) ||
        !newSettings.loadFile(stagedConfig))
        return QObject::tr("Could not read the modpack settings.");
    for (const auto& key : {"PackProvider", "PackId", "PackSlug", "PackName", "PackVersionId",
                           "PackVersionLabel", "PackIconUrl", "PackSourceUrl", "PackInstalledAt",
                           "PackManifestSha512"}) {
        oldSettings.set(key, newSettings.value(key));
    }
    if (!oldSettings.saveFile(stagedConfig))
        return QObject::tr("Could not preserve the instance settings.");

    QTemporaryDir backup(FS::PathCombine(QFileInfo(staging).absolutePath(), "pack-backup-XXXXXX"));
    if (!backup.isValid())
        return QObject::tr("Could not create a backup directory for the update.");

    QList<PackFileChange> changes;
    QSet<QString> destinations;
    const auto plan = [&](const QString& source, const QString& target) {
        if (destinations.contains(target))
            return;
        PackFileChange change;
        change.source = source;
        change.target = target;
        changes.append(change);
        destinations.insert(target);
    };
    const auto isSave = [](const QString& relative) {
        return relative.startsWith("saves/", Qt::CaseInsensitive);
    };
    QStringList retained;
    QDirIterator stagedFiles(stagedGame.absolutePath(), QDir::Files | QDir::Hidden | QDir::NoSymLinks,
                             QDirIterator::Subdirectories);
    while (stagedFiles.hasNext()) {
        const auto source = stagedFiles.next();
        const auto relative = stagedGame.relativeFilePath(source);
        auto target = targetGame.filePath(relative);
        if (isSave(relative) && QFileInfo::exists(target)) {
            incoming.removeAll(relative);
            continue;
        }
        const auto canonical = ModMetadataIndex::canonicalFileName(relative);
        const auto disabled = canonical + QStringLiteral(".disabled");
        if (installed.contains(canonical) || installed.contains(disabled)) {
            const bool hadEnabled = QFileInfo::exists(targetGame.filePath(canonical));
            const bool hadDisabled = QFileInfo::exists(targetGame.filePath(disabled));
            if (hadEnabled != hadDisabled) {
                const auto retainedPath = hadEnabled ? canonical : disabled;
                if (retainedPath != relative) {
                    target = targetGame.filePath(retainedPath);
                    incoming.removeAll(relative);
                    retained.append(retainedPath);
                }
            }
        }
        plan(source, target);
    }
    incoming.append(retained);
    if (!PackContents::write(staging, incoming))
        return QObject::tr("Could not update the list of pack files.");

    QHash<QString, std::shared_ptr<ModMetadataIndex>> indexes;
    for (const auto& relative : PackContents::staleEntries(installed, incoming)) {
        const auto target = targetGame.filePath(relative);
        if (isSave(relative) || !QFileInfo::exists(target))
            continue;
        plan({}, target);

        const auto parent = QFileInfo(target).absolutePath();
        auto& metadata = indexes[parent];
        if (!metadata) {
            metadata = std::make_shared<ModMetadataIndex>(QDir(parent));
            metadata->load();
        }
        const auto sidecar = metadata->sidecarPathFor(QFileInfo(target).fileName());
        if (!sidecar.isEmpty())
            plan({}, sidecar);
    }
    for (const auto& name : {QStringLiteral("mmc-pack.json"), QStringLiteral("instance.cfg"),
                             QFileInfo(PackContents::listPath(staging)).fileName()}) {
        const auto source = FS::PathCombine(staging, name);
        if (!QFileInfo::exists(source))
            return QObject::tr("The staged modpack is missing %1.").arg(name);
        plan(source, FS::PathCombine(root, name));
    }

    for (const auto& change : changes) {
        const QFileInfo target(change.target);
        if (!safePackTarget(root, change.target) || (target.exists() && !target.isFile()))
            return QObject::tr("Cannot replace %1 safely.").arg(change.target);
    }

    int position = 0;
    for (; position < changes.size(); ++position) {
        auto& change = changes[position];
        if (!FS::ensureFilePathExists(change.target))
            break;
        if (QFileInfo::exists(change.target)) {
            const auto original = FS::PathCombine(backup.path(), QString::number(position));
            if (!QFile::rename(change.target, original))
                break;
            change.backup = original;
        }
        if (!change.source.isEmpty()) {
            if (!QFile::rename(change.source, change.target))
                break;
            change.placed = true;
        }
    }

    if (position == changes.size())
        return {};

    bool restored = true;
    const auto failedPath = changes.at(position).target;
    for (; position >= 0; --position) {
        const auto& change = changes.at(position);
        if (change.placed && !QFile::rename(change.target, change.source))
            restored = false;
        if (!change.backup.isEmpty() && !QFile::rename(change.backup, change.target))
            restored = false;
    }
    if (!restored) {
        backup.setAutoRemove(false);
        return QObject::tr("Could not restore every file. The update backup is in %1.").arg(backup.path());
    }
    return QObject::tr("Could not update %1. The original files were restored.").arg(failedPath);
}
}

class InstanceStaging : public Task
{
Q_OBJECT
    const unsigned minBackoff = 1;
    const unsigned maxBackoff = 16;
public:
    InstanceStaging (
        InstanceList * parent,
        Task * child,
        const QString & stagingPath,
        const QString& instanceName,
        const QString& groupName )
    : backoff(minBackoff, maxBackoff)
    {
        m_parent = parent;
        m_child.reset(child);
        if (auto* instanceTask = qobject_cast<InstanceTask*>(child))
            m_overrideInstance = instanceTask->overrideInstance();
        if (!m_overrideInstance.isEmpty())
            m_updateTarget = parent->getInstanceById(m_overrideInstance);
        connect(child, &Task::succeeded, this, &InstanceStaging::childSucceded);
        connect(child, &Task::failed, this, &InstanceStaging::childFailed);
        connect(child, &Task::status, this, &InstanceStaging::setStatus);
        connect(child, &Task::progress, this, &InstanceStaging::setProgress);
        connect(child, &Task::details, this, &InstanceStaging::setDetails);
        propagateStepsFrom(child);
        m_instanceName = instanceName;
        m_groupName = groupName;
        m_stagingPath = stagingPath;
        m_backoffTimer.setSingleShot(true);
        connect(&m_backoffTimer, &QTimer::timeout, this, &InstanceStaging::childSucceded);
    }

    ~InstanceStaging() override
    {
        m_commit.waitForFinished();
        if (m_updatingInstance)
            m_updatingInstance->setApplyingPackUpdate(false);
    }


    // FIXME/TODO: add ability to abort during instance commit retries
    bool abort() override
    {
        if(m_child && m_child->isRunning() && m_child->canAbort())
        {
            return m_child->abort();
        }
        return false;
    }
    bool canAbort() const override
    {
        if(m_child && !m_child->isFinished() && m_child->canAbort())
        {
            return true;
        }
        return false;
    }

protected:
    virtual void executeTask() override
    {
        if (m_stagingPath.isEmpty()) {
            emitFailed(tr("Could not create a directory to prepare the instance."));
            return;
        }
        m_child->start();
    }
    QStringList warnings() const override
    {
        return m_child->warnings();
    }

private slots:
    void childSucceded()
    {
        if (!m_overrideInstance.isEmpty()) {
            auto instance = std::dynamic_pointer_cast<MinecraftInstance>(
                m_parent->getInstanceById(m_overrideInstance));
            if (!instance || instance != m_updateTarget.lock()
                || instance->isRunning() || instance->isApplyingPackUpdate()) {
                m_parent->destroyStagingPath(m_stagingPath);
                emitFailed(tr("The instance is no longer available for an update."));
                return;
            }

            m_updatingInstance = instance;
            instance->setApplyingPackUpdate(true);
            setStatus(tr("Applying modpack update..."));
            setProgress(0, 0);
            connect(&m_commitWatcher, &QFutureWatcher<QString>::finished, this, [this, instance] {
                const auto error = m_commitWatcher.result();
                m_parent->destroyStagingPath(m_stagingPath);
                if (error.isEmpty()) {
                    instance->reloadSettings();
                    instance->getPackProfile()->reload(Net::Mode::Offline);
                }
                m_updatingInstance.reset();
                instance->setApplyingPackUpdate(false);
                if (error.isEmpty())
                    emitSucceeded();
                else
                    emitFailed(error);
            });
            m_commit = QtConcurrent::run(applyPackUpdate, instance->instanceRoot(),
                                         instance->gameRoot(), m_stagingPath);
            m_commitWatcher.setFuture(m_commit);
            return;
        }
        unsigned sleepTime = backoff();
        if(m_parent->commitStagedInstance(m_stagingPath, m_instanceName, m_groupName))
        {
            emitSucceeded();
            return;
        }
        // we actually failed, retry?
        if(sleepTime == maxBackoff)
        {
            emitFailed(tr("Failed to commit instance, even after multiple retries. It is being blocked by something."));
            return;
        }
        qDebug() << "Failed to commit instance" << m_instanceName << "Initiating backoff:" << sleepTime;
        m_backoffTimer.start(sleepTime * 500);
    }
    void childFailed(const QString & reason)
    {
        m_parent->destroyStagingPath(m_stagingPath);
        if (m_child->wasAborted())
            emitAborted();
        else
            emitFailed(reason);
    }

private:
    /*
     * WHY: the whole reason why this uses an exponential backoff retry scheme is antivirus on Windows.
     * Basically, it starts messing things up while the launcher is extracting/creating instances
     * and causes that horrible failure that is NTFS to lock files in place because they are open.
     */
    ExponentialSeries backoff;
    QString m_stagingPath;
    QString m_overrideInstance;
    std::weak_ptr<BaseInstance> m_updateTarget;
    InstancePtr m_updatingInstance;
    QFuture<QString> m_commit;
    QFutureWatcher<QString> m_commitWatcher;
    InstanceList * m_parent;
    unique_qobject_ptr<Task> m_child;
    QString m_instanceName;
    QString m_groupName;
    QTimer m_backoffTimer;
};

Task * InstanceList::wrapInstanceTask(InstanceTask * task)
{
    QString stagingPath;
    if (task->overrideInstance().isEmpty()) {
        stagingPath = getStagedInstancePath(task->targetInstanceDir());
    } else if (const auto instance = getInstanceById(task->overrideInstance())) {
        stagingPath = getStagedInstancePath(QFileInfo(instance->instanceRoot()).absolutePath());
    }
    task->setStagingPath(stagingPath);
    task->setParentSettings(m_globalSettings);
    return new InstanceStaging(this, task, stagingPath, task->name(), task->group());
}

QString InstanceList::getStagedInstancePath(const QString& targetDir)
{
    const auto root = targetDir.isEmpty() ? m_instDir : targetDir;
    if (!m_instDirs.contains(root))
        return {};

    const QDir dir(root);
    if (!dir.exists() || QFileInfo(dir.absoluteFilePath("_LAUNCHER_TEMP")).isSymLink())
        return {};
    const auto relative = FS::PathCombine("_LAUNCHER_TEMP", QUuid::createUuid().toString());
    return dir.mkpath(relative) ? dir.absoluteFilePath(relative) : QString();
}

QString InstanceList::stagingRoot(const QString& path) const
{
    if (path.isEmpty())
        return {};
    const QDir parent = QFileInfo(path).absoluteDir();
    if (parent.dirName() != QStringLiteral("_LAUNCHER_TEMP"))
        return {};
    const auto root = QFileInfo(parent.absolutePath()).absolutePath();
    return m_instDirs.contains(root) ? root : QString();
}

bool InstanceList::commitStagedInstance(const QString& path, const QString& instanceName, const QString& groupName)
{
    const auto root = stagingRoot(path);
    if (root.isEmpty())
        return false;

    auto baseName = FS::RemoveInvalidFilenameChars(instanceName);
    if (baseName.isEmpty() || baseName == QStringLiteral(".") || baseName == QStringLiteral(".."))
        baseName = QStringLiteral("instance");
    QString id;
    for (int suffix = 0; suffix <= 9000; ++suffix) {
        const auto candidate = suffix == 0 ? baseName : baseName + QString::number(suffix);
        const auto occupied = std::any_of(m_instDirs.cbegin(), m_instDirs.cend(), [&](const QString& dir) {
            const QFileInfo entry(FS::PathCombine(dir, candidate));
            return entry.exists() || entry.isSymLink();
        });
        if (!occupied) {
            id = candidate;
            break;
        }
    }
    if (id.isEmpty())
        return false;

    {
        WatchLock lock(m_watcher, root);
        const auto destination = FS::PathCombine(root, id);
        if (!QDir().rename(path, destination)) {
            qWarning() << "Failed to move" << path << "to" << destination;
            return false;
        }
    }
    m_instanceGroupIndex[id] = groupName;
    m_groupNameCache.insert(groupName);
    saveGroupList();
    emit instancesChanged();
    emit instanceSelectRequest(id);
    return true;
}

bool InstanceList::destroyStagingPath(const QString& keyPath)
{
    return FS::deletePath(keyPath);
}

int InstanceList::getTotalPlayTime() {
    updateTotalPlayTime();
    return totalPlayTime;
}

#include "InstanceList.moc"
