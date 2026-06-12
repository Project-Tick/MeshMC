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
// Feed parsing — structured attribute selection
// ---------------------------------------------------------------------------

void UpdateCheckerTest::tst_ParseStableFeedItem_StructuredMatch()
{
	const QString xml =
		QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<rss xmlns:projt="https://projecttick.org/ns/product-feed" version="2.0">
	<channel>
		<item>
			<title>MeshMC Update 7.19.0</title>
			<description><![CDATA[<p>Release notes</p>]]></description>
			<projt:version>7.19.0</projt:version>
			<projt:channel>stable</projt:channel>
			<projt:asset name="MeshMC-Linux-Portable.tar.gz"
			             url="https://example.invalid/linux-portable.tar.gz"
			             platform="linux" arch="x86_64" portable="true"
			             kind="archive" sha256="aaaabbbb" size="100" />
			<projt:asset name="MeshMC-Linux-aarch64-Portable.tar.gz"
			             url="https://example.invalid/linux-aarch64.tar.gz"
			             platform="linux" arch="aarch64" portable="true"
			             kind="archive" sha256="ccccdddd" size="200" />
			<projt:asset name="MeshMC-Windows-MSVC-Setup.exe"
			             url="https://example.invalid/windows.exe"
			             platform="windows" arch="x86_64" portable="false"
			             kind="installer" sha256="eeeeffff" size="300" />
		</item>
	</channel>
</rss>)");

	UpdateChecker::BuildIdentity id;
	id.platform = QStringLiteral("linux");
	id.arch = QStringLiteral("x86_64");
	id.portable = QStringLiteral("true");
	id.kind = QStringLiteral("archive");

	QString version, downloadUrl, releaseNotes, sha256, parseError;
	qint64 fileSize = 0;

	QVERIFY(UpdateChecker::parseStableFeedItem(
		xml.toUtf8(), id, &version, &downloadUrl, &releaseNotes, &sha256,
		&fileSize, &parseError));
	QCOMPARE(version, QStringLiteral("7.19.0"));
	QCOMPARE(downloadUrl,
			 QStringLiteral("https://example.invalid/linux-portable.tar.gz"));
	QCOMPARE(releaseNotes, QStringLiteral("<p>Release notes</p>"));
	QCOMPARE(sha256, QStringLiteral("aaaabbbb"));
	QCOMPARE(fileSize, qint64(100));
	QVERIFY(parseError.isEmpty());
}

void UpdateCheckerTest::
	tst_ParseStableFeedItem_StructuredArchSelectsCorrectAsset()
{
	const QString xml =
		QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<rss xmlns:projt="https://projecttick.org/ns/product-feed" version="2.0">
	<channel>
		<item>
			<projt:version>7.19.0</projt:version>
			<projt:channel>stable</projt:channel>
			<projt:asset name="x86_64.tar.gz"
			             url="https://example.invalid/x86.tar.gz"
			             platform="linux" arch="x86_64" portable="true"
			             kind="archive" />
			<projt:asset name="aarch64.tar.gz"
			             url="https://example.invalid/arm64.tar.gz"
			             platform="linux" arch="aarch64" portable="true"
			             kind="archive" />
		</item>
	</channel>
</rss>)");

	UpdateChecker::BuildIdentity id;
	id.platform = QStringLiteral("linux");
	id.arch = QStringLiteral("aarch64");
	id.portable = QStringLiteral("true");
	id.kind = QStringLiteral("archive");

	QString version, downloadUrl, releaseNotes, sha256, parseError;
	qint64 fileSize = 0;

	QVERIFY(UpdateChecker::parseStableFeedItem(
		xml.toUtf8(), id, &version, &downloadUrl, &releaseNotes, &sha256,
		&fileSize, &parseError));
	QCOMPARE(downloadUrl,
			 QStringLiteral("https://example.invalid/arm64.tar.gz"));
}

void UpdateCheckerTest::tst_ParseStableFeedItem_LegacyArtifactFallback()
{
	const QString xml =
		QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<rss xmlns:projt="https://projecttick.org/ns/product-feed" version="2.0">
	<channel>
		<item>
			<projt:version>7.1.0</projt:version>
			<projt:channel>stable</projt:channel>
			<projt:asset name="MeshMC-Linux-Qt6-Portable-v202604141638.tar.gz"
			             url="https://example.invalid/meshmc.tar.gz" />
		</item>
	</channel>
</rss>)");

	UpdateChecker::BuildIdentity id;
	id.legacyArtifact = QStringLiteral("Linux-Qt6-Portable");

	QString version, downloadUrl, releaseNotes, sha256, parseError;
	qint64 fileSize = 0;

	QVERIFY(UpdateChecker::parseStableFeedItem(
		xml.toUtf8(), id, &version, &downloadUrl, &releaseNotes, &sha256,
		&fileSize, &parseError));
	QCOMPARE(version, QStringLiteral("7.1.0"));
	QCOMPARE(downloadUrl,
			 QStringLiteral("https://example.invalid/meshmc.tar.gz"));
	QVERIFY(parseError.isEmpty());
}

void UpdateCheckerTest::tst_ParseStableFeedItem_NoMatchingAssetForBuild()
{
	const QString xml =
		QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<rss xmlns:projt="https://projecttick.org/ns/product-feed" version="2.0">
	<channel>
		<item>
			<projt:version>7.19.0</projt:version>
			<projt:channel>stable</projt:channel>
			<projt:asset name="windows.exe" url="https://example.invalid/w.exe"
			             platform="windows" arch="x86_64" portable="false"
			             kind="installer" />
		</item>
	</channel>
</rss>)");

	UpdateChecker::BuildIdentity id;
	id.platform = QStringLiteral("linux");
	id.arch = QStringLiteral("x86_64");
	id.portable = QStringLiteral("true");
	id.kind = QStringLiteral("archive");

	QString version, downloadUrl, releaseNotes, sha256, parseError;
	qint64 fileSize = 0;

	// The item parses but no asset matches the running build — version is
	// reported but downloadUrl stays empty.
	QVERIFY(UpdateChecker::parseStableFeedItem(
		xml.toUtf8(), id, &version, &downloadUrl, &releaseNotes, &sha256,
		&fileSize, &parseError));
	QCOMPARE(version, QStringLiteral("7.19.0"));
	QVERIFY(downloadUrl.isEmpty());
}

void UpdateCheckerTest::tst_ParseStableFeedItem_ReportsMalformedXml()
{
	UpdateChecker::BuildIdentity id;
	id.legacyArtifact = QStringLiteral("Linux-Qt6-Portable");

	QString version, downloadUrl, releaseNotes, sha256, parseError;
	qint64 fileSize = 0;

	QVERIFY(!UpdateChecker::parseStableFeedItem(
		"<rss><channel><item>", id, &version, &downloadUrl, &releaseNotes,
		&sha256, &fileSize, &parseError));
	QVERIFY2(!parseError.isEmpty(),
			 "Expected a parse error for malformed XML, got empty string");
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
