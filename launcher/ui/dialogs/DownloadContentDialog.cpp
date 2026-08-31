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

#include "DownloadContentDialog.h"

#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <utility>

#include "Application.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModMetadataIndex.h"
#include "modplatform/flame/FlameContentModel.h"
#include "modplatform/modrinth/ModrinthContentModel.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/pages/modplatform/ContentProviderPage.h"
#include "ui/widgets/PageContainer.h"

namespace
{

	struct UrlHandler {
		/* Matched against "<host><path>", so no scheme. */
		QString pattern;
		QString providerId;
	};

	/* Project page addresses, which differ per kind of content: a mod
	 * lives under modrinth.com/mod/, a shader under modrinth.com/shader/
	 * and so on. Pasting the wrong kind of link should fall through to
	 * the web browser rather than search for nonsense. */
	QList<UrlHandler> urlHandlersFor(ModPlatform::ContentType type)
	{
		QString modrinthPath;
		QString curseForgePath;

		switch (type) {
			case ModPlatform::ContentType::Mod:
				modrinthPath = "mod";
				curseForgePath = "mc-mods";
				break;
			case ModPlatform::ContentType::ResourcePack:
				modrinthPath = "resourcepack";
				curseForgePath = "texture-packs";
				break;
			case ModPlatform::ContentType::ShaderPack:
				modrinthPath = "shader";
				curseForgePath = "shaders";
				break;
			case ModPlatform::ContentType::DataPack:
				modrinthPath = "datapack";
				curseForgePath = "data-packs";
				break;
		}

		return {
			{QString(R"((?:www\.)?modrinth\.com/%1/([^/]+)/?)")
				 .arg(modrinthPath),
			 QStringLiteral("modrinth")},
			{QString(R"((?:www\.)?curseforge\.com/minecraft/%1/([^/]+)/?)")
				 .arg(curseForgePath),
			 QStringLiteral("curseforge")},
			/* The old CurseForge address, still handed out by search
			 * engines and old forum posts. */
			{QString(R"(minecraft\.curseforge\.com/projects/([^/]+)/?)"),
			 QStringLiteral("curseforge")},
		};
	}

} // namespace

DownloadContentDialog::DownloadContentDialog(
	MinecraftInstance* instance, ModPlatform::ContentType contentType,
	QWidget* parent, bool suppressInitialSearch)
	: QDialog(parent), m_instance(instance), m_contentType(contentType),
	  m_suppressInitialSearch(suppressInitialSearch),
	  m_buttons(QDialogButtonBox::Help | QDialogButtonBox::Ok |
				QDialogButtonBox::Cancel),
	  m_layout(this)
{
	setObjectName(QStringLiteral("DownloadContentDialog"));
	setWindowTitle(dialogTitle());
	setWindowIcon(APPLICATION->getThemedIcon("new"));
	setWindowModality(Qt::WindowModal);

	/* Half the parent's width and three quarters of its height, which is
	 * roughly what this needs to show a list and a description side by
	 * side, but never smaller than usable. */
	if (parent != nullptr) {
		resize(std::max(parent->width() / 2, 700),
			   std::max((parent->height() * 3) / 4, 500));
	} else {
		resize(900, 600);
	}

	detectInstanceProfile();

	auto* okButton = m_buttons.button(QDialogButtonBox::Ok);
	okButton->setText(tr("Review and confirm"));
	okButton->setToolTip(
		tr("Close this window and review the %1 you picked before anything "
		   "is downloaded. Shortcut: Ctrl+Return")
			.arg(contentsNoun()));
	okButton->setShortcut(tr("Ctrl+Return"));
	okButton->setDefault(true);
	okButton->setAutoDefault(true);
	okButton->setEnabled(false);

	auto* cancelButton = m_buttons.button(QDialogButtonBox::Cancel);
	cancelButton->setDefault(false);
	cancelButton->setAutoDefault(false);

	auto* helpButton = m_buttons.button(QDialogButtonBox::Help);
	helpButton->setDefault(false);
	helpButton->setAutoDefault(false);

	buildPages();

	connect(okButton, &QPushButton::clicked, this,
			&DownloadContentDialog::accept);
	connect(cancelButton, &QPushButton::clicked, this,
			&DownloadContentDialog::reject);
	connect(helpButton, &QPushButton::clicked, m_container,
			&PageContainer::help);

	const QString saved =
		APPLICATION->settings()->get(geometrySaveKey()).toString();
	if (!saved.isEmpty()) {
		restoreGeometry(QByteArray::fromBase64(saved.toUtf8()));
	}
}

DownloadContentDialog::~DownloadContentDialog() {}

void DownloadContentDialog::detectInstanceProfile()
{
	auto profile = m_instance->getPackProfile();
	if (!profile) {
		return;
	}

	m_mcVersion = profile->getComponentVersion("net.minecraft");

	if (profile->getComponent("net.minecraftforge")) {
		m_loaderType = "forge";
	} else if (profile->getComponent("net.fabricmc.fabric-loader")) {
		m_loaderType = "fabric";
	} else if (profile->getComponent("org.quiltmc.quilt-loader")) {
		m_loaderType = "quilt";
	} else if (profile->getComponent("net.neoforged")) {
		/* NeoForge's component uid is "net.neoforged" - what
		 * InstanceImportTask / FTB / ATL / Technic write and what
		 * VersionPage reads. The longer "net.neoforged.neoforge" never
		 * matched, which left the loader filter empty here. */
		m_loaderType = "neoforge";
	}
}

void DownloadContentDialog::buildPages()
{
	/* An instance with no loader detected searches without a loader
	 * filter rather than with an empty one, and content that is not
	 * loader-specific never carries one - otherwise the filter panel,
	 * which shows no loader boxes for those, would look like it had
	 * cleared the loader the moment anything else was touched, and the
	 * search would be run again for nothing. */
	ModPlatform::SearchFilters filters;
	filters.mcVersions = ModPlatform::singleVersionList(m_mcVersion);
	if (!m_loaderType.isEmpty() &&
		ModPlatform::contentTypeUsesLoader(m_contentType)) {
		filters.loaders = QStringList{m_loaderType};
	}
	/* side and openSourceOnly start off: the panel's boxes start
	 * unticked, and the two have to agree or the first touch of any
	 * other box would look like a change and search again. */

	auto* flameModel = new FlameContentModel(m_contentType, filters);
	auto* modrinthModel = new ModrinthContentModel(m_contentType, filters);

	/* Modrinth first, as the reference launcher does: it is the one that
	 * hands out download links without conditions. */
	m_pages.append(new ContentProviderPage(
		this, modrinthModel, APPLICATION->getThemedIcon("modrinth")));
	m_pages.append(new ContentProviderPage(
		this, flameModel, APPLICATION->getThemedIcon("flame")));

	/* Before the container exists, because building it selects a page,
	 * and selecting a page is what would start that search. */
	for (auto* page : m_pages) {
		page->setSuppressInitialSearch(m_suppressInitialSearch);
	}

	m_layout.setContentsMargins(0, 0, 0, 0);

	m_container = new PageContainer(this, QString(), this);
	m_container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	m_container->layout()->setContentsMargins(0, 0, 0, 0);
	m_layout.addWidget(m_container);

	m_buttons.setContentsMargins(0, 0, 6, 6);
	m_container->addButtons(&m_buttons);

	connect(m_container, &PageContainer::selectedPageChanged, this,
			&DownloadContentDialog::onPageChanged);
}

QList<BasePage*> DownloadContentDialog::getPages()
{
	QList<BasePage*> pages;
	pages.reserve(m_pages.size());
	for (auto* page : m_pages) {
		pages.append(page);
	}
	return pages;
}

QString DownloadContentDialog::dialogTitle()
{
	return tr("Download %1")
		.arg(ModPlatform::contentTypeDisplayName(m_contentType));
}

QString DownloadContentDialog::contentNoun() const
{
	return ModPlatform::contentTypeNoun(m_contentType);
}

QString DownloadContentDialog::contentsNoun() const
{
	return ModPlatform::contentTypeNounPlural(m_contentType);
}

void DownloadContentDialog::setInstalledIndex(
	std::shared_ptr<ModMetadataIndex> index)
{
	m_installedIndex = std::move(index);
	for (auto* page : m_pages) {
		page->setInstalledIndex(m_installedIndex);
	}
}

QString
DownloadContentDialog::installedVersionId(const QString& platform,
										  const QString& projectId) const
{
	if (!m_installedIndex || platform.isEmpty() || projectId.isEmpty()) {
		return QString();
	}
	return m_installedIndex->findByPlatformProject(platform, projectId)
		.versionId;
}

bool DownloadContentDialog::openForVersionChange(const QString& platform,
												 const QString& projectId,
												 const QString& name)
{
	ContentProviderPage* target = nullptr;
	for (auto* page : m_pages) {
		if (page->id() == platform) {
			target = page;
			break;
		}
	}
	if (target == nullptr || projectId.isEmpty()) {
		return false;
	}

	m_container->selectPage(platform);
	setWindowTitle(tr("Change %1 version").arg(name));
	/* No provider to choose any more, and the dialog's own buttons are
	 * replaced by the page's Reinstall/Cancel pair. */
	m_container->hidePageList();
	m_buttons.hide();
	target->openProject(projectId);
	return true;
}

bool DownloadContentDialog::openProjectLink(const QUrl& url)
{
	const QString address = url.host() + url.path();

	for (const auto& handler : urlHandlersFor(m_contentType)) {
		const QRegularExpression expression(
			QRegularExpression::anchoredPattern(handler.pattern));
		const auto match = expression.match(address);
		if (!match.hasMatch()) {
			continue;
		}

		const QString slug = match.captured(1);
		if (slug.isEmpty()) {
			continue;
		}

		for (auto* page : m_pages) {
			if (page->id() != handler.providerId) {
				continue;
			}
			/* The link may well point at the provider the user is not
			 * currently looking at. */
			m_container->selectPage(handler.providerId);
			page->openProjectSlug(slug);
			return true;
		}
	}

	return false;
}

bool DownloadContentDialog::isNameQueued(const QString& name) const
{
	const QString normalized = ModMetadataIndex::normalizeName(name);
	for (const auto& queued : m_queue) {
		if (ModMetadataIndex::normalizeName(queued.name) == normalized) {
			return true;
		}
	}
	return false;
}

void DownloadContentDialog::queueContent(const ModPlatform::SelectedMod& mod)
{
	if (isNameQueued(mod.name)) {
		return;
	}

	/* Something already installed is allowed into the queue on purpose.
	 * That is what changing a version looks like from here, and the
	 * conflict analyzer decides afterwards whether the file has to be
	 * replaced or the pick was a no-op. Refusing it here would make the
	 * checkbox on an installed row a control that does nothing. */

	m_queue.append(mod);
	queueChanged();
}

void DownloadContentDialog::unqueueContent(const QString& name)
{
	const QString normalized = ModMetadataIndex::normalizeName(name);
	for (int i = 0; i < m_queue.size(); ++i) {
		if (ModMetadataIndex::normalizeName(m_queue.at(i).name) == normalized) {
			m_queue.removeAt(i);
			queueChanged();
			return;
		}
	}
}

void DownloadContentDialog::queueChanged()
{
	QSet<QString> names;
	names.reserve(m_queue.size());
	for (const auto& queued : m_queue) {
		names.insert(ModMetadataIndex::normalizeName(queued.name));
	}

	/* Every page, not just the visible one: a mod queued from one site
	 * shows as ticked on the other too. */
	for (auto* page : m_pages) {
		page->queueChanged(names);
	}

	m_buttons.button(QDialogButtonBox::Ok)->setEnabled(!m_queue.isEmpty());
}

void DownloadContentDialog::onPageChanged(BasePage* previous,
										  BasePage* selected)
{
	auto* previousPage = dynamic_cast<ContentProviderPage*>(previous);
	auto* selectedPage = dynamic_cast<ContentProviderPage*>(selected);
	if (previousPage == nullptr || selectedPage == nullptr) {
		return;
	}

	/* Carry the term across, so the two providers feel like one search
	 * bar rather than two independent ones. */
	selectedPage->setSearchTerm(previousPage->searchTerm());
}

QString DownloadContentDialog::geometrySaveKey() const
{
	switch (m_contentType) {
		case ModPlatform::ContentType::Mod:
			return QStringLiteral("ModDownloadGeometry");
		case ModPlatform::ContentType::ResourcePack:
			return QStringLiteral("RPDownloadGeometry");
		case ModPlatform::ContentType::ShaderPack:
			return QStringLiteral("ShaderDownloadGeometry");
		case ModPlatform::ContentType::DataPack:
			return QStringLiteral("DataPackDownloadGeometry");
	}
	return QStringLiteral("ModDownloadGeometry");
}

void DownloadContentDialog::saveGeometryState()
{
	APPLICATION->settings()->set(
		geometrySaveKey(), QString::fromUtf8(saveGeometry().toBase64()));
}

void DownloadContentDialog::accept()
{
	saveGeometryState();
	QDialog::accept();
}

void DownloadContentDialog::reject()
{
	if (!m_queue.isEmpty()) {
		const auto reply =
			CustomMessageBox::selectable(
				this, tr("Confirmation Needed"),
				tr("You have %1 selected %2.\n"
				   "Are you sure you want to close this dialog?")
					.arg(m_queue.size())
					.arg(contentsNoun()),
				QMessageBox::Question, QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No)
				->exec();
		if (reply != QMessageBox::Yes) {
			return;
		}
	}

	saveGeometryState();
	QDialog::reject();
}
