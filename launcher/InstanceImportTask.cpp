/* Copyright 2013-2024 MultiMC Contributors
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
#include "InstanceImportTask.h"
#include "BaseInstance.h"
#include "FileSystem.h"
#include "Application.h"
#include "MMCZip.h"
#include "NullInstance.h"
#include "settings/INISettingsObject.h"
#include "icons/IconUtils.h"
#include <QtConcurrentRun>

// FIXME: this does not belong here, it's Minecraft/Flame specific
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "Json.h"
#include "modplatform/flame/FileResolvingTask.h"
#include "minecraft/mod/ModMetadataIndex.h"
#include "ui/dialogs/BlockedModsDialog.h"
#include "ui/dialogs/UntrustedModsDialog.h"
#include "modplatform/flame/FlameApi.h"
#include <QStandardPaths>
#include <QTimer>
#include <QElapsedTimer>
#include <QMessageBox>
#include "modplatform/modrinth/ModrinthApi.h"
#include "modplatform/PackContents.h"
#include <quazipdir.h>
#include "modplatform/modrinth/ModrinthPackManifest.h"
#include "modplatform/technic/TechnicPackProcessor.h"

#include "icons/IconList.h"
#include "Application.h"
#include "net/ChecksumValidator.h"
#include "net/SizeValidator.h"

#include <algorithm>
#include <iterator>

InstanceImportTask::InstanceImportTask(const QUrl sourceUrl)
{
    m_sourceUrl = sourceUrl;
}

InstanceImportTask::~InstanceImportTask()
{
    m_extractFuture.waitForFinished();
}

void InstanceImportTask::executeTask()
{
    if (m_browserProject > 0) {
        if (m_browserFileName.isEmpty() || m_browserFileName == "." || m_browserFileName == ".." ||
            m_browserFileName.contains('/') || m_browserFileName.contains('\\') ||
            m_browserFileName.contains(':')) {
            emitFailed(tr("CurseForge did not provide a valid archive file name."));
            return;
        }
        BlockedMod archive;
        archive.projectId = m_browserProject;
        archive.fileId = m_browserFile;
        archive.fileName = m_browserFileName;
        QList<BlockedMod> archives{archive};
        BlockedModsDialog dialog(QApplication::activeWindow(), tr("Download Modpack"),
                                 tr("Download the modpack with your browser, then click Continue."), archives);
        if (dialog.exec() != QDialog::Accepted) {
            emitAborted();
            return;
        }
        const QDir downloads(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        m_sourceUrl = QUrl::fromLocalFile(downloads.filePath(m_browserFileName));
    }
    if (m_sourceUrl.isLocalFile())
    {
        m_archivePath = m_sourceUrl.toLocalFile();
        processZipPack();
    }
    else
    {
        setStatus(tr("Downloading modpack:\n%1").arg(m_sourceUrl.toString()));
        m_downloadRequired = true;

        const QString path = m_sourceUrl.host() + '/' + m_sourceUrl.path();
        auto entry = APPLICATION->metacache()->resolveEntry("general", path);
        entry->setStale(true);
        m_filesNetJob = new NetJob(tr("Modpack download"), APPLICATION->network());
        m_filesNetJob->addNetAction(Net::Download::makeCached(m_sourceUrl, entry));
        m_archivePath = entry->getFullPath();
        auto job = m_filesNetJob.get();
        connect(job, &NetJob::succeeded, this, &InstanceImportTask::downloadSucceeded);
        connect(job, &NetJob::progress, this, &InstanceImportTask::downloadProgressChanged);
        connect(job, &NetJob::failed, this, &InstanceImportTask::downloadFailed);
        propagateStepsFrom(m_filesNetJob.get());
        m_filesNetJob->start();
    }
}

bool InstanceImportTask::abort()
{
    if (m_extractFuture.isRunning())
    {
        m_cancelExtraction.store(true);
        return true;
    }
    if (m_modIdResolver)
        return m_modIdResolver->abort();
    if (!m_filesNetJob)
    {
        return false;
    }
    m_stopped = true;
    return m_filesNetJob->abort();
}

void InstanceImportTask::downloadSucceeded()
{
    m_filesNetJob.reset();
    processZipPack();
}

void InstanceImportTask::downloadFailed(QString reason)
{
    m_filesNetJob.reset();
    if (m_stopped)
        emitAborted();
    else
        emitFailed(reason);
}

void InstanceImportTask::downloadProgressChanged(qint64 current, qint64 total)
{
    setProgress(current, total);
}

void InstanceImportTask::processZipPack()
{
    setStatus(tr("Extracting modpack"));
    setProgress(0, 0);
    QDir extractDir(m_stagingPath);
    qDebug() << "Attempting to create instance from" << m_archivePath;

    // open the zip and find relevant files in it
    m_packZip.reset(new QuaZip(m_archivePath));
    if (!m_packZip->open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Unable to open supplied modpack zip file."));
        return;
    }

    QString mmcFound = MMCZip::findFolderOfFileInZip(m_packZip.get(), "instance.cfg");

    QString root;
    if (!mmcFound.isNull())
    {
        // process as MultiMC instance/pack
        qDebug() << "MeshMC:" << mmcFound;
        root = mmcFound;
        m_modpackType = ModpackType::MultiMC;
    }
    if (m_modpackType == ModpackType::Unknown) {
        bool technicFound = QuaZipDir(m_packZip.get()).exists("/bin/modpack.jar") || QuaZipDir(m_packZip.get()).exists("/bin/version.json");
        if (technicFound)
        {
            // process as Technic pack
            qDebug() << "Technic:" << technicFound;
            extractDir.mkpath(".minecraft");
            extractDir.cd(".minecraft");
            m_modpackType = ModpackType::Technic;
        }
    }
    if (m_modpackType == ModpackType::Unknown)
    {
        QString modrinthFound = MMCZip::findFolderOfFileInZip(m_packZip.get(), "modrinth.index.json");

        if(!modrinthFound.isNull())
        {
            // process as Modrinth pack
            qDebug() << "Modrinth:" << modrinthFound;
            root = modrinthFound;
            m_modpackType = ModpackType::Modrinth;
        }
    }
    if (m_modpackType == ModpackType::Unknown)
    {
        const auto flameRoot = MMCZip::findFolderOfFileInZip(m_packZip.get(), "manifest.json");
        if (!flameRoot.isNull())
        {
            root = flameRoot;
            m_modpackType = ModpackType::Flame;
        }
    }
    if(m_modpackType == ModpackType::Unknown)
    {
        emitFailed(tr("Archive does not contain a recognized modpack type."));
        return;
    }

    // make sure we extract just the pack
    m_cancelExtraction.store(false);
    const auto target = extractDir.absolutePath();
    m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), [this, root, target] {
        QElapsedTimer refresh;
        refresh.start();
        return MMCZip::extractSubDirWithProgress(m_packZip.get(), root, target,
            [this, &refresh](int done, int total, const QString& file) {
                if (m_cancelExtraction.load())
                    return false;
                if (done == 0 || done == total || refresh.elapsed() >= 25) {
                    refresh.restart();
                    QTimer::singleShot(0, this, [this, done, total, file] {
                        if (isRunning() && m_packZip) {
                            setDetails(file);
                            setProgress(done, total);
                        }
                    });
                }
                return true;
            });
    });
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &InstanceImportTask::extractFinished);
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &InstanceImportTask::extractAborted);
    m_extractFutureWatcher.setFuture(m_extractFuture);
}

void InstanceImportTask::extractFinished()
{
    m_packZip.reset();
    setDetails({});
    if (m_cancelExtraction.load()) {
        emitAborted();
        return;
    }
    if (!m_extractFuture.result())
    {
        emitFailed(tr("Failed to extract modpack"));
        return;
    }
    QDir extractDir(m_stagingPath);

    qDebug() << "Fixing permissions for extracted pack files...";
    QDirIterator it(extractDir, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        auto filepath = it.next();
        QFileInfo file(filepath);
        auto permissions = QFile::permissions(filepath);
        auto origPermissions = permissions;
        if(file.isDir())
        {
            // Folder +rwx for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser;
        }
        else
        {
            // File +rw for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;
        }
        if(origPermissions != permissions)
        {
            if(!QFile::setPermissions(filepath, permissions))
            {
                logWarning(tr("Could not fix permissions for %1").arg(filepath));
            }
            else
            {
                qDebug() << "Fixed" << filepath;
            }
        }
    }

    switch(m_modpackType)
    {
        case ModpackType::MultiMC:
            processMultiMC();
            return;
        case ModpackType::Technic:
            processTechnic();
            return;
        case ModpackType::Flame:
            processFlame();
            return;
        case ModpackType::Modrinth:
            processModrinth();
            return;
        case ModpackType::Unknown:
            emitFailed(tr("Archive does not contain a recognized modpack type."));
            return;
    }
}

void InstanceImportTask::extractAborted()
{
    emitFailed(tr("Instance import has been stopped."));
    return;
}

void InstanceImportTask::processTechnic()
{
    shared_qobject_ptr<Technic::TechnicPackProcessor> packProcessor = new Technic::TechnicPackProcessor();
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &InstanceImportTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &InstanceImportTask::emitFailed);
    packProcessor->run(m_globalSettings, m_instName, m_instIcon, m_stagingPath);
}

void InstanceImportTask::processMultiMC()
{
    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);
    instanceSettings->registerSetting("InstanceType", "Legacy");

    NullInstance instance(m_globalSettings, instanceSettings, m_stagingPath);

    // reset time played on import... because packs.
    instance.resetTimePlayed();

    // set a new nice name
    instance.setName(m_instName);

    // if the icon was specified by user, use that. otherwise pull icon from the pack
    if (m_instIcon != "default")
    {
        instance.setIconKey(m_instIcon);
    }
    else
    {
        m_instIcon = instance.iconKey();

        auto importIconPath = IconUtils::findBestIconIn(instance.instanceRoot(), m_instIcon);
        if (!importIconPath.isNull() && QFile::exists(importIconPath))
        {
            // import icon
            auto iconList = APPLICATION->icons();
            if (iconList->iconFileExists(m_instIcon))
            {
                iconList->deleteIcon(m_instIcon);
            }
            iconList->installIcons({importIconPath});
        }
    }
    emitSucceeded();
}

namespace {
bool mergeOverrides(const QString &fromDir, const QString &toDir) {
    QDir dir(fromDir);
    if(!dir.exists()) {
        return true;
    }
    if(!FS::ensureFolderPathExists(toDir)) {
        return false;
    }
    const int absSourcePathLength = dir.absoluteFilePath(fromDir).length();

    QDirIterator it(fromDir, QDirIterator::Subdirectories);
    while (it.hasNext()){
        it.next();
        const auto fileInfo = it.fileInfo();
        auto fileName = fileInfo.fileName();
        if(fileName == "." || fileName == "..") {
            continue;
        }
        const QString subPathStructure = fileInfo.absoluteFilePath().mid(absSourcePathLength);
        const QString constructedAbsolutePath = toDir + subPathStructure;

        if(fileInfo.isDir()){
            //Create directory in target folder
            dir.mkpath(constructedAbsolutePath);
        } else if(fileInfo.isFile()) {
            QFileInfo targetFileInfo(constructedAbsolutePath);
            if(targetFileInfo.exists()) {
                continue;
            }
            // move
            QFile::rename(fileInfo.absoluteFilePath(), constructedAbsolutePath);
        }
    }

    dir.removeRecursively();
    return true;
}

}

void InstanceImportTask::processModrinth() {
    std::vector<Modrinth::File> files;
    QString minecraftVersion, fabricVersion, quiltVersion, forgeVersion, neoforgeVersion, packName, packVersion;
    try
    {
        QString indexPath = FS::PathCombine(m_stagingPath, "modrinth.index.json");
        auto doc = Json::requireDocument(indexPath);
        auto obj = Json::requireObject(doc, "modrinth.index.json");
        packName = Json::ensureString(obj, "name");
        packVersion = Json::ensureString(obj, "versionId");
        int formatVersion = Json::requireInteger(obj, "formatVersion", "modrinth.index.json");
        if (formatVersion == 1)
        {
            auto game = Json::requireString(obj, "game", "modrinth.index.json");
            if (game != "minecraft")
            {
                throw JSONValidationError("Unknown game: " + game);
            }

            auto jsonFiles = Json::requireIsArrayOf<QJsonObject>(obj, "files", "modrinth.index.json");
            for(auto & obj: jsonFiles) {
                Modrinth::File file;
                auto dirtyPath = Json::requireString(obj, "path");
                dirtyPath.replace('\\', '/');
                auto simplifiedPath = QDir::cleanPath(dirtyPath);
                QFileInfo fileInfo (simplifiedPath);
                if (simplifiedPath.isEmpty() || simplifiedPath == "." || simplifiedPath == ".." ||
                    simplifiedPath.startsWith("../") || fileInfo.isAbsolute() || simplifiedPath.contains(':')) {
                    throw JSONValidationError("Invalid path found in modpack files:\n\n" + simplifiedPath);
                }
                file.path = simplifiedPath;

                // env doesn't have to be present, in that case mod is required
                auto env = Json::ensureObject(obj, "env");
                auto clientEnv = Json::ensureString(env, "client", "required");

                if(clientEnv == "required") {
                    // NOOP
                }
                else if(clientEnv == "optional") {
                    file.path += ".disabled";
                }
                else if(clientEnv == "unsupported") {
                    continue;
                }

                QJsonObject hashes = Json::requireObject(obj, "hashes");
                file.sha1 = Json::ensureString(hashes, "sha1").toLower();
                QString hash;
                QCryptographicHash::Algorithm hashAlgorithm;
                hash = Json::ensureString(hashes, "sha256");
                hashAlgorithm = QCryptographicHash::Sha256;
                if (hash.isEmpty())
                {
                    hash = Json::ensureString(hashes, "sha512");
                    hashAlgorithm = QCryptographicHash::Sha512;
                    if (hash.isEmpty())
                    {
                        hash = Json::ensureString(hashes, "sha1");
                        hashAlgorithm = QCryptographicHash::Sha1;
                        if (hash.isEmpty())
                        {
                            throw JSONValidationError("No hash found for: " + file.path);
                        }
                    }
                }
                file.hash = QByteArray::fromHex(hash.toLatin1());
                file.hashAlgorithm = hashAlgorithm;
                if (obj.contains("fileSize"))
                {
                    file.size = Json::requireBoundedInteger(obj, "fileSize", 0, Json::MaxExactInteger);
                }
                // Do not use requireUrl, which uses StrictMode, instead use QUrl's default TolerantMode (as Modrinth seems to incorrectly handle spaces)
                const auto urls = Json::requireArray(obj, "downloads");
                for (const auto& url : urls) {
                    const QUrl mirror(url.toString());
                    if (mirror.isValid() && mirror.scheme() == "https" && !mirror.host().isEmpty())
                        file.mirrors.append(mirror);
                }
                if (file.mirrors.isEmpty())
                {
                    throw JSONValidationError("Download URL for " + file.path + " is not a correctly formatted URL");
                }
                file.download = file.mirrors.first();
                files.push_back(file);
            }

            auto dependencies = Json::requireObject(obj, "dependencies", "modrinth.index.json");
            for (auto it = dependencies.begin(), end = dependencies.end(); it != end; ++it)
            {
                QString name = it.key();
                if (name == "minecraft")
                {
                    if (!minecraftVersion.isEmpty())
                        throw JSONValidationError("Duplicate Minecraft version");
                    minecraftVersion = Json::requireValueString(*it, "Minecraft version");
                }
                else if (name == "fabric-loader")
                {
                    if (!fabricVersion.isEmpty())
                        throw JSONValidationError("Duplicate Fabric Loader version");
                    fabricVersion = Json::requireValueString(*it, "Fabric Loader version");
                }
                else if (name == "quilt-loader")
                {
                    if (!quiltVersion.isEmpty())
                        throw JSONValidationError("Duplicate Quilt Loader version");
                    quiltVersion = Json::requireValueString(*it, "Quilt Loader version");
                }
                else if (name == "forge")
                {
                    if (!forgeVersion.isEmpty())
                        throw JSONValidationError("Duplicate Forge version");
                    forgeVersion = Json::requireValueString(*it, "Forge version");
                }
                else if (name == "neoforge")
                {
                    if (!neoforgeVersion.isEmpty())
                        throw JSONValidationError("Duplicate NeoForge version");
                    neoforgeVersion = Json::requireValueString(*it, "NeoForge version");
                }
                else
                {
                    throw JSONValidationError("Unknown dependency type: " + name);
                }
            }
        }
        else
        {
            throw JSONValidationError(QStringLiteral("Unknown format version: %s").arg(formatVersion));
        }
        QFile::remove(indexPath);
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(tr("Could not understand pack index:\n") + e.cause());
        return;
    }
    QString clientOverridePath = FS::PathCombine(m_stagingPath, "client-overrides");
    if (QFile::exists(clientOverridePath)) {
        QString mcPath = FS::PathCombine(m_stagingPath, ".minecraft");
        if (!QFile::rename(clientOverridePath, mcPath)) {
            emitFailed(tr("Could not rename the overrides folder:\n") + "overrides");
            return;
        }
    }

    // TODO: only extract things we actually want instead of everything only to just delete it afterwards ...
    if(!mergeOverrides(FS::PathCombine(m_stagingPath, "client-overrides"), FS::PathCombine(m_stagingPath, ".minecraft"))) {
        emitFailed(tr("Could not merge the overrides folder:\n") + "client-overrides");
        return;
    }

    if(!mergeOverrides(FS::PathCombine(m_stagingPath, "overrides"), FS::PathCombine(m_stagingPath, ".minecraft"))) {
        emitFailed(tr("Could not merge the overrides folder:\n") + "overrides");
        return;
    }

    FS::deletePath(FS::PathCombine(m_stagingPath, "server-overrides"));


    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);
    instanceSettings->registerSetting("InstanceType", "Legacy");
    instanceSettings->set("InstanceType", "OneSix");
    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", minecraftVersion, true);
    if (!fabricVersion.isEmpty())
        components->setComponentVersion("net.fabricmc.fabric-loader", fabricVersion, true);
    if (!quiltVersion.isEmpty())
        components->setComponentVersion("org.quiltmc.quilt-loader", quiltVersion, true);
    if (!forgeVersion.isEmpty())
        components->setComponentVersion("net.minecraftforge", forgeVersion, true);
    if (!neoforgeVersion.isEmpty())
        components->setComponentVersion("net.neoforged", neoforgeVersion, true);
    if (m_instIcon != "default")
    {
        instance.setIconKey(m_instIcon);
    }
    instance.setName(m_instName);
    recordPackSource(*instanceSettings, "modrinth", packName, packVersion);
    instance.saveNow();

    const QDir gameDir(FS::PathCombine(m_stagingPath, ".minecraft"));
    QStringList externalFiles;
    for (const auto& file : files) {
        for (const auto& url : file.mirrors) {
            if (!ModrinthApi::isMrpackHost(url))
                externalFiles.append(file.path + " (" + url.host() + ")");
        }
    }
    if (!confirmPackSources(gameDir, externalFiles))
        return;
    m_filesNetJob = new NetJob(tr("Mod download"), APPLICATION->network());
    for (auto &file : files)
    {
        auto path = FS::PathCombine(m_stagingPath, ".minecraft", file.path);
        qDebug() << "Will download" << file.download << "to" << path;
        auto dl = Net::Download::makeFileWithMirrors(file.mirrors, path);
        dl->addValidator(new Net::ChecksumValidator(file.hashAlgorithm, file.hash));
        if (file.size >= 0)
        {
            dl->addValidator(new Net::SizeValidator(file.size));
        }
        m_filesNetJob->addNetAction(dl);
    }
    connect(m_filesNetJob.get(), &NetJob::succeeded, this, [this, files, gameDir]()
    {
        QHash<QString, std::shared_ptr<ModMetadataIndex>> indexes;
        for (const auto& file : files) {
            const auto folder = QFileInfo(file.path).path();
            if (folder != "mods" && folder != "resourcepacks" && folder != "shaderpacks" &&
                folder != "texturepacks" && folder != "coremods")
                continue;
            QUrl source;
            for (const auto& url : file.mirrors) {
                if (url.host() == ModrinthApi::cdnHost()) {
                    source = url;
                    break;
                }
            }
            const auto parts = source.path().split('/', QtCompat::SkipEmptyParts);
            if (parts.size() < 5 || parts.at(0) != "data" || parts.at(2) != "versions")
                continue;

            auto& index = indexes[folder];
            if (!index) {
                index = std::make_shared<ModMetadataIndex>(QDir(gameDir.filePath(folder)));
                index->load();
            }
            ModMetadataIndex::Entry entry;
            entry.fileName = ModMetadataIndex::canonicalFileName(QFileInfo(file.path).fileName());
            entry.name = QFileInfo(entry.fileName).completeBaseName();
            entry.platform = "modrinth";
            entry.projectId = parts.at(1);
            entry.versionNumber = parts.at(3);
            entry.sha1 = file.sha1;
            entry.downloadUrl = source.toString();
            entry.fileSize = file.size;
            index->put(entry);
        }
        m_filesNetJob.reset();
        finishPackImport();
    });
    connect(m_filesNetJob.get(), &NetJob::failed, this, &InstanceImportTask::downloadFailed);
    connect(m_filesNetJob.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        setProgress(current, total);
    });
    setStatus(tr("Downloading mods..."));
    propagateStepsFrom(m_filesNetJob.get());
    m_filesNetJob->start();
}

static QString selectFlameIcon(const QString& instIcon,
							   const Flame::Manifest& pack)
{
	if (instIcon != "default")
		return instIcon;
	if (pack.name.contains("Direwolf20"))
		return "steve";
	if (pack.name.contains("FTB") || pack.name.contains("Feed The Beast"))
		return "ftb_logo";
	// default to something other than the MeshMC default to distinguish these
	return "flame";
}

void InstanceImportTask::configureFlameInstance(Flame::Manifest& pack)
{
	const static QMap<QString, QString> forgemap = {{"1.2.5", "3.4.9.171"},
													{"1.4.2", "6.0.1.355"},
													{"1.4.7", "6.6.2.534"},
													{"1.5.2", "7.8.1.737"}};

	struct FlameLoaderMapping {
		const char* prefix;
		QString version;
		const char* componentId;
	};
	FlameLoaderMapping loaderMappings[] = {
		{"forge-", {}, "net.minecraftforge"},
		{"fabric-", {}, "net.fabricmc.fabric-loader"},
		{"neoforge-", {}, "net.neoforged"},
		{"quilt-", {}, "org.quiltmc.quilt-loader"},
	};
	for (auto& loader : pack.minecraft.modLoaders) {
		auto id = loader.id;
		bool matched = false;
		for (auto& mapping : loaderMappings) {
			if (id.startsWith(mapping.prefix)) {
				id.remove(mapping.prefix);
				mapping.version = id;
				matched = true;
				break;
			}
		}
		if (!matched) {
			logWarning(tr("Unknown mod loader in manifest: %1").arg(id));
		}
	}

	QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
	auto instanceSettings = std::make_shared<INISettingsObject>(configPath);
	instanceSettings->registerSetting("InstanceType", "Legacy");
	instanceSettings->set("InstanceType", "OneSix");
	MinecraftInstance instance(m_globalSettings, instanceSettings,
							   m_stagingPath);
	auto mcVersion = pack.minecraft.version;
	// Hack to correct some 'special sauce'...
	if (mcVersion.endsWith('.')) {
		mcVersion.remove(QRegularExpression("[.]+$"));
		logWarning(tr("Mysterious trailing dots removed from Minecraft version "
					  "while importing pack."));
	}
	auto components = instance.getPackProfile();
	components->buildingFromScratch();
	components->setComponentVersion("net.minecraft", mcVersion, true);

	// Handle Forge "recommended" version mapping
	auto& forgeMapping = loaderMappings[0];
	if (forgeMapping.version == "recommended") {
		if (forgemap.contains(mcVersion)) {
			forgeMapping.version = forgemap[mcVersion];
		} else {
			logWarning(
				tr("Could not map recommended forge version for Minecraft %1")
					.arg(mcVersion));
		}
	}

	for (const auto& mapping : loaderMappings) {
		if (!mapping.version.isEmpty()) {
			components->setComponentVersion(mapping.componentId,
											mapping.version);
		}
	}

	instance.setIconKey(selectFlameIcon(m_instIcon, pack));

	QString jarmodsPath =
		FS::PathCombine(m_stagingPath, QStringLiteral(".minecraft"), "jarmods");
	QFileInfo jarmodsInfo(jarmodsPath);
	if (jarmodsInfo.isDir()) {
		// install all the jar mods
		qDebug() << "Found jarmods:";
		QDir jarmodsDir(jarmodsPath);
		QStringList jarMods;
		for (auto info :
			 jarmodsDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files)) {
			qDebug() << info.fileName();
			jarMods.push_back(info.absoluteFilePath());
		}
		auto profile = instance.getPackProfile();
		profile->installJarMods(jarMods);
		// nuke the original files
		FS::deletePath(jarmodsPath);
	}
	instance.setName(m_instName);
    recordPackSource(*instanceSettings, "curseforge", pack.name, pack.version);

}

void InstanceImportTask::processFlame()
{
    Flame::Manifest pack;
    QDir stage(m_stagingPath);
    const auto manifest = stage.filePath("manifest.json");
    try
    {
        Flame::loadManifest(pack, manifest);
    }
    catch (const JSONValidationError& error)
    {
        emitFailed(tr("Could not understand pack manifest:\n") + error.cause());
        return;
    }
    QFile::remove(manifest);

    const auto overridePath = QDir::cleanPath(pack.overrides);
    if (!pack.overrides.isEmpty() &&
        (QDir::isAbsolutePath(overridePath) || overridePath == "." || overridePath == ".." ||
         overridePath.startsWith("../") || pack.overrides.contains('\\') || pack.overrides.contains(':')))
    {
        emitFailed(tr("Invalid overrides path in CurseForge manifest."));
        return;
    }
    if (!pack.overrides.isEmpty() && stage.exists(overridePath) && overridePath != ".minecraft")
    {
        if (!stage.rename(overridePath, ".minecraft"))
        {
            emitFailed(tr("Could not move the overrides folder: %1").arg(pack.overrides));
            return;
        }
    }
    if (!confirmPackSources(QDir(stage.filePath(".minecraft")), {}))
        return;
    configureFlameInstance(pack);

    m_modIdResolver = new Flame::FileResolvingTask(APPLICATION->network(), pack);
    auto* resolver = m_modIdResolver.get();
    connect(resolver, &Task::succeeded, this, &InstanceImportTask::downloadFlameFiles);
    connect(resolver, &Task::failed, this, [this](const QString& reason) {
        const bool cancelled = m_modIdResolver->wasAborted();
        m_modIdResolver.reset();
        if (cancelled)
            emitAborted();
        else
            emitFailed(reason);
    });
    connect(resolver, &Task::status, this, &Task::setStatus);
    connect(resolver, &Task::progress, this, &Task::setProgress);
    propagateStepsFrom(resolver);
    resolver->start();
}

void InstanceImportTask::downloadFlameFiles()
{
    const auto files = m_modIdResolver->getResults().files;
    m_modIdResolver.reset();

    const QDir gameDir(FS::PathCombine(m_stagingPath, ".minecraft"));
    QStringList externalFiles;
    for (const auto& file : files) {
        const auto host = file.url.host();
        if (!file.url.isEmpty() && host != FlameApi::siteHost() &&
            host != "forgecdn.net" && !host.endsWith(".forgecdn.net"))
            externalFiles.append(file.fileName + " (" + host + ")");
    }
    if (!externalFiles.isEmpty()) {
        UntrustedModsDialog dialog(externalFiles, QApplication::activeWindow());
        if (dialog.exec() != QDialog::Accepted) {
            emitAborted();
            return;
        }
    }

    QList<BlockedMod> blocked;
    m_filesNetJob = new NetJob(tr("Downloading CurseForge files"), APPLICATION->network());
    for (const auto& file : files)
    {
        if (file.fileName.isEmpty() || file.fileName == "." || file.fileName == ".." ||
            file.fileName.contains('/') || file.fileName.contains('\\') || file.fileName.contains(':'))
        {
            m_filesNetJob.reset();
            emitFailed(tr("Could not resolve CurseForge file %1 for project %2.").arg(file.fileId).arg(file.projectId));
            return;
        }
        const auto folder = QDir::cleanPath(file.targetFolder);
        if (QDir::isAbsolutePath(folder) || folder == ".." || folder.startsWith("../") ||
            file.targetFolder.contains('\\') || file.targetFolder.contains(':'))
        {
            m_filesNetJob.reset();
            emitFailed(tr("Invalid target folder for %1.").arg(file.fileName));
            return;
        }
        const auto name = file.required ? file.fileName : file.fileName + ".disabled";
        const auto path = gameDir.filePath(FS::PathCombine(file.targetFolder, name));
        if (!file.resolved || file.url.isEmpty())
        {
            BlockedMod item;
            item.projectId = file.projectId;
            item.fileId = file.fileId;
            item.fileName = file.fileName;
            item.targetPath = path;
            blocked.append(item);
            continue;
        }
        if (!file.url.isValid() || file.url.scheme() != "https" || file.url.host().isEmpty())
        {
            m_filesNetJob.reset();
            emitFailed(tr("Invalid download URL for %1.").arg(file.fileName));
            return;
        }
        auto download = Net::Download::makeFile(file.url, path);
        if (!file.sha1.isEmpty())
            download->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, QByteArray::fromHex(file.sha1.toLatin1())));
        if (file.fileSize > 0)
            download->addValidator(new Net::SizeValidator(file.fileSize));
        m_filesNetJob->addNetAction(download);
    }

    if (!blocked.isEmpty())
    {
        BlockedModsDialog dialog(QApplication::activeWindow(), tr("Restricted Mods"),
            tr("Download the restricted files in your browser, then click Continue to import them."), blocked);
        if (dialog.exec() != QDialog::Accepted)
        {
            m_filesNetJob.reset();
            emitAborted();
            return;
        }
        const QDir downloads(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        for (const auto& item : blocked)
        {
            const auto sourcePath = downloads.filePath(item.fileName);
            const auto matchesFile = [&] {
                for (const auto& file : files)
                {
                    if (file.projectId != item.projectId || file.fileId != item.fileId)
                        continue;
                    QFile source(sourcePath);
                    if (!source.open(QIODevice::ReadOnly) ||
                        (file.fileSize > 0 && source.size() != file.fileSize))
                        return false;
                    if (file.sha1.isEmpty())
                        return true;
                    QCryptographicHash digest(QCryptographicHash::Sha1);
                    return digest.addData(&source) && digest.result().toHex() == file.sha1.toLatin1().toLower();
                }
                return false;
            };
            if (!item.found || !matchesFile() || !FS::ensureFilePathExists(item.targetPath) ||
                !QFile::copy(sourcePath, item.targetPath))
            {
                m_filesNetJob.reset();
                emitFailed(tr("Could not import the restricted file %1.").arg(item.fileName));
                return;
            }
        }
    }

    connect(m_filesNetJob.get(), &Task::progress, this, &Task::setProgress);
    connect(m_filesNetJob.get(), &Task::failed, this, &InstanceImportTask::downloadFailed);
    connect(m_filesNetJob.get(), &Task::succeeded, this, [this, files, gameDir] {
        for (const auto& file : files)
        {
            const QDir folder(gameDir.filePath(file.targetFolder));
            const auto name = file.required ? file.fileName : file.fileName + ".disabled";
            if (!folder.exists(name))
                continue;
            ModMetadataIndex::Entry entry;
            entry.fileName = file.fileName;
            entry.name = file.fileName;
            entry.platform = "curseforge";
            entry.projectId = QString::number(file.projectId);
            entry.versionId = QString::number(file.fileId);
            entry.downloadUrl = file.url.toString();
            entry.sha1 = file.sha1;
            entry.fileSize = file.fileSize;
            ModMetadataIndex index(folder);
            index.load();
            index.put(entry);
        }
        m_filesNetJob.reset();
        finishPackImport();
    });
    setStatus(tr("Downloading CurseForge files"));
    propagateStepsFrom(m_filesNetJob.get());
    m_filesNetJob->start();
}

void InstanceImportTask::recordPackSource(SettingsObject& settings, const QString& provider,
                                          const QString& name, const QString& version)
{
    const bool sameProvider = m_packHint.provider == provider;
    settings.set("PackProvider", provider);
    settings.set("PackId", sameProvider ? m_packHint.packId : QString());
    settings.set("PackSlug", sameProvider ? m_packHint.packSlug : QString());
    settings.set("PackName", sameProvider && !m_packHint.packName.isEmpty() ? m_packHint.packName : name);
    settings.set("PackVersionId", sameProvider ? m_packHint.versionId : QString());
    settings.set("PackVersionLabel", sameProvider && !m_packHint.versionLabel.isEmpty() ? m_packHint.versionLabel : version);
    settings.set("PackIconUrl", sameProvider ? m_packHint.iconUrl : QString());
    settings.set("PackSourceUrl", sameProvider ? m_packHint.sourceUrl : QString());
    settings.set("PackInstalledAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
}

void InstanceImportTask::finishPackImport()
{
    const QDir game(FS::PathCombine(m_stagingPath, ".minecraft"));
    QStringList paths;
    QDirIterator files(game.absolutePath(), QDir::Files | QDir::Hidden | QDir::NoSymLinks,
                       QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const auto path = game.relativeFilePath(files.next());
        if (!path.split('/').contains(".index"))
            paths.append(path);
    }
    if (!PackContents::write(m_stagingPath, paths)) {
        emitFailed(tr("Could not record the files installed by this modpack."));
        return;
    }
    emitSucceeded();
}

bool InstanceImportTask::confirmPackSources(const QDir& gameDir, QStringList externalFiles)
{
    if (!m_trustedSource) {
        for (const auto& folder : {"mods", "coremods", "jarmods", "plugins"}) {
            QDirIterator files(gameDir.filePath(folder), QDir::Files | QDir::Hidden,
                               QDirIterator::Subdirectories);
            while (files.hasNext())
                externalFiles.append(gameDir.relativeFilePath(files.next()));
        }
    }
    if (externalFiles.isEmpty())
        return true;
    externalFiles.removeDuplicates();
    externalFiles.sort();

    UntrustedModsDialog dialog(externalFiles, QApplication::activeWindow());
    if (dialog.exec() == QDialog::Accepted)
        return true;
    emitAborted();
    return false;
}
