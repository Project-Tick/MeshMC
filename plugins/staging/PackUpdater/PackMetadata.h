/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * PackMetadata — pack-source provenance backed by the launcher's
 * own `instance.cfg`.
 *
 * The keys live on BaseInstance (pre-registered at construction in
 * BaseInstance.cpp) and are written by InstanceImportTask when a
 * pack is imported through Modrinth / CurseForge. The PackUpdater
 * plugin only reads (and, in the attach flow, writes) — it never
 * owns its own sidecar file.
 *
 * All I/O goes through the S24 per-instance settings API, so the
 * plugin doesn't touch the INI parser directly.
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"

namespace pack_updater
{

	/* Which catalogue the pack came from. The string corresponds to
	 * what InstanceImportTask writes into `PackProvider`. */
	enum class Provider {
		Unknown = 0,
		Modrinth,
		CurseForge,
		MultiMC, /* raw zip; no upstream catalogue to check against */
	};

	const char* providerToString(Provider p);
	Provider providerFromString(const QString& s);

	/* The set of keys we read from / write to instance.cfg. Kept as
	 * a struct so the UI / source adapters can pass a single value
	 * around without re-querying for each field. */
	struct PackRecord {
		Provider provider = Provider::Unknown;

		QString packId;	  /* Modrinth project id / CF projectID as string */
		QString packSlug; /* Modrinth slug; on CF we reuse the addonId */
		QString installedVersionId;	   /* version id / file id */
		QString installedVersionLabel; /* human-readable "1.2.3" */
		QString sourceUrl;			   /* canonical pack page */
		QString iconUrl;			   /* upstream icon */
		QString manifestSha512;		   /* future: used by the planner */
		QString installedAtIso8601;
	};

	/* Returns std::nullopt when the instance has no PackProvider
	 * recorded (i.e. it's not a pack-managed instance). All other
	 * fields are optional and may be empty strings. */
	std::optional<PackRecord> load(MMCOContext* ctx, const QString& instanceId);

	/* Write the record back to instance.cfg. Returns true on
	 * success. Used by the attach flow and by the "Apply update"
	 * step once it's wired. */
	bool save(MMCOContext* ctx, const QString& instanceId,
			  const PackRecord& rec);

	/* Convenience predicate — does the instance have a non-empty
	 * PackProvider key? */
	bool exists(MMCOContext* ctx, const QString& instanceId);

	/* Wipe every pack-source key. Used by the Detach flow. */
	bool clear(MMCOContext* ctx, const QString& instanceId);

} /* namespace pack_updater */
