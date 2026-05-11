/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 */

#include "plugin/sdk/mmco_sdk.h"
#include "NewsViewerDialog.h"
#include "BuildConfig.h"

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
 *
 * sidebarVisible = true  → "More News" mode: sidebar open, all entries
 * sidebarVisible = false → "Latest" mode: sidebar hidden, first entry shown
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

	if (p && p->more_news_action) {
		if (auto* action = static_cast<QAction*>(p->more_news_action)) {
			action->disconnect();
			QObject::connect(action, &QAction::triggered, qApp,
							 []() { openNewsDialog(true); });
			MMCO_LOG(g_ctx, "NewsViewer: wired actionMoreNews (fast path).");
		}
	} else {
		/* Fallback: scan toolbars for actionMoreNews. */
		for (auto* w : qApp->allWidgets()) {
			if (auto* toolbar = qobject_cast<QToolBar*>(w)) {
				for (auto* action : toolbar->actions()) {
					if (action->objectName() ==
						QStringLiteral("actionMoreNews")) {
						action->disconnect();
						QObject::connect(action, &QAction::triggered, qApp,
										 []() { openNewsDialog(true); });
						MMCO_LOG(g_ctx,
								 "NewsViewer: wired actionMoreNews (scan).");
						break;
					}
				}
			}
		}
	}

	if (p && p->news_label_button) {
		if (auto* btn = static_cast<QToolButton*>(p->news_label_button)) {
			btn->disconnect();
			QObject::connect(btn, &QAbstractButton::clicked, qApp,
							 []() { openNewsDialog(false); });
			MMCO_LOG(g_ctx, "NewsViewer: wired newsLabel (fast path).");
		}
	} else {
		/* Fallback: walk the news toolbar for a QToolButton that is not
		 * the More News action widget. */
		for (auto* w : qApp->allWidgets()) {
			if (auto* btn = qobject_cast<QToolButton*>(w)) {
				auto* tb = qobject_cast<QToolBar*>(btn->parent());
				if (tb && tb->objectName() == QStringLiteral("newsToolBar")) {
					if (btn->defaultAction() == nullptr ||
						btn->defaultAction()->objectName() !=
							QStringLiteral("actionMoreNews")) {
						btn->disconnect();
						QObject::connect(btn, &QAbstractButton::clicked, qApp,
										 []() { openNewsDialog(false); });
						MMCO_LOG(g_ctx, "NewsViewer: wired newsLabel (scan).");
						break;
					}
				}
			}
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

	/* Extra feeds are defined at build time via -DMeshMC_NEWS_EXTRA_FEEDS=
	 * and baked into BuildConfig — users cannot change them at runtime. */
	if (!BuildConfig.NEWS_EXTRA_FEEDS.isEmpty()) {
		const QStringList urls = BuildConfig.NEWS_EXTRA_FEEDS.split(
			QLatin1Char(';'), Qt::SkipEmptyParts);
		for (const QString& url : urls) {
			QString trimmed = url.trimmed();
			if (!trimmed.isEmpty())
				ctx->news_add_feed_url(ctx->module_handle,
									   trimmed.toUtf8().constData());
		}
	}

	ctx->hook_register(ctx->module_handle, MMCO_HOOK_UI_MAIN_READY,
					   on_ui_main_ready, nullptr);
	ctx->hook_register(ctx->module_handle, MMCO_HOOK_NEWS_UPDATED,
					   on_news_updated, nullptr);

	ctx->ui_register_instance_action_cb(ctx->module_handle, "News",
										"Open the news viewer", "news",
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
