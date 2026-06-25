/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 */

#include "plugin/sdk/mmco_cxx_sdk.hpp"
#include "NewsViewerDialog.h"

MMCO_DEFINE_MODULE(
	"NewsViewer", "2.0.0", "Project Tick",
	"News viewer dialog with multi-feed RSS support and cmark rendering",
	"GPL-3.0-or-later");

/* ── module state ─────────────────────────────────────────────────── */

static MMCOContext* g_ctx = nullptr;
static QPointer<NewsViewerDialog> g_dialog;

/* ── helpers ──────────────────────────────────────────────────────── */

static QWidget* findMainWindow()
{
	for (auto* w : qApp->topLevelWidgets()) {
		if (w->objectName() == QStringLiteral("MainWindow"))
			return w;
	}
	return nullptr;
}

/*
 * openNewsDialog — opens (or raises) the dialog.
 */
static void openNewsDialog(bool sidebarVisible)
{
	if (!g_ctx)
		return;

	if (g_dialog && g_dialog->isVisible()) {
		/* Dialog already open — just switch sidebar state and re-select */
		g_dialog->loadEntries(sidebarVisible);
		g_dialog->raise();
		g_dialog->activateWindow();
		return;
	}

	g_dialog = new NewsViewerDialog(g_ctx, findMainWindow());
	g_dialog->setAttribute(Qt::WA_DeleteOnClose);
	g_dialog->loadEntries(sidebarVisible);
	g_dialog->show();
}

/* ── hook: UI_MAIN_READY ──────────────────────────────────────────── */

static int on_ui_main_ready(void* /*mh*/, uint32_t /*hook_id*/, void* payload,
							void* /*ud*/)
{
	/* Fast path: MainWindow hands us direct opaque handles to the news
	 * widgets via the MMCOUiMainReadyPayload. No widget-tree walk needed.
	 *
	 * Older hosts may still call us with a null payload (or a payload
	 * carrying null handles for some entries). In that case we fall back
	 * to the historical scan so the plugin keeps working in mixed
	 * deployments. */

	auto* p = static_cast<MMCOUiMainReadyPayload*>(payload);

	if (p && p->news_label_button) {
		if (auto* btn = static_cast<QToolButton*>(p->news_label_button)) {
			btn->disconnect();
			QObject::connect(btn, &QAbstractButton::clicked, qApp,
							 []() { openNewsDialog(false); });
			MMCO_LOG(g_ctx, "NewsViewer: wired newsLabel (fast path).");
		}
	}

	return 0;
}

/* ── hook: NEWS_UPDATED ───────────────────────────────────────────── */

static int on_news_updated(void* /*mh*/, uint32_t /*hook_id*/,
						   void* /*payload*/, void* /*ud*/)
{
	if (g_dialog && g_dialog->isVisible())
		g_dialog->loadEntries(g_dialog->isSidebarVisible());
	return 0;
}

/* ── toolbar action callback ──────────────────────────────────────── */

static void on_news_toolbar_action(void* /*ud*/)
{
	openNewsDialog(true);
}

/* ── mmco_init ────────────────────────────────────────────────────── */

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;
	MMCO_LOG(ctx, "NewsViewer initializing...");

	/* Extra feeds configured at build time via -DMeshMC_NEWS_EXTRA_FEEDS
	 * are seeded into the host's news feed list by PluginManager during
	 * its own init, before this plugin is loaded; the dialog picks them
	 * up via the standard news_get_feed_count / news_get_feed_url API. */

	ctx->hook_register(ctx->module_handle, MMCO_HOOK_UI_MAIN_READY,
					   on_ui_main_ready, nullptr);
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_NEWS_UPDATED,
					   on_news_updated, nullptr);

	ctx->ui_register_instance_action_cb(
		ctx->module_handle, "All News",
		"Open the news viewer with all sections", "news",
		on_news_toolbar_action, nullptr);

	MMCO_LOG(ctx, "NewsViewer initialized.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx)
		MMCO_LOG(g_ctx, "NewsViewer unloading.");
	if (g_dialog)
		g_dialog->close();
	g_ctx = nullptr;
	g_dialog = nullptr;
}

} /* extern "C" */
