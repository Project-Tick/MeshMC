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

#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QTest>

/*
 * artwork.js's cover(): which of an instance's pictures its card shows.
 * The script is a QML JavaScript library; its one QML-only line (the
 * ".pragma library" header) is dropped and the rest run in a plain
 * QJSEngine, which is all its functions need.
 */
class QmlArtworkTest : public QObject
{
	Q_OBJECT

	QJSEngine m_engine;

	// cover() for an entry with InstanceList's role names, as "source|logo".
	QString coverOf(const QVariantMap& entry)
	{
		QJSValue result = m_engine.globalObject()
							  .property(QStringLiteral("cover"))
							  .call({m_engine.toScriptValue(entry)});
		if (result.isError()) {
			return QStringLiteral("error: ") + result.toString();
		}
		return result.property(QStringLiteral("source")).toString() +
			   QLatin1Char('|') +
			   (result.property(QStringLiteral("logo")).toBool()
					? QStringLiteral("logo")
					: QStringLiteral("picture"));
	}

  private slots:
	void initTestCase()
	{
		QFile file(QStringLiteral(ARTWORK_JS));
		QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
		QStringList lines;
		for (const QString& line : QString::fromUtf8(file.readAll()).split('\n')) {
			if (!line.trimmed().startsWith(QLatin1String(".pragma"))) {
				lines.append(line);
			}
		}
		const QJSValue loaded = m_engine.evaluate(lines.join('\n'));
		QVERIFY2(!loaded.isError(), qPrintable(loaded.toString()));
	}

	// The pack's cover photo comes first, ahead of everything else, and
	// is cropped like a picture.
	void packCoverWinsAndIsAPicture()
	{
		QCOMPARE(coverOf({{"instanceId", "atm"},
						  {"packCover", "file:///covers/atm.png"},
						  {"coverImage", "file:///shots/newest.png"},
						  {"packArt", "image://instanceicon/curseforge_925200"}}),
				 QStringLiteral("file:///covers/atm.png|picture"));
	}

	// Without a cover, a screenshot fills the card before the pack's
	// logo would sit in the middle of it.
	void screenshotBeforePackLogo()
	{
		QCOMPARE(coverOf({{"instanceId", "atm"},
						  {"packCover", ""},
						  {"coverImage", "file:///shots/newest.png"},
						  {"packArt", "image://instanceicon/curseforge_925200"}}),
				 QStringLiteral("file:///shots/newest.png|picture"));
	}

	void packLogoShownWhole()
	{
		QCOMPARE(coverOf({{"instanceId", "atm"},
						  {"packArt", "image://instanceicon/curseforge_925200"}}),
				 QStringLiteral("image://instanceicon/curseforge_925200|logo"));
	}

	// No picture of its own: the instance's icon, shown whole like a logo.
	void iconLast()
	{
		QCOMPARE(coverOf({{"instanceId", "vanilla"}, {"iconKey", "creeper"}}),
				 QStringLiteral("image://instanceicon/creeper|logo"));
		QCOMPARE(coverOf({{"instanceId", "vanilla"}, {"iconKey", ""}}),
				 QStringLiteral("image://instanceicon/default|logo"));
	}

	// Only the icon fallback says so: the instance page's banner, which
	// already shows the icon, leaves it out.
	void onlyTheIconIsMarked()
	{
		const auto isIcon = [this](const QVariantMap& entry) {
			return m_engine.globalObject()
				.property(QStringLiteral("cover"))
				.call({m_engine.toScriptValue(entry)})
				.property(QStringLiteral("icon"))
				.toBool();
		};
		QVERIFY(isIcon({{"instanceId", "vanilla"}}));
		QVERIFY(!isIcon({{"instanceId", "atm"},
						 {"packArt", "image://instanceicon/curseforge_925200"}}));
		QVERIFY(!isIcon({{"instanceId", "atm"},
						 {"coverImage", "file:///shots/newest.png"}}));
	}
};

QTEST_GUILESS_MAIN(QmlArtworkTest)

#include "QmlArtwork_test.moc"
