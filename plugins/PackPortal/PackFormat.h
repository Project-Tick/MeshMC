/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Common types used by every PackPortal format adapter (mrpack,
 * CurseForge, MultiMC). Keeping them in one header lets the export
 * engine speak about "files I want to ship" without caring which
 * manifest dialect they end up in.
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"
#include <optional>

namespace pack
{

/* A single mod / world / config file we want to include in the export. */
struct PackFile {
	/* Path RELATIVE to the instance's .minecraft directory. Always
	 * uses forward slashes. */
	QString instancePath;

	/* Display name shown in the UI tree. */
	QString displayName;

	/* SHA-1 / SHA-512 / MD5 / file size. Filled lazily by the export
	 * engine before each adapter emits a manifest. SHA-512 (not
	 * SHA-256) because that's the hash Modrinth's mrpack format
	 * mandates — using anything else would make our manifests reject
	 * themselves on reimport. */
	QByteArray sha1;
	QByteArray sha512;
	QByteArray md5;
	qint64 sizeBytes = 0;

	/* Optional CurseForge identifier (project id + file id). Lets the
	 * Flame adapter reference the file from CurseForge instead of
	 * shipping the binary itself, which is often the only legal way
	 * to redistribute paid CurseForge mods. */
	std::optional<int> curseProjectId;
	std::optional<int> curseFileId;

	/* Optional Modrinth identifier (slug + version id). Used by the
	 * mrpack adapter so mods come from Modrinth instead of being
	 * embedded. */
	QString modrinthSlug;
	QString modrinthVersionId;
	QUrl downloadUrl; /* if present, used as `downloads[0]` */

	/* If true, the file is embedded directly into the archive
	 * (overrides/), not referenced via an external download. */
	bool embed = true;

	/* If true, the file lives under the server-only side. mrpack
	 * splits client/server through the `env` block; we mirror it for
	 * the CurseForge case (which only has one side). */
	bool clientOnly = true;
	bool serverOnly = false;
};

/* Resolved component versions for the instance, used by every adapter
 * to fill its modloader section. */
struct ComponentSet {
	QString minecraftVersion;
	QString forgeVersion;	  /* uid net.minecraftforge */
	QString neoForgeVersion;  /* uid net.neoforged */
	QString fabricVersion;	  /* uid net.fabricmc.fabric-loader */
	QString quiltVersion;	  /* uid org.quiltmc.quilt-loader */
	QString liteloaderVersion;
};

/* Metadata that's the same regardless of adapter. */
struct PackInfo {
	QString name;
	QString version;
	QString author;
	QString summary;
	QString iconPath; /* absolute path to icon-256.png (optional) */
	QString iconKey;  /* launcher icon-key for the instance — passed
					   * through verbatim in the MultiMC export so the
					   * imported instance keeps the same icon */
	ComponentSet components;
};

/* A staging directory describing one in-flight export. */
struct ExportStage {
	QString stageDir;	  /* tmp dir we build the archive in */
	QString instanceRoot; /* absolute path to the instance root */
	PackInfo info;
	QList<PackFile> files;
};

/* Format identifier for the UI. */
enum class Format {
	MrPack,
	CurseForge,
	MultiMC,
};

} // namespace pack
