/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * AttachDialog — modal pack picker used by the "Attach to pack…"
 * button on PackUpdaterPage.
 *
 * The dialog does NOT re-download or reinstall anything. Its job
 * is purely to let the user say:
 *
 *   "This instance corresponds to <Modrinth/CurseForge pack X>,
 *    version <Y>, and I'd like PackUpdater to track it from now
 *    on."
 *
 * It then writes the Pack* keys into the instance's `instance.cfg`
 * through the S24 settings API — exactly the same keys that
 * InstanceImportTask seeds for fresh pack imports.
 *
 * Flow:
 *
 *   1. Pick a provider (Modrinth / CurseForge). Modrinth is the
 *      default because its API is open; CurseForge needs the
 *      BuildConfig API key.
 *   2. Type a query → live search of pack catalogue (250ms debounce).
 *   3. Pick a result → version list loads in the dropdown.
 *   4. Pick a version → "Attach" enables.
 *   5. Click Attach → instance.cfg is written, dialog closes.
 *
 * Async-ness: every catalogue call goes through S11 `http_get` /
 * S30 `http_get_with_headers`; callbacks land on the GUI thread.
 * The dialog owns the in-flight state itself so a slow response
 * for query "N" gets discarded when the user has already typed
 * query "N+1".
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QRadioButton;
class QTimer;

class AttachDialog : public QDialog
{
	Q_OBJECT

  public:
	AttachDialog(const QString& instanceId, MMCOContext* ctx,
				 QWidget* parent = nullptr);
	~AttachDialog() override = default;

  private slots:
	void onProviderChanged();
	void onQueryEdited();
	void runSearch();
	void onResultSelected();
	void onVersionSelected();
	void onAttachClicked();

  private:
	void buildUi();
	void clearResults();
	void clearVersions();
	void updateAttachState();

	/* Provider-specific search dispatchers. Each shapes the URL +
	 * headers, fires the HTTP request, and parses the response
	 * into a uniform "result row" model that the list widget can
	 * render. */
	void searchModrinth(const QString& query, int reqId);
	void searchCurseForge(const QString& query, int reqId);

	void fetchVersionsModrinth(const QString& slug, int reqId);
	void fetchVersionsCurseForge(const QString& projectId, int reqId);

	/* Static callback shims for the C-ABI HTTP layer. They forward
	 * to the instance methods through a heap-allocated thunk that
	 * carries the dialog pointer + request id (for staleness check)
	 * + which step in the flow we're servicing. */
	struct HttpThunk;
	static void onHttpRaw(void* user_data, int status,
						  const void* response_body, size_t size);

	enum class Step {
		SearchModrinth,
		SearchCurseForge,
		VersionsModrinth,
		VersionsCurseForge,
	};

	void handleResponse(Step step, int reqId, int status,
						const QByteArray& body);

	QString m_instanceId;
	MMCOContext* m_ctx = nullptr;

	QRadioButton* m_btnModrinth = nullptr;
	QRadioButton* m_btnCurseForge = nullptr;
	QLineEdit* m_queryEdit = nullptr;
	QListWidget* m_resultList = nullptr;
	QComboBox* m_versionCombo = nullptr;
	QLabel* m_statusLabel = nullptr;
	QPushButton* m_attachBtn = nullptr;

	QTimer* m_searchDebounce = nullptr;

	/* Bumped on every outgoing request; callbacks compare against
	 * this and drop their results when stale. Saves us cancelling
	 * the underlying NetJob (which we can't from a plugin). */
	int m_nextRequestId = 0;
	int m_inflightSearchId = -1;
	int m_inflightVersionsId = -1;

	/* Current selection — the cells of instance.cfg we'll write. */
	struct ResultRow {
		QString provider;
		QString packId;
		QString packSlug;
		QString name;
		QString iconUrl;
		QString sourceUrl;
	};
	struct VersionRow {
		QString versionId;
		QString versionLabel;
	};
	ResultRow m_selectedPack;
	VersionRow m_selectedVersion;
};
