/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 */

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "modplatform/BrandLogos.h"

/*
 * BrandLogos without a running launcher: what it keeps of a download, and
 * how it answers from the cache. Nothing here touches the network -- with no
 * LauncherContext, fetch() does nothing.
 */
class BrandLogosTest : public QObject
{
	Q_OBJECT

  private slots:
	void onlyTheFetchedSitesHaveSources()
	{
		for (const QString& id : BrandLogos::ids()) {
			QVERIFY(BrandLogos::sourceUrl(id).startsWith(QLatin1String("https://")));
		}
		QVERIFY(BrandLogos::sourceUrl(QStringLiteral("fabric")).isEmpty());
		QVERIFY(BrandLogos::cachedPath(QStringLiteral("fabric")).isEmpty());
	}

	void keepsARasterAsPngOfItsLargestFrame()
	{
		QImage image(32, 32, QImage::Format_ARGB32);
		image.fill(Qt::green);
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		QVERIFY(image.save(&buffer, "BMP"));

		QString suffix;
		const QByteArray kept = BrandLogos::normalised(bytes, &suffix);
		QCOMPARE(suffix, QStringLiteral("png"));
		QImage back;
		QVERIFY(back.loadFromData(kept, "PNG"));
		QCOMPARE(back.size(), QSize(32, 32));
	}

	void refusesWhatIsNoImage()
	{
		QString suffix;
		QVERIFY(BrandLogos::normalised(QByteArray("<html>Not found</html>"), &suffix).isEmpty());
		QVERIFY(BrandLogos::normalised(QByteArray(), &suffix).isEmpty());
	}

	void answersFromTheCache()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString previous = QDir::currentPath();
		QVERIFY(QDir::setCurrent(dir.path()));

		auto* logos = BrandLogos::instance();
		const QString id = QStringLiteral("modrinth");
		/* Not cached, and no launcher to fetch it with. */
		QVERIFY(logos->url(id).isEmpty());

		QVERIFY(QDir().mkpath(QStringLiteral("cache/brands")));
		QImage image(16, 16, QImage::Format_ARGB32);
		image.fill(Qt::green);
		QVERIFY(image.save(QStringLiteral("cache/brands/modrinth.png"), "PNG"));

		const QUrl url(logos->url(id));
		QVERIFY(url.isLocalFile());
		QCOMPARE(QFileInfo(url.toLocalFile()).canonicalFilePath(),
				 QFileInfo(QStringLiteral("cache/brands/modrinth.png")).canonicalFilePath());
		QVERIFY(logos->url(QStringLiteral("nowhere")).isEmpty());

		QVERIFY(QDir::setCurrent(previous));
	}
};

QTEST_GUILESS_MAIN(BrandLogosTest)

#include "BrandLogos_test.moc"
