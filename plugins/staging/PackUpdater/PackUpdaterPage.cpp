/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * PackUpdaterPage — implementation.
 *
 * Layout sketch (linked state):
 *
 *   ┌──────┐  Provider:  Modrinth
 *   │ icon │  Pack:      Adrenaline
 *   │ 96px │  Version:   1.2.3
 *   └──────┘  Installed: 2026-05-24 10:23 UTC
 *             Source:    https://modrinth.com/modpack/adrenaline
 *
 *   [ Check for Updates ]                       [ Detach ]
 *
 *   <status line — "Up to date", "Update available: 1.3.0", "...">
 *
 * Unlinked state is a single explanatory paragraph + "Attach to
 * pack…" button. We use QStackedWidget to swap; refreshState()
 * picks the right page based on whether `PackMetadata::load`
 * returns a record.
 */

#include "PackUpdaterPage.h"
#include "AttachDialog.h"
#include "ApplyProgressDialog.h"

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace
{
	/* Cheap, dependency-free provider → display string. We do this
	 * here rather than in PackMetadata because PackMetadata is the
	 * on-disk dialect (lowercase enum strings); the UI wants
	 * capitalised brand names. */
	QString providerDisplay(pack_updater::Provider p)
	{
		switch (p) {
			case pack_updater::Provider::Modrinth:
				return QStringLiteral("Modrinth");
			case pack_updater::Provider::CurseForge:
				return QStringLiteral("CurseForge");
			case pack_updater::Provider::MultiMC:
				return QStringLiteral("MultiMC raw zip");
			case pack_updater::Provider::Unknown:
			default:
				return QObject::tr("Unknown");
		}
	}

	/* Format the "Installed:" timestamp. The stored value is ISO
	 * 8601 UTC; we render it in the user's local timezone with a
	 * gentle "UTC" tail when parse fails (defensive — old or
	 * hand-edited files might carry junk). */
	QString prettifyTimestamp(const QString& iso)
	{
		if (iso.isEmpty())
			return QStringLiteral("—");
		QDateTime dt = QDateTime::fromString(iso, Qt::ISODate);
		if (!dt.isValid())
			return iso;
		dt.setTimeSpec(Qt::UTC);
		return dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
	}
} /* namespace */

PackUpdaterPage::PackUpdaterPage(const QString& instanceId,
								 const QString& instanceRoot, MMCOContext* ctx,
								 QWidget* parent)
	: QWidget(parent), m_instanceId(instanceId), m_instanceRoot(instanceRoot),
	  m_ctx(ctx)
{
	buildUi();
	refreshState();
}

QIcon PackUpdaterPage::icon() const
{
	/* SDK plugin-icon resolver. Falls back gracefully if the icon
	 * set isn't shipped — the launcher's stock "checkupdate" tab
	 * icon makes sense semantically and is always available. */
	if (m_ctx && m_ctx->ui_plugin_icon) {
		const char* resource =
			m_ctx->ui_plugin_icon(m_ctx->module_handle, "modpack");
		if (resource && *resource)
			return QIcon(QString::fromUtf8(resource));
	}
	return QIcon::fromTheme(QStringLiteral("checkupdate"));
}

void PackUpdaterPage::openedImpl()
{
	/* Re-read the record every time the tab is shown — the user
	 * may have just come back from the (yet-to-be-built) attach
	 * dialog, or another instance of the launcher may have written
	 * over the file. Cheap enough (single JSON read) that we don't
	 * bother caching. */
	refreshState();
}

void PackUpdaterPage::buildUi()
{
	m_stack = new QStackedWidget(this);
	auto* outer = new QVBoxLayout(this);
	outer->setContentsMargins(12, 12, 12, 12);
	outer->addWidget(m_stack);

	/* ── Linked page ─────────────────────────────────────────── */
	auto* linked = new QWidget(m_stack);
	auto* linkedLayout = new QVBoxLayout(linked);

	auto* header = new QHBoxLayout();
	m_iconLabel = new QLabel(linked);
	m_iconLabel->setFixedSize(96, 96);
	m_iconLabel->setAlignment(Qt::AlignCenter);
	m_iconLabel->setFrameShape(QFrame::Box);
	m_iconLabel->setText(tr("(logo)"));
	header->addWidget(m_iconLabel);

	auto* metaCol = new QVBoxLayout();
	m_providerLabel = new QLabel(linked);
	m_packNameLabel = new QLabel(linked);
	QFont packFont = m_packNameLabel->font();
	packFont.setBold(true);
	packFont.setPointSize(packFont.pointSize() + 3);
	m_packNameLabel->setFont(packFont);
	m_versionLabel = new QLabel(linked);
	m_installedAtLabel = new QLabel(linked);
	m_sourceUrlLabel = new QLabel(linked);
	m_sourceUrlLabel->setOpenExternalLinks(true);
	m_sourceUrlLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
	metaCol->addWidget(m_packNameLabel);
	metaCol->addWidget(m_providerLabel);
	metaCol->addWidget(m_versionLabel);
	metaCol->addWidget(m_installedAtLabel);
	metaCol->addWidget(m_sourceUrlLabel);
	metaCol->addStretch(1);
	header->addLayout(metaCol, 1);
	linkedLayout->addLayout(header);

	auto* btnRow = new QHBoxLayout();
	m_checkBtn = new QPushButton(tr("Check for Updates"), linked);
	connect(m_checkBtn, &QPushButton::clicked, this,
			&PackUpdaterPage::onCheckForUpdates);
	btnRow->addWidget(m_checkBtn);

	m_applyBtn = new QPushButton(tr("Apply Update"), linked);
	m_applyBtn->setVisible(false);
	connect(m_applyBtn, &QPushButton::clicked, this,
			&PackUpdaterPage::onApplyClicked);
	btnRow->addWidget(m_applyBtn);

	btnRow->addStretch(1);
	m_detachBtn = new QPushButton(tr("Detach"), linked);
	m_detachBtn->setToolTip(
		tr("Forget the modpack source for this instance. Useful if you "
		   "moved away from the upstream pack and want to manage mods "
		   "by hand from now on."));
	connect(m_detachBtn, &QPushButton::clicked, this,
			&PackUpdaterPage::onDetachClicked);
	btnRow->addWidget(m_detachBtn);
	linkedLayout->addLayout(btnRow);

	m_statusLabel = new QLabel(linked);
	m_statusLabel->setWordWrap(true);
	linkedLayout->addWidget(m_statusLabel);
	linkedLayout->addStretch(1);

	m_stack->addWidget(linked);

	/* ── Unlinked page ───────────────────────────────────────── */
	auto* unlinked = new QWidget(m_stack);
	auto* unlinkedLayout = new QVBoxLayout(unlinked);
	m_unlinkedInfoLabel = new QLabel(
		tr("<p>This instance isn't linked to a modpack source yet.</p>"
		   "<p>Linking lets PackUpdater check the upstream catalogue for "
		   "new releases of the pack and apply them in one go (mods, "
		   "overrides, loader version, all in lockstep).</p>"
		   "<p>If this instance was created from a Modrinth (.mrpack) or "
		   "CurseForge (.zip) pack, the link should have happened "
		   "automatically during import. If you're seeing this anyway, "
		   "the manifest probably wasn't preserved — you can attach it "
		   "manually below.</p>"),
		unlinked);
	m_unlinkedInfoLabel->setWordWrap(true);
	unlinkedLayout->addWidget(m_unlinkedInfoLabel);

	auto* attachRow = new QHBoxLayout();
	m_attachBtn = new QPushButton(tr("Attach to pack…"), unlinked);
	connect(m_attachBtn, &QPushButton::clicked, this,
			&PackUpdaterPage::onAttachClicked);
	attachRow->addWidget(m_attachBtn);
	attachRow->addStretch(1);
	unlinkedLayout->addLayout(attachRow);
	unlinkedLayout->addStretch(1);

	m_stack->addWidget(unlinked);
}

void PackUpdaterPage::refreshState()
{
	auto rec = pack_updater::load(m_ctx, m_instanceId);
	if (rec) {
		renderLinked(*rec);
		m_stack->setCurrentIndex(0);
	} else {
		renderUnlinked();
		m_stack->setCurrentIndex(1);
	}
}

void PackUpdaterPage::renderLinked(const pack_updater::PackRecord& rec)
{
	m_providerLabel->setText(tr("Provider: %1").arg(providerDisplay(rec.provider)));
	m_packNameLabel->setText(rec.packSlug.isEmpty() ? tr("(unnamed pack)")
													: rec.packSlug);
	m_versionLabel->setText(tr("Version: %1").arg(
		rec.installedVersionLabel.isEmpty() ? QStringLiteral("—")
											: rec.installedVersionLabel));
	m_installedAtLabel->setText(
		tr("Installed: %1").arg(prettifyTimestamp(rec.installedAtIso8601)));
	if (rec.sourceUrl.isEmpty()) {
		m_sourceUrlLabel->clear();
		m_sourceUrlLabel->hide();
	} else {
		m_sourceUrlLabel->show();
		m_sourceUrlLabel->setText(QStringLiteral("<a href=\"%1\">%1</a>")
									  .arg(rec.sourceUrl.toHtmlEscaped()));
	}
	m_statusLabel->clear();

	loadOrFetchIcon(rec.iconUrl);
}

QString PackUpdaterPage::iconCachePath() const
{
	/* Stash the icon under .mmc/ so it doesn't pollute .minecraft/
	 * (mod scanners) and doesn't show up at the instance root
	 * (where the user would wonder what it is). PackPortal's raw
	 * MultiMC zip export ignores `.mmc/`, so the cached icon stays
	 * a launcher-side detail. */
	return m_instanceRoot + QStringLiteral("/.mmc/packupdater-icon");
}

void PackUpdaterPage::applyIconPixmap(const QByteArray& pngBytes)
{
	QPixmap pm;
	if (!pm.loadFromData(pngBytes))
		return; /* bad payload — keep the placeholder */
	m_iconLabel->setPixmap(pm.scaled(96, 96, Qt::KeepAspectRatio,
									 Qt::SmoothTransformation));
}

struct PackUpdaterPage::IconThunk {
	QPointer<PackUpdaterPage> page;
	QString cachePath;
};

void PackUpdaterPage::onIconHttp(void* user_data, int status,
								 const void* response_body, size_t size)
{
	std::unique_ptr<IconThunk> thunk(static_cast<IconThunk*>(user_data));
	if (!thunk->page)
		return; /* page destroyed mid-flight */
	if (status < 200 || status >= 300)
		return;
	const QByteArray bytes(static_cast<const char*>(response_body),
						   static_cast<int>(size));
	if (bytes.isEmpty())
		return;

	/* Write-through to disk so the next time this tab opens the
	 * render is instant. `.mmc/` may not exist on legacy instances
	 * — create it on demand. */
	QDir().mkpath(QFileInfo(thunk->cachePath).absolutePath());
	QFile out(thunk->cachePath);
	if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		out.write(bytes);
		out.close();
	}
	thunk->page->applyIconPixmap(bytes);
}

void PackUpdaterPage::loadOrFetchIcon(const QString& iconUrl)
{
	const QString cache = iconCachePath();

	/* Disk cache hit — render immediately, no network. */
	if (QFile::exists(cache)) {
		QFile in(cache);
		if (in.open(QIODevice::ReadOnly)) {
			const QByteArray bytes = in.readAll();
			in.close();
			applyIconPixmap(bytes);
			return;
		}
	}

	/* No cache and no URL → keep the placeholder. */
	if (iconUrl.isEmpty() || !m_ctx || !m_ctx->http_get)
		return;

	/* S11 rejects non-HTTP(S); bail early to avoid a wasted queue
	 * slot. */
	if (!iconUrl.startsWith(QLatin1String("http://")) &&
		!iconUrl.startsWith(QLatin1String("https://")))
		return;

	auto* thunk = new IconThunk{this, cache};
	const QByteArray urlUtf8 = iconUrl.toUtf8();
	if (m_ctx->http_get(m_ctx->module_handle, urlUtf8.constData(),
						&PackUpdaterPage::onIconHttp, thunk) != 0) {
		delete thunk;
	}
}

void PackUpdaterPage::renderUnlinked()
{
	/* Nothing to refresh — the labels are static. Kept as a hook
	 * so future enhancements (e.g. "we detected a manifest under
	 * .mmc/, click to import") can drop in cleanly. */
}

void PackUpdaterPage::onCheckForUpdates()
{
	auto rec = pack_updater::load(m_ctx, m_instanceId);
	if (!rec) {
		m_statusLabel->setText(
			tr("<i>This instance isn't linked to a pack source.</i>"));
		return;
	}

	auto src = pack_updater::makeSource(rec->provider);
	if (!src) {
		m_statusLabel->setText(
			tr("<i>%1 has no upstream catalogue to check against.</i>")
				.arg(QString::fromLatin1(
					pack_updater::providerToString(rec->provider))));
		return;
	}

	m_checkBtn->setEnabled(false);
	m_statusLabel->setText(tr("Checking…"));

	/* Capture-by-value because the lambda outlives the stack frame
	 * — the callback fires asynchronously when the HTTP response
	 * lands. We hold a copy of the installed record so we can diff
	 * version labels without re-reading the file. */
	const pack_updater::PackRecord installed = *rec;
	const QString instanceId = m_instanceId;
	MMCOContext* ctx = m_ctx;

	/* `src` is local to this slot — we move ownership into the
	 * heap-captured lambda so the adapter stays alive until the
	 * callback completes. */
	auto* keepAlive = src.release();

	keepAlive->fetchLatest(
		m_ctx, installed,
		[this, installed, instanceId, ctx,
		 keepAlive](pack_updater::LatestVersion result) {
			std::unique_ptr<pack_updater::UpdateSource> owner(keepAlive);

			m_checkBtn->setEnabled(true);

			if (!result.ok) {
				m_statusLabel->setText(
					tr("<b style='color:#cc6666'>Check failed:</b> %1")
						.arg(result.errorMessage.toHtmlEscaped()));
				return;
			}

			const bool sameById = !result.versionId.isEmpty() &&
								  result.versionId == installed.installedVersionId;
			const bool sameByLabel =
				!result.versionLabel.isEmpty() &&
				result.versionLabel == installed.installedVersionLabel;

			if (sameById || sameByLabel) {
				m_statusLabel->setText(
					tr("<b style='color:#66aa66'>Up to date.</b> "
					   "Installed version: %1")
						.arg(installed.installedVersionLabel.toHtmlEscaped()));
				m_pendingUpdateUrl.clear();
				m_applyBtn->setVisible(false);
				/* Clear the badge if a previous check set it and
				 * the user has since applied the update through
				 * other means. */
				if (ctx && ctx->instance_set_update_available) {
					ctx->instance_set_update_available(
						ctx->module_handle, instanceId.toUtf8().constData(),
						0);
				}
				return;
			}

			m_statusLabel->setText(
				tr("<b style='color:#dd9933'>Update available:</b> "
				   "%1 → %2")
					.arg(installed.installedVersionLabel.toHtmlEscaped(),
						 result.versionLabel.toHtmlEscaped()));

			/* Stash everything the apply dialog needs so the user
			 * can hit "Apply Update" without us re-querying the
			 * catalogue. */
			m_pendingUpdateUrl = QUrl(result.manifestUrl);
			m_pendingVersionId = result.versionId;
			m_pendingVersionLabel = result.versionLabel;
			if (ctx) {
				const QString line =
					QStringLiteral("pending update url=%1 versionId=%2 "
								   "label=%3 valid=%4")
						.arg(result.manifestUrl,
							 result.versionId,
							 result.versionLabel,
							 m_pendingUpdateUrl.isValid() ? "yes" : "no");
				MMCO_LOG(ctx, line.toUtf8().constData());
			}
			m_applyBtn->setVisible(m_pendingUpdateUrl.isValid());

			/* Light the InstanceView badge so the user sees the
			 * available update from outside this page too. This is
			 * the whole reason S04's
			 * `instance_set_update_available` exists. */
			if (ctx && ctx->instance_set_update_available) {
				ctx->instance_set_update_available(
					ctx->module_handle, instanceId.toUtf8().constData(), 1);
			}
		});
}

void PackUpdaterPage::onAttachClicked()
{
	/* Modal picker. AttachDialog writes the Pack* keys into
	 * instance.cfg on accept; on cancel nothing changes. Either
	 * way refreshState() flips between the unlinked and linked
	 * views as appropriate. */
	AttachDialog dlg(m_instanceId, m_ctx, this);
	dlg.exec();
	refreshState();
}

void PackUpdaterPage::onDetachClicked()
{
	auto ret = QMessageBox::question(
		this, tr("Detach modpack source"),
		tr("Forget the modpack source for this instance? You can re-"
		   "attach it later, but PackUpdater won't show update "
		   "notifications until you do."),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (ret != QMessageBox::Yes)
		return;

	pack_updater::clear(m_ctx, m_instanceId);
	refreshState();
}

void PackUpdaterPage::onApplyClicked()
{
	if (!m_pendingUpdateUrl.isValid()) {
		QMessageBox::information(
			this, tr("Apply Update"),
			tr("Run \"Check for Updates\" first."));
		return;
	}
	auto rec = pack_updater::load(m_ctx, m_instanceId);
	if (!rec) {
		QMessageBox::warning(
			this, tr("Apply Update"),
			tr("Instance is no longer pack-managed."));
		return;
	}

	/* The apply dialog walks the full pipeline (download → diff
	 * → confirm → backup → mods → overrides → components →
	 * metadata). It handles its own failure path including the
	 * restore prompt. */
	ApplyProgressDialog dlg(m_instanceId, m_instanceRoot, *rec,
							m_pendingUpdateUrl, m_pendingVersionId,
							m_pendingVersionLabel, m_ctx, this);
	dlg.exec();

	/* Whatever happened — applied, cancelled, restored — refresh
	 * so the page shows the new installed version (or the rolled-
	 * back original). */
	m_pendingUpdateUrl.clear();
	m_applyBtn->setVisible(false);
	refreshState();
}
