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

#include "Translations.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>

#include <KLocalizedString>

#include "settings/SettingsObject.h"

// KI18n looks for <locale dir>/<language>/LC_MESSAGES/<domain>.mo.
static const QLatin1String catalogDomain("meshmc");

// The language of the source strings; it needs no catalog.
static const QLatin1String sourceLanguage("en_US");

QString Translations::bundledCatalogDir()
{
	return QDir::cleanPath(QCoreApplication::applicationDirPath() +
						   QStringLiteral("/" MESHMC_LOCALE_RELATIVE_DIR));
}

QString Translations::overridesFilePath()
{
	// where KSwitchLanguageDialog keeps it
	return QStandardPaths::writableLocation(
			   QStandardPaths::GenericConfigLocation) +
		   QStringLiteral("/klanguageoverridesrc");
}

QString Translations::currentLanguageKey(const QString& key)
{
	// The translation index of the Qt-era releases called the English and
	// Portuguese catalogs en_GB and pt_PT; the catalogs themselves have
	// always been en and pt.
	if (key == QLatin1String("en_GB")) {
		return QStringLiteral("en");
	}
	if (key == QLatin1String("pt_PT")) {
		return QStringLiteral("pt");
	}
	// Japanese in kanji is not a locale, so the dialog cannot offer it.
	if (key == QLatin1String("ja_KANJI")) {
		return QStringLiteral("ja");
	}
	return key;
}

QString Translations::migrateLegacyLanguage(SettingsObject* settings,
											const QString& overridesFile)
{
	const QString legacy = settings->get("Language").toString();
	if (legacy.isEmpty()) {
		return QString();
	}
	const QString key = currentLanguageKey(legacy);

	QSettings overrides(overridesFile, QSettings::IniFormat);
	overrides.beginGroup(QStringLiteral("Language"));
	// the key the dialog uses: the name of the executable
	if (overrides.contains(qAppName())) {
		// the user has chosen with the dialog; that is the newer choice
		settings->reset("Language");
		return QString();
	}
	overrides.setValue(qAppName(), key.toUtf8());
	overrides.sync();
	if (overrides.status() != QSettings::NoError) {
		qWarning() << "Could not write" << overridesFile
				   << "; the language stays where it is.";
		return QString();
	}
	settings->reset("Language");
	return key;
}

void Translations::initialize(SettingsObject* settings)
{
	// KI18n on its own only looks in $XDG_DATA_DIRS/locale, which says
	// nothing about a macOS bundle or a portable install, so the directory of
	// this installation is registered explicitly.
	// The application domain is set in main(), before the Application is
	// created.
	KLocalizedString::addDomainLocaleDir(catalogDomain.latin1(),
										 bundledCatalogDir());

	const QString moved =
		migrateLegacyLanguage(settings, overridesFilePath());
	if (!moved.isEmpty()) {
		qDebug() << "The language" << moved
				 << "was moved from the settings to the language dialog's"
					" file.";
		// KXmlGui reads the file when the QCoreApplication is created, which
		// is before this; do what it would have done.
		const QByteArray now = qgetenv("LANGUAGE");
		qputenv("LANGUAGE", now.isEmpty() ? moved.toUtf8()
										  : moved.toUtf8() + ':' + now);
	}

	// The domain was set before the application existed, and KI18n worked out
	// its languages then, without the LANGUAGE that KXmlGui set up while the
	// application was created. KI18n reads them again when the application
	// tells it that the language changed.
	QEvent languageChange(QEvent::LanguageChange);
	QCoreApplication::sendEvent(QCoreApplication::instance(),
								&languageChange);

	// The language the application is shown in: the first of the list that
	// there is a catalog for. English, the language of the source strings,
	// needs none.
	QString primary;
	for (const QString& language : KLocalizedString::languages()) {
		if (language == sourceLanguage) {
			break;
		}
		if (KLocalizedString::isApplicationTranslatedInto(language)) {
			primary = language;
			break;
		}
	}
	qDebug() << "Languages:" << KLocalizedString::languages()
			 << "- the application is shown in"
			 << (primary.isEmpty() ? QString(sourceLanguage) : primary);
	if (primary.isEmpty()) {
		return;
	}

	QLocale::setDefault(QLocale(primary));

	// Qt's own strings (standard buttons, file dialogs) come from Qt's
	// translation files.
	auto* qtTranslator = new QTranslator(QCoreApplication::instance());
	if (qtTranslator->load("qt_" + primary,
						   QLibraryInfo::path(QLibraryInfo::TranslationsPath)) &&
		QCoreApplication::installTranslator(qtTranslator)) {
		qDebug() << "Loaded the Qt translation for" << primary;
	} else {
		delete qtTranslator;
	}
}
