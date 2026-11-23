/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "PackwizSidecar.h"

#include <QDateTime>
#include <QDebug>
#include <QLatin1Char>
#include <QLatin1String>

#include <cstdlib>
#include <memory>
#include <toml.h>

namespace {
    const QLatin1String kSuffix(".pw.toml");

    QString readString(const toml_table_t* table, const char* key)
    {
        if (!table)
            return {};
        const auto value = toml_string_in(table, key);
        if (!value.ok)
            return {};
        std::unique_ptr<char, decltype(&std::free)> text(value.u.s, &std::free);
        return QString::fromUtf8(text.get());
    }

    QString readId(const toml_table_t* table, const char* key)
    {
        const auto value = toml_int_in(table, key);
        return value.ok ? QString::number(value.u.i) : readString(table, key);
    }

    QByteArray tomlString(const QString& text)
    {
        QByteArray result = "\"";
        for (const unsigned char byte : text.toUtf8()) {
            if (byte == '"' || byte == '\\') {
                result += '\\';
                result += byte;
            } else if (byte < 0x20 || byte == 0x7f) {
                result += "\\u00" + QByteArray::number(byte, 16).toUpper().rightJustified(2, '0');
            } else {
                result += byte;
            }
        }
        return result + '"';
    }

	bool isPlatform(const QString& platform, QLatin1String name)
	{
		return platform.compare(name, Qt::CaseInsensitive) == 0;
	}

	/* Base name for a sidecar we have no slug for. `.disabled` and the
	 * archive extension come off so that toggling a mod on and off does
	 * not change which file its metadata lives in. */
	QString baseNameOf(const QString& fileName)
	{
		QString name = ModMetadataIndex::canonicalFileName(fileName);
		static const QLatin1String archiveSuffixes[] = {
			QLatin1String(".jar"), QLatin1String(".zip"),
			QLatin1String(".litemod")};
		for (const QLatin1String& suffix : archiveSuffixes) {
			if (name.endsWith(suffix, Qt::CaseInsensitive)) {
				name.chop(suffix.size());
				break;
			}
		}
		return name;
	}

	/* Sidecar names are looked up case-insensitively by other tools, and
	 * they end up in URLs and shell commands often enough that keeping
	 * them to slug-shaped characters is worth the small loss of
	 * fidelity. The slug itself passes through untouched. */
	QString sanitizeBaseName(const QString& name)
	{
		QString out;
		out.reserve(name.size());
		for (const QChar c : name) {
			const bool plain = (c >= QLatin1Char('a') && c <= QLatin1Char('z'))
							   || (c >= QLatin1Char('0')
								   && c <= QLatin1Char('9'))
							   || c == QLatin1Char('.')
							   || c == QLatin1Char('_')
							   || c == QLatin1Char('-');
			if (plain) {
				out.append(c);
			} else if (c.isLetterOrNumber()) {
				out.append(c.toLower());
			} else if (!out.endsWith(QLatin1Char('-'))) {
				out.append(QLatin1Char('-'));
			}
		}
		while (out.endsWith(QLatin1Char('-'))) {
			out.chop(1);
		}
		return out;
	}
}

bool Packwiz::isSidecarFileName(const QString& fileName)
{
	return fileName.endsWith(kSuffix, Qt::CaseInsensitive)
		   && fileName.size() > kSuffix.size();
}

QString Packwiz::slugFromFileName(const QString& fileName)
{
	if (!isSidecarFileName(fileName)) {
		return QString();
	}
	QString slug = fileName;
	slug.chop(kSuffix.size());
	return slug;
}

QString Packwiz::sidecarFileName(const ModMetadataIndex::Entry& entry)
{
	const QString base = sanitizeBaseName(entry.slug);
	if (base.isEmpty()) {
		return fallbackSidecarFileName(entry);
	}
	return base + kSuffix;
}

QString Packwiz::fallbackSidecarFileName(const ModMetadataIndex::Entry& entry)
{
	const QString base = sanitizeBaseName(baseNameOf(entry.fileName));
	if (base.isEmpty()) {
		return QString();
	}
	return base + kSuffix;
}

QByteArray Packwiz::serialize(const ModMetadataIndex::Entry& entry)
{
	if (entry.fileName.isEmpty()) {
		return QByteArray();
	}

	const bool curseForge =
		isPlatform(entry.platform, QLatin1String("curseforge"));
	const bool modrinth = isPlatform(entry.platform, QLatin1String("modrinth"));

	QByteArray root;
	QByteArray download;
	QByteArray update;
	root += "name = " + tomlString(entry.name) + "\n";
	root += "filename = " + tomlString(entry.fileName) + "\n";
	if (!entry.side.isEmpty()) {
		/* Left out when unknown rather than guessed: an absent `side`
		 * already means "both" to every reader of this format, while a
		 * wrong one would keep a mod out of a server install. */
		root += "side = " + tomlString(entry.side) + "\n";
	}


	/* CurseForge asks third parties not to hand out its file URLs, so the
	 * format has a mode that says "resolve this through the API instead".
	 * Our own copy of the URL still goes in, under our own key, because
	 * we do use it (see the blocked-download path in ModFolderPage) and
	 * because a foreign reader in metadata mode ignores it. */
	download += "mode = " + tomlString(QString::fromLatin1(curseForge ? "metadata:curseforge" : "url")) + "\n";
	if (!curseForge && !entry.downloadUrl.isEmpty()) {
		download += "url = " + tomlString(entry.downloadUrl) + "\n";
	}

	QString hashFormat = entry.hashFormat.toLower();
	QString hash = entry.hash;
	if (!entry.sha1.isEmpty()) {
		/* Prefer the SHA-1 when we have one: it is the digest this
		 * launcher verifies downloads against, and every reader of the
		 * format takes whatever `hash-format` says. A SHA-512 carried in
		 * from another tool is only kept when that is all we have. */
		hashFormat = QStringLiteral("sha1");
		hash = entry.sha1.toLower();
	}
	if (!hashFormat.isEmpty() && !hash.isEmpty()) {
		download += "hash-format = " + tomlString(hashFormat) + "\n";
		download += "hash = " + tomlString(hash) + "\n";
	}


	bool wroteUpdate = false;
	if (curseForge) {
		/* Both IDs are numbers in this format, and a zero would be read
		 * back as a real project. Falling back to our own keys keeps the
		 * provenance instead of dropping the file's history. */
		const int projectId = entry.projectId.toInt();
		const int fileId = entry.versionId.toInt();
		if (projectId > 0 && fileId > 0) {
			QByteArray provider;
			provider += "project-id = " + QByteArray::number(projectId) + "\n";
			provider += "file-id = " + QByteArray::number(fileId) + "\n";
			update = "\n[update.curseforge]\n" + provider;
			wroteUpdate = true;
		}
	} else if (modrinth && !entry.projectId.isEmpty()
			   && !entry.versionId.isEmpty()) {
		QByteArray provider;
		provider += "mod-id = " + tomlString(entry.projectId) + "\n";
		provider += "version = " + tomlString(entry.versionId) + "\n";
		update = "\n[update.modrinth]\n" + provider;
		wroteUpdate = true;
	}

	if (!entry.slug.isEmpty()) {
		root += "x-meshmc-slug = " + tomlString(entry.slug) + "\n";
	}
	if (!wroteUpdate && !entry.platform.isEmpty()) {
		/* Manually added files are recorded as "local", and they have no
		 * update source to describe. */
		root += "x-meshmc-platform = " + tomlString(entry.platform) + "\n";
		if (!entry.projectId.isEmpty()) {
			root += "x-meshmc-project-id = " + tomlString(entry.projectId) + "\n";
		}
		if (!entry.versionId.isEmpty()) {
			root += "x-meshmc-version-id = " + tomlString(entry.versionId) + "\n";
		}
	}
	/* Outside the block above on purpose: the version as a person reads
	 * it is worth keeping whether or not the file has an update source,
	 * and the format has nowhere else to put it - packwiz's own provider
	 * tables take ids. */
	if (!entry.versionNumber.isEmpty()) {
		root += "x-meshmc-version-number = " + tomlString(entry.versionNumber) + "\n";
	}
	if (curseForge && !entry.downloadUrl.isEmpty()) {
		root += "x-meshmc-download-url = " + tomlString(entry.downloadUrl) + "\n";
	}
	if (entry.isDependency) {
		root += "x-meshmc-dependency = true\n";
	}
	if (entry.fileSize > 0) {
		root += "x-meshmc-file-size = " + QByteArray::number(entry.fileSize) + "\n";
	}
	const QDateTime stamp = entry.installedAt.isValid()
								? entry.installedAt
								: QDateTime::currentDateTimeUtc();
	root += "x-meshmc-installed-at = " + tomlString(stamp.toString(Qt::ISODate)) + "\n";

    return root + "\n[download]\n" + download + update;
}

ModMetadataIndex::Entry Packwiz::parse(const QByteArray& bytes,
									   const QString& slugHint)
{
	ModMetadataIndex::Entry entry;

    QByteArray input = bytes;
    char error[256] = {};
    std::unique_ptr<toml_table_t, decltype(&toml_free)> parsed(
        toml_parse(input.data(), error, sizeof(error)), &toml_free);
    if (!parsed) {
        qWarning() << "Packwiz: malformed sidecar:" << error;
        return entry;
    }
    const auto table = parsed.get();

	entry.fileName = readString(table, "filename");
	if (entry.fileName.isEmpty()) {
		return entry;
	}
	entry.name = readString(table, "name");
	entry.side = readString(table, "side");

	if (const auto download = toml_table_in(table, "download")) {
		entry.downloadUrl = readString(download, "url");
		entry.hashFormat = readString(download, "hash-format").toLower();
		entry.hash = readString(download, "hash");
		if (entry.hashFormat == QLatin1String("sha1")) {
			entry.sha1 = entry.hash.toLower();
		}
	}
	if (entry.downloadUrl.isEmpty()) {
		entry.downloadUrl = readString(table, "x-meshmc-download-url");
	}

	const auto update = toml_table_in(table, "update");
	if (auto provider = update ? toml_table_in(update, "curseforge") : nullptr) {
		entry.platform = QStringLiteral("curseforge");
		entry.projectId = readId(provider, "project-id");
		entry.versionId = readId(provider, "file-id");
	} else if ((provider = update ? toml_table_in(update, "modrinth") : nullptr)) {
		entry.platform = QStringLiteral("modrinth");
		entry.projectId = readId(provider, "mod-id");
		entry.versionId = readId(provider, "version");
	} else {
		entry.platform = readString(table, "x-meshmc-platform");
		entry.projectId = readString(table, "x-meshmc-project-id");
		entry.versionId = readString(table, "x-meshmc-version-id");
	}

	entry.versionNumber = readString(table, "x-meshmc-version-number");

	entry.slug = readString(table, "x-meshmc-slug");
	if (entry.slug.isEmpty()) {
		/* How every reader of this format gets the slug for a file it did
		 * not write itself: it is the sidecar's own name. */
		entry.slug = slugHint;
	}
    const auto dependency = toml_bool_in(table, "x-meshmc-dependency");
    entry.isDependency = dependency.ok && dependency.u.b;
    const auto size = toml_int_in(table, "x-meshmc-file-size");
    entry.fileSize = size.ok ? size.u.i : 0;
	const QString stamp = readString(table, "x-meshmc-installed-at");
	if (!stamp.isEmpty()) {
		entry.installedAt = QDateTime::fromString(stamp, Qt::ISODate);
	}

	return entry;
}
