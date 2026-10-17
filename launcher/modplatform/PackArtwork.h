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
#include <QPointer>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QVariantList>

class BaseInstance;

/*
 * A platform pack's own pictures, for an instance installed from it: the
 * cover photo its catalogue leads with (InstanceList's PackCoverRole, the
 * picture on the instance's card) and its logo, which becomes the
 * instance's icon while that is still the stock one the install picked.
 *
 * Both come from what the instance recorded about its pack: PackCoverUrl
 * and PackIconUrl, which an install from Discover fills in because the
 * pack's page already had them at hand. An instance that lacks the cover
 * record - installed by an older MeshMC, or updated since - has its pack
 * looked up once, by the pack id it did record, and the answer recorded,
 * so the catalogue is asked once per instance rather than once per
 * launch. The pictures themselves are downloaded once as well: the cover
 * into the launcher's cache folder, named after its URL, the logo into
 * the icons folder like any other icon.
 *
 * Everything that touches the network or decodes an image happens after
 * the call that asked for it has returned, and every failure leaves the
 * instance looking as it did before - the card falls back to its
 * screenshot or scene, the icon stays the stock one - and is not retried
 * until the next launch.
 */
class PackArtwork : public QObject
{
	Q_OBJECT

  public:
	explicit PackArtwork(QObject* parent = nullptr);

	/* Where the covers are kept. Defaults to "PackCovers" in the
	 * launcher's cache folder; set by tests, which have no launcher. */
	void setCacheDir(const QString& dir);

	/* file:// URL of @p instance's cached cover photo, or an empty string
	 * when it has none, or none yet - in which case it is fetched (see
	 * ensure()) and coverChanged() says when it is there.
	 *
	 * A cover already on disk is answered straight away, so a card shows
	 * its photo from the first paint rather than flashing its fallback
	 * on every launch; that costs one stat() per instance per session,
	 * after which the answer comes from memory. */
	QString coverFor(BaseInstance* instance);

	/* Starts whatever @p instance's pack pictures still need, once per
	 * session: the catalogue lookup, the cover download, the logo
	 * download. Nothing for an instance that is not a platform pack.
	 * Asked from every view that shows the instance's icon, so the logo
	 * arrives even where no cover is on screen. */
	void ensure(BaseInstance* instance);

	/* Drops what is known about @p instanceId, for an instance that has
	 * gone away. */
	void forget(const QString& instanceId);

	/* ---- The decisions, kept apart from the fetching so they can be
	 * tested without a network ------------------------------------- */

	/* What the catalogue's record of a pack gave. `recognised` is false
	 * when the reply was not a record of a pack at all (an error body, a
	 * page from a proxy): then nothing it lacks may be recorded as
	 * missing. `title` is only for the log, to say which pack an id
	 * turned out to be. */
	struct Found {
		bool recognised = false;
		QString title;
		QString cover;
		QString logo;
	};

	/* "curseforge" for "flame" (what upstream launchers call it), and
	 * every provider lower-cased - the spellings PackProvider has had. */
	static QString canonicalProvider(const QString& provider);

	/* The endpoint whose reply names @p packId's pictures on
	 * @p provider: Modrinth's project, CurseForge's mod (only with an API
	 * key in this build), modpacks.ch's modpack. Empty for an empty id
	 * and for platforms with no such record (ATLauncher, Technic). */
	static QUrl lookupUrl(const QString& provider, const QString& packId);

	/* The cover and logo in @p reply from lookupUrl(): Modrinth's
	 * featured gallery image, else its first, at full size (raw_url)
	 * where the reply has it - its "url" is a 350-pixel thumbnail, too
	 * small for a card - and its icon_url; CurseForge's first screenshot
	 * and its logo; modpacks.ch's splash art and its square art. Either
	 * is empty when the pack has none, or the reply is not one. */
	static Found parseLookup(const QString& provider, const QByteArray& reply);

	/* The logo that can be named from the id alone, without a lookup:
	 * ATLauncher's, which its CDN keeps under the pack's safe name. */
	static QString derivedLogoUrl(const QString& provider,
								  const QString& packId);

	/* The cover in a Discover pack page's gallery (entries {url, rawUrl,
	 * featured}, as the modpack models hand QML): the featured entry,
	 * else the first, full size where known. */
	static QString coverFromGallery(const QVariantList& gallery);

	/* Where the cover at @p url is kept in @p cacheDir: named after a
	 * hash of the URL, so a pack that changes its cover gets the new
	 * picture rather than the old file, keeping the picture's own
	 * extension as a hint for the image reader. */
	static QString cachePath(const QString& cacheDir, const QString& url);

	/* Whether @p iconKey is one of the stock icons an install from
	 * @p provider falls back to when it has no logo of the pack's own -
	 * the only icons the pack's logo may replace. A key the user picked
	 * is never stock, except where the user picked the very icon the
	 * install would have: no way to tell those apart, and the pack's own
	 * logo is the better guess. */
	static bool isStockIconKey(const QString& provider, const QString& iconKey);

	/* The icon key a pack's downloaded logo is filed under:
	 * "<provider>_<pack id>", with anything but letters, digits, '-' and
	 * '_' replaced - the key is the file's name up to its first dot. */
	static QString iconKeyFor(const QString& provider, const QString& packId);

  signals:
	/* @p instanceId's cover became available (coverFor() now has it). */
	void coverChanged(const QString& instanceId);

  private:
	QString cacheDir() const;
	void lookUp(QPointer<BaseInstance> instance);
	void fetchPictures(QPointer<BaseInstance> instance);
	void fetchCover(const QString& instanceId, const QString& url);
	void fetchLogo(QPointer<BaseInstance> instance, const QString& url);
	void useLogo(QPointer<BaseInstance> instance, const QString& key);
	void publishCover(const QString& instanceId, const QString& path);

	QString m_cacheDir;
	/* file:// URL of each instance's cover, by instance id; an empty
	 * string is "none, for this session". An id absent from here has not
	 * been asked about yet. */
	QHash<QString, QString> m_covers;
	/* Instances ensure() has already started on this session. */
	QSet<QString> m_started;
};
