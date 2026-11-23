/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QDialog>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>

struct BlockedMod {
	int projectId;
	int fileId;
	QString fileName;
	QString targetPath;
	bool found = false;
};

class BlockedModsDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit BlockedModsDialog(QWidget* parent, const QString& title,
							   const QString& text, QList<BlockedMod>& mods);

	/// Returns the list of mods with updated `found` status
	QList<BlockedMod>& resultMods()
	{
		return m_mods;
	}

  private slots:
	void onDownloadDirChanged(const QString& path);
	void openModDownload(int index);

  private:
	void scanDownloadsFolder();
	void updateModStatus();
	void setupWatch();

	QList<BlockedMod>& m_mods;
	QString m_downloadDir;
	QFileSystemWatcher m_watcher;

	struct ModRow {
		QLabel* nameLabel;
		QLabel* statusLabel;
		QPushButton* downloadButton;
	};
	QList<ModRow> m_rows;

	QDialogButtonBox* m_buttons;
};
