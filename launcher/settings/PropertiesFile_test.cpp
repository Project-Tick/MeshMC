#include <QTest>

#include "settings/PropertiesFile.h"

class PropertiesFileTest : public QObject
{
	Q_OBJECT
  private slots:
	void test_ForgeVersionFile()
	{
		// the shape of forgeversion.properties
		const QByteArray data = "#Mon Dec 17 22:16:11 UTC 2012\n"
								"forge.major.number=6\n"
								"forge.minor.number=6\n"
								"forge.revision.number=2\n"
								"forge.build.number=534\n";
		PropertiesFile props(data);

		QVERIFY(props.contains("forge.major.number"));
		QCOMPARE(props.get("forge.major.number"), QString("6"));
		QCOMPARE(props.get("forge.minor.number"), QString("6"));
		QCOMPARE(props.get("forge.revision.number"), QString("2"));
		QCOMPARE(props.get("forge.build.number"), QString("534"));
	}

	void test_Missing()
	{
		PropertiesFile props("a=1\n");
		QVERIFY(!props.contains("b"));
		QVERIFY(props.get("b").isNull());
		QCOMPARE(props.get("b", "0"), QString("0"));
	}

	void test_CrLfAndComments()
	{
		PropertiesFile props("# comment\r\nfmlbuild.mcversion=1.4.7\r\n");
		QCOMPARE(props.get("fmlbuild.mcversion"), QString("1.4.7"));
	}

	void test_Empty()
	{
		PropertiesFile props(QByteArray{});
		QVERIFY(!props.contains("anything"));
	}
};

QTEST_GUILESS_MAIN(PropertiesFileTest)

#include "PropertiesFile_test.moc"
