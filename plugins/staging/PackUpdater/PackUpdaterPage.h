/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * PackUpdaterPage — per-instance "Modpack" tab.
 *
 * Two visual states driven by whether the instance has a
 * `mmc-packupdater.json` record on disk:
 *
 *   - **Linked**: shows provider, pack name, installed version,
 *     install timestamp, and a "Check for Updates" action.
 *     "Detach" is offered as a destructive secondary action.
 *
 *   - **Unlinked**: shows a short explanation and an "Attach to
 *     pack…" primary action that will (in a later step) launch the
 *     Modrinth / CurseForge source picker.
 *
 * The page reuses the launcher's `BasePage` interface so it slots
 * into the standard instance settings tab strip alongside Worlds,
 * Mods, etc. Identifier is "modpack" so it gets a deterministic
 * place in the tab list.
 *
 * NOTE: the *behaviour* behind the buttons is intentionally
 * stubbed in this first cut — wiring the Modrinth source adapter
 * and the update planner comes in the next change. The tab itself
 * needs to ship first so the user can see the plugin is alive and
 * so source-attach has a destination view to land back in.
 */

#pragma once

#include "plugin/sdk/mmco_cxx_sdk.hpp"
#include "PackMetadata.h"
#include "UpdateSource.h"

class QLabel;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;

class PackUpdaterPage : public QWidget, public BasePage
{
	Q_OBJECT

  public:
	PackUpdaterPage(const QString& instanceId, const QString& instanceRoot,
					MMCOContext* ctx, QWidget* parent = nullptr);
	~PackUpdaterPage() override = default;

	/* BasePage overrides */
	QString id() const override
	{
		return QStringLiteral("modpack");
	}
	QString displayName() const override
	{
		return tr("Modpack");
	}
	QIcon icon() const override;
	QString helpPage() const override
	{
		return QStringLiteral("Instance-Modpack");
	}
	bool shouldDisplay() const override
	{
		/* Always show — the unlinked view is a meaningful UX even
		 * when the instance was created by hand. It tells the user
		 * "you can attach this to a Modrinth/CurseForge pack and
		 * get update tracking for free". */
		return true;
	}

	void openedImpl() override;

  private slots:
	void onCheckForUpdates();
	void onAttachClicked();
	void onDetachClicked();
	void onApplyClicked();

  private:
	void buildUi();
	void refreshState();
	void renderLinked(const pack_updater::PackRecord& rec);
	void renderUnlinked();

	/* Resolve the local icon cache path, render it if it's already
	 * on disk, and otherwise kick off a one-shot S11 download from
	 * `iconUrl`. Idempotent — safe to call on every refreshState. */
	void loadOrFetchIcon(const QString& iconUrl);
	QString iconCachePath() const;
	void applyIconPixmap(const QByteArray& pngBytes);

	/* HTTP callback shim. The thunk carries a QPointer back to the
	 * page so a slow icon response for a closed dialog doesn't
	 * crash us. */
	struct IconThunk;
	static void onIconHttp(void* user_data, int status,
						   const void* response_body, size_t size);

	QString m_instanceId;
	QString m_instanceRoot;
	MMCOContext* m_ctx = nullptr;

	/* Two-page stack — linked vs. unlinked. */
	QStackedWidget* m_stack = nullptr;

	/* Linked view widgets */
	QLabel* m_iconLabel = nullptr;
	QLabel* m_providerLabel = nullptr;
	QLabel* m_packNameLabel = nullptr;
	QLabel* m_versionLabel = nullptr;
	QLabel* m_installedAtLabel = nullptr;
	QLabel* m_sourceUrlLabel = nullptr;
	QLabel* m_statusLabel = nullptr;
	QPushButton* m_checkBtn = nullptr;
	QPushButton* m_applyBtn = nullptr;
	QPushButton* m_detachBtn = nullptr;

	/* Cached result from the last successful check — populated by
	 * onCheckForUpdates so onApplyClicked can launch the apply
	 * dialog without re-querying. Cleared on detach / refresh. */
	QUrl m_pendingUpdateUrl;
	QString m_pendingVersionId;
	QString m_pendingVersionLabel;

	/* Unlinked view widgets */
	QLabel* m_unlinkedInfoLabel = nullptr;
	QPushButton* m_attachBtn = nullptr;
};
