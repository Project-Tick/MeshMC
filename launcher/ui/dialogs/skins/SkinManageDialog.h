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
#include <QHash>
#include <QImage>
#include <QItemSelection>
#include <QPixmap>
#include <QPointer>
#include <QString>

#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/skins/SkinEntry.h"
#include "minecraft/skins/SkinLibrary.h"
#include "ui/dialogs/skins/render/SkinPreviewSurface.h"

class QLabel;

namespace Ui
{
	class SkinManageDialog;
}

/* Pick, edit and upload a skin.
 *
 * The dialog is the skin library's editor as well as its uploader: the local
 * list on the right is the library, and the controls on the left edit the
 * *selected* entry in place -- switching arm width or cape writes straight
 * into the entry, and the index is saved with it. Accepting uploads whatever
 * the selection currently says.
 *
 * Also the SkinPreviewSource for the 3D preview, which asks back for the
 * selection when its GL context comes up.
 */
class SkinManageDialog : public QDialog, public skinrender::SkinPreviewSource
{
	Q_OBJECT

  public:
	SkinManageDialog(QWidget* parent, MinecraftAccountPtr account);
	~SkinManageDialog() override;

	/* skinrender::SkinPreviewSource */
	const SkinEntry* previewSkin() const override;
	QImage previewCape(const QString& capeId) const override;

  public slots:
	/* Uploads the selected skin (and its cape, if it changed) before closing.
	 * Rejects instead of closing when there is nothing to upload or the
	 * upload failed. */
	void accept() override;

  protected:
	bool eventFilter(QObject* watched, QEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

  private slots:
	/* Connected by name from the .ui through connectSlotsByName(). */
	void on_openFolderButton_clicked();
	void on_resetSkinButton_clicked();
	void on_importFileButton_clicked();
	void on_importUrlButton_clicked();
	void on_importUserButton_clicked();
	void on_capeCombo_currentIndexChanged(int index);
	void on_classicRadio_toggled(bool checked);
	void on_actionRenameSkin_triggered();
	void on_actionDeleteSkin_triggered();

	void onSkinActivated(const QModelIndex& index);
	void onSelectionChanged(const QItemSelection& selected,
							const QItemSelection& deselected);
	void onElytraToggled(bool checked);
	void showContextMenu(const QPoint& position);

  private:
	void setUpSkinList();

	/* Fill the cape combo, downloading any cape textures that are not
	 * cached yet. Blocking, with a progress dialog, because the combo cannot
	 * be built without them. */
	void loadCapes();

	/* Push the selection into whichever preview is in use. */
	void refreshPreview();

	/* Redraw the cape thumbnail under the combo. */
	void refreshCapePreview();

	/* The selected entry, or nullptr when nothing usable is selected. */
	SkinEntry* selectedSkin();

	QString currentCapeId() const;

	MinecraftAccountPtr m_account;
	Ui::SkinManageDialog* m_ui;
	SkinLibrary m_library;

	/* Exactly one of these two is used, decided once at construction by
	 * whether a GL context can be had at all. */
	skinrender::SkinPreviewSurface* m_preview = nullptr;
	QPointer<QWidget> m_previewContainer;
	QLabel* m_flatPreview = nullptr;

	/* Cape textures by cape id, and the combo row each id sits on. Row 0 is
	 * always "No Cape", so value() returning 0 for an unknown id is exactly
	 * the right fallback. */
	QHash<QString, QImage> m_capes;
	QHash<QString, int> m_capeRows;

	QString m_selectedName;
};
