/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * PackUpdater — MMCO entry point.
 *
 * Tracks where each instance's modpack came from and lets the user
 * pull newer releases of that pack from the original catalogue.
 *
 * Provenance recording is the launcher's job — InstanceImportTask
 * writes `PackProvider` / `PackSlug` / `PackVersionId` / etc. into
 * `instance.cfg` when a pack is imported through the Modrinth or
 * CurseForge browser. This plugin reads those keys through the S24
 * per-instance settings API; it does NOT sniff manifests.
 *
 * Plugin responsibilities:
 *
 *   1. Inject a "Modpack" tab into instance settings (linked /
 *      unlinked view).
 *   2. On user request, ask the upstream source for the latest
 *      version, diff it against the recorded version, light the
 *      `instance_set_update_available` badge, and (in a later step)
 *      apply mod + override + loader changes through
 *      S05/S09/S10/S29.
 *   3. Provide an "Attach to pack…" flow for instances that
 *      weren't created through the launcher's browser pages. (UI
 *      placeholder until the upstream search adapters land.)
 *
 * Why a launcher-side scanner doesn't exist: MeshMC core
 * intentionally has no built-in modpack-update logic. The
 * `BaseInstance::m_hasUpdate` flag is the badge slot; this plugin
 * is what fills it via S04 `instance_set_update_available`.
 */

#include "plugin/sdk/mmco_sdk.h"
#include "PackMetadata.h"
#include "PackUpdaterPage.h"

#include <QList>

MMCO_DEFINE_MODULE("PackUpdater", "0.2.0", "Project Tick",
				   "Track modpack provenance and pull pack updates from "
				   "Modrinth / CurseForge.",
				   "GPL-3.0-or-later");

static MMCOContext* g_ctx = nullptr;

namespace
{

	/* Inject our "Modpack" tab into every instance's settings
	 * dialog. The launcher walks every registered plugin's
	 * UI_INSTANCE_PAGES hook each time the user opens an instance's
	 * Edit dialog; we append a single PackUpdaterPage that decides
	 * itself whether to render the linked or unlinked view based on
	 * the `PackProvider` key in instance.cfg. */
	int on_instance_pages(void* /*mh*/, uint32_t /*hook_id*/, void* payload,
						  void* /*ud*/)
	{
		auto* evt = static_cast<MMCOInstancePagesEvent*>(payload);
		if (!evt || !evt->page_list_handle || !evt->instance_id)
			return 0;
		auto* pages = static_cast<QList<BasePage*>*>(evt->page_list_handle);
		const QString instId = QString::fromUtf8(evt->instance_id);
		const QString instPath =
			evt->instance_path ? QString::fromUtf8(evt->instance_path)
							   : QString();
		pages->append(new PackUpdaterPage(instId, instPath, g_ctx));
		return 0;
	}

} /* namespace */

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;
	MMCO_LOG(ctx, "PackUpdater initialising…");

	int rc = ctx->hook_register(ctx->module_handle,
								MMCO_HOOK_UI_INSTANCE_PAGES,
								on_instance_pages, nullptr);
	if (rc != 0) {
		MMCO_ERR(ctx, "PackUpdater: failed to register UI_INSTANCE_PAGES hook");
		return rc;
	}

	MMCO_LOG(ctx, "PackUpdater ready.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx)
		MMCO_LOG(g_ctx, "PackUpdater unloading.");
	g_ctx = nullptr;
}

} /* extern "C" */
