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

#include "ModFolderPage.h"
#include "ui_ModFolderPage.h"
#include "FileSystem.h"
#include "ui/pages/BasePageContainer.h"

#include "ui/dialogs/BlockedModsDialog.h"
#include "ui/dialogs/DownloadContentDialog.h"
#include "ui/dialogs/DownloadSummaryDialog.h"
#include "ui/dialogs/ExportListDialog.h"
#include "modplatform/flame/FlameApi.h"
#include "modplatform/modrinth/ModrinthApi.h"
#include "ui/dialogs/ModUpdateDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/VersionSelectDialog.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "minecraft/Component.h"
#include <QInputDialog>
#include "modplatform/ContentDownloadTask.h"
#include "modplatform/DependencyResolver.h"
#include "modplatform/ModInstallConflictAnalyzer.h"
#include "modplatform/ModUpdateCheckTask.h"
#include "modplatform/VersionPicker.h"
#include "minecraft/mod/ModMetadataIndex.h"
#include <algorithm>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QSaveFile>
#include <JlCompress.h>

#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>
#include <QAbstractItemModel>
#include <QMenu>
#include <QSortFilterProxyModel>

#include "Application.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/GuiUtil.h"

#include "DesktopServices.h"

#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/PackProfile.h"

#include "Version.h"

namespace {
    // FIXME: wasteful
    void RemoveThePrefix(QString & string) {
        QRegularExpression regex(QStringLiteral("^(([Tt][Hh][eE])|([Tt][eE][Hh])) +"));
        string.remove(regex);
        string = string.trimmed();
    }
}

class ModSortProxy : public QSortFilterProxyModel
{
public:
    explicit ModSortProxy(QObject *parent = 0) : QSortFilterProxyModel(parent)
    {
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override {
        ModFolderModel *model = qobject_cast<ModFolderModel *>(sourceModel());
        if(!model) {
            return false;
        }
        const auto &mod = model->at(source_row);
        if(mod.name().contains(filterRegExp())) {
            return true;
        }
        if(mod.description().contains(filterRegExp())) {
            return true;
        }
        for(auto & author: mod.authors()) {
            if (author.contains(filterRegExp())) {
                return true;
            }
        }
        return false;
    }

    bool lessThan(const QModelIndex & source_left, const QModelIndex & source_right) const override
    {
        ModFolderModel *model = qobject_cast<ModFolderModel *>(sourceModel());
        if(
            !model ||
            !source_left.isValid() ||
            !source_right.isValid() ||
            source_left.column() != source_right.column()
        ) {
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        }

        // we are now guaranteed to have two valid indexes in the same column... we love the provided invariants unconditionally and proceed.

        auto column = (ModFolderModel::Columns) source_left.column();
        bool invert = false;
        switch(column) {
            // GH-2550 - sort by enabled/disabled
            case ModFolderModel::ActiveColumn: {
                auto dataL = source_left.data(Qt::CheckStateRole).toBool();
                auto dataR = source_right.data(Qt::CheckStateRole).toBool();
                if(dataL != dataR) {
                    return dataL > dataR;
                }
                // fallthrough
                invert = sortOrder() == Qt::DescendingOrder;
            }
            // GH-2722 - sort mod names in a way that discards "The" prefixes
            case ModFolderModel::NameColumn: {
                auto dataL = model->data(model->index(source_left.row(), ModFolderModel::NameColumn)).toString();
                RemoveThePrefix(dataL);
                auto dataR = model->data(model->index(source_right.row(), ModFolderModel::NameColumn)).toString();
                RemoveThePrefix(dataR);

                auto less = dataL.compare(dataR, sortCaseSensitivity());
                if(less != 0) {
                    return invert ? (less > 0) : (less < 0);
                }
                // fallthrough
                invert = sortOrder() == Qt::DescendingOrder;
            }
            // GH-2762 - sort versions by parsing them as versions
            case ModFolderModel::VersionColumn: {
                auto dataL = Version(model->data(model->index(source_left.row(), ModFolderModel::VersionColumn)).toString());
                auto dataR = Version(model->data(model->index(source_right.row(), ModFolderModel::VersionColumn)).toString());
                return invert ? (dataL > dataR) : (dataL < dataR);
            }
            default: {
                return QSortFilterProxyModel::lessThan(source_left, source_right);
            }
        }
    }
};

ModFolderPage::ModFolderPage(
    BaseInstance *inst,
    std::shared_ptr<ModFolderModel> mods,
    QString id,
    QString iconName,
    QString displayName,
    QString helpPage,
    QWidget *parent
) :
    QMainWindow(parent),
    ui(new Ui::ModFolderPage)
{
    ui->setupUi(this);
    ui->actionsToolbar->insertSpacer(ui->actionView_configs);

    m_inst = inst;
    on_RunningState_changed(m_inst && m_inst->isRunning());
    m_mods = mods;
    m_id = id;
    if (id == "resourcepacks")
        m_contentType = ModPlatform::ContentType::ResourcePack;
    else if (id == "shaderpacks")
        m_contentType = ModPlatform::ContentType::ShaderPack;
    else if (id == "datapacks")
        m_contentType = ModPlatform::ContentType::DataPack;
    const bool supportsDownloads = id == "mods" || id == "resourcepacks" ||
                                   id == "shaderpacks" || id == "datapacks";
    ui->actionVerifyDependencies->setVisible(id == "mods");
    auto* updateMenu = new QMenu(this);
    auto* checkForUpdates = updateMenu->addAction(ui->actionUpdate->text());
    connect(checkForUpdates, &QAction::triggered, ui->actionUpdate, &QAction::trigger);
    updateMenu->addAction(ui->actionVerifyDependencies);
    updateMenu->addAction(ui->actionResetMetadata);
    ui->actionUpdate->setMenu(updateMenu);
    ui->actionsToolbar->refreshActions();
    for (auto* action : {ui->actionDownload, ui->actionChangeVersion, ui->actionUpdate})
        action->setVisible(supportsDownloads);
    m_displayName = displayName;
    m_iconName = iconName;
    m_helpName = helpPage;
    m_fileSelectionFilter = "%1 (*.zip *.jar)";
    m_filterModel = new ModSortProxy(this);
    m_filterModel->setDynamicSortFilter(true);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSourceModel(m_mods.get());
    m_filterModel->setFilterKeyColumn(-1);
    ui->modTreeView->setModel(m_filterModel);
    ui->modTreeView->installEventFilter(this);
    ui->modTreeView->sortByColumn(1, Qt::AscendingOrder);
    ui->modTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->modTreeView, &ModListView::customContextMenuRequested, this, &ModFolderPage::ShowContextMenu);
    connect(ui->modTreeView, &ModListView::activated, this, &ModFolderPage::modItemActivated);

    auto smodel = ui->modTreeView->selectionModel();
    connect(smodel, &QItemSelectionModel::currentChanged, this, &ModFolderPage::modCurrent);
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &ModFolderPage::on_filterTextChanged);
    connect(m_inst, &BaseInstance::runningStatusChanged, this, &ModFolderPage::on_RunningState_changed);
}

void ModFolderPage::modItemActivated(const QModelIndex&)
{
    if(!m_controlsEnabled) {
        return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->setModStatus(selection.indexes(), ModFolderModel::Toggle);
}

QMenu * ModFolderPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->actionsToolbar->toggleViewAction() );
    return filteredMenu;
}

void ModFolderPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->actionsToolbar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->modTreeView->mapToGlobal(pos));
    delete menu;
}

void ModFolderPage::openedImpl()
{
    m_mods->startWatching();
}

void ModFolderPage::closedImpl()
{
    m_mods->stopWatching();
}

void ModFolderPage::on_filterTextChanged(const QString& newContents)
{
    m_viewFilter = newContents;
    m_filterModel->setFilterFixedString(m_viewFilter);
}


CoreModFolderPage::CoreModFolderPage(BaseInstance *inst, std::shared_ptr<ModFolderModel> mods,
                                     QString id, QString iconName, QString displayName,
                                     QString helpPage, QWidget *parent)
    : ModFolderPage(inst, mods, id, iconName, displayName, helpPage, parent)
{
}

ModFolderPage::~ModFolderPage()
{
    m_mods->stopWatching();
    delete ui;
}

void ModFolderPage::on_RunningState_changed(bool running)
{
    if(m_controlsEnabled == !running) {
        return;
    }
    m_controlsEnabled = !running;
    ui->actionAdd->setEnabled(m_controlsEnabled);
    ui->actionDownload->setEnabled(m_controlsEnabled);
    ui->actionChangeVersion->setEnabled(m_controlsEnabled);
    ui->actionUpdate->setEnabled(m_controlsEnabled);
    ui->actionVerifyDependencies->setEnabled(m_controlsEnabled);
    ui->actionResetMetadata->setEnabled(m_controlsEnabled);
    ui->actionDisable->setEnabled(m_controlsEnabled);
    ui->actionEnable->setEnabled(m_controlsEnabled);
    ui->actionRemove->setEnabled(m_controlsEnabled);
}

bool ModFolderPage::shouldDisplay() const
{
    return true;
}

bool CoreModFolderPage::shouldDisplay() const
{
    if (ModFolderPage::shouldDisplay())
    {
        auto inst = dynamic_cast<MinecraftInstance *>(m_inst);
        if (!inst)
            return true;
        auto version = inst->getPackProfile();
        if (!version)
            return true;
        if(!version->getComponent("net.minecraftforge"))
        {
            return false;
        }
        if(!version->getComponent("net.minecraft"))
        {
            return false;
        }
        if(version->getComponent("net.minecraft")->getReleaseDateTime() < g_VersionFilterData.legacyCutoffDate)
        {
            return true;
        }
    }
    return false;
}

bool ModFolderPage::modListFilter(QKeyEvent *keyEvent)
{
    switch (keyEvent->key())
    {
    case Qt::Key_Delete:
        on_actionRemove_triggered();
        return true;
    case Qt::Key_Plus:
        on_actionAdd_triggered();
        return true;
    default:
        break;
    }
    return QWidget::eventFilter(ui->modTreeView, keyEvent);
}

bool ModFolderPage::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() != QEvent::KeyPress)
    {
        return QWidget::eventFilter(obj, ev);
    }
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
    if (obj == ui->modTreeView)
        return modListFilter(keyEvent);
    return QWidget::eventFilter(obj, ev);
}

void ModFolderPage::on_actionAdd_triggered()
{
    if(!m_controlsEnabled) {
        return;
    }
    auto list = GuiUtil::BrowseForFiles(
        m_helpName,
        tr("Select %1",
           "Select whatever type of files the page contains. Example: 'Loader Mods'")
            .arg(m_displayName),
        m_fileSelectionFilter.arg(m_displayName), APPLICATION->settings()->get("CentralModsDir").toString(),
        this->parentWidget());
    if (!list.empty())
    {
        for (auto filename : list)
        {
            m_mods->installMod(filename);
        }
    }
}

void ModFolderPage::on_actionEnable_triggered()
{
    if(!m_controlsEnabled) {
        return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->setModStatus(selection.indexes(), ModFolderModel::Enable);
}

void ModFolderPage::on_actionDisable_triggered()
{
    if(!m_controlsEnabled) {
        return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->setModStatus(selection.indexes(), ModFolderModel::Disable);
}

void ModFolderPage::on_actionRemove_triggered()
{
    if(!m_controlsEnabled) {
        return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->deleteMods(selection.indexes());
}

void ModFolderPage::on_actionView_configs_triggered()
{
    DesktopServices::openDirectory(m_inst->instanceConfigFolder());
}

void ModFolderPage::on_actionView_Folder_triggered()
{
    DesktopServices::openDirectory(m_mods->dir().absolutePath());
}

void ModFolderPage::modCurrent(const QModelIndex &current, const QModelIndex &previous)
{
    if (!current.isValid())
    {
        ui->frame->clear();
        return;
    }
    auto sourceCurrent = m_filterModel->mapToSource(current);
    int row = sourceCurrent.row();
    Mod &m = m_mods->operator[](row);
    ui->frame->updateWithMod(m);
}

void ModFolderPage::on_actionDownload_triggered()
{
	if (!m_controlsEnabled || !ui->actionDownload->isVisible() || !ensureModLoaderPresent()) {
		return;
	}

	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!mcInst) {
		return;
	}


	// Step 1: Open browse dialog. Hand it the persistent install index so
	// it can refuse to re-add a mod that is already installed in this
	// instance (prevents queuing a second copy/version of the same mod).
	DownloadContentDialog browseDialog(mcInst, m_contentType,
									   this->parentWidget());
	browseDialog.setInstalledIndex(m_mods->metadataIndex());
	if (browseDialog.exec() != QDialog::Accepted) {
		return;
	}

	auto selectedMods = browseDialog.selectedMods();
	if (selectedMods.isEmpty()) {
		return;
	}

	installSelection(selectedMods, browseDialog.mcVersion(),
					 browseDialog.loaderType());
}

void ModFolderPage::on_actionChangeVersion_triggered()
{
	if (!m_controlsEnabled || !ui->actionDownload->isVisible() || !ensureModLoaderPresent()) {
		return;
	}

	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!mcInst) {
		return;
	}

	auto metaIndex = m_mods->metadataIndex();
	if (!metaIndex) {
		return;
	}

	const auto rows = ui->modTreeView->selectionModel()->selectedRows();
    const int row = rows.size() == 1 ? m_filterModel->mapToSource(rows.first()).row() : -1;
	if (row < 0) {
		return;
	}

	const Mod& mod = m_mods->at(size_t(row));
	const auto entry = metaIndex->get(mod.filename().fileName());
	if (!entry.hasPlatformOrigin()) {
		QMessageBox::information(
			this->parentWidget(), tr("Cannot Change Version"),
			tr("There is no record of where '%1' came from, so its other "
			   "versions cannot be listed. Download it through this page "
			   "once and the version can be changed from then on.")
				.arg(mod.name()));
		return;
	}


	/* The pages would otherwise search the moment the dialog is built,
	 * only for the single-project lookup below to throw the results
	 * away. */
	DownloadContentDialog browseDialog(mcInst, m_contentType,
									   this->parentWidget(), true);
	browseDialog.setInstalledIndex(metaIndex);
	if (!browseDialog.openForVersionChange(
			entry.platform, entry.projectId,
			entry.name.isEmpty() ? mod.name() : entry.name)) {
		QMessageBox::information(
			this->parentWidget(), tr("Cannot Change Version"),
			tr("'%1' was installed from a source this page cannot browse.")
				.arg(mod.name()));
		return;
	}

	if (browseDialog.exec() != QDialog::Accepted) {
		return;
	}

	auto selectedMods = browseDialog.selectedMods();
	if (selectedMods.isEmpty()) {
		return;
	}

	/* Identical from here on: replacing a file is what the conflict
	 * analyzer makes of a pick whose project is already on disk, which
	 * is the same thing it does when the same mod is picked out of an
	 * ordinary search. */
	installSelection(selectedMods, browseDialog.mcVersion(),
					 browseDialog.loaderType());
}

void ModFolderPage::installSelection(
	const QList<ModPlatform::SelectedMod>& selectedMods,
	const QString& mcVersion, const QString& loaderType)
{
	// Step 2: Resolve dependencies (only for mods). Feed the resolver
	// the persistent install index so transitive deps that are already
	// on disk are not re-fetched and re-downloaded.
	QList<ModPlatform::DependencyInfo> dependencies;
	QList<ModPlatform::UnresolvedDep> unresolvedDeps;
	if (m_contentType == ModPlatform::ContentType::Mod) {
		/* Shared ownership, not `new` and `delete`: Skip aborts the
		 * resolver while its lookups are still unwinding, and the plain
		 * delete that used to sit at the end of this block freed the
		 * object those replies were about to land in. shared_qobject_ptr
		 * destroys through deleteLater, so the resolver outlives its own
		 * callbacks. */
		shared_qobject_ptr<DependencyResolver> resolver(
			new DependencyResolver(selectedMods, mcVersion, loaderType));
		resolver->setInstalledIndex(m_mods->metadataIndex());

		ProgressDialog depProgress(this->parentWidget());
		/* "Abort" rather than "Skip", matching the reference launcher's
		 * wording on every one of these dialogs. */
		depProgress.setSkipButton(true, tr("Abort"));
		if (depProgress.execWithTask(resolver.get()) != QDialog::Accepted) {
			if (!resolver->wasSuccessful()) {
				qWarning() << "Dependency resolution failed or was skipped";
			}
		}
		/* Whatever was resolved before Skip is still worth having. */
		dependencies = resolver->resolvedDependencies();
		unresolvedDeps = resolver->unresolvedDependencies();
	}

	reviewAndInstall(selectedMods, dependencies, unresolvedDeps);
}

void ModFolderPage::reviewAndInstall(
	const QList<ModPlatform::SelectedMod>& selectedMods,
	const QList<ModPlatform::DependencyInfo>& dependencies,
	const QList<ModPlatform::UnresolvedDep>& unresolvedDeps)
{
	// Step 3: Show summary dialog
	DownloadSummaryDialog summaryDialog(selectedMods, dependencies,
										unresolvedDeps, m_contentType,
										this->parentWidget());
	if (summaryDialog.exec() != QDialog::Accepted) {
		return;
	}

	// Step 3.5: Conflict / duplicate analysis against the on-disk index.
	// This drops items whose exact version is already installed and turns
	// items that match an existing project into in-place replacements.
	auto downloadItems = summaryDialog.downloadItems();
	auto decisions = ModInstallConflictAnalyzer::analyze(
		downloadItems, m_mods->metadataIndex());

	int alreadyInstalled = 0;
	int updates = 0;
	for (const auto& d : decisions) {
		switch (d.status) {
			case ModInstallConflictAnalyzer::Status::AlreadyInstalled:
				alreadyInstalled++;
				break;
			case ModInstallConflictAnalyzer::Status::UpdateAvailable:
			case ModInstallConflictAnalyzer::Status::NameConflict:
			case ModInstallConflictAnalyzer::Status::FileNameClash:
				updates++;
				break;
			default:
				break;
		}
	}
	if (alreadyInstalled > 0 || updates > 0) {
		qDebug() << "Mod install plan:" << alreadyInstalled
				 << "already installed," << updates << "replace,"
				 << (decisions.size() - alreadyInstalled - updates) << "fresh";
	}

	const auto plan = ModInstallConflictAnalyzer::toDownloadPlan(decisions);

	if (plan.isEmpty()) {
		QMessageBox::information(
			this->parentWidget(), tr("Nothing to do"),
			tr("Every selected mod (and its dependencies) is already "
			   "installed at the requested version."));
		return;
	}

	// Step 4: Download everything
	QString targetDir = m_mods->dir().absolutePath();

	/* Shared ownership for the same reason as the resolver above: the
	 * download can now be called off, and the transfers report back as
	 * they unwind - after the plain delete that used to sit here had
	 * already freed what they report to. */
	shared_qobject_ptr<ContentDownloadTask> downloadTask(
		new ContentDownloadTask(plan, targetDir));
	downloadTask->setMetadataIndex(m_mods->metadataIndex());
	ProgressDialog downloadProgress(this->parentWidget());
	downloadProgress.setSkipButton(true, tr("Abort"));
	downloadProgress.execWithTask(downloadTask.get());

	if (!downloadTask->wasAborted() && !downloadTask->wasSuccessful() &&
        !offerManualDownloads(plan, targetDir)) {
		QMessageBox::warning(this->parentWidget(), tr("Download Failed"),
							 tr("Some files failed to download: %1")
								 .arg(downloadTask->failReason()));
	}

	m_mods->update();
}

bool ModFolderPage::offerManualDownloads(
	const QList<ModPlatform::DownloadItem>& plan, const QString& targetDir)
{
	/* Only files whose author blocked third-party downloads are worth
	 * offering this for. Anything else that failed - a dead connection,
	 * a checksum mismatch - is not something fetching by hand fixes, and
	 * the error message is the honest answer there. */
	QList<BlockedMod> blocked;
	const QDir dir(targetDir);
    const auto isLocalName = [](const QString& name) {
            return !name.isEmpty() && name != "." && name != ".." &&
                   !name.contains('/') && !name.contains('\\') && !name.contains(':');
    };
    for (const auto& item : plan) {
		if (!item.browserDownloadOnly || !isLocalName(item.fileName) ||
            (!item.replacesFileName.isEmpty() && !isLocalName(item.replacesFileName))) {
			continue;
		}
        auto path = dir.filePath(item.fileName);
        if (!item.replacesFileName.isEmpty()) {
            const auto previous = dir.filePath(item.replacesFileName);
            if (!QFileInfo::exists(previous) && QFileInfo::exists(previous + ".disabled"))
                path += ".disabled";
        }
        if (QFile::exists(path) && item.replacesFileName != item.fileName)
            continue;

        BlockedMod entry;
        entry.projectId = item.projectId.toInt();
        entry.fileId = item.versionId.toInt();
        entry.fileName = item.fileName;
        entry.targetPath = path;
		blocked.append(entry);
	}

	if (blocked.isEmpty()) {
		return false;
	}

	BlockedModsDialog dialog(
		this->parentWidget(), tr("Restricted Downloads"),
		tr("The authors of these files have blocked downloads outside the "
		   "CurseForge website, and fetching them through it did not work.\n"
		   "Click Download next to each one to open its page in your "
		   "browser. Once the files are in your Downloads folder, click "
		   "Continue and they will be copied into place."),
		blocked);

	if (dialog.exec() != QDialog::Accepted) {
		return true;
	}

	const QString downloadDir =
		QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    auto index = m_mods->metadataIndex();
    QStringList failedFiles;
    for (const auto& item : plan) {
        const auto match = std::find_if(blocked.cbegin(), blocked.cend(), [&item](const BlockedMod& entry) {
            return entry.fileName == item.fileName && entry.found;
        });
        if (match == blocked.cend())
            continue;

        QFile source(FS::PathCombine(downloadDir, item.fileName));
        bool valid = source.open(QIODevice::ReadOnly) && (item.fileSize <= 0 || source.size() == item.fileSize);
        if (valid && !item.sha1.isEmpty()) {
            QCryptographicHash hash(QCryptographicHash::Sha1);
            valid = hash.addData(&source) && hash.result().toHex() == item.sha1.toLatin1().toLower();
        }
        QSaveFile destination(match->targetPath);
        if (!valid || !source.seek(0) || !FS::ensureFilePathExists(match->targetPath) ||
            !destination.open(QIODevice::WriteOnly) || !JlCompress::copyData(source, destination) ||
            !destination.commit()) {
            failedFiles.append(item.fileName);
            continue;
        }

        if (!item.replacesFileName.isEmpty() && item.replacesFileName != item.fileName) {
            const auto previous = dir.filePath(item.replacesFileName);
            for (const auto& path : {previous, previous + QStringLiteral(".disabled")}) {
                if (QFile::exists(path) && !QFile::remove(path))
                    failedFiles.append(QFileInfo(path).fileName());
            }
            index->remove(item.replacesFileName);
        }
        ModMetadataIndex::Entry entry;
        entry.fileName = item.fileName;
        entry.platform = item.platform;
        entry.projectId = item.projectId;
        entry.versionId = item.versionId;
        entry.name = item.name;
        entry.slug = item.slug;
        entry.downloadUrl = item.downloadUrl;
        entry.sha1 = item.sha1.toLower();
        entry.fileSize = item.fileSize;
        entry.isDependency = item.isDependency;
        entry.installedAt = QDateTime::currentDateTimeUtc();
        index->put(entry);
    }
    if (!failedFiles.isEmpty()) {
        QMessageBox::warning(this, tr("Download Failed"),
                             tr("Could not install or replace these files:\n%1").arg(failedFiles.join(QLatin1Char('\n'))));
    }
    return true;
}

void ModFolderPage::on_actionUpdate_triggered()
{
	runUpdateCheck();
}

void ModFolderPage::runUpdateCheck()
{
	if (!m_controlsEnabled || !ui->actionDownload->isVisible()) {
		return;
	}
	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!mcInst) {
		return;
	}

	auto index = m_mods->metadataIndex();
	if (!index) {
		return;
	}

	const QString mcVersion = mcInst->getPackProfile()->getComponentVersion("net.minecraft");
	const QString loader = ModPlatform::instanceModLoader(*mcInst->getPackProfile());

	if (mcVersion.isEmpty()) {
		QMessageBox::warning(
			this->parentWidget(), tr("Cannot Check Updates"),
			tr("This instance has no Minecraft version configured."));
		return;
	}

	/* Shared ownership, and Abort now actually calls the lookups off:
	 * the button was there before but the task had no abort() to
	 * override, so pressing it did nothing at all, and the plain delete
	 * below would have freed the object its replies land in. */
	shared_qobject_ptr<ModUpdateCheckTask> check(
		new ModUpdateCheckTask(index, mcVersion, loader, m_contentType));
	ProgressDialog progress(this->parentWidget());
	progress.setSkipButton(true, tr("Abort"));
	progress.execWithTask(check.get());

	/* Whatever was checked before Abort is still worth offering. */
	const auto updates = check->availableUpdates();

	ModUpdateDialog dlg(updates, this->parentWidget());
	if (dlg.exec() != QDialog::Accepted) {
		return;
	}

	const auto plan = dlg.selectedDownloadItems();
	if (plan.isEmpty()) {
		return;
	}

	shared_qobject_ptr<ContentDownloadTask> dl(
		new ContentDownloadTask(plan, m_mods->dir().absolutePath()));
	dl->setMetadataIndex(index);
	ProgressDialog dlProgress(this->parentWidget());
	dlProgress.setSkipButton(true, tr("Abort"));
	dlProgress.execWithTask(dl.get());
	if (!dl->wasAborted() && !dl->wasSuccessful() &&
        !offerManualDownloads(plan, m_mods->dir().absolutePath())) {
		QMessageBox::warning(
			this->parentWidget(), tr("Update Failed"),
			tr("Some updates failed to download: %1").arg(dl->failReason()));
	}
    m_mods->update();
}

void ModFolderPage::on_actionExportList_triggered()
{
    const auto metadata = m_mods->metadataIndex();
    QList<int> rows;
    const auto selected = ui->modTreeView->selectionModel()->selectedRows();
    for (const auto& row : selected)
        rows.append(m_filterModel->mapToSource(row).row());
    if (rows.isEmpty()) {
        for (size_t i = 0; i < m_mods->size(); ++i)
            rows.append(static_cast<int>(i));
    }

    QList<ContentListExport::Item> items;
    for (const int row : rows) {
        if (row < 0 || static_cast<size_t>(row) >= m_mods->size())
            continue;
        const auto& mod = m_mods->at(row);
        ContentListExport::Item item;
        item.name = mod.name();
        item.modId = mod.mmc_id();
        item.fileName = mod.filename().fileName();
        item.authors = mod.authors();
        item.version = mod.version();
        item.url = mod.homeurl();

        const auto entry = metadata->get(item.fileName);
        if (item.version.isEmpty())
            item.version = entry.versionNumber.isEmpty() ? entry.versionId : entry.versionNumber;
        if (entry.hasPlatformOrigin()) {
            if (entry.platform == FlameApi::get().id())
                item.url = FlameApi::get().projectPageUrl(entry.projectId).toString();
            else if (entry.platform == ModrinthApi::get().id())
                item.url = ModrinthApi::get().projectPageUrl(entry.projectId).toString();
        }
        items.append(item);
    }
    if (items.isEmpty())
        return;

    ExportListDialog dialog(m_inst->name(), items, this);
    dialog.exec();
}

void ModFolderPage::on_actionVerifyDependencies_triggered()
{
	if (!m_controlsEnabled || m_contentType != ModPlatform::ContentType::Mod) {
		return;
	}
	auto index = m_mods->metadataIndex();
	if (!index) {
		return;
	}
	const auto instance = dynamic_cast<MinecraftInstance*>(m_inst);
    if (!instance || !ensureModLoaderPresent()) {
		return;
	}

	const auto profile = instance->getPackProfile();
    const QString mcVersion = profile->getComponentVersion("net.minecraft");
	if (mcVersion.isEmpty()) {
		QMessageBox::warning(
			this->parentWidget(), tr("Cannot Verify Dependencies"),
			tr("This instance has no Minecraft version configured."));
		return;
	}

	/* The installed mods stand in for a selection: the resolver asks each
	 * provider what its recorded version requires, and drops anything
	 * already on disk at that exact version. What is left is what is
	 * missing. */
	QList<ModPlatform::SelectedMod> installed;
	for (const auto& entry : index->all()) {
		if (!entry.hasPlatformOrigin()) {
			continue;
		}
		/* Either identifier will do. A file installed from an mrpack has
		 * no version id recorded - that format lists hashes and download
		 * URLs and no version ids at all - and dropping those from the
		 * check is how "verify dependencies" came to quietly ignore every
		 * mod a modpack installed. The resolver knows how to look a
		 * version up by hash. */
		if (entry.versionId.isEmpty() && entry.sha1.isEmpty()) {
			continue;
		}
		ModPlatform::SelectedMod mod;
		mod.name = entry.name;
		mod.projectId = entry.projectId;
		mod.versionId = entry.versionId;
		mod.sha1 = entry.sha1;
		mod.slug = entry.slug;
		mod.fileName = entry.fileName;
		mod.platform = entry.platform;
		mod.mcVersion = mcVersion;
		installed.append(mod);
	}

	if (installed.isEmpty()) {
		QMessageBox::information(
			this->parentWidget(), tr("Verify Dependencies"),
			tr("None of the installed mods has a recorded origin, so there "
			   "is nothing to look up. Download mods through this page and "
			   "their dependencies can be checked from then on."));
		return;
	}

	shared_qobject_ptr<DependencyResolver> resolver(
		new DependencyResolver(installed, mcVersion, ModPlatform::instanceModLoader(*profile)));
	resolver->setInstalledIndex(index);

	ProgressDialog progress(this->parentWidget());
	progress.setSkipButton(true, tr("Abort"));
	    if (progress.execWithTask(resolver.get()) != QDialog::Accepted) {
        if (!resolver->wasAborted())
            QMessageBox::warning(this, tr("Verify Dependencies"), resolver->failReason());
        return;
    }

	/* Whatever was resolved before Abort is still worth offering. */
	const auto dependencies = resolver->resolvedDependencies();
	const auto unresolved = resolver->unresolvedDependencies();

	if (dependencies.isEmpty() && unresolved.isEmpty()) {
		QMessageBox::information(
			this->parentWidget(), tr("Verify Dependencies"),
			tr("Every dependency of the installed mods is present."));
		return;
	}

	/* Deliberately not the reference launcher's arrangement: there,
	 * Verify Dependencies is the update flow with the dependency step
	 * switched on (ModFolderPage.cpp:90), so the review list also holds
	 * every available update. Here it stands on its own, which keeps the
	 * list to what the action asked about - what is missing. Updates
	 * have their own entry in the same menu. */
	reviewAndInstall({}, dependencies, unresolved);
}

bool ModFolderPage::ensureModLoaderPresent()
{
    if (m_contentType != ModPlatform::ContentType::Mod)
        return true;
    auto* instance = dynamic_cast<MinecraftInstance*>(m_inst);
    if (!instance)
        return false;
    const auto profile = instance->getPackProfile();
    if (profile && !ModPlatform::instanceModLoader(*profile).isEmpty())
        return true;

    if (!profile) {
        QMessageBox::warning(this, tr("No Mod Loader"), tr("Could not read this instance's components."));
        return false;
    }

    const QStringList loaderNames{"Forge", "NeoForge", "Fabric", "Quilt"};
    const QStringList loaderIds{"net.minecraftforge", "net.neoforged",
                                "net.fabricmc.fabric-loader", "org.quiltmc.quilt-loader"};
    bool accepted = false;
    const auto loaderName = QInputDialog::getItem(this, tr("Install a Mod Loader"),
        tr("Choose a mod loader to install for this instance."), loaderNames, 0, false, &accepted);
    if (!accepted)
        return false;

    const int index = loaderNames.indexOf(loaderName);
    if (index < 0)
        return false;
    const auto uid = loaderIds.at(index);
    const auto versions = APPLICATION->metadataIndex()->get(uid);
    if (!versions)
        return false;
    VersionSelectDialog dialog(versions.get(), tr("Select %1 version").arg(loaderName), this);
    if (index < 2)
        dialog.setExactFilter(BaseVersionList::ParentVersionRole, profile->getComponentVersion("net.minecraft"));
    dialog.setCurrentVersion(profile->getComponentVersion(uid));
    if (dialog.exec() != QDialog::Accepted || !dialog.selectedVersion())
        return false;
    profile->setComponentVersion(uid, dialog.selectedVersion()->descriptor());
    if (auto* component = profile->getComponent(uid))
        component->setEnabled(true);
    profile->resolve(Net::Mode::Online);
    if (m_container)
        m_container->refreshContainer();
    return !ModPlatform::instanceModLoader(*profile).isEmpty();
}

void ModFolderPage::on_actionResetMetadata_triggered()
{
    if (!m_controlsEnabled)
        return;
    const auto selected = ui->modTreeView->selectionModel()->selectedRows();
    if (selected.isEmpty())
        return;
    if (QMessageBox::question(this, tr("Reset Metadata"),
        tr("Remove the recorded download source for %n selected file(s)?", nullptr, selected.size()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    const auto metadata = m_mods->metadataIndex();
    for (const auto& index : selected) {
        const auto row = m_filterModel->mapToSource(index).row();
        if (row >= 0 && static_cast<size_t>(row) < m_mods->size())
            metadata->remove(m_mods->at(row).filename().fileName());
    }
}

void ModFolderPage::on_actionViewHomepage_triggered()
{
    const auto selected = ui->modTreeView->selectionModel()->selectedRows();
    if (selected.size() != 1)
        return;
    const auto row = m_filterModel->mapToSource(selected.first()).row();
    if (row < 0 || static_cast<size_t>(row) >= m_mods->size())
        return;
    const auto& mod = m_mods->at(row);
    const auto entry = m_mods->metadataIndex()->get(mod.filename().fileName());
    QUrl url(mod.homeurl());
    if (!entry.projectId.isEmpty()) {
        if (entry.platform == ModrinthApi::get().id())
            url = ModrinthApi::get().projectPageUrl(entry.projectId);
        else if (entry.platform == FlameApi::get().id())
            url = FlameApi::get().projectPageUrl(entry.projectId);
    }
    if (url.scheme() == "https" || url.scheme() == "http")
        DesktopServices::openUrl(url);
}
