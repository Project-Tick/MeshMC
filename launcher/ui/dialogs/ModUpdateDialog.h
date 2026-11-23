/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QDialog>
#include <QList>
#include <QTreeWidget>

#include "modplatform/ModDownloadTypes.h"
#include "modplatform/ModUpdateCheckTask.h"

/*
 * ModUpdateDialog
 *
 * Shows the user the set of mods for which a newer compatible version was
 * found, lets them tick which updates to apply, and produces a list of
 * DownloadItem objects ready to be fed into a ContentDownloadTask.
 */
class ModUpdateDialog : public QDialog
{
	Q_OBJECT
  public:
	ModUpdateDialog(const QList<ModUpdateCheckTask::UpdateInfo>& updates,
					QWidget* parent = nullptr);

	/* Returns only the items whose row is checked. */
	QList<ModPlatform::DownloadItem> selectedDownloadItems() const;

  private:
	void setupUi(const QList<ModUpdateCheckTask::UpdateInfo>& updates);

	QTreeWidget* m_tree = nullptr;
	QList<ModUpdateCheckTask::UpdateInfo> m_updates;
};
