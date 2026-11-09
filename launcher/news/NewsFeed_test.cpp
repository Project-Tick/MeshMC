/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QTest>

#include <QDateTime>
#include <QTimeZone>

#include <KLocalizedString>

#include "news/NewsFeed.h"

// What the news bar gets out of a feed: the RSS one that MeshMC's site
// publishes, and Atom, which Syndication reads as well.
class NewsFeedTest : public QObject
{
	Q_OBJECT

  private slots:
	void initTestCase()
	{
		KLocalizedString::setApplicationDomain("meshmc");
	}

	void test_AnRssFeedIsRead()
	{
		const QByteArray rss = QByteArray(
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<rss version=\"2.0\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n"
			" <channel>\n"
			"  <title>MeshMC</title>\n"
			"  <link>https://example.org/</link>\n"
			"  <description>News</description>\n"
			"  <item>\n"
			"   <title>Fish &amp; Chips</title>\n"
			"   <link>https://example.org/fish</link>\n"
			"   <description><![CDATA[<p>Hello <b>world</b></p>]]></description>\n"
			"   <dc:creator>Alice</dc:creator>\n"
			"   <pubDate>Mon, 02 Jan 2006 15:04:05 +0000</pubDate>\n"
			"  </item>\n"
			"  <item>\n"
			"   <title>Second</title>\n"
			"   <link>https://example.org/second</link>\n"
			"   <description>Plain</description>\n"
			"   <pubDate>Tue, 03 Jan 2006 10:00:00 +0200</pubDate>\n"
			"  </item>\n"
			" </channel>\n"
			"</rss>");
		QList<NewsEntryPtr> entries;
		QString error;
		QVERIFY2(News::parseFeed(rss, "https://example.org/feed.xml", 3,
								 &entries, &error),
				 qPrintable(error));
		QCOMPARE(entries.size(), 2);

		QCOMPARE(entries[0]->title, QString("Fish & Chips"));
		QCOMPARE(entries[0]->link, QString("https://example.org/fish"));
		QCOMPARE(entries[0]->author, QString("Alice"));
		QVERIFY2(entries[0]->content.contains("<b>world</b>"),
				 qPrintable(entries[0]->content));
		QCOMPARE(entries[0]->feedIndex, 3);
		// the time zone is taken into account
		QCOMPARE(entries[0]->pubDate.toUTC(),
				 QDateTime(QDate(2006, 1, 2), QTime(15, 4, 5), QTimeZone::UTC));
		QCOMPARE(entries[1]->pubDate.toUTC(),
				 QDateTime(QDate(2006, 1, 3), QTime(8, 0, 0), QTimeZone::UTC));
	}

	void test_AnAtomFeedIsRead()
	{
		const QByteArray atom = QByteArray(
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<feed xmlns=\"http://www.w3.org/2005/Atom\">\n"
			" <title>MeshMC</title>\n"
			" <id>urn:uuid:60a76c80-d399-11d9-b93C-0003939e0af6</id>\n"
			" <updated>2006-01-02T15:04:05Z</updated>\n"
			" <entry>\n"
			"  <title>An entry</title>\n"
			"  <link href=\"https://example.org/entry\"/>\n"
			"  <id>urn:uuid:1225c695-cfb8-4ebb-aaaa-80da344efa6a</id>\n"
			"  <updated>2006-01-02T15:04:05Z</updated>\n"
			"  <author><name>Bob</name></author>\n"
			"  <content type=\"html\">&lt;p&gt;Body&lt;/p&gt;</content>\n"
			" </entry>\n"
			"</feed>");
		QList<NewsEntryPtr> entries;
		QString error;
		QVERIFY2(News::parseFeed(atom, QString(), 0, &entries, &error),
				 qPrintable(error));
		QCOMPARE(entries.size(), 1);
		QCOMPARE(entries[0]->title, QString("An entry"));
		QCOMPARE(entries[0]->link, QString("https://example.org/entry"));
		QCOMPARE(entries[0]->author, QString("Bob"));
		QVERIFY2(entries[0]->content.contains("Body"),
				 qPrintable(entries[0]->content));
		QCOMPARE(entries[0]->pubDate.toUTC(),
				 QDateTime(QDate(2006, 1, 2), QTime(15, 4, 5), QTimeZone::UTC));
	}

	void test_AMissingDateAndAuthorAreHandled()
	{
		const QByteArray rss = QByteArray(
			"<rss version=\"2.0\"><channel>\n"
			"<title>t</title><link>https://example.org/</link><description>d</description>\n"
			"<item><title>Undated</title><link>https://example.org/u</link>\n"
			"<description>text</description></item>\n"
			"</channel></rss>");
		QList<NewsEntryPtr> entries;
		QString error;
		QVERIFY2(News::parseFeed(rss, QString(), 0, &entries, &error),
				 qPrintable(error));
		QCOMPARE(entries.size(), 1);
		// no date is an invalid one, which sorts last in NewsChecker
		QVERIFY(!entries[0]->pubDate.isValid());
		// the words NewsEntry has for a missing author
		QVERIFY(!entries[0]->author.isEmpty());
	}

	void test_AFeedWithoutEntriesIsAFeed()
	{
		const QByteArray rss = QByteArray(
			"<rss version=\"2.0\"><channel>\n"
			"<title>t</title><link>https://example.org/</link><description>d</description>\n"
			"</channel></rss>");
		QList<NewsEntryPtr> entries;
		QString error;
		QVERIFY2(News::parseFeed(rss, QString(), 0, &entries, &error),
				 qPrintable(error));
		QVERIFY(entries.isEmpty());
	}

	void test_ThingsThatAreNotFeedsAreRejected_data()
	{
		QTest::addColumn<QByteArray>("data");
		QTest::newRow("empty") << QByteArray();
		QTest::newRow("text") << QByteArray("hello");
		QTest::newRow("html") << QByteArray("<html><body>no</body></html>");
		QTest::newRow("broken xml") << QByteArray("<rss><channel>");
	}

	void test_ThingsThatAreNotFeedsAreRejected()
	{
		QFETCH(QByteArray, data);
		QList<NewsEntryPtr> entries;
		QString error;
		QVERIFY(!News::parseFeed(data, QString(), 0, &entries, &error));
		QVERIFY(!error.isEmpty());
	}
};

QTEST_MAIN(NewsFeedTest)

#include "NewsFeed_test.moc"
