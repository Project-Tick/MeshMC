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

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

class QNetworkReply;

/*
 * The logos of the modpack sites whose owners do not license them for
 * redistribution (Modrinth, CurseForge, ATLauncher): not shipped with
 * MeshMC, but fetched from each owner's own site the first time the
 * interface shows them, and kept in the data folder's cache
 * (cache/brands/<id>.png or .svg) from then on, offline too. The logos
 * whose owners do license them openly are in the resources instead
 * (resources/meshmc/brands, see the .license file beside each).
 *
 * The same pattern as DefaultSkins: url() answers from the cache, or ""
 * while there is nothing there yet and starts the download; `revision`
 * moves when one lands, so a binding through url() asks again. A failed
 * download is tried again a few minutes later, not on every call.
 *
 * Core: QtCore/QtGui/QtNetwork only, no QtWidgets, no ui/.
 */
class BrandLogos : public QObject
{
	Q_OBJECT
	/// Moves whenever a logo arrives in the cache.
	Q_PROPERTY(int revision READ revision NOTIFY changed)

  public:
	/// The one instance, created on first use.
	static BrandLogos* instance();

	/// The cached logo of @p id ("modrinth", "curseforge", "atlauncher") as
	/// a file URL an Image can load; "" until it is there -- the download
	/// then starts, if it is not already running -- and for any other id.
	Q_INVOKABLE QString url(const QString& id);

	int revision() const
	{
		return m_revision;
	}

	/// The ids fetched here.
	static QStringList ids();
	/// Where @p id's logo is downloaded from: the owner's own site; empty
	/// for an id that is not fetched.
	static QString sourceUrl(const QString& id);
	/// The cached file of @p id if there is one, else "": relative to the
	/// data folder, which is the working directory.
	static QString cachedPath(const QString& id);
	/*!
	 * What is kept of a download: an SVG as it came, any other image as a
	 * PNG of its largest frame (a favicon.ico holds several sizes).
	 * @p suffix becomes "svg" or "png". Empty for bytes that are no image.
	 */
	static QByteArray normalised(const QByteArray& bytes, QString* suffix);

  signals:
	void changed();

  private:
	explicit BrandLogos(QObject* parent);
	void fetch(const QString& id);
	void onFinished(QNetworkReply* reply, const QString& id);

	QSet<QString> m_fetching;
	/// When each failed download failed (msecs since epoch).
	QHash<QString, qint64> m_failedAt;
	int m_revision = 0;
};
