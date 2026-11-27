/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "NewsViewerDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QTextBrowser>
#include <QTextDocument>
#include <QUrl>
#include <QVBoxLayout>

#include "DesktopServices.h"
#include "HoeDown.h"
#include "news/NewsChecker.h"

namespace
{
bool isWebUrl(const QUrl& url)
{
    return url.isValid() && !url.host().isEmpty()
        && (url.scheme() == QStringLiteral("https") || url.scheme() == QStringLiteral("http"));
}

class ArticleBrowser : public QTextBrowser
{
public:
    using QTextBrowser::QTextBrowser;

protected:
    QVariant loadResource(int, const QUrl&) override
    {
        // Feed markup is remote input. Render it without loading embedded resources.
        return {};
    }
};
}

NewsViewerDialog::NewsViewerDialog(NewsChecker& checker, bool showArticleList, QWidget* parent)
    : QDialog(parent), m_checker(checker)
{
    setWindowTitle(tr("News"));
    resize(800, 500);

    auto* layout = new QVBoxLayout(this);
    auto* split = new QSplitter(Qt::Horizontal, this);
    layout->addWidget(split, 1);

    m_articles = new QListWidget(split);
    m_articles->setTextElideMode(Qt::ElideRight);
    auto* article = new QWidget(split);
    auto* articleLayout = new QVBoxLayout(article);
    m_title = new QLabel(article);
    m_title->setTextFormat(Qt::PlainText);
    m_title->setWordWrap(true);
    articleLayout->addWidget(m_title);
    m_body = new ArticleBrowser(article);
    m_body->setOpenLinks(false);
    articleLayout->addWidget(m_body);
    split->setStretchFactor(1, 1);
    split->setSizes({ 220, 580 });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    m_toggleList = buttons->addButton(QString(), QDialogButtonBox::ActionRole);
    m_openArticle = buttons->addButton(tr("Open in browser"), QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_articles, &QListWidget::currentRowChanged, this, &NewsViewerDialog::showArticle);
    connect(m_toggleList, &QPushButton::clicked, this, [this] {
        setArticleListVisible(m_articles->isHidden());
    });
    connect(m_openArticle, &QPushButton::clicked, this, [this] {
        const auto url = m_body->document()->baseUrl();
        if (isWebUrl(url))
            DesktopServices::openUrl(url);
    });
    connect(m_body, &QTextBrowser::anchorClicked, this, [this](const QUrl& link) {
        const auto url = m_body->document()->baseUrl().resolved(link);
        if (isWebUrl(url))
            DesktopServices::openUrl(url);
    });
    connect(&checker, &NewsChecker::newsLoaded, this, &NewsViewerDialog::reloadEntries);
    connect(&checker, &NewsChecker::newsLoadingFailed, this, &NewsViewerDialog::reloadEntries);

    setArticleListVisible(showArticleList);
    reloadEntries();
}

void NewsViewerDialog::setArticleListVisible(bool visible)
{
    m_articles->setVisible(visible);
    m_toggleList->setText(visible ? tr("Hide article list") : tr("Show article list"));
}

void NewsViewerDialog::reloadEntries()
{
    m_articles->clear();
    m_entries = m_checker.getNewsEntries();
    for (const auto& entry : m_entries)
        m_articles->addItem(entry->title.isEmpty() ? tr("Untitled") : entry->title);

    if (m_entries.isEmpty())
        showArticle(-1);
    else
        m_articles->setCurrentRow(0);
}

void NewsViewerDialog::showArticle(int row)
{
    m_body->clear();
    m_body->document()->setBaseUrl(QUrl());
    m_openArticle->setEnabled(false);
    if (row < 0 || row >= m_entries.size()) {
        m_title->setText(m_checker.isLoadingNews() ? tr("Loading news...") : tr("No news available."));
        return;
    }

    const auto& entry = m_entries.at(row);
    m_title->setText(entry->title.isEmpty() ? tr("Untitled") : entry->title);
    const QUrl url(entry->link);
    if (isWebUrl(url)) {
        m_body->document()->setBaseUrl(url);
        m_openArticle->setEnabled(true);
    }

    static const QRegularExpression htmlTag(QStringLiteral("<[a-zA-Z][^>]*>"));
    if (entry->content.isEmpty())
        m_body->setPlainText(tr("No content."));
    else if (entry->content.contains(htmlTag))
        m_body->setHtml(entry->content);
    else
        m_body->setHtml(HoeDown().process(entry->content.toUtf8()));
}
