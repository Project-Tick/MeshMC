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

#pragma once

#include <QString>
#include <QStringList>

/* What a modpack version put into an instance.
 *
 * Updating a pack in place merges the new version over the old one,
 * which is what keeps the worlds, the screenshots and the config edits
 * the pack has no opinion about. The cost of that is that nothing gets
 * removed: a mod the new version dropped stays on disk and keeps being
 * loaded, usually next to the version that was meant to replace it.
 *
 * To remove it we have to know it was ours. "Everything in mods/" is not
 * an answer - the user is allowed to add their own mods to a modpack and
 * an update must not eat them. So each install records the files it is
 * responsible for, and the next update removes the ones it no longer
 * ships.
 *
 * The list is deliberately provider-agnostic: a flat set of paths
 * relative to the instance's game directory, with no trace of whether
 * they came from a Modrinth index, a CurseForge manifest or an overrides
 * folder. Anything else would mean re-parsing a provider manifest to
 * answer a question that has nothing to do with the provider, and would
 * have to grow a new case for every pack format added later.
 */
namespace PackContents
{
	/* Where the list lives, given an instance's root directory. Beside
	 * instance.cfg rather than inside the game directory, so that it is
	 * launcher bookkeeping the game never sees. */
	QString listPath(const QString& instanceRoot);

	/* Record @p gameRelativePaths as the contents of the version that
	 * has just been installed into @p instanceRoot.
	 *
	 * Paths are normalised, de-duplicated and sorted on the way in, so
	 * that two installs of the same version produce the same file and a
	 * diff between versions is honest about what actually changed. */
	bool write(const QString& instanceRoot,
			   const QStringList& gameRelativePaths);

	/* Read back a previously recorded list.
	 *
	 * Returns false when there is nothing usable to read - no list at
	 * all (an instance installed before this was recorded, or by another
	 * launcher), or one written in a format this build does not
	 * understand. The distinction from "an empty list" matters: an empty
	 * list means the version shipped nothing and cleanup can proceed,
	 * while no list at all means we do not know what belongs to the pack
	 * and must not delete anything.
	 */
	bool read(const QString& instanceRoot, QStringList& out);

	/* A path in the canonical form used inside the list: forward
	 * slashes, no redundant separators, relative to the game directory.
	 *
	 * Returns an empty string for anything that cannot be treated as a
	 * path inside the game directory - absolute paths, and paths that
	 * climb out with "..". Those are dropped rather than corrected: this
	 * list is fed to a delete loop, so a path we cannot vouch for is one
	 * we must not record. */
	QString normalizePath(const QString& path);

	/* The files @p oldPaths installed that @p newPaths does not, as
	 * normalised game-relative paths.
	 *
	 * Each stale entry brings its enabled/disabled counterpart along: a
	 * mod the user turned off is still a file the pack put there, and it
	 * is listed under the name it had at install time, not the name it
	 * has now.
	 *
	 * Nothing the new version ships is ever returned, whether it was
	 * asked for directly or reached through that counterpart rule. An
	 * update that flips a mod from required to optional renames
	 * "mods/x.jar" to "mods/x.jar.disabled", and deleting the file the
	 * update just installed would be worse than leaving a stale one.
	 */
	QStringList staleEntries(const QStringList& oldPaths,
							 const QStringList& newPaths);
} // namespace PackContents
