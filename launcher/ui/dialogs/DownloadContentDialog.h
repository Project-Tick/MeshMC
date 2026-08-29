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

#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QList>
#include <QString>
#include <QUrl>
#include <QVBoxLayout>
#include <memory>

#include "minecraft/MinecraftInstance.h"
#include "modplatform/ContentType.h"
#include "modplatform/ModDownloadTypes.h"
#include "ui/pages/BasePageProvider.h"

class ContentProviderPage;
class ModMetadataIndex;
class PageContainer;

/* Browse CurseForge and Modrinth for one kind of content and build up a
 * list of things to download.
 *
 * The dialog is only the frame: each provider is a page inside a
 * PageContainer, exactly like the settings window, and the pages do the
 * searching. What the dialog owns is the queue - one list shared by all
 * pages, so the same mod cannot be picked up twice just because it is
 * published on both sites - plus the buttons and the window geometry.
 *
 * Nothing is downloaded here. On accept the caller reads selectedMods()
 * and takes it from there (dependency resolution, review, download). */
class DownloadContentDialog final : public QDialog, public BasePageProvider
{
	Q_OBJECT

  public:
	/* `suppressInitialSearch` keeps the pages from searching the moment
	 * they are shown. Set it when the dialog is going to be handed to
	 * openForVersionChange(), whose lookup replaces those results
	 * anyway - two requests where one will do, and on a slow connection
	 * the discarded one is the reply that arrives last. */
	explicit DownloadContentDialog(MinecraftInstance* instance,
								   ModPlatform::ContentType contentType,
								   QWidget* parent = nullptr,
								   bool suppressInitialSearch = false);
	~DownloadContentDialog() override;

	/* Turns the whole dialog into a version picker for one project that
	 * is already installed: the provider list goes away, so do the
	 * dialog's own buttons, and the provider's page grows its own
	 * Reinstall/Cancel pair.
	 *
	 * Returns false when no page serves that platform - a file recorded
	 * as coming from somewhere we cannot browse has no version list to
	 * offer, and the caller should say so rather than show an empty
	 * window. */
	bool openForVersionChange(const QString& platform,
							  const QString& projectId, const QString& name);

	/* BasePageProvider */
	QList<BasePage*> getPages() override;
	QString dialogTitle() override;

	/* What the target instance already has. Matching rows are faded out
	 * and tagged, and the version they are on is marked in the version
	 * box. They stay selectable: picking a different version of an
	 * installed project is how it gets changed. */
	void setInstalledIndex(std::shared_ptr<ModMetadataIndex> index);

	QList<ModPlatform::SelectedMod> selectedMods() const
	{
		return m_queue;
	}
	QString mcVersion() const
	{
		return m_mcVersion;
	}
	QString loaderType() const
	{
		return m_loaderType;
	}

	/* Lower-case nouns for button and placeholder text. */
	QString contentNoun() const;
	QString contentsNoun() const;

	ModPlatform::ContentType contentType() const
	{
		return m_contentType;
	}

	/* Queue handling, called by the pages. Matching is by name rather
	 * than project id, so the same mod from the two providers counts
	 * once. */
	bool isNameQueued(const QString& name) const;
	void queueContent(const ModPlatform::SelectedMod& mod);
	void unqueueContent(const QString& name);

	/* Version of `projectId` already installed, or an empty string. Used
	 * to tag it in the version list. */
	QString installedVersionId(const QString& platform,
							   const QString& projectId) const;

	/* If `url` points at a project page on one of the providers - the
	 * kind of link people paste out of a browser - switch to that
	 * provider and look the project up. Returns false when the link is
	 * something else, so the caller can hand it to the web browser. */
	bool openProjectLink(const QUrl& url);

	void accept() override;
	void reject() override;

  private slots:
	void onPageChanged(BasePage* previous, BasePage* selected);

  private:
	void detectInstanceProfile();
	void buildPages();
	/* Hands the queue to every page's model, so rows can draw the tick,
	 * and refreshes the pages' buttons. */
	void queueChanged();
	void saveGeometryState();
	QString geometrySaveKey() const;

  private:
	MinecraftInstance* m_instance;
	ModPlatform::ContentType m_contentType;
	QString m_mcVersion;
	QString m_loaderType;
	bool m_suppressInitialSearch;
	std::shared_ptr<ModMetadataIndex> m_installedIndex;

	QList<ModPlatform::SelectedMod> m_queue;

	QList<ContentProviderPage*> m_pages;
	PageContainer* m_container = nullptr;
	QDialogButtonBox m_buttons;
	QVBoxLayout m_layout;
};
