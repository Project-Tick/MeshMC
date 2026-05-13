/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 *  SkinManager — MMCO plugin entry point.
 *
 *  Replaces the launcher's built-in "Upload Skin" dialog with a richer
 *  one that includes a real-time 3D skin preview.
 *
 *  Injection strategy
 *  ──────────────────
 *  The launcher exposes its "Upload Skin" action through AccountListPage
 *  (Settings → Accounts), specifically as a QAction with objectName
 *  "actionUploadSkin" inside an AccountListPage QMainWindow widget.
 *  Whenever the global settings dialog opens, the page is reconstructed
 *  and a fresh QAction is created, so we can't capture the action once
 *  and remember it forever — we have to re-hook on every open.
 *
 *  Pattern used (NVIDIAPrime / LinuxPerf precedent):
 *    1. Connect to Application::globalSettingsAboutToOpen.
 *    2. On the next event-loop turn (QTimer::singleShot(0)), walk the
 *       widget tree for an AccountListPage and its actionUploadSkin
 *       child action.
 *    3. action->disconnect(); action->connect(... → openOurDialog).
 *    4. openOurDialog reads the selected account from the page's
 *       QListView selection, builds a SkinManagerDialog, and exec()s it.
 *
 *  All Qt signal connections belong to g_guard, a QObject created in
 *  mmco_init and dropped (not deleted) in mmco_unload — see the same
 *  comment-block in NVIDIAPrime for why deleting it during shutdown
 *  is unsafe.
 */

#include "plugin/sdk/mmco_sdk.h"
#include "SkinManagerDialog.h"

#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/MinecraftAccount.h"

#include <QAbstractItemView>
#include <QPointer>
#include <QToolBar>
#include <QToolButton>

MMCO_DEFINE_MODULE("SkinManager", "1.0.0", "Project Tick",
				   "3D skin preview integrated into the Upload Skin dialog. "
				   "Replaces the launcher's stock skin-upload window with "
				   "a richer, viewer-equipped version.",
				   "LGPL-3.0-or-later");

static MMCOContext* g_ctx = nullptr;
static QObject* g_guard = nullptr;

/* Find the AccountListPage widget — set by Qt Designer's objectName
 * attribute on its top-level QMainWindow node. Returns nullptr if the
 * settings dialog has not built it yet (we re-try on the next event
 * loop turn in that case). */
static QWidget* findAccountListPage()
{
	for (auto* w : qApp->allWidgets()) {
		if (w->objectName() == QStringLiteral("AccountListPage"))
			return w;
	}
	return nullptr;
}

/* Open the SkinManagerDialog against whichever account is currently
 * selected in the AccountListPage's list view. Falls back to a
 * user-visible message box on each of the three reasonable failure
 * modes (no selection, offline-only selection, broken model row).
 *
 * Implementation note: the launcher's AccountListPage.ui declares the
 * accounts list with objectName "listView" but the actual class is
 * VersionListView, which derives from QTreeView (not QListView). We
 * therefore search for the common base — QAbstractItemView — so the
 * findChild<>() succeeds regardless of which subclass the .ui actually
 * picks. */
static void openSkinManagerForSelection(QWidget* accountListPage)
{
	if (!accountListPage)
		return;

	auto* listView = accountListPage->findChild<QAbstractItemView*>(
		QStringLiteral("listView"));
	if (!listView || !listView->selectionModel()) {
		MMCO_WARN(g_ctx, "SkinManager: AccountListPage has no listView; cannot "
						 "open the skin manager.");
		return;
	}
	const auto indexes = listView->selectionModel()->selectedIndexes();
	if (indexes.isEmpty()) {
		QMessageBox::information(
			accountListPage, QObject::tr("No account selected"),
			QObject::tr("Select a Microsoft account from the list and "
						"click Upload Skin again."));
		return;
	}

	const QModelIndex idx = indexes.first();
	MinecraftAccountPtr account =
		idx.data(AccountList::PointerRole).value<MinecraftAccountPtr>();
	if (!account) {
		MMCO_WARN(g_ctx, "SkinManager: selection has no MinecraftAccountPtr — "
						 "ignoring click.");
		return;
	}
	if (!account->isMSA()) {
		QMessageBox::information(
			accountListPage, QObject::tr("Offline account"),
			QObject::tr("Offline accounts have no Mojang profile to "
						"upload a skin to. Pick a Microsoft account "
						"instead."));
		return;
	}

	SkinManagerDialog dlg(account, accountListPage);
	dlg.exec();
}

/* Walk the AccountListPage for its actionUploadSkin QAction and
 * replace its triggered() handler with ours. Re-runs each time the
 * settings dialog opens because Qt re-builds the page every time. */
static void rewireUploadSkinAction()
{
	QWidget* page = findAccountListPage();
	if (!page)
		return;

	auto* action =
		page->findChild<QAction*>(QStringLiteral("actionUploadSkin"));
	if (!action) {
		MMCO_WARN(g_ctx,
				  "SkinManager: actionUploadSkin not found on "
				  "AccountListPage; skin-manager dialog won't replace the "
				  "built-in one this session.");
		return;
	}

	/* Sever every existing connection — including the .ui's auto-wired
	 * on_actionUploadSkin_triggered slot — and add our own. The page
	 * widget itself is captured (not the action) because Qt deletes
	 * the action with the page; capturing `page` lets us re-resolve
	 * the listView every click defensively. */
	QPointer<QWidget> pageGuard = page;
	action->disconnect();
	QObject::connect(action, &QAction::triggered, g_guard, [pageGuard]() {
		if (pageGuard)
			openSkinManagerForSelection(pageGuard);
	});

	/*
	 * Force the action permanently enabled.
	 *
	 * Why this matters: AccountListPage::updateButtonStates() disables
	 * actionUploadSkin whenever:
	 *   • no row is selected, or
	 *   • the selected account is still refreshing (isActive() == true)
	 *
	 * Both restrictions exist because the *built-in* SkinUploadDialog
	 * requires a pre-selected MSA account to write into. Our replacement
	 * dialog handles those edge cases itself with friendly message
	 * boxes, so the action should always be clickable.
	 *
	 * We can't simply call setEnabled(true) once: updateButtonStates()
	 * runs again on every selectionChanged() and accountActivityChanged()
	 * signal and flips the action back to disabled. We also can't hook
	 * QAction::changed and bounce setEnabled(true) inside that slot —
	 * even with a `!isEnabled()` guard, Qt's property-change cascade
	 * during AccountListPage construction sometimes lets the disabled
	 * state win.
	 *
	 * The bulletproof workaround is a low-frequency polling timer
	 * tied to the page's lifetime via g_guard. The poll only runs
	 * while the settings dialog is alive (we check the page pointer
	 * each tick) and stops itself once the page is destroyed. */
	/* Force-enable the action AND its WideBar button.
	 *
	 * AccountListPage uses a WideBar (a custom QToolBar subclass —
	 * launcher/ui/widgets/WideBar.cpp). Internally WideBar replaces
	 * every action with an ActionButton — a private QToolButton
	 * subclass that mirrors action->isEnabled() into its own
	 * setEnabled() inside an actionChanged() slot bound to
	 * QAction::changed.
	 *
	 * Consequence: setting action->setEnabled(true) is *not enough*.
	 * AccountListPage::updateButtonStates() calls action->setEnabled(
	 * false) every time the selection or refresh state changes, and
	 * the ActionButton dutifully greys itself out before we get a
	 * chance to flip the action back.
	 *
	 * We fight back on TWO levels:
	 *   (1) keep the action enabled
	 *   (2) keep every QToolButton inside the page's toolBar enabled
	 *       (specifically the one bound to actionUploadSkin — the
	 *       defaultAction() of an ActionButton is the underlying
	 *       QAction).
	 *
	 * A 30 ms polling timer drives both, so we beat the page's own
	 * updateButtonStates() to the next paint cycle. The timer is
	 * parented to g_guard (long-lived) but tied to the page's
	 * lifetime via QPointer<QWidget> — once the page is gone, the
	 * timer self-destructs. */
	auto* targetAction = action;
	QObject::connect(action, &QAction::changed, g_guard, [targetAction]() {
		if (targetAction && !targetAction->isEnabled())
			targetAction->setEnabled(true);
	});
	action->setEnabled(true);

	auto* keepEnabled = new QTimer(g_guard);
	keepEnabled->setInterval(30);
	QPointer<QAction> actionRef = action;
	QObject::connect(
		keepEnabled, &QTimer::timeout, g_guard,
		[keepEnabled, actionRef, pageGuard]() {
			if (!pageGuard || !actionRef) {
				keepEnabled->deleteLater();
				return;
			}
			/* Re-enable the QAction itself. */
			if (!actionRef->isEnabled())
				actionRef->setEnabled(true);

			/*
			 * Walk every QToolButton inside the page and force-enable
			 * the one bound to our action. WideBar::ActionButton does
			 * NOT call setDefaultAction(), so defaultAction() is null —
			 * we cannot match the button to the action by that field.
			 * Instead, the WideBar widget hierarchy is:
			 *
			 *   WideBar (QToolBar, objectName "toolBar")
			 *     └─ ActionButton (QToolButton; one per WideBar entry)
			 *           ↑ has a private QAction* m_action linking it
			 *             back to actionUploadSkin via a constructor
			 *             argument, but that member is not visible to
			 *             us.
			 *
			 * The pragmatic workaround is to compare button text to
			 * the action text: WideBar::ActionButton::actionChanged()
			 * mirrors the action's text into setText() on every change,
			 * so the button labelled with our action's text *is* the
			 * one bound to that action. */
			if (!actionRef->isEnabled())
				return;
			const QString wanted = actionRef->text();
			if (wanted.isEmpty())
				return;
			const auto buttons = pageGuard->findChildren<QToolButton*>();
			for (auto* btn : buttons) {
				if (btn && btn->text() == wanted && !btn->isEnabled())
					btn->setEnabled(true);
			}
		});
	keepEnabled->start();

	/* First tick immediately so the user never sees a greyed button
	 * even on the very first paint. */
	QTimer::singleShot(0, g_guard, [actionRef, pageGuard]() {
		if (!pageGuard || !actionRef)
			return;
		actionRef->setEnabled(true);
		const QString wanted = actionRef->text();
		const auto buttons = pageGuard->findChildren<QToolButton*>();
		for (auto* btn : buttons) {
			if (btn && btn->text() == wanted && !btn->isEnabled())
				btn->setEnabled(true);
		}
	});

	MMCO_DBG(g_ctx, "SkinManager: actionUploadSkin rewired to plugin dialog.");
}

/* Application::globalSettingsAboutToOpen fires *before* the dialog is
 * built, so the AccountListPage widget may not exist yet when we get
 * here. Re-try a few times on the event loop to cover both the very
 * first open (slow construction) and immediate re-opens. */
static void scheduleRewire(int retries = 5)
{
	QTimer::singleShot(0, qApp, [retries]() {
		if (findAccountListPage()) {
			rewireUploadSkinAction();
			return;
		}
		if (retries <= 0)
			return;
		QTimer::singleShot(50, qApp,
						   [retries]() { scheduleRewire(retries - 1); });
	});
}

extern "C" {

MMCO_EXPORT int mmco_init(MMCOContext* ctx)
{
	g_ctx = ctx;
	MMCO_LOG(ctx, "SkinManager initializing...");

	g_guard = new QObject();

	/* Re-hook every time the settings dialog opens — the page is
	 * destroyed and rebuilt each open, so the action handle is fresh. */
	QObject::connect(APPLICATION, &Application::globalSettingsAboutToOpen,
					 g_guard, []() { scheduleRewire(); });

	/* The plugin may load *after* the settings dialog is already
	 * visible (rare, but possible if the user opens settings before
	 * mmco_init runs). Cover that by trying once on init. */
	scheduleRewire();

	MMCO_LOG(ctx, "SkinManager initialized.");
	return 0;
}

MMCO_EXPORT void mmco_unload()
{
	if (g_ctx)
		MMCO_LOG(g_ctx, "SkinManager unloading.");

	/* g_guard intentionally not deleted — same rationale as NVIDIAPrime:
	 * tearing down a QObject mid-Application-shutdown can trip Qt's
	 * signal-slot bookkeeping. The OS reclaims memory at process exit. */
	g_guard = nullptr;
	g_ctx = nullptr;
}

} /* extern "C" */
