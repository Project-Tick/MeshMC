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

#include "TranslationsModel.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLocale>
#include <QSet>
#include <QTranslator>
#include <QDebug>

#include <KLocalizedString>

const static QLatin1String defaultLangCode("en_US");

// KI18n looks for <locale dir>/<language>/LC_MESSAGES/<domain>.mo.
const static QLatin1String catalogDomain("meshmc");

namespace
{
	// Language keys that earlier versions wrote into the settings. The
	// translation index of the Qt-era releases called the English and
	// Portuguese catalogs en_GB and pt_PT; the catalogs themselves have always
	// been en and pt.
	QString currentLanguageKey(const QString& key)
	{
		if (key == QLatin1String("en_GB")) {
			return QStringLiteral("en");
		}
		if (key == QLatin1String("pt_PT")) {
			return QStringLiteral("pt");
		}
		return key;
	}

	// Languages with a catalog in <localeDir>/<language>/LC_MESSAGES/.
	QStringList catalogsIn(const QString& localeDir)
	{
		QStringList found;
		const QDir dir(localeDir);
		const auto subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
		for (const auto& lang : subdirs) {
			const QString catalog = lang + QStringLiteral("/LC_MESSAGES/") +
									catalogDomain + QStringLiteral(".mo");
			if (QFileInfo::exists(dir.filePath(catalog))) {
				found.append(lang);
			}
		}
		return found;
	}
} // namespace

struct Language {
	Language() = default;
	explicit Language(const QString& _key) : key(_key), locale(QLocale(_key)) {}

	QString languageName() const
	{
		QString result;
		if (key == "ja_KANJI") {
			result = locale.nativeLanguageName() + " (漢字)";
		} else if (key == "es_UY") {
			result = "español de Latinoamérica";
		} else {
			result = locale.nativeLanguageName();
		}
		return result;
	}

	QString key;
	QLocale locale;
};

struct TranslationsModel::Private {
	// English, the language of the source strings, needs no catalog and is
	// always there.
	QVector<Language> m_languages = {Language(defaultLangCode)};

	QString m_selectedLanguage = defaultLangCode;
	// Translates Qt's own strings (standard buttons, file dialogs). The
	// application's strings are not in a QTranslator, see selectLanguage().
	std::unique_ptr<QTranslator> m_qt_translator;
};

QString TranslationsModel::bundledCatalogDir()
{
	return QDir::cleanPath(QCoreApplication::applicationDirPath() +
						   QStringLiteral("/" MESHMC_LOCALE_RELATIVE_DIR));
}

TranslationsModel::TranslationsModel(QObject* parent)
	: QAbstractListModel(parent)
{
	d.reset(new Private);

	// KI18n on its own only looks in $XDG_DATA_DIRS/locale, which says nothing
	// about a macOS bundle or a portable install, so the directory of this
	// installation is registered explicitly.
	// The application domain is set in main(), before the Application (and
	// with it this model) is created.
	KLocalizedString::addDomainLocaleDir(catalogDomain.latin1(),
										 bundledCatalogDir());

	loadLanguages();
}

TranslationsModel::~TranslationsModel() {}

void TranslationsModel::loadLanguages()
{
	// The installation's own directory, plus whatever KI18n finds in the
	// standard locations (where a distribution package puts the catalogs).
	QSet<QString> codes;
	for (const auto& lang : catalogsIn(bundledCatalogDir())) {
		codes.insert(lang);
	}
	const auto standard = KLocalizedString::availableApplicationTranslations();
	for (const auto& lang : standard) {
		codes.insert(lang);
	}
	codes.remove(defaultLangCode);

	QVector<Language> languages = {Language(defaultLangCode)};
	for (const auto& code : codes) {
		languages.append(Language(code));
	}
	std::sort(languages.begin(), languages.end(),
			  [](const Language& a, const Language& b) {
				  return a.key.compare(b.key) < 0;
			  });

	beginResetModel();
	d->m_languages = languages;
	endResetModel();
}

QVariant TranslationsModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	if (row < 0 || row >= d->m_languages.size())
		return QVariant();

	auto& lang = d->m_languages[row];
	switch (role) {
		case Qt::DisplayRole:
			return lang.languageName();
		case Qt::ToolTipRole:
			return lang.key;
		case Qt::UserRole:
			return lang.key;
		default:
			return QVariant();
	}
}

QVariant TranslationsModel::headerData(int section, Qt::Orientation orientation,
									   int role) const
{
	if (section == 0) {
		if (role == Qt::DisplayRole) {
			return i18n("Language");
		} else if (role == Qt::ToolTipRole) {
			return i18n("The native language name.");
		}
	}
	return QAbstractListModel::headerData(section, orientation, role);
}

int TranslationsModel::rowCount(const QModelIndex& parent) const
{
	return d->m_languages.size();
}

int TranslationsModel::columnCount(const QModelIndex& parent) const
{
	return 1;
}

Language* TranslationsModel::findLanguage(const QString& key)
{
	auto found = std::find_if(d->m_languages.begin(), d->m_languages.end(),
							  [&](Language& lang) { return lang.key == key; });
	if (found == d->m_languages.end()) {
		return nullptr;
	} else {
		return &(*found);
	}
}

bool TranslationsModel::selectLanguage(QString key)
{
	QString& langCode = key;
	auto langPtr = findLanguage(currentLanguageKey(key));
	if (!langPtr) {
		qWarning() << "Selected invalid language" << key << ", defaulting to"
				   << defaultLangCode;
		langCode = defaultLangCode;
	} else {
		langCode = langPtr->key;
	}

	// uninstall the Qt translator if there is one
	if (d->m_qt_translator) {
		QCoreApplication::removeTranslator(d->m_qt_translator.get());
		d->m_qt_translator.reset();
	}

	/*
	 * FIXME: potential source of crashes:
	 * In a multithreaded application, the default locale should be set at
	 * application startup, before any non-GUI threads are created. This
	 * function is not reentrant.
	 */
	QLocale locale = QLocale(langCode);
	QLocale::setDefault(locale);

	// The application's strings come from the "meshmc" gettext catalog through
	// KI18n, which keeps its own language list and ignores QLocale. Naming
	// only the selected language is deliberate: the language list that is
	// derived from the locale would drag in the system language as well.
	// The source language is English, so en_US needs no catalog.
	KLocalizedString::setLanguages(QStringList{langCode});

	// Qt's own strings still come from Qt's translation files.
	if (langCode != defaultLangCode) {
		d->m_qt_translator.reset(new QTranslator());
		if (d->m_qt_translator->load(
				"qt_" + langCode,
				QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
			qDebug() << "Loading Qt Language File for"
					 << langCode.toLocal8Bit().constData() << "...";
			if (!QCoreApplication::installTranslator(
					d->m_qt_translator.get())) {
				qCritical() << "Loading Qt Language File failed.";
				d->m_qt_translator.reset();
			}
		} else {
			d->m_qt_translator.reset();
		}
	}

	d->m_selectedLanguage = langCode;

	// KI18n does not announce a language change. Widgets that implement
	// QEvent::LanguageChange (MainWindow, the setup wizard, ...) retranslate
	// themselves in response, which is what installing a QTranslator used to
	// trigger.
	if (QCoreApplication::instance()) {
		QEvent languageChange(QEvent::LanguageChange);
		QCoreApplication::sendEvent(QCoreApplication::instance(),
									&languageChange);
	}
	return langPtr != nullptr;
}

QModelIndex TranslationsModel::selectedIndex()
{
	auto found = findLanguage(d->m_selectedLanguage);
	if (found) {
		// QVector iterator freely converts to pointer to contained type
		return index(found - d->m_languages.data(), 0, QModelIndex());
	}
	return QModelIndex();
}

QString TranslationsModel::selectedLanguage()
{
	return d->m_selectedLanguage;
}
