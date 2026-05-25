/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * UpdateSource — provider-agnostic interface for "what's the newest
 * version of this pack?" lookups.
 *
 * Each catalogue (Modrinth, CurseForge) ships an implementation in
 * its own translation unit; the page-level code only talks to this
 * interface so swapping or adding a provider is a build-system
 * change, not a UI rewrite.
 *
 * The interface is intentionally narrow:
 *
 *   - `fetchLatest(record, cb)` — kick off a single async lookup;
 *     `cb` fires exactly once with the result.
 *
 * Async-ness is delegated to S11 (`http_get` / S30
 * `http_get_with_headers`). All callbacks land on the GUI thread.
 */

#pragma once

#include "PackMetadata.h"
#include <functional>

namespace pack_updater
{

	/* What the source resolves to. Keep it flat — anything richer
	 * (full file list, hashes, override list) belongs to the
	 * upcoming UpdatePlanner step. For "is there an update" the UI
	 * only needs version label + id + a manifest URL it can fetch
	 * later. */
	struct LatestVersion {
		bool ok = false;	  /* false ⇒ network/parse failure */
		QString errorMessage; /* populated when !ok */

		QString versionId;	  /* e.g. Modrinth version id */
		QString versionLabel; /* e.g. "1.2.3" */
		QString iconUrl;	  /* fresh icon URL from upstream */
		QString manifestUrl;  /* mrpack file URL / CF manifest dl URL */
		QString sourceUrl;	  /* canonical pack page */
	};

	using LatestVersionCallback = std::function<void(LatestVersion)>;

	/* Interface every provider satisfies. Implementations are stateless
	 * by convention — they take `ctx` for the HTTP API and don't hold
	 * onto anything across calls. */
	class UpdateSource
	{
	  public:
		virtual ~UpdateSource() = default;
		virtual void fetchLatest(MMCOContext* ctx, const PackRecord& rec,
								 LatestVersionCallback cb) = 0;
	};

	/* Factory — picks the right source for a record. Returns nullptr
	 * for Provider::Unknown / MultiMC (those have no upstream
	 * catalogue to ask). Ownership is transferred to the caller;
	 * usually a unique_ptr in the page. */
	std::unique_ptr<UpdateSource> makeSource(Provider p);

} /* namespace pack_updater */
