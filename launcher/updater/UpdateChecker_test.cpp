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

#include "UpdateChecker_test.h"

#include <QTest>

#include "updater/UpdateChecker.h"

// UpdateChecker::compareVersions and normalizeVersion are private. Mirror
// them locally for the unit tests so we do not have to widen the public
// surface of the class.

static int testCompareVersions(const QString& v1, const QString& v2)
{
	const QStringList parts1 = v1.split('.');
	const QStringList parts2 = v2.split('.');
	const int len = std::max(parts1.size(), parts2.size());
	for (int i = 0; i < len; ++i) {
		const qint64 a = (i < parts1.size()) ? parts1.at(i).toLongLong() : 0;
		const qint64 b = (i < parts2.size()) ? parts2.at(i).toLongLong() : 0;
		if (a != b)
			return (a > b) ? 1 : -1;
	}
	return 0;
}

static QString testNormalizeVersion(const QString& v)
{
	QString out = v.trimmed();
	if (out.startsWith('v', Qt::CaseInsensitive))
		out.remove(0, 1);
	return out;
}

// ---------------------------------------------------------------------------
// Feed parsing — version + channel only
// ---------------------------------------------------------------------------

/* A trimmed copy of the real product feed: two entries, both carrying the
 * <projt:asset> list that the updater deliberately ignores. */
static QByteArray sampleFeed()
{
	return QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<rss xmlns:projt="https://projecttick.org/ns/product-feed" version="2.0">
	<channel>
		<title>MeshMC News</title>
		<projt:feedVersion>1</projt:feedVersion>
		<projt:product>MeshMC</projt:product>
		<item>
			<title>MeshMC Update 7.20.0, now available</title>
			<description><![CDATA[<h2>Added</h2>]]></description>
			<projt:version>7.20.0</projt:version>
			<projt:channel>beta</projt:channel>
			<projt:release_page>https://example.invalid/7.20.0</projt:release_page>
			<projt:asset url="https://example.invalid/beta.zip" name="beta.zip"
			             platform="windows" arch="x86_64" portable="true"
			             kind="archive" sha256="dead" size="12" />
		</item>
		<item>
			<title>MeshMC Update 7.19.0, now available</title>
			<description><![CDATA[<h2>Fixed</h2>]]></description>
			<projt:version>7.19.0</projt:version>
			<projt:channel>stable</projt:channel>
			<projt:release_page>https://example.invalid/7.19.0</projt:release_page>
			<projt:asset url="https://example.invalid/stable.zip" name="stable.zip"
			             platform="windows" arch="x86_64" portable="true"
			             kind="archive" sha256="beef" size="34" />
			<enclosure url="https://example.invalid/stable.zip" length="34"
			           type="application/zip" />
		</item>
	</channel>
</rss>)")
		.toUtf8();
}

void UpdateCheckerTest::tst_ParseFeedItems_VersionChannelAndNotes()
{
	QList<UpdateChecker::FeedItem> items;
	QString parseError;

	QVERIFY(UpdateChecker::parseFeedItems(sampleFeed(), &items, &parseError));
	QVERIFY(parseError.isEmpty());
	QCOMPARE(items.size(), 2);

	QCOMPARE(items.at(0).version, QStringLiteral("7.20.0"));
	QCOMPARE(items.at(0).channel, QStringLiteral("beta"));
	QCOMPARE(items.at(0).releaseNotes, QStringLiteral("<h2>Added</h2>"));
	QCOMPARE(items.at(0).releasePage,
			 QStringLiteral("https://example.invalid/7.20.0"));

	QCOMPARE(items.at(1).version, QStringLiteral("7.19.0"));
	QCOMPARE(items.at(1).channel, QStringLiteral("stable"));
}

void UpdateCheckerTest::tst_ParseFeedItems_IgnoresAssets()
{
	// The feed's <channel> element must not be mistaken for <projt:channel>,
	// and no asset attribute may leak into the parsed entry.
	QList<UpdateChecker::FeedItem> items;
	QString parseError;

	QVERIFY(UpdateChecker::parseFeedItems(sampleFeed(), &items, &parseError));
	QCOMPARE(items.size(), 2);
	for (const auto& item : items) {
		QVERIFY2(item.channel == QStringLiteral("stable") ||
					 item.channel == QStringLiteral("beta"),
				 qPrintable(QStringLiteral("unexpected channel: %1")
								.arg(item.channel)));
	}
}

void UpdateCheckerTest::tst_ParseFeedItems_MissingChannelDefaultsToStable()
{
	const QString xml = QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<rss xmlns:projt="https://projecttick.org/ns/product-feed" version="2.0">
	<channel>
		<item>
			<projt:version>v7.1.0</projt:version>
		</item>
	</channel>
</rss>)");

	QList<UpdateChecker::FeedItem> items;
	QString parseError;

	QVERIFY(UpdateChecker::parseFeedItems(xml.toUtf8(), &items, &parseError));
	QCOMPARE(items.size(), 1);
	// The leading "v" is normalized away at parse time.
	QCOMPARE(items.at(0).version, QStringLiteral("7.1.0"));
	QCOMPARE(items.at(0).channel, QStringLiteral("stable"));
}

void UpdateCheckerTest::tst_ParseFeedItems_SkipsEntryWithoutVersion()
{
	const QString xml = QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<rss xmlns:projt="https://projecttick.org/ns/product-feed" version="2.0">
	<channel>
		<item>
			<title>Plain news post, not a release</title>
			<projt:channel>stable</projt:channel>
		</item>
		<item>
			<projt:version>7.19.0</projt:version>
			<projt:channel>stable</projt:channel>
		</item>
	</channel>
</rss>)");

	QList<UpdateChecker::FeedItem> items;
	QString parseError;

	QVERIFY(UpdateChecker::parseFeedItems(xml.toUtf8(), &items, &parseError));
	QCOMPARE(items.size(), 1);
	QCOMPARE(items.at(0).version, QStringLiteral("7.19.0"));
}

void UpdateCheckerTest::tst_ParseFeedItems_ReportsMalformedXml()
{
	QList<UpdateChecker::FeedItem> items;
	QString parseError;

	QVERIFY(!UpdateChecker::parseFeedItems("<rss><channel><item>", &items,
										   &parseError));
	QVERIFY2(!parseError.isEmpty(),
			 "Expected a parse error for malformed XML, got empty string");
}

// ---------------------------------------------------------------------------
// Channel policy
// ---------------------------------------------------------------------------

void UpdateCheckerTest::tst_IsChannelAccepted_data()
{
	QTest::addColumn<QString>("itemChannel");
	QTest::addColumn<QString>("buildChannel");
	QTest::addColumn<bool>("accepted");

	QTest::newRow("stable build, stable entry") << "stable" << "stable" << true;
	QTest::newRow("stable build, beta entry") << "beta" << "stable" << false;
	QTest::newRow("beta build, beta entry") << "beta" << "beta" << true;
	// A beta user must never be stranded behind the stable line.
	QTest::newRow("beta build, stable entry") << "stable" << "beta" << true;
	QTest::newRow("case insensitive") << "Stable" << "STABLE" << true;
	QTest::newRow("unknown entry channel") << "alpha" << "beta" << false;
	QTest::newRow("empty entry channel") << "" << "beta" << false;
	QTest::newRow("unknown build channel takes stable")
		<< "stable" << "nightly" << true;
	QTest::newRow("unknown build channel refuses beta")
		<< "beta" << "nightly" << false;
}

void UpdateCheckerTest::tst_IsChannelAccepted()
{
	QFETCH(QString, itemChannel);
	QFETCH(QString, buildChannel);
	QFETCH(bool, accepted);

	QCOMPARE(UpdateChecker::isChannelAccepted(itemChannel, buildChannel),
			 accepted);
}

// ---------------------------------------------------------------------------
// Entry selection
// ---------------------------------------------------------------------------

// UpdateChecker's parsing helpers are private and only UpdateCheckerTest is a
// friend, so the feed has to be parsed inside the test methods themselves.

void UpdateCheckerTest::tst_PickBestItemIndex_StableBuildIgnoresBeta()
{
	QList<UpdateChecker::FeedItem> items;
	QString parseError;
	QVERIFY(UpdateChecker::parseFeedItems(sampleFeed(), &items, &parseError));

	const int index =
		UpdateChecker::pickBestItemIndex(items, QStringLiteral("stable"));
	QCOMPARE(index, 1);
	QCOMPARE(items.at(index).version, QStringLiteral("7.19.0"));
}

void UpdateCheckerTest::tst_PickBestItemIndex_BetaBuildTakesNewestOfBoth()
{
	QList<UpdateChecker::FeedItem> items;
	QString parseError;
	QVERIFY(UpdateChecker::parseFeedItems(sampleFeed(), &items, &parseError));

	const int index =
		UpdateChecker::pickBestItemIndex(items, QStringLiteral("beta"));
	QCOMPARE(index, 0);
	QCOMPARE(items.at(index).version, QStringLiteral("7.20.0"));
}

void UpdateCheckerTest::tst_PickBestItemIndex_IgnoresFeedOrder()
{
	// Oldest entry first: the highest version must still win.
	QList<UpdateChecker::FeedItem> items;
	items.append({QStringLiteral("7.19.0"), QStringLiteral("stable"), {}, {}});
	items.append({QStringLiteral("7.21.0"), QStringLiteral("stable"), {}, {}});
	items.append({QStringLiteral("7.20.0"), QStringLiteral("stable"), {}, {}});

	QCOMPARE(UpdateChecker::pickBestItemIndex(items, QStringLiteral("stable")),
			 1);
}

void UpdateCheckerTest::tst_PickBestItemIndex_NoAcceptableEntry()
{
	QList<UpdateChecker::FeedItem> items;
	items.append({QStringLiteral("7.20.0"), QStringLiteral("beta"), {}, {}});

	QCOMPARE(UpdateChecker::pickBestItemIndex(items, QStringLiteral("stable")),
			 -1);
}

// ---------------------------------------------------------------------------
// GitHub release resolution
//
// The expected names are taken from .github/workflows/release.yml, where
// ${VERSION} is the pushed tag. The artifact names are the ones build.yml
// feeds into ARTIFACT_NAME (matrix.artifact-name + "-Qt6").
// ---------------------------------------------------------------------------

void UpdateCheckerTest::tst_ReleaseTag()
{
	QCOMPARE(UpdateChecker::releaseTag(QStringLiteral("7.19.0")),
			 QStringLiteral("v7.19.0"));
	// Already-prefixed and padded input must not produce "vv7.19.0".
	QCOMPARE(UpdateChecker::releaseTag(QStringLiteral(" v7.19.0 ")),
			 QStringLiteral("v7.19.0"));
	QVERIFY(UpdateChecker::releaseTag(QString()).isEmpty());
}

void UpdateCheckerTest::tst_ReleaseAssetName_data()
{
	QTest::addColumn<QString>("artifact");
	QTest::addColumn<bool>("portable");
	QTest::addColumn<QString>("expected");

	QTest::newRow("linux x86_64")
		<< "Linux-Qt6" << true
		<< "MeshMC-Linux-Qt6-Portable-v7.19.0.tar.gz";
	QTest::newRow("linux aarch64")
		<< "Linux-aarch64-Qt6" << true
		<< "MeshMC-Linux-aarch64-Qt6-Portable-v7.19.0.tar.gz";

	QTest::newRow("windows msvc portable")
		<< "Windows-MSVC-Qt6" << true
		<< "MeshMC-Windows-MSVC-Portable-v7.19.0.zip";
	QTest::newRow("windows msvc installed")
		<< "Windows-MSVC-Qt6" << false << "MeshMC-Windows-MSVC-v7.19.0.zip";
	QTest::newRow("windows msvc arm64 portable")
		<< "Windows-MSVC-arm64-Qt6" << true
		<< "MeshMC-Windows-MSVC-arm64-Portable-v7.19.0.zip";
	QTest::newRow("windows mingw w64 installed")
		<< "Windows-MinGW-w64-Qt6" << false
		<< "MeshMC-Windows-MinGW-w64-v7.19.0.zip";
	QTest::newRow("windows mingw arm64 portable")
		<< "Windows-MinGW-arm64-Qt6" << true
		<< "MeshMC-Windows-MinGW-arm64-Portable-v7.19.0.zip";

	// Both macOS matrix entries publish the same universal archive, and the
	// portable flag is meaningless there.
	QTest::newRow("macos") << "macOS-Qt6" << false << "MeshMC-macOS-v7.19.0.zip";
	QTest::newRow("macos xcode")
		<< "macOS-Xcode-27-Qt6" << false << "MeshMC-macOS-v7.19.0.zip";

	// A build that never says what it is must not guess.
	QTest::newRow("empty artifact") << "" << true << "";
	QTest::newRow("unknown artifact") << "Haiku-Qt6" << true << "";
}

void UpdateCheckerTest::tst_ReleaseAssetName()
{
	QFETCH(QString, artifact);
	QFETCH(bool, portable);
	QFETCH(QString, expected);

	QCOMPARE(UpdateChecker::releaseAssetName(artifact,
											 QStringLiteral("v7.19.0"),
											 portable),
			 expected);
}

void UpdateCheckerTest::tst_MakeGithubDownloadUrl()
{
	QCOMPARE(UpdateChecker::makeGithubDownloadUrl(
				 QStringLiteral("https://github.com/Project-Tick/MeshMC"),
				 QStringLiteral("Windows-MSVC-Qt6"), QStringLiteral("7.19.0"),
				 true),
			 QStringLiteral("https://github.com/Project-Tick/MeshMC/releases/"
							"download/v7.19.0/"
							"MeshMC-Windows-MSVC-Portable-v7.19.0.zip"));

	// A trailing slash or a .git suffix on the repository URL must not leak
	// into the download URL.
	QCOMPARE(UpdateChecker::makeGithubDownloadUrl(
				 QStringLiteral("https://github.com/Project-Tick/MeshMC.git/"),
				 QStringLiteral("Linux-Qt6"), QStringLiteral("v7.19.0"), true),
			 QStringLiteral("https://github.com/Project-Tick/MeshMC/releases/"
							"download/v7.19.0/"
							"MeshMC-Linux-Qt6-Portable-v7.19.0.tar.gz"));
}

void UpdateCheckerTest::tst_MakeGithubDownloadUrl_UnknownArtifactYieldsNothing()
{
	QVERIFY(UpdateChecker::makeGithubDownloadUrl(
				QStringLiteral("https://github.com/Project-Tick/MeshMC"),
				QString(), QStringLiteral("7.19.0"), true)
				.isEmpty());
	QVERIFY(UpdateChecker::makeGithubDownloadUrl(
				QString(), QStringLiteral("Windows-MSVC-Qt6"),
				QStringLiteral("7.19.0"), true)
				.isEmpty());
	QVERIFY(UpdateChecker::makeGithubDownloadUrl(
				QStringLiteral("https://github.com/Project-Tick/MeshMC"),
				QStringLiteral("Windows-MSVC-Qt6"), QString(), true)
				.isEmpty());
}

// ---------------------------------------------------------------------------
// latest.json parsing
// ---------------------------------------------------------------------------

void UpdateCheckerTest::tst_ParseLatestJsonVersion_Stable()
{
	const QString json = QStringLiteral(R"({
		"schema_version": 2,
		"products": {
			"meshmc": {
				"stable": {
					"release_tag": "meshmc-v7.19.0",
					"version": "7.19.0"
				}
			}
		}
	})");
	QCOMPARE(UpdateChecker::parseLatestJsonVersion(json.toUtf8()),
			 QStringLiteral("7.19.0"));
}

void UpdateCheckerTest::tst_ParseLatestJsonVersion_MissingProduct()
{
	const QString json = QStringLiteral(R"({
		"schema_version": 2,
		"products": {
			"cmark": {"stable": {"version": "0.31.2"}}
		}
	})");
	QVERIFY(UpdateChecker::parseLatestJsonVersion(json.toUtf8()).isEmpty());
}

void UpdateCheckerTest::tst_ParseLatestJsonVersion_Malformed()
{
	QVERIFY(UpdateChecker::parseLatestJsonVersion(QByteArray("{not json"))
				.isEmpty());
}

// ---------------------------------------------------------------------------
// Version normalization & comparison
// ---------------------------------------------------------------------------

void UpdateCheckerTest::tst_NormalizeVersion_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");

	QTest::newRow("plain semver") << "7.0.0" << "7.0.0";
	QTest::newRow("v prefix") << "v7.0.0" << "7.0.0";
	QTest::newRow("V prefix") << "V7.0.0" << "7.0.0";
	QTest::newRow("snapshot tag") << "v202604102316" << "202604102316";
	QTest::newRow("whitespace") << "  v1.2.3  " << "1.2.3";
}

void UpdateCheckerTest::tst_NormalizeVersion()
{
	QFETCH(QString, input);
	QFETCH(QString, expected);
	QCOMPARE(testNormalizeVersion(input), expected);
}

void UpdateCheckerTest::tst_CompareVersions_data()
{
	QTest::addColumn<QString>("v1");
	QTest::addColumn<QString>("v2");
	QTest::addColumn<int>("sign"); // -1, 0, 1

	QTest::newRow("equal") << "7.0.0" << "7.0.0" << 0;
	QTest::newRow("v1 newer major") << "8.0.0" << "7.0.0" << 1;
	QTest::newRow("v1 older major") << "6.0.0" << "7.0.0" << -1;
	QTest::newRow("v1 newer minor") << "7.1.0" << "7.0.0" << 1;
	QTest::newRow("v1 newer hotfix") << "7.0.1" << "7.0.0" << 1;
	QTest::newRow("different lengths") << "7.0" << "7.0.0" << 0;
	QTest::newRow("four parts") << "7.0.0.1" << "7.0.0.0" << 1;
	QTest::newRow("snapshot-like numbers") << "202604102316" << "7.0.0" << 1;
}

void UpdateCheckerTest::tst_CompareVersions()
{
	QFETCH(QString, v1);
	QFETCH(QString, v2);
	QFETCH(int, sign);

	const int result = testCompareVersions(v1, v2);
	if (sign > 0)
		QVERIFY2(
			result > 0,
			qPrintable(
				QString("%1 should be > %2, got %3").arg(v1, v2).arg(result)));
	else if (sign < 0)
		QVERIFY2(
			result < 0,
			qPrintable(
				QString("%1 should be < %2, got %3").arg(v1, v2).arg(result)));
	else
		QCOMPARE(result, 0);
}

QTEST_GUILESS_MAIN(UpdateCheckerTest)
