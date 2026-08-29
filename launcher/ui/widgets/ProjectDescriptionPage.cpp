/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#include "ProjectDescriptionPage.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QResizeEvent>
#include <QTextDocument>
#include <QTimer>

#include "Application.h"
#include "net/Download.h"
#include "net/HttpMetaCache.h"
#include "net/NetJob.h"

namespace
{

	/* Anything larger is almost certainly not meant to be read inside a
	 * side pane, and decoding it would cost more than it is worth. */
	constexpr int kMaxImagePixels = 4000;

} // namespace

ProjectDescriptionPage::ProjectDescriptionPage(QWidget* parent)
	: QTextBrowser(parent), m_metaEntry(QStringLiteral("ContentImages"))
{
	/* Links are handled by the page that owns this browser, so that a
	 * project link can switch providers instead of opening a browser. */
	setOpenLinks(false);
	setOpenExternalLinks(false);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void ProjectDescriptionPage::setMetaEntry(const QString& entry)
{
	m_metaEntry = entry;
}

void ProjectDescriptionPage::flush()
{
	m_pending.clear();
	m_generation++;
}

int ProjectDescriptionPage::contentWidth() const
{
	/* textWidth() is what the document lays out into and already
	 * accounts for the scrollbar; the margin is inside it. */
	const int width =
		int(document()->textWidth() - 2 * document()->documentMargin());
	return width > 0 ? width : viewport()->width();
}

QImage ProjectDescriptionPage::fitToWidth(const QImage& image) const
{
	const int width = contentWidth();
	if (width <= 0 || image.width() <= width) {
		return image;
	}
	return image.scaledToWidth(width, Qt::SmoothTransformation);
}

QVariant ProjectDescriptionPage::loadResource(int type, const QUrl& name)
{
	if (type != QTextDocument::ImageResource) {
		return QTextBrowser::loadResource(type, name);
	}

	if (m_images.contains(name)) {
		return fitToWidth(m_images.value(name));
	}

	if (name.scheme() == "http" || name.scheme() == "https") {
		/* Called from the layout pass, so it must return immediately.
		 * The picture appears when the download reports back. */
		requestImage(name);
		return QVariant();
	}

	return QTextBrowser::loadResource(type, name);
}

void ProjectDescriptionPage::requestImage(const QUrl& url)
{
	if (m_pending.contains(url)) {
		return;
	}
	m_pending.insert(url);

	/* Hashed, because a description URL is arbitrary and often longer
	 * than a file name may be. */
	const QString key = QString::fromLatin1(
		QCryptographicHash::hash(url.toEncoded(), QCryptographicHash::Sha1)
			.toHex());

	MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry(
		m_metaEntry, QStringLiteral("images/") + key);

	auto* job = new NetJob(QString("Description image %1").arg(url.fileName()),
						   APPLICATION->network());
	job->addNetAction(Net::Download::makeCached(url, entry));

	const QString path = entry->getFullPath();
	const quint64 generation = m_generation;

	connect(job, &NetJob::succeeded, this, [this, job, url, path, generation] {
		job->deleteLater();
		imageArrived(generation, url, path);
	});
	connect(job, &NetJob::failed, this, [this, job, url](QString reason) {
		job->deleteLater();
		qDebug() << "Could not load description image" << url << ":" << reason;
		/* Left out of m_images, so the box stays empty rather than the
		 * layout being redone for nothing. */
		m_pending.remove(url);
	});

	/* Started from the event loop, not from here.
	 *
	 * loadResource() - our only caller - is what the document calls while
	 * it is laying itself out, and an image that is already in the cache
	 * finishes the instant the job starts. That ran imageArrived(), and
	 * with it a document change, inside that layout pass; Qt's document
	 * internals are not re-entrant and the result was an assertion
	 * failure deep in Qt's array code (a shared array appended to while
	 * the layout was walking it). The one-line delay means every reply
	 * arrives on its own trip through the event loop, with no layout in
	 * progress. */
	QTimer::singleShot(0, job, [job] { job->start(); });
}

void ProjectDescriptionPage::imageArrived(quint64 generation, const QUrl& url,
										  const QString& path)
{
	m_pending.remove(url);

	/* The pane moved on to another project while this was downloading. */
	if (generation != m_generation) {
		return;
	}

	QImage image(path);
	if (image.isNull()) {
		return;
	}
	if (image.width() > kMaxImagePixels || image.height() > kMaxImagePixels) {
		qDebug() << "Ignoring oversized description image" << url;
		return;
	}

	m_images.insert(url, image);
	document()->addResource(QTextDocument::ImageResource, url,
							fitToWidth(image));

	/* The document has already been laid out with an empty box where
	 * this belongs, so it has to be told to do it again. */
	scheduleRelayout();
}

void ProjectDescriptionPage::scheduleRelayout()
{
	if (m_relayoutQueued) {
		return;
	}
	m_relayoutQueued = true;
	QTimer::singleShot(0, this, [this] {
		m_relayoutQueued = false;
		document()->markContentsDirty(0, document()->characterCount());
	});
}

void ProjectDescriptionPage::rescaleAll()
{
	if (m_images.isEmpty()) {
		return;
	}

	for (auto it = m_images.constBegin(); it != m_images.constEnd(); ++it) {
		document()->addResource(QTextDocument::ImageResource, it.key(),
								fitToWidth(it.value()));
	}
	scheduleRelayout();
}

void ProjectDescriptionPage::resizeEvent(QResizeEvent* event)
{
	QTextBrowser::resizeEvent(event);

	/* Only when the width actually moved: a resize fires for height
	 * changes too, and rescaling every image is not free. */
	const int width = contentWidth();
	if (width != m_lastWidth) {
		m_lastWidth = width;
		rescaleAll();
	}
}
