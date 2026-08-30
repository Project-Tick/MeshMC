/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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
