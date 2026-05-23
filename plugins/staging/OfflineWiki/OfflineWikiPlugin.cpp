/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * OfflineWikiPlugin — MMCO entry point. Auto-discovers bundles under
 * <plugin_data>/bundles/ on init and exposes a global settings page.
 */

#include "plugin/sdk/mmco_sdk.h"
#include "WikiBundle.h"
#include "SearchIndex.h"
#include "WikiPage.h"

MMCO_DEFINE_MODULE(
	"OfflineWiki", "1.0.0", "Project Tick",
	"Local-first Minecraft / mod wiki viewer (Markdown bundles + ZIM)",
	"GPL-3.0-or-later");

namespace
{
	MMCOContext* g_ctx = nullptr;
	QList<WikiBundle*> g_bundles;
	SearchIndex* g_index = nullptr;

	QString bundlesDir()
	{
		if (!g_ctx)
			return {};
		QString plug =
			QString::fromUtf8(g_ctx->fs_plugin_data_dir(g_ctx->module_handle));
		return QDir(plug).filePath("bundles");
	}

	void scanBundlesDir()
	{
		QString dir = bundlesDir();
		QDir().mkpath(dir);
		QDir d(dir);
		if (!d.exists())
			return;

		// Each subdirectory or .zim file inside bundles/ is a bundle.
		for (const auto& fi :
			 d.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
			QString path = fi.absoluteFilePath();
			if (fi.isFile() &&
				!path.endsWith(QStringLiteral(".zim"), Qt::CaseInsensitive))
				continue;
			auto* b = openBundle(path);
			if (b)
				g_bundles.append(b);
		}

		g_index->reset();
		for (auto* b : g_bundles)
			g_index->addBundle(b);
	}
} // namespace

static int on_global_settings_pages(void*, uint32_t, void* payload, void*)
{
	auto* evt = static_cast<MMCOGlobalSettingsPagesEvent*>(payload);
	if (!evt || !evt->page_list_handle)
		return 0;
	auto* pages = static_cast<QList<BasePage*>*>(evt->page_list_handle);
	pages->append(new WikiPage(&g_bundles, g_index));
	return 0;
}

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;
	MMCO_LOG(ctx, "OfflineWiki initialising…");

	g_index = new SearchIndex();
	scanBundlesDir();

	{
		QByteArray msg = "OfflineWiki: opened " +
						 QByteArray::number(g_bundles.size()) + " bundle(s)";
		MMCO_LOG(ctx, msg.constData());
	}

	ctx->hook_register(ctx->module_handle, MMCO_HOOK_UI_GLOBAL_SETTINGS_PAGES,
					   on_global_settings_pages, nullptr);

	MMCO_LOG(ctx, "OfflineWiki ready.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx)
		MMCO_LOG(g_ctx, "OfflineWiki unloading.");
	for (auto* b : g_bundles)
		delete b;
	g_bundles.clear();
	delete g_index;
	g_index = nullptr;
	g_ctx = nullptr;
}

} /* extern "C" */
