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

#include <QHash>
#include <QImage>
#include <QSet>
#include <QString>
#include <QTextBrowser>
#include <QUrl>

/* A text browser that can show the pictures in a project description.
 *
 * A plain QTextBrowser only draws images it can resolve synchronously,
 * which for a description full of remote screenshots means a page of
 * broken-image boxes. This one answers "not yet" for an unknown image,
 * fetches it through the shared HTTP cache, and re-lays the document out
 * once it lands.
 *
 * Images are also scaled down to the width of the pane. Project pages
 * are written for a browser window and routinely embed 1920px banners,
 * which would otherwise force a horizontal scrollbar over the whole
 * description. The original is kept so that widening the pane sharpens
 * the picture again instead of stretching it. */
class ProjectDescriptionPage final : public QTextBrowser
{
	Q_OBJECT

  public:
	explicit ProjectDescriptionPage(QWidget* parent = nullptr);

	/* Metacache bucket the downloaded images are stored in. Must be a
	 * bucket registered by the application. */
	void setMetaEntry(const QString& entry);

	/* Forget what is in flight. Call before replacing the contents, so
	 * that images requested for the previous project cannot land in the
	 * new one. */
	void flush();

  protected:
	QVariant loadResource(int type, const QUrl& name) override;
	void resizeEvent(QResizeEvent* event) override;

  private:
	void requestImage(const QUrl& url);
	void imageArrived(quint64 generation, const QUrl& url,
					  const QString& path);
	/* Shrinks anything wider than the pane; never enlarges. */
	QImage fitToWidth(const QImage& image) const;
	int contentWidth() const;
	void rescaleAll();

	/* Ask for one re-layout from the event loop.
	 *
	 * Never marks the document dirty on the spot. A description can pull
	 * in a dozen pictures, and marking after each one re-lays the whole
	 * document out, which asks for the images that have not arrived yet,
	 * which starts more requests from inside that layout pass. Coalescing
	 * keeps it to a single pass per batch and keeps document changes out
	 * of the layout itself, which Qt does not allow. */
	void scheduleRelayout();

  private:
	QString m_metaEntry;
	/* Originals, so a resize can rescale from full quality. */
	QHash<QUrl, QImage> m_images;
	QSet<QUrl> m_pending;
	/* Bumped by flush(); replies from an older generation are dropped. */
	quint64 m_generation = 0;
	int m_lastWidth = 0;
	bool m_relayoutQueued = false;
};
