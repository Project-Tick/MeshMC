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

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QString>
#include <QUrl>

#include "Version.h"

/*!
 * One downloadable file attached to a release.
 *
 * Mirrors the fields of the GitHub releases API that the updater actually
 * uses; the rest of the (large) asset object is dropped at parse time so that
 * nothing downstream can start depending on it by accident.
 */
struct GitHubReleaseAsset {
	qint64 id = -1;
	QString name;
	QString label;
	QString contentType;
	qint64 size = 0;
	QDateTime createdAt;
	QDateTime updatedAt;
	QUrl downloadUrl;

	/*!
	 * An asset is usable when the API gave it an identity *and* somewhere to
	 * download it from. The URL is checked here rather than at the download,
	 * so an incomplete asset is rejected while there are still alternatives
	 * to fall back to.
	 */
	bool isValid() const
	{
		return id > 0 && downloadUrl.isValid() && !downloadUrl.isEmpty();
	}
};

/*!
 * A single GitHub release, plus the parsed form of its tag.
 *
 * \a version is derived from \a tagName once, at parse time: every comparison
 * the updater makes goes through it, and re-parsing a tag at each comparison
 * is how two code paths end up disagreeing about which release is newer.
 */
struct GitHubRelease {
	qint64 id = -1;
	QString name;
	QString tagName;
	QDateTime createdAt;
	QDateTime publishedAt;
	bool prerelease = false;
	bool draft = false;
	QString body;
	QList<GitHubReleaseAsset> assets;
	Version version;

	bool isValid() const
	{
		return id > 0;
	}

	//! What to show the user: the release name, or the tag when it has none.
	QString displayName() const
	{
		return name.isEmpty() ? tagName : name;
	}
};

namespace GitHub
{

	/*!
	 * Strip the leading "v" from a release tag.
	 *
	 * Tags are published as "v7.19.0" while the launcher reports its own
	 * version as "7.19.0". Version splits on '.' and compares each part
	 * numerically where it can, but "v7" has no leading digits, so it
	 * degrades to a string comparison and "v7.19.0" stops being reliably
	 * newer than "7.18.0". Normalising once, where a tag becomes a Version,
	 * keeps every later comparison honest.
	 */
	QString normalizeVersionTag(const QString& tag);

	/*!
	 * Parse one page of `GET /repos/{owner}/{repo}/releases`.
	 *
	 * Appends to \a releases and returns the number of entries the page held,
	 * which is how the caller detects the last page (a short page is the end).
	 * Returns -1 and fills \a error when the response is not a release array
	 * at all -- a rate-limit message, an HTML error page, a truncated body.
	 *
	 * A single malformed entry inside an otherwise valid array is skipped with
	 * a warning rather than failing the page: one bad release must not make
	 * the launcher un-updatable.
	 */
	int parseReleasePage(const QByteArray& response,
						 QList<GitHubRelease>* releases, QString* error);

	/*!
	 * Turn a repository URL into the releases endpoint.
	 *
	 * Accepts the shapes a human or a build script might supply --
	 * "https://github.com/owner/repo", with or without ".git", with or
	 * without a trailing slash. Returns an empty string for anything that is
	 * not a GitHub repository URL, which the caller reports rather than
	 * quietly requesting a nonsense address.
	 */
	QString releasesApiUrl(const QUrl& repositoryUrl, QString* error);

} // namespace GitHub

QDebug operator<<(QDebug debug, const GitHubReleaseAsset& asset);
QDebug operator<<(QDebug debug, const GitHubRelease& release);
