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

#pragma once
#include <cmark.h>
#include <QString>
#include <QByteArray>

/**
 * Markdown-to-HTML wrapper using cmark (CommonMark)
 *
 * Drop-in replacement for the old hoedown-based HoeDown class.
 * Kept the class name for source compatibility.
 */
class HoeDown
{
  public:
	QString process(QByteArray input)
	{
		/* CMARK_OPT_UNSAFE lets raw HTML through instead of dropping it.
		 *
		 * Everything this renders comes from a content platform, and
		 * those do not send clean CommonMark. CurseForge sends HTML and
		 * nothing else - its changelog and description endpoints have no
		 * markdown at all - so without this option those come out
		 * *completely empty*: cmark discards raw HTML blocks by default.
		 * Modrinth does send markdown, but with <details>, <img> and
		 * <center> in it, and dropping those silently removes half of
		 * some descriptions.
		 *
		 * "Unsafe" is about injecting arbitrary HTML into the output.
		 * The output goes into a QTextBrowser, which runs no scripts, and
		 * every pane that shows it refuses to open a link whose scheme is
		 * not http(s). What is left is remote images, which these panes
		 * fetch deliberately.
		 *
		 * CMARK_OPT_NOBREAKS renders a single newline as a space rather
		 * than a hard break, which is how a paragraph typed with a
		 * narrow editor is meant to read. */
		char* html = cmark_markdown_to_html(
			input.constData(), input.size(),
			CMARK_OPT_UNSAFE | CMARK_OPT_NOBREAKS);
		QString result = QString::fromUtf8(html);
		free(html);
		return result;
	}
};
