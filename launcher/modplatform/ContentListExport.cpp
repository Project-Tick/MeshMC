/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "ContentListExport.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLatin1Char>

namespace
{
	using ContentListExport::Fields;
	using ContentListExport::Item;

	/* Backslash comes first on purpose: escaping it afterwards would
	 * also escape the backslashes this loop has just added. */
	const QString kMarkdownSpecials = QStringLiteral("\\`*_{}[]<>()#+-.!|");

	QString markdownEscaped(const QString& text)
	{
		QString out = text;
		for (const QChar ch : kMarkdownSpecials) {
			const QString escaped = QStringLiteral("\\") + ch;
			out.replace(ch, escaped);
		}
		return out;
	}

	QString joinedAuthors(const Item& item)
	{
		return item.authors.join(QStringLiteral(", "));
	}

	QString htmlList(const QList<Item>& items, Fields fields)
	{
		QStringList lines;
		for (const Item& item : items) {
			QString name = item.name.toHtmlEscaped();
			if (fields.testFlag(ContentListExport::Url)
				&& !item.url.isEmpty()) {
				name = QStringLiteral("<a href=\"%1\">%2</a>")
						   .arg(item.url.toHtmlEscaped(), name);
			}
			QString line = name;
			if (fields.testFlag(ContentListExport::Version)
				&& !item.version.isEmpty()) {
				line += QStringLiteral(" [%1]")
							.arg(item.version.toHtmlEscaped());
			}
			if (fields.testFlag(ContentListExport::Authors)
				&& !item.authors.isEmpty()) {
				line += QStringLiteral(" by ")
						+ joinedAuthors(item).toHtmlEscaped();
			}
			if (fields.testFlag(ContentListExport::FileName)) {
				line += QStringLiteral(" (%1)")
							.arg(item.fileName.toHtmlEscaped());
			}
			lines.append(QStringLiteral("<li>%1</li>").arg(line));
		}
		return QStringLiteral("<html><body><ul>\n\t%1\n</ul></body></html>")
			.arg(lines.join(QStringLiteral("\n\t")));
	}

	QString markdownList(const QList<Item>& items, Fields fields)
	{
		QStringList lines;
		for (const Item& item : items) {
			QString name = markdownEscaped(item.name);
			if (fields.testFlag(ContentListExport::Url)
				&& !item.url.isEmpty()) {
				name = QStringLiteral("[%1](%2)").arg(name, item.url);
			}
			QString line = name;
			if (fields.testFlag(ContentListExport::Version)
				&& !item.version.isEmpty()) {
				line += QStringLiteral(" [%1]")
							.arg(markdownEscaped(item.version));
			}
			if (fields.testFlag(ContentListExport::Authors)
				&& !item.authors.isEmpty()) {
				line += QStringLiteral(" by ")
						+ markdownEscaped(joinedAuthors(item));
			}
			if (fields.testFlag(ContentListExport::FileName)) {
				line += QStringLiteral(" (%1)")
							.arg(markdownEscaped(item.fileName));
			}
			lines.append(QStringLiteral("- ") + line);
		}
		return lines.join(QStringLiteral("\n"));
	}

	QString plainList(const QList<Item>& items, Fields fields)
	{
		QStringList lines;
		for (const Item& item : items) {
			QString line = item.name;
			/* The address goes straight after the name here, unlike the
			 * marked-up formats where it wraps it. */
			if (fields.testFlag(ContentListExport::Url)
				&& !item.url.isEmpty()) {
				line += QStringLiteral(" (%1)").arg(item.url);
			}
			if (fields.testFlag(ContentListExport::Version)
				&& !item.version.isEmpty()) {
				line += QStringLiteral(" [%1]").arg(item.version);
			}
			if (fields.testFlag(ContentListExport::Authors)
				&& !item.authors.isEmpty()) {
				line += QStringLiteral(" by ") + joinedAuthors(item);
			}
			if (fields.testFlag(ContentListExport::FileName)) {
				line += QStringLiteral(" (%1)").arg(item.fileName);
			}
			lines.append(line);
		}
		return lines.join(QStringLiteral("\n"));
	}

	QString jsonList(const QList<Item>& items, Fields fields)
	{
		QJsonArray array;
		for (const Item& item : items) {
			QJsonObject entry;
			entry.insert(QStringLiteral("name"), item.name);
			if (fields.testFlag(ContentListExport::Url)
				&& !item.url.isEmpty()) {
				entry.insert(QStringLiteral("url"), item.url);
			}
			if (fields.testFlag(ContentListExport::Version)
				&& !item.version.isEmpty()) {
				entry.insert(QStringLiteral("version"), item.version);
			}
			if (fields.testFlag(ContentListExport::Authors)
				&& !item.authors.isEmpty()) {
				entry.insert(QStringLiteral("authors"),
							 QJsonArray::fromStringList(item.authors));
			}
			if (fields.testFlag(ContentListExport::FileName)) {
				entry.insert(QStringLiteral("filename"), item.fileName);
			}
			array.append(entry);
		}
		QJsonDocument doc;
		doc.setArray(array);
		return QString::fromUtf8(doc.toJson());
	}

	QString csvList(const QList<Item>& items, Fields fields)
	{
		QStringList lines;
		for (const Item& item : items) {
			QStringList columns;
			columns << item.name;
			if (fields.testFlag(ContentListExport::Url)) {
				columns << item.url;
			}
			if (fields.testFlag(ContentListExport::Version)) {
				columns << item.version;
			}
			if (fields.testFlag(ContentListExport::Authors)) {
				/* Quoted only when there is more than one, because the
				 * separator between them is the column separator. */
				QString authors;
				if (item.authors.size() == 1) {
					authors = item.authors.constLast();
				} else if (item.authors.size() > 1) {
					authors = QStringLiteral("\"%1\"")
								  .arg(item.authors.join(QLatin1Char(',')));
				}
				columns << authors;
			}
			if (fields.testFlag(ContentListExport::FileName)) {
				columns << item.fileName;
			}
			lines.append(columns.join(QLatin1Char(',')));
		}
		return lines.join(QStringLiteral("\n"));
	}
} // namespace

QString ContentListExport::render(const QList<Item>& items, Format format,
								  Fields fields)
{
	switch (format) {
		case Format::Html:
			return htmlList(items, fields);
		case Format::Markdown:
			return markdownList(items, fields);
		case Format::PlainText:
			return plainList(items, fields);
		case Format::Json:
			return jsonList(items, fields);
		case Format::Csv:
			return csvList(items, fields);
		case Format::Custom:
			/* Has no fixed shape to render; the caller is expected to
			 * use the template overload. */
			return QString();
	}
	return QString();
}

QString ContentListExport::render(const QList<Item>& items,
								  const QString& lineTemplate)
{
	QStringList lines;
	for (const Item& item : items) {
		QString line = lineTemplate;
		line.replace(QStringLiteral("{name}"), item.name);
		line.replace(QStringLiteral("{mod_id}"), item.modId);
		line.replace(QStringLiteral("{url}"), item.url);
		line.replace(QStringLiteral("{version}"), item.version);
		line.replace(QStringLiteral("{authors}"), joinedAuthors(item));
		line.replace(QStringLiteral("{filename}"), item.fileName);
		lines.append(line);
	}
	return lines.join(QStringLiteral("\n"));
}

QString ContentListExport::exampleLine(Format format)
{
	switch (format) {
		case Format::Html:
			return QStringLiteral(
				"<li><a href=\"{url}\">{name}</a> [{version}] by "
				"{authors}</li>");
		case Format::Markdown:
			return QStringLiteral("[{name}]({url}) [{version}] by {authors}");
		case Format::PlainText:
			return QStringLiteral("{name} ({url}) [{version}] by {authors}");
		case Format::Json:
			return QStringLiteral(
				"{\"name\":\"{name}\",\"url\":\"{url}\","
				"\"version\":\"{version}\",\"authors\":\"{authors}\"},");
		case Format::Csv:
			return QStringLiteral("{name},{url},{version},\"{authors}\"");
		case Format::Custom:
			return QString();
	}
	return QString();
}

QString ContentListExport::fileExtension(Format format)
{
	switch (format) {
		case Format::Html:
			return QStringLiteral(".html");
		case Format::Markdown:
			return QStringLiteral(".md");
		case Format::Json:
			return QStringLiteral(".json");
		case Format::Csv:
			return QStringLiteral(".csv");
		case Format::PlainText:
		case Format::Custom:
			return QStringLiteral(".txt");
	}
	return QStringLiteral(".txt");
}
