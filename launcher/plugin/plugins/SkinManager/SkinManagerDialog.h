/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 *  SkinManagerDialog — the "Upload Skin" replacement.
 *
 *  Drop-in replacement for the launcher's built-in SkinUploadDialog,
 *  injected by SkinManagerPlugin when the user opens
 *  Settings → Accounts and clicks the toolbar's "Upload Skin"
 *  action. The plugin intercepts the action's `triggered` signal and
 *  shows this dialog instead of the stock one.
 *
 *  Behaviour parity with the launcher's SkinUploadDialog:
 *    • Optional skin PNG path (drag-drop or Browse)
 *    • Classic / Slim model selector
 *    • Cape combo (owned capes only)
 *    • OK uploads the skin and / or changes the cape
 *    • Cancel does nothing
 *
 *  Extras on top of vanilla:
 *    • Real-time 3D preview of the chosen skin + cape
 *    • Drop-zone PNG support
 *    • Auto-rotate + overlay toggles
 *    • "Reset to default" button (SkinDelete task)
 */

#pragma once

#include "plugin/sdk/mmco_cxx_sdk.hpp"
#include "SkinModel.h"

namespace Ui
{
	class SkinManagerDialog;
}

namespace SkinManagerNS
{
	class SkinViewerWidget;
}

class SkinManagerDialog : public QDialog
{
	Q_OBJECT

  public:
	SkinManagerDialog(const QString& accountId, MMCOContext* ctx,
					  QWidget* parent = nullptr);
	~SkinManagerDialog() override;

  private slots:
	void onBrowseClicked();
	void onResetClicked();
	void onVariantToggled();
	void onOverlayToggled(bool on);
	void onAutoRotateToggled(bool on);
	void onCapeChanged(int row);
	void onSkinFileDropped(const QString& path);

	void onAccept();

  private:
	void loadAccountState();
	void loadSkinFile(const QString& path);
	void setStatus(const QString& text, bool error = false);

	Ui::SkinManagerDialog* ui;
	SkinManagerNS::SkinViewerWidget* m_viewer = nullptr;
	QString m_accountId;
	MMCOContext* m_ctx = nullptr;

	/* Tracks the user's intent. m_chosenSkinPath empty == "keep
	 * current skin", otherwise upload from that path on OK. */
	QString m_chosenSkinPath;
	QImage m_chosenSkinImage;
};
