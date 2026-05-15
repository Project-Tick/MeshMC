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
 */

#pragma once

#include "plugin/PluginMetadata.h"

#include <QByteArray>
#include <QString>

/*
 * PluginSignature — utilities for working with the GPG signature trailer
 * appended to .mmco files, and for deciding whether a given SPDX license
 * is an open-source license that exempts a module from signature
 * requirements.
 *
 * The trailer layout is described in MMCOFormat.h.
 */

namespace PluginSignature
{

	/* Holds the raw bytes pulled out of a .mmco file: the original module
	 * payload (everything before the trailer) and the detached signature
	 * itself. Either field may be empty if the file has no trailer. */
	struct ExtractedTrailer {
		QByteArray payload;		/* Bytes that were signed. */
		QByteArray signature;	/* ASCII-armored detached signature. */
		bool present = false;	/* True if a trailer was found. */
		bool malformed = false; /* True if trailer magic matched but layout
								 * could not be parsed. */
	};

	/*
	 * Read the trailer from a .mmco file. The return value's `present`
	 * field tells you whether a signature was actually appended.
	 *
	 * On I/O errors the function returns an empty struct with present=false.
	 * On a malformed-but-present trailer it sets malformed=true and leaves
	 * the byte arrays empty so callers can flag the module as untrusted.
	 */
	ExtractedTrailer extractTrailer(const QString& filePath);

	/*
	 * Verify the detached signature against the payload using GpgME. The
	 * keyring used is the one configured by setKeyringPath() (falls back
	 * to the default GnuPG home directory if no override is set).
	 *
	 * Output parameters:
	 *   detail      — human-readable status / error message
	 *   fingerprint — fingerprint of the signing key (empty on failure)
	 *
	 * Returns one of the Valid / Untrusted / BadSignature / Error states.
	 */
	PluginSignatureState verify(const QByteArray& payload,
								const QByteArray& signature, QString& detail,
								QString& fingerprint);

	/*
	 * Convenience: read the trailer from the file, then verify it. If
	 * the file has no trailer, returns Absent without touching GpgME.
	 *
	 * Result is memoised in a persistent disk cache keyed on
	 * (path, size, mtime). The cache is the single biggest reason a
	 * second-and-later launcher startup is fast; the first startup
	 * after a `make install` still runs the full GpgME verification
	 * because the (size, mtime) tuple is new. Set `bypassCache=true`
	 * to force a fresh verification (used by the plugins dialog's
	 * "Re-verify" button).
	 */
	PluginSignatureState verifyFile(const QString& filePath, QString& detail,
									QString& fingerprint,
									bool bypassCache = false);

	/*
	 * Override the path used as the GnuPG home directory (where the
	 * trusted keyring lives). Empty string resets to the GpgME default.
	 *
	 * Persisted across calls; intended to be called once during launcher
	 * startup from a value read out of the application settings.
	 */
	void setKeyringPath(const QString& path);

	/*
	 * Configure where the on-disk verification cache lives. Calling
	 * this also loads the existing cache from disk (no-op if the file
	 * is missing or unreadable). Empty path disables the cache.
	 *
	 * Called once during launcher startup, before the first plugin is
	 * loaded. The cache is shared across every verifyFile() call in
	 * the process. Thread-safe.
	 */
	void setCachePath(const QString& path);

	/*
	 * Flush the in-memory cache out to its configured path. Safe to
	 * call on shutdown; cheap (few hundred bytes per cached plugin).
	 * Called automatically at the end of PluginLoader::discoverModules().
	 */
	void flushCache();

	/*
	 * Check whether the given SPDX license expression refers to an
	 * OSI-approved open-source license (or an FSF-recognised free
	 * software license).
	 *
	 * The check is liberal: it accepts compound expressions such as
	 * "GPL-3.0-or-later WITH Classpath-exception-2.0" and "Apache-2.0 OR
	 * MIT" — the module is treated as OSS if any clause is OSS.
	 *
	 * An empty / missing license string is considered non-OSS and will
	 * therefore require a signature.
	 */
	bool isOpenSourceLicense(const QString& spdxLicense);

	/*
	 * Convert a PluginSignatureState to a short human-readable label.
	 * Used by the plugins dialog to render the state column.
	 */
	const char* stateLabel(PluginSignatureState state);

} // namespace PluginSignature
