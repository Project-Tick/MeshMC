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
 */

#include "FlamePage.h"
#include "ui_FlamePage.h"

#include <QKeyEvent>

#include "Application.h"
#include "Json.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "InstanceImportTask.h"
#include "FlameModel.h"
#include "modplatform/flame/FlameApi.h"

FlamePage::FlamePage(NewInstanceDialog* dialog, QWidget* parent)
	: QWidget(parent), ui(new Ui::FlamePage), dialog(dialog)
{
	ui->setupUi(this);
	connect(ui->searchButton, &QPushButton::clicked, this,
			&FlamePage::triggerSearch);
	ui->searchEdit->installEventFilter(this);
	listModel = new Flame::ListModel(this);
	ui->packView->setModel(listModel);

	ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(
		Qt::ScrollBarAsNeeded);
	ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

	/* Filled from the provider itself: the combo index is handed
	 * straight back to FlameApi, so the two must not be allowed to
	 * drift apart. */
	for (const auto& sorting : FlameApi::get().sortingMethods()) {
		ui->sortByBox->addItem(sorting.readableName);
	}

	connect(ui->sortByBox, &QComboBox::currentIndexChanged, this,
			&FlamePage::triggerSearch);
	connect(ui->packView->selectionModel(),
			&QItemSelectionModel::currentChanged, this,
			&FlamePage::onSelectionChanged);
	connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this,
			&FlamePage::onVersionSelectionChanged);
}

FlamePage::~FlamePage()
{
	delete ui;
}

bool FlamePage::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Return) {
			triggerSearch();
			keyEvent->accept();
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}

bool FlamePage::shouldDisplay() const
{
	return true;
}

void FlamePage::openedImpl()
{
	suggestCurrent();
	triggerSearch();
}

void FlamePage::triggerSearch()
{
	listModel->searchWithTerm(ui->searchEdit->text(),
							  ui->sortByBox->currentIndex());
}

void FlamePage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
	ui->versionSelectionBox->clear();

	if (!first.isValid()) {
		if (isOpened) {
			dialog->setSuggestedPack();
		}
		return;
	}

	current = listModel->data(first, Qt::UserRole).value<Flame::IndexedPack>();
	QString text = "";
	QString name = current.name;

	if (current.websiteUrl.isEmpty())
		text = name;
	else
		text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";
	if (!current.authors.empty()) {
		auto authorToStr = [](Flame::ModpackAuthor& author) {
			if (author.url.isEmpty()) {
				return author.name;
			}
			return QString("<a href=\"%1\">%2</a>")
				.arg(author.url, author.name);
		};
		QStringList authorStrs;
		for (auto& author : current.authors) {
			authorStrs.push_back(authorToStr(author));
		}
		text += "<br>" + tr(" by ") + authorStrs.join(", ");
	}
	text += "<br><br>";

	ui->packDescription->setHtml(text + current.description);

	if (isOpened) {
		dialog->setSuggestedPack(current.name);
	}

	if (current.versionsLoaded == false) {
		qDebug() << "Loading flame modpack versions";
		NetJob* netJob =
			new NetJob(QString("Flame::PackVersions(%1)").arg(current.name),
					   APPLICATION->network());
		std::shared_ptr<QByteArray> response = std::make_shared<QByteArray>();
		int addonId = current.addonId;
		netJob->addNetAction(Net::Download::makeByteArray(
			FlameApi::allProjectFilesUrl(QString::number(addonId)),
			response.get()));

		QObject::connect(netJob, &NetJob::succeeded, this, [this, response] {
			QJsonParseError parse_error;
			QJsonDocument doc =
				QJsonDocument::fromJson(*response, &parse_error);
			if (parse_error.error != QJsonParseError::NoError) {
				qWarning()
					<< "Error while parsing JSON response from CurseForge at "
					<< parse_error.offset
					<< " reason: " << parse_error.errorString();
				qWarning() << *response;
				return;
			}
			QJsonArray arr;
			if (doc.isObject() && doc.object().contains("data")) {
				arr = doc.object().value("data").toArray();
			} else {
				arr = doc.array();
			}
			try {
				Flame::loadIndexedPackVersions(current, arr);
			} catch (const JSONValidationError& e) {
				qDebug() << *response;
				qWarning() << "Error while reading flame modpack version: "
						   << e.cause();
			}

			for (auto version : current.versions) {
				ui->versionSelectionBox->addItem(version.version,
												 QVariant(version.downloadUrl));
			}

			suggestCurrent();
		});
		netJob->start();
	} else {
		for (auto version : current.versions) {
			ui->versionSelectionBox->addItem(version.version,
											 QVariant(version.downloadUrl));
		}

		suggestCurrent();
	}
}

void FlamePage::suggestCurrent()
{
	if (!isOpened) {
		return;
	}

	if (selectedVersionIndex < 0 ||
		selectedVersionIndex >= current.versions.size()) {
		dialog->setSuggestedPack();
		return;
	}

	auto& version = current.versions[selectedVersionIndex];

	/* Build pack source hint from the browser's own state — same
	 * thing PackUpdater would otherwise have to guess from the
	 * downloaded manifest.json. CurseForge has no slug concept; we
	 * fall back to the addonId both as `packId` and (for display)
	 * as `packSlug`. */
	InstanceImportTask::PackSourceHint hint;
	hint.provider = QStringLiteral("curseforge");
	hint.packId = QString::number(version.addonId);
	hint.versionId = QString::number(version.fileId);
	hint.versionLabel = version.version;
	hint.iconUrl = current.logoUrl;
	hint.sourceUrl = current.websiteUrl;

	QString downloadUrl = version.downloadUrl;
	if (downloadUrl.isEmpty()) {
		// Restricted download — construct CurseForge browser download URL
		// This URL triggers a browser download when opened, respecting ToS
		downloadUrl =
			FlameApi::browserDownloadUrl(QString::number(version.addonId),
										 QString::number(version.fileId));
		qDebug() << "Pack has no API download URL, using browser download URL:"
				 << downloadUrl;
	}
	auto* task = new InstanceImportTask(downloadUrl);
	/* The launcher chose this download itself, from the catalogue the
	 * user is browsing, so the mod files come from CurseForge's own CDN.
	 * There is nothing here the user has not already picked. */
	task->setTrustedSource(true);
	/* The task may have to ask something before it can finish - whether
	 * to update an instance this pack is already installed in, most
	 * likely - and a question about what the user is doing in this window
	 * belongs to this window. */
	task->setDialogParent(this);
	task->setPackSourceHint(hint);
	dialog->setSuggestedPack(current.name, task);

	QString editedLogoName;
	editedLogoName = "curseforge_" + current.logoName.section(".", 0, 0);
	listModel->getLogo(current.logoName, current.logoUrl,
					   [this, editedLogoName](QString logo) {
						   dialog->setSuggestedIconFromFile(logo,
															editedLogoName);
					   });
}

void FlamePage::onVersionSelectionChanged(QString data)
{
	if (data.isNull() || data.isEmpty()) {
		selectedVersion = "";
		selectedVersionIndex = -1;
		return;
	}
	selectedVersion = ui->versionSelectionBox->currentData().toString();
	selectedVersionIndex = ui->versionSelectionBox->currentIndex();
	suggestCurrent();
}
