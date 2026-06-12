/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "AttachDialog.h"
#include "PackMetadata.h"

#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTimer>
#include <QVBoxLayout>

/* HTTP callback thunk — heap-allocated, self-deleting. Carries the
 * dialog pointer (guarded by QPointer so a dialog closed mid-flight
 * doesn't crash us) and enough context to route the response to the
 * right handler. */
struct AttachDialog::HttpThunk {
	QPointer<AttachDialog> dlg;
	Step step;
	int reqId;
};

void AttachDialog::onHttpRaw(void* user_data, int status,
							 const void* response_body, size_t size)
{
	std::unique_ptr<HttpThunk> thunk(static_cast<HttpThunk*>(user_data));
	if (!thunk->dlg)
		return; /* dialog was destroyed — drop the response */
	const QByteArray body(static_cast<const char*>(response_body),
						  static_cast<int>(size));
	thunk->dlg->handleResponse(thunk->step, thunk->reqId, status, body);
}

static QString curseForgeApiKey(MMCOContext* ctx)
{
	if (!ctx || !ctx->app_setting_get)
		return {};
	const char* v =
		ctx->app_setting_get(ctx->module_handle, "CurseForgeAPIKey");
	return QString::fromUtf8(v);
}

AttachDialog::AttachDialog(const QString& instanceId, MMCOContext* ctx,
						   QWidget* parent)
	: QDialog(parent), m_instanceId(instanceId), m_ctx(ctx)
{
	setWindowTitle(tr("Attach to Modpack"));
	setMinimumSize(560, 480);
	buildUi();

	/* If the instance already has a record we pre-load it so the
	 * user can fix a wrong attach without retyping everything. */
	if (auto rec = pack_updater::load(m_ctx, m_instanceId)) {
		if (rec->provider == pack_updater::Provider::CurseForge)
			m_btnCurseForge->setChecked(true);
		m_queryEdit->setText(rec->packSlug.isEmpty() ? rec->packId
													 : rec->packSlug);
	}
}

void AttachDialog::buildUi()
{
	auto* outer = new QVBoxLayout(this);

	/* Provider row */
	auto* provRow = new QHBoxLayout();
	m_btnModrinth = new QRadioButton(tr("Modrinth"), this);
	m_btnCurseForge = new QRadioButton(tr("CurseForge"), this);
	m_btnModrinth->setChecked(true);
	provRow->addWidget(new QLabel(tr("Source:"), this));
	provRow->addWidget(m_btnModrinth);
	provRow->addWidget(m_btnCurseForge);
	provRow->addStretch(1);
	outer->addLayout(provRow);
	connect(m_btnModrinth, &QRadioButton::toggled, this,
			&AttachDialog::onProviderChanged);

	/* Query input */
	m_queryEdit = new QLineEdit(this);
	m_queryEdit->setPlaceholderText(tr("Type pack name or slug…"));
	outer->addWidget(m_queryEdit);
	connect(m_queryEdit, &QLineEdit::textEdited, this,
			&AttachDialog::onQueryEdited);

	/* Results list */
	m_resultList = new QListWidget(this);
	outer->addWidget(m_resultList, 1);
	connect(m_resultList, &QListWidget::currentItemChanged, this,
			[this](QListWidgetItem*, QListWidgetItem*) { onResultSelected(); });

	/* Version dropdown */
	auto* verRow = new QHBoxLayout();
	verRow->addWidget(
		new QLabel(tr("Version installed in this instance:"), this));
	m_versionCombo = new QComboBox(this);
	m_versionCombo->setMinimumWidth(200);
	verRow->addWidget(m_versionCombo, 1);
	outer->addLayout(verRow);
	connect(m_versionCombo, &QComboBox::currentIndexChanged, this,
			[this](int) { onVersionSelected(); });

	/* Status / footer */
	m_statusLabel = new QLabel(this);
	m_statusLabel->setWordWrap(true);
	outer->addWidget(m_statusLabel);

	auto* btnBox = new QDialogButtonBox(this);
	m_attachBtn = btnBox->addButton(tr("Attach"), QDialogButtonBox::AcceptRole);
	btnBox->addButton(QDialogButtonBox::Cancel);
	connect(m_attachBtn, &QPushButton::clicked, this,
			&AttachDialog::onAttachClicked);
	connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	outer->addWidget(btnBox);
	m_attachBtn->setEnabled(false);

	/* Debounce timer — 250ms after last keystroke fires the
	 * search. Keeps us off the catalogue's rate limiter and
	 * means rapid typing only sends one request, not one per
	 * letter. */
	m_searchDebounce = new QTimer(this);
	m_searchDebounce->setSingleShot(true);
	m_searchDebounce->setInterval(250);
	connect(m_searchDebounce, &QTimer::timeout, this, &AttachDialog::runSearch);
}

void AttachDialog::onProviderChanged()
{
	/* Wipe the in-progress search state so we don't mix Modrinth
	 * results with CurseForge clicks. */
	clearResults();
	clearVersions();
	if (!m_queryEdit->text().isEmpty())
		m_searchDebounce->start();
}

void AttachDialog::onQueryEdited()
{
	m_searchDebounce->start();
}

void AttachDialog::clearResults()
{
	m_resultList->clear();
	m_selectedPack = ResultRow{};
	updateAttachState();
}

void AttachDialog::clearVersions()
{
	m_versionCombo->clear();
	m_selectedVersion = VersionRow{};
	updateAttachState();
}

void AttachDialog::updateAttachState()
{
	m_attachBtn->setEnabled(!m_selectedPack.packId.isEmpty() &&
							!m_selectedVersion.versionId.isEmpty());
}

void AttachDialog::runSearch()
{
	const QString q = m_queryEdit->text().trimmed();
	clearResults();
	clearVersions();
	if (q.isEmpty()) {
		m_statusLabel->clear();
		return;
	}

	const int reqId = ++m_nextRequestId;
	m_inflightSearchId = reqId;
	m_statusLabel->setText(tr("Searching…"));

	if (m_btnModrinth->isChecked())
		searchModrinth(q, reqId);
	else
		searchCurseForge(q, reqId);
}

void AttachDialog::searchModrinth(const QString& query, int reqId)
{
	/* Public v2 search — no auth needed. `facets` restricts the
	 * result set to modpacks (we don't want mods polluting the
	 * list). `limit=20` keeps the response small. */
	const QString url =
		QStringLiteral("https://api.modrinth.com/v2/search?"
					   "facets=%5B%5B%22project_type%3Amodpack%22%5D%5D&"
					   "limit=20&query=%1")
			.arg(QString::fromUtf8(QUrl::toPercentEncoding(query)));

	auto* thunk = new HttpThunk{this, Step::SearchModrinth, reqId};
	const QByteArray urlUtf8 = url.toUtf8();
	if (m_ctx->http_get(m_ctx->module_handle, urlUtf8.constData(), &onHttpRaw,
						thunk) != 0) {
		delete thunk;
		m_statusLabel->setText(tr("Could not queue Modrinth request"));
	}
}

void AttachDialog::searchCurseForge(const QString& query, int reqId)
{
	const QString apiKey = curseForgeApiKey(m_ctx);
	if (apiKey.isEmpty()) {
		m_statusLabel->setText(
			tr("This build has no CurseForge API key configured."));
		return;
	}
	if (!m_ctx->http_get_with_headers) {
		m_statusLabel->setText(
			tr("Host launcher too old: http_get_with_headers is required."));
		return;
	}

	/* classId 4471 = Modpacks, gameId 432 = Minecraft. The CF
	 * search endpoint uses URL-encoded query params; sortField=2
	 * (Popularity) gives the most useful default ordering. */
	const QString url =
		QStringLiteral("https://api.curseforge.com/v1/mods/search?"
					   "gameId=432&classId=4471&sortField=2&"
					   "sortOrder=desc&pageSize=20&searchFilter=%1")
			.arg(QString::fromUtf8(QUrl::toPercentEncoding(query)));

	const QByteArray header =
		QStringLiteral("x-api-key: %1").arg(apiKey).toUtf8();
	const char* headers[] = {header.constData()};

	auto* thunk = new HttpThunk{this, Step::SearchCurseForge, reqId};
	const QByteArray urlUtf8 = url.toUtf8();
	if (m_ctx->http_get_with_headers(m_ctx->module_handle, urlUtf8.constData(),
									 headers, 1, &onHttpRaw, thunk) != 0) {
		delete thunk;
		m_statusLabel->setText(tr("Could not queue CurseForge request"));
	}
}

void AttachDialog::onResultSelected()
{
	auto* item = m_resultList->currentItem();
	if (!item) {
		m_selectedPack = ResultRow{};
		clearVersions();
		return;
	}
	m_selectedPack.provider = item->data(Qt::UserRole + 0).toString();
	m_selectedPack.packId = item->data(Qt::UserRole + 1).toString();
	m_selectedPack.packSlug = item->data(Qt::UserRole + 2).toString();
	m_selectedPack.name = item->data(Qt::UserRole + 3).toString();
	m_selectedPack.iconUrl = item->data(Qt::UserRole + 4).toString();
	m_selectedPack.sourceUrl = item->data(Qt::UserRole + 5).toString();

	clearVersions();
	updateAttachState();

	const int reqId = ++m_nextRequestId;
	m_inflightVersionsId = reqId;
	m_statusLabel->setText(
		tr("Loading versions for %1…").arg(m_selectedPack.name));

	if (m_selectedPack.provider == QLatin1String("modrinth"))
		fetchVersionsModrinth(m_selectedPack.packSlug, reqId);
	else
		fetchVersionsCurseForge(m_selectedPack.packId, reqId);
}

void AttachDialog::fetchVersionsModrinth(const QString& slug, int reqId)
{
	const QString url =
		QStringLiteral("https://api.modrinth.com/v2/project/%1/version")
			.arg(slug);
	auto* thunk = new HttpThunk{this, Step::VersionsModrinth, reqId};
	const QByteArray urlUtf8 = url.toUtf8();
	if (m_ctx->http_get(m_ctx->module_handle, urlUtf8.constData(), &onHttpRaw,
						thunk) != 0) {
		delete thunk;
		m_statusLabel->setText(tr("Could not queue Modrinth versions request"));
	}
}

void AttachDialog::fetchVersionsCurseForge(const QString& projectId, int reqId)
{
	const QString url =
		QStringLiteral(
			"https://api.curseforge.com/v1/mods/%1/files?pageSize=50")
			.arg(projectId);
	const QString apiKey = curseForgeApiKey(m_ctx);
	const QByteArray header =
		QStringLiteral("x-api-key: %1").arg(apiKey).toUtf8();
	const char* headers[] = {header.constData()};

	auto* thunk = new HttpThunk{this, Step::VersionsCurseForge, reqId};
	const QByteArray urlUtf8 = url.toUtf8();
	if (m_ctx->http_get_with_headers(m_ctx->module_handle, urlUtf8.constData(),
									 headers, 1, &onHttpRaw, thunk) != 0) {
		delete thunk;
		m_statusLabel->setText(tr("Could not queue CurseForge files request"));
	}
}

void AttachDialog::handleResponse(Step step, int reqId, int status,
								  const QByteArray& body)
{
	/* Drop stale responses — the user has already moved on to a
	 * different query or selected a different pack. */
	const bool isSearch =
		step == Step::SearchModrinth || step == Step::SearchCurseForge;
	if (isSearch && reqId != m_inflightSearchId)
		return;
	if (!isSearch && reqId != m_inflightVersionsId)
		return;

	if (status < 200 || status >= 300) {
		m_statusLabel->setText(tr("HTTP %1 from upstream").arg(status));
		return;
	}

	QJsonParseError err{};
	const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
	if (err.error != QJsonParseError::NoError) {
		m_statusLabel->setText(
			tr("Could not parse response: %1").arg(err.errorString()));
		return;
	}

	switch (step) {
		case Step::SearchModrinth: {
			/* Modrinth /v2/search returns { hits: [...] } */
			const QJsonArray hits =
				doc.object().value(QStringLiteral("hits")).toArray();
			m_resultList->clear();
			for (const auto& v : hits) {
				const QJsonObject o = v.toObject();
				const QString slug = o.value(QStringLiteral("slug")).toString();
				const QString title =
					o.value(QStringLiteral("title")).toString();
				const QString author =
					o.value(QStringLiteral("author")).toString();
				const QString id =
					o.value(QStringLiteral("project_id")).toString();
				const QString icon =
					o.value(QStringLiteral("icon_url")).toString();
				auto* item = new QListWidgetItem(
					QStringLiteral("%1 — by %2").arg(title, author));
				item->setData(Qt::UserRole + 0, QStringLiteral("modrinth"));
				item->setData(Qt::UserRole + 1, id);
				item->setData(Qt::UserRole + 2, slug);
				item->setData(Qt::UserRole + 3, title);
				item->setData(Qt::UserRole + 4, icon);
				item->setData(Qt::UserRole + 5,
							  QStringLiteral("https://modrinth.com/modpack/%1")
								  .arg(slug));
				m_resultList->addItem(item);
			}
			m_statusLabel->setText(tr("%1 result(s).").arg(hits.size()));
			break;
		}
		case Step::SearchCurseForge: {
			const QJsonArray data =
				doc.object().value(QStringLiteral("data")).toArray();
			m_resultList->clear();
			for (const auto& v : data) {
				const QJsonObject o = v.toObject();
				const QString name = o.value(QStringLiteral("name")).toString();
				const QString id =
					QString::number(o.value(QStringLiteral("id")).toInteger());
				const QString slug = o.value(QStringLiteral("slug")).toString();
				const QString websiteUrl =
					o.value(QStringLiteral("links"))
						.toObject()
						.value(QStringLiteral("websiteUrl"))
						.toString();
				const QString iconUrl =
					o.value(QStringLiteral("logo"))
						.toObject()
						.value(QStringLiteral("thumbnailUrl"))
						.toString();
				auto* item = new QListWidgetItem(name);
				item->setData(Qt::UserRole + 0, QStringLiteral("curseforge"));
				item->setData(Qt::UserRole + 1, id);
				item->setData(Qt::UserRole + 2, slug);
				item->setData(Qt::UserRole + 3, name);
				item->setData(Qt::UserRole + 4, iconUrl);
				item->setData(Qt::UserRole + 5, websiteUrl);
				m_resultList->addItem(item);
			}
			m_statusLabel->setText(tr("%1 result(s).").arg(data.size()));
			break;
		}
		case Step::VersionsModrinth: {
			const QJsonArray arr = doc.array();
			m_versionCombo->clear();
			for (const auto& v : arr) {
				const QJsonObject o = v.toObject();
				const QString id = o.value(QStringLiteral("id")).toString();
				const QString label =
					o.value(QStringLiteral("version_number")).toString();
				const QString mc = o.value(QStringLiteral("game_versions"))
									   .toArray()
									   .first()
									   .toString();
				const QString display =
					mc.isEmpty() ? label
								 : QStringLiteral("%1  [%2]").arg(label, mc);
				m_versionCombo->addItem(display, id);
				m_versionCombo->setItemData(m_versionCombo->count() - 1, label,
											Qt::UserRole + 1);
			}
			m_statusLabel->setText(
				tr("%1 version(s) — pick the one matching this instance.")
					.arg(arr.size()));
			break;
		}
		case Step::VersionsCurseForge: {
			const QJsonArray data =
				doc.object().value(QStringLiteral("data")).toArray();
			m_versionCombo->clear();
			for (const auto& v : data) {
				const QJsonObject o = v.toObject();
				const QString id =
					QString::number(o.value(QStringLiteral("id")).toInteger());
				const QString label =
					o.value(QStringLiteral("displayName")).toString();
				m_versionCombo->addItem(label, id);
				m_versionCombo->setItemData(m_versionCombo->count() - 1, label,
											Qt::UserRole + 1);
			}
			m_statusLabel->setText(
				tr("%1 file(s) — pick the one matching this instance.")
					.arg(data.size()));
			break;
		}
	}
}

void AttachDialog::onVersionSelected()
{
	const int idx = m_versionCombo->currentIndex();
	if (idx < 0) {
		m_selectedVersion = VersionRow{};
	} else {
		m_selectedVersion.versionId =
			m_versionCombo->itemData(idx, Qt::UserRole).toString();
		m_selectedVersion.versionLabel =
			m_versionCombo->itemData(idx, Qt::UserRole + 1).toString();
	}
	updateAttachState();
}

void AttachDialog::onAttachClicked()
{
	if (m_selectedPack.packId.isEmpty() ||
		m_selectedVersion.versionId.isEmpty())
		return;

	pack_updater::PackRecord rec;
	rec.provider = pack_updater::providerFromString(m_selectedPack.provider);
	rec.packId = m_selectedPack.packId;
	rec.packSlug = m_selectedPack.packSlug;
	rec.installedVersionId = m_selectedVersion.versionId;
	rec.installedVersionLabel = m_selectedVersion.versionLabel;
	rec.iconUrl = m_selectedPack.iconUrl;
	rec.sourceUrl = m_selectedPack.sourceUrl;
	rec.installedAtIso8601 =
		QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

	if (!pack_updater::save(m_ctx, m_instanceId, rec)) {
		QMessageBox::warning(this, tr("Attach failed"),
							 tr("Could not write the pack source into "
								"this instance's settings. The keys may "
								"not be registered on this launcher build."));
		return;
	}
	accept();
}
