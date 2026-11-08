#include <QTest>

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFontComboBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QWidget>

#include <KConfigDialogManager>
#include <KCoreConfigSkeleton>

#include "settings/KConfigSettingsObject.h"
#include "settings/Setting.h"
#include "ui/widgets/ChoiceBinding.h"

// Widgets named "kcfg_<ID>" are bound to the setting with that ID by
// KConfigDialogManager, through KConfigSettingsObject::skeleton(). This is
// what the pages of the settings dialog rely on instead of a loadSettings()
// and an applySettings() of their own.
class SettingsBindingTest : public QObject
{
	Q_OBJECT

	static QByteArray slurp(const QString& path)
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			return QByteArray();
		}
		return file.readAll();
	}

  private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
	}

	void test_WidgetsShowTheSettingsAndSaveThem()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		{
			QFile file(path);
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write("Flag=true\nText=from the file\n");
		}

		KConfigSettingsObject settings(path);
		auto flagSetting = settings.registerSetting("Flag", false);
		settings.registerSetting("Number", 7);
		settings.registerSetting("Text", QString("default"));

		QWidget page;
		auto* flag = new QCheckBox(&page);
		flag->setObjectName("kcfg_Flag");
		auto* number = new QSpinBox(&page);
		number->setObjectName("kcfg_Number");
		number->setMaximum(1000);
		auto* text = new QLineEdit(&page);
		text->setObjectName("kcfg_Text");

		KConfigDialogManager manager(&page, settings.skeleton());

		// the widgets start from the settings
		QVERIFY(flag->isChecked());
		QCOMPARE(number->value(), 7);
		QCOMPARE(text->text(), QString("from the file"));
		QVERIFY(!manager.hasChanged());

		int flagChanged = 0;
		connect(flagSetting.get(), &Setting::SettingChanged, this,
				[&flagChanged] { ++flagChanged; });
		flag->setChecked(false);
		number->setValue(9);
		text->setText("edited");
		QVERIFY(manager.hasChanged());

		// nothing is written before the manager is asked to
		QCOMPARE(settings.get("Number").toInt(), 7);

		manager.updateSettings();

		QCOMPARE(settings.get("Flag").toBool(), false);
		QCOMPARE(settings.get("Number").toInt(), 9);
		QCOMPARE(settings.get("Text").toString(), QString("edited"));
		QCOMPARE(flagChanged, 1);
		const QByteArray written = slurp(path);
		QVERIFY(written.contains("Number=9"));
		QVERIFY(written.contains("Text=edited"));
		QVERIFY(!manager.hasChanged());
	}

	void test_DefaultsComeFromTheRegisteredDefaults()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("test.cfg"));
		settings.registerSetting("Flag", true);
		settings.registerSetting("Number", 7);
		settings.set("Flag", false);
		settings.set("Number", 500);

		QWidget page;
		auto* flag = new QCheckBox(&page);
		flag->setObjectName("kcfg_Flag");
		auto* number = new QSpinBox(&page);
		number->setObjectName("kcfg_Number");
		number->setMaximum(1000);

		KConfigDialogManager manager(&page, settings.skeleton());
		QVERIFY(!flag->isChecked());
		QCOMPARE(number->value(), 500);
		QVERIFY(!manager.isDefault());

		manager.updateWidgetsDefault();
		QVERIFY(flag->isChecked());
		QCOMPARE(number->value(), 7);
		QVERIFY(manager.isDefault());
	}

	void test_FontComboBoxIsBoundToTheFamilyName()
	{
		// MeshMCPage: the console font is a family name in a setting. The
		// families are the ones the machine has (a CI machine has few, and a
		// family that is not installed cannot be chosen in the combo box), so
		// they are taken from the combo box itself.
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		KConfigSettingsObject settings(path);

		QWidget page;
		auto* font = new QFontComboBox(&page);
		font->setObjectName("kcfg_ConsoleFont");
		if (font->count() < 2) {
			QSKIP("fewer than two font families are installed");
		}
		const QString initial = font->itemText(0);
		const QString other = font->itemText(1);

		settings.registerSetting("ConsoleFont", other);
		settings.set("ConsoleFont", initial);
		KConfigDialogManager manager(&page, settings.skeleton());

		// the text, not currentFont(): that only follows a change of the item,
		// and the first item is the one a new combo box already has
		QCOMPARE(font->currentText(), initial);

		font->setCurrentFont(QFont(other));
		QVERIFY(manager.hasChanged());
		manager.updateSettings();
		QCOMPARE(settings.get("ConsoleFont").toString(), other);

		font->setCurrentFont(QFont(initial));
		QVERIFY(!manager.isDefault());
		manager.updateWidgetsDefault();
		QCOMPARE(font->currentText(), other);
	}

	void test_ASettingChangedBehindTheSkeletonIsNotOverwritten()
	{
		// AppearancePage is not bound: it writes ApplicationTheme and IconTheme
		// with set() the moment they are chosen, while the dialog's skeleton
		// still holds what they were when the dialog opened. Saving another
		// page's widgets must not put the old values back.
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		KConfigSettingsObject settings(path);
		auto theme = settings.registerSetting("ApplicationTheme",
											  QString("system"));
		settings.registerSetting("Number", 7);

		QWidget page;
		auto* number = new QSpinBox(&page);
		number->setObjectName("kcfg_Number");
		number->setMaximum(1000);
		KConfigDialogManager manager(&page, settings.skeleton());

		int themeChanged = 0;
		connect(theme.get(), &Setting::SettingChanged, this,
				[&themeChanged] { ++themeChanged; });

		// the page, live
		settings.set("ApplicationTheme", QString("dark"));
		QCOMPARE(themeChanged, 1);

		// another page, on OK
		number->setValue(9);
		manager.updateSettings();

		QCOMPARE(settings.get("ApplicationTheme").toString(),
				 QString("dark"));
		QCOMPARE(settings.get("Number").toInt(), 9);
		QVERIFY(slurp(path).contains("ApplicationTheme=dark"));
		QCOMPARE(themeChanged, 1);

		// a reopened dialog starts from what is there
		settings.skeleton()->load();
		QCOMPARE(settings.get("ApplicationTheme").toString(),
				 QString("dark"));
	}

	void test_RadioButtonsAreBoundToAText()
	{
		// ProxyPage: the proxy type is a choice of radio buttons
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		KConfigSettingsObject settings(path);
		settings.registerSetting("ProxyType", QString("None"));
		settings.set("ProxyType", QString("SOCKS5"));

		QWidget page;
		auto* none = new QRadioButton(&page);
		auto* socks = new QRadioButton(&page);
		auto* http = new QRadioButton(&page);
		auto* binding = new ChoiceBinding("ProxyType", &page);
		binding->addChoice(none, "None");
		binding->addChoice(socks, "SOCKS5");
		binding->addChoice(http, "HTTP");

		KConfigDialogManager manager(&page, settings.skeleton());
		QVERIFY(socks->isChecked());
		QVERIFY(!manager.hasChanged());

		http->setChecked(true);
		QVERIFY(manager.hasChanged());
		manager.updateSettings();
		QCOMPARE(settings.get("ProxyType").toString(), QString("HTTP"));
		QVERIFY(slurp(path).contains("ProxyType=HTTP"));

		manager.updateWidgetsDefault();
		QVERIFY(none->isChecked());
		QVERIFY(!binding->isVisible());
	}

	void test_RadioButtonWithALineEditIsBoundToAText()
	{
		// PasteEEPage: "meshmc", or a key of one's own
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("test.cfg"));
		settings.registerSetting("PasteEEAPIKey", QString("meshmc"));
		settings.set("PasteEEAPIKey", QString("secret"));

		QWidget page;
		auto* fixed = new QRadioButton(&page);
		auto* custom = new QRadioButton(&page);
		auto* edit = new QLineEdit(&page);
		auto* binding = new ChoiceBinding("PasteEEAPIKey", &page);
		binding->addChoice(fixed, "meshmc");
		binding->addTextChoice(custom, edit);

		KConfigDialogManager manager(&page, settings.skeleton());
		QVERIFY(custom->isChecked());
		QCOMPARE(edit->text(), QString("secret"));

		edit->setText("other");
		QVERIFY(manager.hasChanged());
		manager.updateSettings();
		QCOMPARE(settings.get("PasteEEAPIKey").toString(), QString("other"));

		fixed->setChecked(true);
		manager.updateSettings();
		QCOMPARE(settings.get("PasteEEAPIKey").toString(), QString("meshmc"));
	}

	void test_ComboBoxCanBeBoundByItsItemData()
	{
		// JavaPage: the vendor is the ID of a provider, not the shown name
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		KConfigSettingsObject settings(path);
		settings.registerSetting("JavaAutoDownloadVendor", QString("adoptium"));
		settings.set("JavaAutoDownloadVendor", QString("azul"));

		QWidget page;
		auto* combo = new QComboBox(&page);
		combo->addItem("Eclipse Temurin", "adoptium");
		combo->addItem("Azul Zulu", "azul");
		auto* binding = new ChoiceBinding("JavaAutoDownloadVendor", &page);
		binding->setComboBox(combo);

		KConfigDialogManager manager(&page, settings.skeleton());
		QCOMPARE(combo->currentData().toString(), QString("azul"));
		QVERIFY(!manager.hasChanged());

		combo->setCurrentIndex(0);
		QVERIFY(manager.hasChanged());
		manager.updateSettings();
		QCOMPARE(settings.get("JavaAutoDownloadVendor").toString(),
				 QString("adoptium"));
	}

	void test_AWidgetWithoutASettingIsLeftAlone()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("test.cfg"));
		settings.registerSetting("Flag", false);

		QWidget page;
		auto* flag = new QCheckBox(&page);
		flag->setObjectName("kcfg_Flag");
		auto* other = new QLineEdit(&page);
		other->setObjectName("notBound");
		other->setText("mine");

		KConfigDialogManager manager(&page, settings.skeleton());
		manager.updateWidgets();
		QCOMPARE(other->text(), QString("mine"));
	}
};

QTEST_MAIN(SettingsBindingTest)

#include "SettingsBinding_test.moc"
