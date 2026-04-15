/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 *
 */

#include "DownloadContentDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QTimer>
#include <QRegularExpression>

#include "Application.h"
#include "Json.h"
#include "minecraft/PackProfile.h"
#include "modplatform/ContentType.h"
#include "modplatform/flame/FlameModModel.h"
#include "modplatform/modrinth/ModrinthModModel.h"
#include "net/Download.h"
#include "net/NetJob.h"

static QString normalizeModName(const QString& name)
{
	QString n = name.toLower().trimmed();
	n.remove(QRegularExpression("\\s*\\([^)]*\\)\\s*"));
	n.remove(QRegularExpression("[^a-z0-9 ]"));
	return n.simplified();
}

DownloadContentDialog::DownloadContentDialog(
	MinecraftInstance* instance, ModPlatform::ContentType contentType,
	QWidget* parent)
	: QDialog(parent), m_instance(instance), m_contentType(contentType)
{
	// Get Minecraft version and loader from instance
	auto profile = m_instance->getPackProfile();
	if (profile) {
		m_mcVersion = profile->getComponentVersion("net.minecraft");
		// Detect loader
		if (profile->getComponent("net.minecraftforge")) {
			m_loaderType = "forge";
		} else if (profile->getComponent("net.fabricmc.fabric-loader")) {
			m_loaderType = "fabric";
		} else if (profile->getComponent("org.quiltmc.quilt-loader")) {
			m_loaderType = "quilt";
		} else if (profile->getComponent("net.neoforged.neoforge")) {
			m_loaderType = "neoforge";
		}
	}

	setupUi();

	// Setup search debounce timer
	m_searchTimer = new QTimer(this);
	m_searchTimer->setInterval(500); // 500ms delay
	connect(m_searchTimer, &QTimer::timeout, this,
			&DownloadContentDialog::triggerSearch);

	m_flameModel = new Flame::ModListModel(contentType, this);
	m_modrinthModel = new Modrinth::ModListModel(contentType, this);

	m_curseForgeView->setModel(m_flameModel);
	m_modrinthView->setModel(m_modrinthModel);

	connect(m_curseForgeView->selectionModel(),
			&QItemSelectionModel::currentChanged, this,
			&DownloadContentDialog::onModSelectionChanged);
	connect(m_modrinthView->selectionModel(),
			&QItemSelectionModel::currentChanged, this,
			&DownloadContentDialog::onModSelectionChanged);
	connect(m_curseForgeView, &QListView::doubleClicked, this,
			&DownloadContentDialog::onModDoubleClicked);
	connect(m_modrinthView, &QListView::doubleClicked, this,
			&DownloadContentDialog::onModDoubleClicked);

	// Trigger initial search
	QTimer::singleShot(0, this, &DownloadContentDialog::triggerSearch);
}

DownloadContentDialog::~DownloadContentDialog() {}

void DownloadContentDialog::setupUi()
{
	QString title =
		tr("Download %1")
			.arg(ModPlatform::contentTypeDisplayName(m_contentType));
	setWindowTitle(title);
	resize(900, 600);

	auto* mainLayout = new QHBoxLayout(this);

	// Left panel: platform selection
	auto* leftPanel = new QVBoxLayout();
	m_platformList = new QListWidget(this);
	m_platformList->addItem(tr("CurseForge"));
	m_platformList->addItem(tr("Modrinth"));
	m_platformList->setCurrentRow(0);
	m_platformList->setMaximumWidth(150);
	m_platformList->setMinimumWidth(120);
	connect(m_platformList, &QListWidget::currentRowChanged, this,
			&DownloadContentDialog::onPlatformChanged);

	// Selected mods section below platform list
	auto* selectedGroup = new QGroupBox(tr("Selected"), this);
	auto* selectedLayout = new QVBoxLayout(selectedGroup);
	m_selectedListWidget = new QListWidget(this);
	m_selectedListWidget->setMaximumWidth(150);
	m_selectedListWidget->setMinimumWidth(120);
	selectedLayout->addWidget(m_selectedListWidget);
	connect(m_selectedListWidget, &QListWidget::doubleClicked, this,
			[this](const QModelIndex& idx) { removeSelectedMod(idx.row()); });

	leftPanel->addWidget(m_platformList);
	leftPanel->addWidget(selectedGroup);

	// Right panel: search + results
	auto* rightPanel = new QVBoxLayout();

	// Search bar
	auto* searchLayout = new QHBoxLayout();
	m_searchEdit = new QLineEdit(this);
	m_searchEdit->setPlaceholderText(
		tr("Search %1...")
			.arg(ModPlatform::contentTypeDisplayName(m_contentType)));
	m_searchEdit->setClearButtonEnabled(true);

	m_sortBox = new QComboBox(this);
	m_sortBox->addItem(tr("Sort by relevance"));
	m_sortBox->addItem(tr("Sort by downloads"));
	m_sortBox->addItem(tr("Sort by updated"));
	m_sortBox->addItem(tr("Sort by newest"));
	m_sortBox->addItem(tr("Sort by follows"));

	searchLayout->addWidget(m_searchEdit, 1);
	searchLayout->addWidget(m_sortBox);

	connect(m_searchEdit, &QLineEdit::returnPressed, this,
			&DownloadContentDialog::triggerSearch);
	connect(m_searchEdit, &QLineEdit::textChanged, this,
			&DownloadContentDialog::onSearchTextChanged);
	connect(m_sortBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &DownloadContentDialog::onSortChanged);

	// Content stack (CurseForge / Modrinth views)
	m_contentStack = new QStackedWidget(this);

	m_curseForgeView = new QListView(this);
	m_curseForgeView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_curseForgeView->setIconSize(QSize(48, 48));
	m_curseForgeView->setSpacing(2);

	m_modrinthView = new QListView(this);
	m_modrinthView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_modrinthView->setIconSize(QSize(48, 48));
	m_modrinthView->setSpacing(2);

	m_contentStack->addWidget(m_curseForgeView);
	m_contentStack->addWidget(m_modrinthView);

	// Mod detail area
	auto* detailLayout = new QHBoxLayout();
	m_descriptionLabel = new QLabel(this);
	m_descriptionLabel->setWordWrap(true);
	m_descriptionLabel->setMaximumHeight(80);
	m_descriptionLabel->setTextFormat(Qt::RichText);
	m_descriptionLabel->setOpenExternalLinks(true);

	m_versionBox = new QComboBox(this);
	m_versionBox->setMinimumWidth(200);
	m_versionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	m_addButton = new QPushButton(tr("Add"), this);
	m_addButton->setEnabled(false);
	connect(m_addButton, &QPushButton::clicked, this, [this] {
		auto currentView =
			(m_currentPlatform == 0) ? m_curseForgeView : m_modrinthView;
		auto idx = currentView->currentIndex();
		if (idx.isValid()) {
			onModDoubleClicked(idx);
		}
	});

	detailLayout->addWidget(m_descriptionLabel, 1);
	detailLayout->addWidget(m_versionBox);
	detailLayout->addWidget(m_addButton);

	// Bottom buttons
	auto* buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();
	m_cancelButton = new QPushButton(tr("Cancel"), this);
	m_continueButton = new QPushButton(tr("Continue"), this);
	m_continueButton->setDefault(true);
	m_continueButton->setEnabled(false);
	connect(m_cancelButton, &QPushButton::clicked, this,
			&DownloadContentDialog::onCancelClicked);
	connect(m_continueButton, &QPushButton::clicked, this,
			&DownloadContentDialog::onContinueClicked);
	buttonLayout->addWidget(m_cancelButton);
	buttonLayout->addWidget(m_continueButton);

	rightPanel->addLayout(searchLayout);
	rightPanel->addWidget(m_contentStack, 1);
	rightPanel->addLayout(detailLayout);
	rightPanel->addLayout(buttonLayout);

	mainLayout->addLayout(leftPanel);
	mainLayout->addLayout(rightPanel, 1);
}

void DownloadContentDialog::onPlatformChanged(int index)
{
	m_currentPlatform = index;
	m_contentStack->setCurrentIndex(index);
	m_versionBox->clear();
	m_descriptionLabel->clear();
	m_addButton->setEnabled(false);
	triggerSearch();
}

void DownloadContentDialog::onSearchTextChanged()
{
	if (m_searchTimer) {
		m_searchTimer->stop();
		m_searchTimer->start();
	}
}

void DownloadContentDialog::onSortChanged(int index)
{
	triggerSearch();
}

void DownloadContentDialog::triggerSearch()
{
	QString term = m_searchEdit->text();
	int sort = m_sortBox->currentIndex();

	if (m_currentPlatform == 0) {
		m_flameModel->searchWithTerm(term, m_mcVersion, m_loaderType, sort);
	} else {
		m_modrinthModel->searchWithTerm(term, m_mcVersion, m_loaderType, sort);
	}
}

void DownloadContentDialog::onModSelectionChanged(const QModelIndex& current,
												  const QModelIndex& previous)
{
	m_versionBox->clear();
	m_descriptionLabel->clear();
	m_addButton->setEnabled(false);

	if (!current.isValid())
		return;

	if (m_currentPlatform == 0) {
		// CurseForge
		auto mod = m_flameModel->modAt(current.row());
		m_descriptionLabel->setText(QString("<b>%1</b> by %2<br>%3")
										.arg(mod.name.toHtmlEscaped(),
											 mod.author.toHtmlEscaped(),
											 mod.description.toHtmlEscaped()));

		m_currentMod.platform = "curseforge";
		m_currentMod.projectId = QString::number(mod.addonId);
		m_currentMod.name = mod.name;
		m_currentMod.addonId = mod.addonId;

		loadVersionsForMod(current);
	} else {
		// Modrinth
		auto mod = m_modrinthModel->modAt(current.row());
		m_descriptionLabel->setText(QString("<b>%1</b> by %2<br>%3")
										.arg(mod.name.toHtmlEscaped(),
											 mod.author.toHtmlEscaped(),
											 mod.description.toHtmlEscaped()));

		m_currentMod.platform = "modrinth";
		m_currentMod.projectId = mod.projectId;
		m_currentMod.name = mod.name;
		m_currentMod.addonId = 0;

		loadVersionsForMod(current);
	}
}

void DownloadContentDialog::loadVersionsForMod(const QModelIndex& index)
{
	m_versionBox->clear();

	if (m_currentMod.platform == "curseforge") {
		auto* response = new QByteArray();
		NetJob* job =
			new NetJob(QString("CF::Versions(%1)").arg(m_currentMod.name),
					   APPLICATION->network());

		QString url = QString("https://api.curseforge.com/v1/mods/%1/files")
						  .arg(m_currentMod.projectId);
		QStringList cfParams;
		if (!m_mcVersion.isEmpty()) {
			cfParams << "gameVersion=" + m_mcVersion;
		}
		if (!m_loaderType.isEmpty()) {
			// Only filter by loader for mods - resource packs and shaders are
			// loader-agnostic
			if (m_contentType == ModPlatform::ContentType::Mod) {
				int loaderType =
					ModPlatform::loaderToCurseForgeModLoaderType(m_loaderType);
				if (loaderType > 0) {
					cfParams << "modLoaderType=" + QString::number(loaderType);
				}
			}
		}
		if (!cfParams.isEmpty()) {
			url += "?" + cfParams.join("&");
		}
		job->addNetAction(Net::Download::makeByteArray(QUrl(url), response));

		connect(job, &NetJob::succeeded, this, [this, response, job] {
			job->deleteLater();
			QJsonDocument doc = QJsonDocument::fromJson(*response);
			delete response;

			QJsonArray arr;
			if (doc.isObject() && doc.object().contains("data")) {
				arr = doc.object().value("data").toArray();
			} else {
				arr = doc.array();
			}

			for (auto fileRaw : arr) {
				auto fileObj = fileRaw.toObject();
				QString displayName =
					Json::ensureString(fileObj, "displayName", "");
				QString fileName = Json::ensureString(fileObj, "fileName", "");
				int fileId = Json::ensureInteger(fileObj, "id", 0);
				QString downloadUrl =
					Json::ensureString(fileObj, "downloadUrl", "");

				// Handle restricted downloads (downloadUrl is null)
				if (downloadUrl.isEmpty() && fileId > 0 &&
					!fileName.isEmpty()) {
					downloadUrl = QString("https://www.curseforge.com/api/v1/"
										  "mods/%1/files/%2/download")
									  .arg(m_currentMod.projectId)
									  .arg(fileId);
				}

				if (displayName.isEmpty())
					displayName = fileName;

				QVariantMap data;
				data["fileId"] = fileId;
				data["fileName"] = fileName;
				data["downloadUrl"] = downloadUrl;
				m_versionBox->addItem(displayName, QVariant(data));
			}

			bool alreadySelected = false;
			for (const auto& existing : m_selectedMods) {
				if (normalizeModName(existing.name) ==
					normalizeModName(m_currentMod.name)) {
					alreadySelected = true;
					break;
				}
			}
			m_addButton->setEnabled(m_versionBox->count() > 0 &&
									!alreadySelected);
		});
		connect(job, &NetJob::failed, this, [response, job](QString) {
			job->deleteLater();
			delete response;
		});
		job->start();

	} else if (m_currentMod.platform == "modrinth") {
		auto* response = new QByteArray();
		NetJob* job =
			new NetJob(QString("MR::Versions(%1)").arg(m_currentMod.name),
					   APPLICATION->network());

		QString url = QString("https://api.modrinth.com/v2/project/%1/version")
						  .arg(m_currentMod.projectId);
		QStringList params;
		if (!m_mcVersion.isEmpty()) {
			params << QString("game_versions=[\"%1\"]").arg(m_mcVersion);
		}
		// Only filter by loader for mods - resource packs and shaders are
		// loader-agnostic
		if (!m_loaderType.isEmpty() &&
			m_contentType == ModPlatform::ContentType::Mod) {
			params << QString("loaders=[\"%1\"]").arg(m_loaderType);
		}
		if (!params.isEmpty()) {
			url += "?" + params.join("&");
		}
		job->addNetAction(Net::Download::makeByteArray(QUrl(url), response));

		connect(job, &NetJob::succeeded, this, [this, response, job] {
			job->deleteLater();
			QJsonDocument doc = QJsonDocument::fromJson(*response);
			delete response;

			if (!doc.isArray())
				return;

			for (auto versionRaw : doc.array()) {
				auto vObj = versionRaw.toObject();
				QString name = Json::ensureString(vObj, "name", "");
				QString versionNumber =
					Json::ensureString(vObj, "version_number", "");
				QString versionId = Json::ensureString(vObj, "id", "");

				QString downloadUrl;
				QString fileName;
				QString sha1;
				int fileSize = 0;
				auto files = Json::ensureArray(vObj, "files");
				for (auto fileRaw : files) {
					auto fileObj = fileRaw.toObject();
					bool primary =
						Json::ensureBoolean(fileObj, "primary", false);
					if (primary || files.size() == 1) {
						downloadUrl = Json::ensureString(fileObj, "url", "");
						fileName = Json::ensureString(fileObj, "filename", "");
						fileSize = Json::ensureInteger(fileObj, "size", 0);
						auto hashes = Json::ensureObject(fileObj, "hashes");
						sha1 = Json::ensureString(hashes, "sha1", "");
						break;
					}
				}

				if (downloadUrl.isEmpty())
					continue;

				QString displayText =
					name.isEmpty()
						? versionNumber
						: QString("%1 (%2)").arg(name, versionNumber);

				QVariantMap data;
				data["versionId"] = versionId;
				data["fileName"] = fileName;
				data["downloadUrl"] = downloadUrl;
				data["sha1"] = sha1;
				data["fileSize"] = fileSize;
				m_versionBox->addItem(displayText, QVariant(data));
			}

			bool alreadySelected = false;
			for (const auto& existing : m_selectedMods) {
				if (normalizeModName(existing.name) ==
					normalizeModName(m_currentMod.name)) {
					alreadySelected = true;
					break;
				}
			}
			m_addButton->setEnabled(m_versionBox->count() > 0 &&
									!alreadySelected);
		});
		connect(job, &NetJob::failed, this, [response, job](QString) {
			job->deleteLater();
			delete response;
		});
		job->start();
	}
}

void DownloadContentDialog::onModDoubleClicked(const QModelIndex& index)
{
	if (m_versionBox->count() == 0)
		return;

	auto data = m_versionBox->currentData().toMap();

	ModPlatform::SelectedMod mod;
	mod.name = m_currentMod.name;
	mod.platform = m_currentMod.platform;
	mod.projectId = m_currentMod.projectId;
	mod.mcVersion = m_mcVersion;
	mod.loaders = m_loaderType;

	if (m_currentMod.platform == "curseforge") {
		mod.versionId = QString::number(data["fileId"].toInt());
		mod.fileName = data["fileName"].toString();
		mod.downloadUrl = data["downloadUrl"].toString();
	} else {
		mod.versionId = data["versionId"].toString();
		mod.fileName = data["fileName"].toString();
		mod.downloadUrl = data["downloadUrl"].toString();
		mod.sha1 = data["sha1"].toString();
		mod.fileSize = data["fileSize"].toInt();
	}

	// Don't add duplicates (check by normalized name across platforms)
	for (const auto& existing : m_selectedMods) {
		if (normalizeModName(existing.name) == normalizeModName(mod.name)) {
			return;
		}
	}

	m_selectedMods.append(mod);
	updateSelectedList();
	m_addButton->setEnabled(false); // just added this mod, disable Add
}

void DownloadContentDialog::removeSelectedMod(int index)
{
	if (index >= 0 && index < m_selectedMods.size()) {
		QString removedName = m_selectedMods[index].name;
		m_selectedMods.removeAt(index);
		updateSelectedList();
		// Re-enable Add if the removed mod matches the currently viewed mod
		if (removedName == m_currentMod.name && m_versionBox->count() > 0) {
			m_addButton->setEnabled(true);
		}
	}
}

void DownloadContentDialog::updateSelectedList()
{
	m_selectedListWidget->clear();
	for (const auto& mod : m_selectedMods) {
		QString prefix = (mod.platform == "curseforge") ? "[CF] " : "[MR] ";
		m_selectedListWidget->addItem(prefix + mod.name);
	}
	m_continueButton->setEnabled(!m_selectedMods.isEmpty());
}

void DownloadContentDialog::onContinueClicked()
{
	accept();
}

void DownloadContentDialog::onCancelClicked()
{
	reject();
}
