#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QList>
#include <QString>
#include <QVBoxLayout>

#include "ui/pages/BasePageProvider.h"

class LoaderVersionPage;
class PackProfile;
class PageContainer;
class QLineEdit;

/* Pick a mod loader and a version of it, and write both into the
 * instance.
 *
 * The version page used to carry one toolbar button per loader, each
 * with its own forty-line handler that differed from the next only by a
 * uid and some wording. Five buttons also meant five places to keep in
 * step, and they had already fallen out of step: Forge's was the only
 * one nobody remembered to disable while the instance was running.
 *
 * So: one button, one dialog, one page per row of knownModLoaders().
 * Adding a loader is adding a row to that table - there is nothing to
 * write here for it.
 *
 * The frame follows DownloadContentDialog, which is the launcher's other
 * "pick a thing from one of several sources" window: a PageContainer
 * holding the pages, the dialog owning only the buttons, the shared
 * search box and the writing-back.
 *
 * Writes happen on accept, straight into the PackProfile, with no
 * separate apply step - the same way the version page's other component
 * operations behave. */
class InstallLoaderDialog final : public QDialog, public BasePageProvider
{
	Q_OBJECT

  public:
	/* `initialUid` decides which loader's page opens first. Pass a
	 * component uid when the user asked to change an installed loader's
	 * version; pass nothing to land on the first page. */
	explicit InstallLoaderDialog(PackProfile* profile,
								 const QString& initialUid = QString(),
								 QWidget* parent = nullptr);
	~InstallLoaderDialog() override;

	/* BasePageProvider */
	QList<BasePage*> getPages() override;
	QString dialogTitle() override;

	void accept() override;

  private slots:
	void onPageChanged(BasePage* previous, BasePage* selected);
	void onSelectionChanged();
	void onSearchChanged(const QString& term);
	void onRefreshClicked();

  private:
	void buildPages(const QString& initialUid);

	/* The page on screen, or nullptr before the container has shown one. */
	LoaderVersionPage* currentPage() const;

	/* Enables OK only when the visible page has a version selected. */
	void updateOkButton();

	/* Walks everything the chosen loader is known to break against and
	 * asks the user what to do with each. Returns false if they backed
	 * out, in which case the caller must not write anything. */
	bool settleConflicts(const LoaderVersionPage* page);

	/* Installs the page's selection. Split out of accept() so the
	 * conflict step above can veto before anything is touched. */
	void applySelection(const LoaderVersionPage* page);

	PackProfile* m_profile;
	QList<LoaderVersionPage*> m_pages;
	PageContainer* m_container = nullptr;
	QLineEdit* m_search = nullptr;
	QDialogButtonBox m_buttons;
	QVBoxLayout m_layout;
};
