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

#include "BrandLogos.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QSaveFile>
#include <QUrl>

#include "core/LauncherContext.h"

namespace
{
	/* Each owner's own copy of its logo: the favicon its site serves, or
	 * the logo its home page shows. */
	struct Source {
		const char* id;
		const char* url;
	};
	const Source kSources[] = {
		{ "modrinth", "https://modrinth.com/favicon-32x32.png" },
		{ "curseforge", "https://docs.curseforge.com/img/favicon.ico" },
		{ "atlauncher", "https://atlauncher.com/assets/images/logo.svg" },
	};

	const char* const kCacheDir = "cache/brands";
	/* A failed download waits this long before the next try, so a page of
	 * rows asking while offline is not a request each. */
	constexpr qint64 kRetryAfterMs = 5 * 60 * 1000;
	/* Nothing a logo needs is anywhere near this big. */
	constexpr qint64 kMaxBytes = 1024 * 1024;
} // namespace

BrandLogos* BrandLogos::instance()
{
	/* Parented to the application, so it goes away with it rather than at
	 * static destruction, after the network stack it may still hold a reply
	 * from. */
	static QPointer<BrandLogos> s_instance;
	if (!s_instance) {
		s_instance = new BrandLogos(QCoreApplication::instance());
	}
	return s_instance;
}

BrandLogos::BrandLogos(QObject* parent) : QObject(parent) {}

QStringList BrandLogos::ids()
{
	QStringList list;
	for (const Source& source : kSources) {
		list.append(QLatin1String(source.id));
	}
	return list;
}

QString BrandLogos::sourceUrl(const QString& id)
{
	for (const Source& source : kSources) {
		if (id == QLatin1String(source.id)) {
			return QLatin1String(source.url);
		}
	}
	return QString();
}

QString BrandLogos::cachedPath(const QString& id)
{
	if (sourceUrl(id).isEmpty()) {
		return QString();
	}
	const QDir dir{QLatin1String(kCacheDir)};
	for (const char* suffix : { "svg", "png" }) {
		const QString path =
			dir.filePath(id + QLatin1Char('.') + QLatin1String(suffix));
		if (QFileInfo::exists(path)) {
			return path;
		}
	}
	return QString();
}

QString BrandLogos::url(const QString& id)
{
	const QString path = cachedPath(id);
	if (!path.isEmpty()) {
		/* The revision makes a re-download a new URL, past the image
		 * cache of whoever showed the old file. */
		QUrl url = QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath());
		url.setQuery(QStringLiteral("r=%1").arg(m_revision));
		return url.toString();
	}
	if (!sourceUrl(id).isEmpty()) {
		fetch(id);
	}
	return QString();
}

QByteArray BrandLogos::normalised(const QByteArray& bytes, QString* suffix)
{
	QBuffer buffer;
	buffer.setData(bytes);
	buffer.open(QIODevice::ReadOnly);
	QImageReader reader(&buffer);
	const QByteArray format = reader.format();
	if (format.isEmpty()) {
		return QByteArray();
	}
	if (format == "svg" || format == "svgz") {
		if (reader.read().isNull()) {
			return QByteArray();
		}
		*suffix = QStringLiteral("svg");
		return bytes;
	}

	/* The largest of the sizes the file holds. */
	QImage best;
	const int count = qMax(1, reader.imageCount());
	for (int i = 0; i < count; ++i) {
		if (i > 0 && !reader.jumpToImage(i)) {
			break;
		}
		const QImage image = reader.read();
		if (!image.isNull() && image.width() * image.height() >
								   best.width() * best.height()) {
			best = image;
		}
	}
	if (best.isNull()) {
		return QByteArray();
	}
	QByteArray png;
	QBuffer out(&png);
	out.open(QIODevice::WriteOnly);
	best.save(&out, "PNG");
	*suffix = QStringLiteral("png");
	return png;
}

void BrandLogos::fetch(const QString& id)
{
	auto* context = LauncherContext::instance();
	if (!context || !context->network() || m_fetching.contains(id)) {
		return;
	}
	const auto failed = m_failedAt.constFind(id);
	if (failed != m_failedAt.constEnd() &&
		QDateTime::currentMSecsSinceEpoch() - failed.value() < kRetryAfterMs) {
		return;
	}
	m_fetching.insert(id);
	QNetworkReply* reply =
		context->network()->get(QNetworkRequest(QUrl(sourceUrl(id))));
	connect(reply, &QNetworkReply::finished, this,
			[this, reply, id] { onFinished(reply, id); });
}

void BrandLogos::onFinished(QNetworkReply* reply, const QString& id)
{
	reply->deleteLater();
	m_fetching.remove(id);

	QString suffix;
	QByteArray kept;
	if (reply->error() == QNetworkReply::NoError) {
		const QByteArray bytes = reply->read(kMaxBytes);
		kept = normalised(bytes, &suffix);
	} else {
		qWarning() << "Could not fetch the" << id
				   << "logo:" << reply->errorString();
	}
	if (kept.isEmpty()) {
		m_failedAt.insert(id, QDateTime::currentMSecsSinceEpoch());
		return;
	}
	m_failedAt.remove(id);

	const QString path = QDir{QLatin1String(kCacheDir)}
							 .filePath(id + QLatin1Char('.') + suffix);
	QSaveFile file(path);
	if (!QDir().mkpath(QFileInfo(path).absolutePath()) ||
		!file.open(QIODevice::WriteOnly) || file.write(kept) != kept.size() ||
		!file.commit()) {
		qWarning() << "Could not keep the" << id << "logo at" << path;
		m_failedAt.insert(id, QDateTime::currentMSecsSinceEpoch());
		return;
	}
	++m_revision;
	emit changed();
}
