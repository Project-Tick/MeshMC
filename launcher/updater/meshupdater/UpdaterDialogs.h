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

#pragma once

#include <QDialog>

#include "GitHubRelease.h"
#include "Version.h"

class QTreeWidgetItem;

namespace Ui
{
	class SelectReleaseDialog;
}

/*!
 * "Which release would you like?" -- shown for --select-ui, and whenever the
 * updater is asked to install something other than simply the newest thing.
 *
 * The list is a tree rather than a combo box because the release notes are
 * part of the decision: picking a version updates the pane below it, so the
 * user chooses on the strength of what changed and not just a number.
 */
class SelectReleaseDialog : public QDialog
{
	Q_OBJECT

  public:
	SelectReleaseDialog(const Version& currentVersion,
						const QList<GitHubRelease>& releases,
						QWidget* parent = nullptr);
	~SelectReleaseDialog() override;

	/*!
	 * The release the user settled on.
	 *
	 * Invalid (isValid() == false) when the dialog was cancelled, or when it
	 * was accepted without anything selected -- the caller checks, rather
	 * than this class inventing a default the user never chose.
	 */
	GitHubRelease selectedRelease() const
	{
		return m_selectedRelease;
	}

  private slots:
	void selectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

  private:
	void loadReleases();
	void appendRelease(const GitHubRelease& release);
	GitHubRelease releaseForItem(const QTreeWidgetItem* item) const;

	QList<GitHubRelease> m_releases;
	GitHubRelease m_selectedRelease;
	Version m_currentVersion;

	Ui::SelectReleaseDialog* ui;
};

/*!
 * "Which of these files?" -- only reached when one release carries several
 * artifacts that all match this installation.
 *
 * That is rare, and when it happens guessing is worse than asking: the
 * candidates differ in ways (a portable archive versus an installer, one Qt
 * build versus another) that change what the user ends up with.
 *
 * Shares the release dialog's layout with the notes pane hidden -- an asset
 * has no release notes of its own, and inventing a second layout for a rare
 * dialog is how two dialogs end up looking gratuitously different.
 */
class SelectReleaseAssetDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit SelectReleaseAssetDialog(const QList<GitHubReleaseAsset>& assets,
									  QWidget* parent = nullptr);
	~SelectReleaseAssetDialog() override;

	GitHubReleaseAsset selectedAsset() const
	{
		return m_selectedAsset;
	}

  private slots:
	void selectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

  private:
	void loadAssets();
	void appendAsset(const GitHubReleaseAsset& asset);
	GitHubReleaseAsset assetForItem(const QTreeWidgetItem* item) const;

	QList<GitHubReleaseAsset> m_assets;
	GitHubReleaseAsset m_selectedAsset;

	Ui::SelectReleaseDialog* ui;
};
