/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 */

#include "NewsViewerDialog.h"

#include <QDesktopServices>
#include <QUrl>
#include <QFrame>
#include <QFont>
#include <QPalette>

#include <cmark.h>

/* ── helpers ──────────────────────────────────────────────────────── */

static bool looksLikeHtml(const QString& s)
{
    return s.contains(QRegularExpression(QStringLiteral("<[a-zA-Z][^>]*>")));
}

/* ── NewsViewerDialog ─────────────────────────────────────────────── */

NewsViewerDialog::NewsViewerDialog(MMCOContext* ctx, QWidget* parent)
    : QDialog(parent), m_ctx(ctx)
{
    setWindowTitle(tr("News"));
    setMinimumSize(640, 480);
    resize(900, 600);

    /* ── Top bar ── */
    m_toggleSideBar = new QToolButton(this);
    m_toggleSideBar->setToolTip(tr("Toggle the news list sidebar"));

    m_refreshBtn = new QPushButton(tr("Refresh"), this);
    m_refreshBtn->setToolTip(tr("Reload all news feeds"));

    auto* topBar = new QHBoxLayout();
    topBar->addWidget(m_toggleSideBar);
    topBar->addStretch();
    topBar->addWidget(m_refreshBtn);

    /* ── Sidebar ── */
    m_sidebar = new QWidget(this);
    m_sidebar->setMinimumWidth(200);
    m_sidebar->setMaximumWidth(320);

    m_entryList = new QListWidget(m_sidebar);
    m_entryList->setAlternatingRowColors(true);
    m_entryList->setWordWrap(true);
    m_entryList->setSpacing(2);

    auto* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(0, 0, 4, 0);
    sidebarLayout->addWidget(m_entryList);

    /* ── Content pane ── */
    m_contentPane = new QWidget(this);

    m_titleLabel = new QLabel(tr("Select a news item"), m_contentPane);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 3);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setWordWrap(true);

    m_metaLabel = new QLabel(m_contentPane);
    m_metaLabel->setWordWrap(true);
    QPalette metaPal = m_metaLabel->palette();
    metaPal.setColor(QPalette::WindowText,
                     m_metaLabel->palette().color(QPalette::Mid));
    m_metaLabel->setPalette(metaPal);

    auto* separator = new QFrame(m_contentPane);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    m_contentView = new QTextBrowser(m_contentPane);
    m_contentView->setOpenExternalLinks(false);
    m_contentView->setReadOnly(true);
    QObject::connect(m_contentView, &QTextBrowser::anchorClicked,
                     this, [](const QUrl& url) {
                         QDesktopServices::openUrl(url);
                     });

    m_openBtn = new QPushButton(tr("Open in browser"), m_contentPane);
    m_openBtn->setEnabled(false);

    m_closeBtn = new QPushButton(tr("Close"), m_contentPane);

    /* Bottom button row: [Open in browser]  stretch  [Close] */
    auto* bottomBar = new QHBoxLayout();
    bottomBar->addWidget(m_openBtn);
    bottomBar->addStretch();
    bottomBar->addWidget(m_closeBtn);

    auto* contentLayout = new QVBoxLayout(m_contentPane);
    contentLayout->setContentsMargins(8, 4, 8, 8);
    contentLayout->addWidget(m_titleLabel);
    contentLayout->addWidget(m_metaLabel);
    contentLayout->addWidget(separator);
    contentLayout->addWidget(m_contentView, 1);
    contentLayout->addLayout(bottomBar);

    /* ── Splitter ── */
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_sidebar);
    m_splitter->addWidget(m_contentPane);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({220, 680});

    /* ── Root layout ── */
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->addLayout(topBar);
    root->addWidget(m_splitter, 1);

    /* ── Connections ── */
    QObject::connect(m_entryList, &QListWidget::currentRowChanged,
                     this, &NewsViewerDialog::onEntrySelected);
    QObject::connect(m_toggleSideBar, &QToolButton::clicked,
                     this, &NewsViewerDialog::onToggleSidebar);
    QObject::connect(m_openBtn, &QPushButton::clicked,
                     this, &NewsViewerDialog::onOpenInBrowser);
    QObject::connect(m_refreshBtn, &QPushButton::clicked,
                     this, &NewsViewerDialog::onRefresh);
    QObject::connect(m_closeBtn, &QPushButton::clicked,
                     this, &QDialog::close);
}

/* ── setSidebarVisible ────────────────────────────────────────────── */

void NewsViewerDialog::setSidebarVisible(bool visible)
{
    m_sidebarVisible = visible;
    m_sidebar->setVisible(visible);
    m_toggleSideBar->setText(visible ? tr("◀ Hide sidebar")
                                     : tr("▶ Show sidebar"));
}

/* ── loadEntries ──────────────────────────────────────────────────── */

void NewsViewerDialog::loadEntries(bool sidebarVisible)
{
    m_entries.clear();
    m_entryList->clear();

    if (!m_ctx)
        return;

    int count = m_ctx->news_get_entry_count(m_ctx->module_handle);
    if (count <= 0) {
        setSidebarVisible(sidebarVisible);
        return;
    }

    for (int i = 0; i < count; i++) {
        NewsEntry e;
        e.feedIndex = m_ctx->news_get_entry_feed_index(m_ctx->module_handle, i);

        auto safeStr = [&](const char* s) -> QString {
            return s ? QString::fromUtf8(s) : QString{};
        };

        e.title   = safeStr(m_ctx->news_get_entry_title(m_ctx->module_handle, i));
        e.link    = safeStr(m_ctx->news_get_entry_link(m_ctx->module_handle, i));
        e.content = safeStr(m_ctx->news_get_entry_content(m_ctx->module_handle, i));
        e.author  = safeStr(m_ctx->news_get_entry_author(m_ctx->module_handle, i));
        e.date    = safeStr(m_ctx->news_get_entry_date(m_ctx->module_handle, i));

        m_entries.append(e);

        QString feedLabel;
        if (e.feedIndex == 0) {
            feedLabel = tr("Official");
        } else {
            const char* feedUrl =
                m_ctx->news_get_feed_url(m_ctx->module_handle, e.feedIndex);
            feedLabel = feedUrl ? QUrl(QString::fromUtf8(feedUrl)).host()
                                : tr("Feed %1").arg(e.feedIndex);
        }

        auto* item = new QListWidgetItem(m_entryList);
        item->setText(e.title.isEmpty() ? tr("(no title)") : e.title);
        item->setToolTip(
            QStringLiteral("[%1] %2").arg(feedLabel, e.date));
        m_entryList->addItem(item);
    }

    /* Apply sidebar visibility */
    setSidebarVisible(sidebarVisible);

    /* Always select the first (latest) entry */
    if (!m_entries.isEmpty())
        m_entryList->setCurrentRow(0);
}

/* ── onEntrySelected ──────────────────────────────────────────────── */

void NewsViewerDialog::onEntrySelected(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;

    const NewsEntry& e = m_entries[row];

    m_titleLabel->setText(e.title.isEmpty() ? tr("(no title)") : e.title);

    QStringList metaParts;
    if (!e.author.isEmpty())
        metaParts << e.author;
    if (!e.date.isEmpty()) {
        QDateTime dt = QDateTime::fromString(e.date, Qt::ISODate);
        metaParts << (dt.isValid()
                          ? dt.toString(QStringLiteral("d MMM yyyy"))
                          : e.date);
    }
    if (e.feedIndex > 0) {
        const char* feedUrl =
            m_ctx->news_get_feed_url(m_ctx->module_handle, e.feedIndex);
        QString host = feedUrl
                           ? QUrl(QString::fromUtf8(feedUrl)).host()
                           : tr("Feed %1").arg(e.feedIndex);
        metaParts << host;
    }
    m_metaLabel->setText(metaParts.join(QStringLiteral(" · ")));

    m_contentView->setHtml(renderContent(e.content));
    m_openBtn->setEnabled(!e.link.isEmpty());
}

/* ── onToggleSidebar ──────────────────────────────────────────────── */

void NewsViewerDialog::onToggleSidebar()
{
    setSidebarVisible(!m_sidebarVisible);
}

/* ── onOpenInBrowser ──────────────────────────────────────────────── */

void NewsViewerDialog::onOpenInBrowser()
{
    int row = m_entryList->currentRow();
    if (row < 0 || row >= m_entries.size())
        return;
    const QString& link = m_entries[row].link;
    if (!link.isEmpty())
        QDesktopServices::openUrl(QUrl(link));
}

/* ── onRefresh ────────────────────────────────────────────────────── */

void NewsViewerDialog::onRefresh()
{
    if (!m_ctx)
        return;
    m_ctx->news_reload(m_ctx->module_handle);
    QTimer::singleShot(2500, this, [this]() {
        loadEntries(m_sidebarVisible);
    });
}

/* ── renderContent ────────────────────────────────────────────────── */

/* static */
QString NewsViewerDialog::renderContent(const QString& raw)
{
    if (raw.isEmpty())
        return QStringLiteral("<p><i>No content.</i></p>");

    if (looksLikeHtml(raw)) {
        return QStringLiteral(
                   "<html><body style='font-family:sans-serif;'>") +
               raw +
               QStringLiteral("</body></html>");
    }

    QByteArray utf8 = raw.toUtf8();
    char* html = cmark_markdown_to_html(utf8.constData(),
                                        static_cast<size_t>(utf8.size()),
                                        CMARK_OPT_DEFAULT);
    QString result;
    if (html) {
        result = QStringLiteral(
                     "<html><body style='font-family:sans-serif;'>") +
                 QString::fromUtf8(html) +
                 QStringLiteral("</body></html>");
        free(html);
    } else {
        result = QStringLiteral("<pre>") +
                 raw.toHtmlEscaped() +
                 QStringLiteral("</pre>");
    }
    return result;
}
