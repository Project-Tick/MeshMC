#include "InstallLoaderDialog.h"

#include <QAbstractItemView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>

#include "Application.h"
#include "Version.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "minecraft/Component.h"
#include "minecraft/PackProfile.h"
#include "ui/widgets/PageContainer.h"
/* VersionSelectWidget.h only forward-declares the view, and connecting
 * to a QAbstractItemView signal through it needs the derived type
 * complete. */
#include "ui/widgets/VersionListView.h"
#include "ui/widgets/VersionSelectWidget.h"

/* One loader's version list, dressed up as a page in the sidebar.
 *
 * Everything that distinguishes one page from another comes out of a
 * ModLoaderInfo row, so this class has no idea which loaders exist and
 * does not need to be touched when that changes. */
class LoaderVersionPage final : public VersionSelectWidget, public BasePage
{
	Q_OBJECT

  public:
	LoaderVersionPage(const ModLoaderInfo& loader, PackProfile* profile)
		: VersionSelectWidget(nullptr), m_loader(&loader)
	{
		m_minecraftVersion = profile->getComponentVersion("net.minecraft");

		/* A stated floor means the metadata cannot rule this loader out
		 * by itself - its builds name no Minecraft version, so every one
		 * of them would look applicable. Deciding here also spares a
		 * download nobody will read. */
		if (!m_loader->earliestMinecraft.isEmpty() &&
			Version(m_minecraftVersion) <
				Version(m_loader->earliestMinecraft)) {
			m_supported = false;
			setEmptyString(tr("%1 does not run on Minecraft %2. The earliest "
							  "version it supports is %3.")
							   .arg(m_loader->brandName, m_minecraftVersion,
									m_loader->earliestMinecraft));
			return;
		}

		setEmptyString(tr("No %1 builds are published for Minecraft %2.")
						   .arg(m_loader->brandName, m_minecraftVersion));
		setEmptyErrorString(
			tr("The %1 version list could not be downloaded.")
				.arg(m_loader->brandName));

		/* Builds that name a Minecraft version are held to the one this
		 * instance runs. Builds that name none - Fabric Loader and Quilt
		 * Loader, which are game-version independent - are all kept,
		 * which is what "if present" buys over a strict match. */
		setExactIfPresentFilter(BaseVersionList::ParentVersionRole,
								m_minecraftVersion);

		const QString installed = profile->getComponentVersion(m_loader->uid);
		if (!installed.isEmpty()) {
			setCurrentVersion(installed);
		}
	}

	const ModLoaderInfo& loader() const
	{
		return *m_loader;
	}

	/* BasePage */
	QString id() const override
	{
		return m_loader->uid;
	}
	QString displayName() const override
	{
		return m_loader->brandName;
	}
	QIcon icon() const override
	{
		return APPLICATION->getThemedIcon(m_loader->iconName);
	}

	/* Five loaders is five metadata downloads. Fetching them all when
	 * the window opens would stall it on lists the user will never
	 * scroll, so each page waits until it is looked at. */
	void openedImpl() override
	{
		if (!m_supported) {
			/* Nothing to load, but the reason still has to be readable
			 * rather than an unexplained blank list. */
			showEmptyMessage();
			return;
		}
		if (m_listReady) {
			return;
		}

		const auto versions = APPLICATION->metadataIndex()->get(m_loader->uid);
		if (!versions) {
			showEmptyMessage();
			return;
		}

		initialize(versions.get());
		m_listReady = true;
	}

	/* Re-reads the list from the metadata server. Unlike the reference
	 * launcher there is no "force" flag to thread through the version
	 * list classes: Meta::BaseEntity::load() already starts a fresh
	 * download on every call that does not find one in flight, which is
	 * exactly why VersionSelectDialog's own Refresh button is a bare
	 * loadList() too.
	 *
	 * Guarded, because a page whose uid is missing from the metadata
	 * index never got a list to reload. */
	void reload()
	{
		if (m_listReady) {
			loadList();
		}
	}

	/* Narrows the list to build strings containing `term`; an empty term
	 * puts everything back. This rides the filter machinery the widget
	 * already has rather than adding a second, parallel one. Matching is
	 * case-sensitive, which never comes up: loader build numbers are
	 * digits and dots. */
	void setSearchTerm(const QString& term)
	{
		if (!m_supported) {
			return;
		}
		setFuzzyFilter(BaseVersionList::VersionRole, term);
	}

  private:
	const ModLoaderInfo* m_loader;
	QString m_minecraftVersion;
	bool m_supported = true;
	bool m_listReady = false;
};

namespace
{
	enum class ConflictChoice { Keep, Disable, Uninstall, Abort };

	/* One question about one already-installed loader. Which answers are
	 * offered depends on what the component allows: a dependency-only or
	 * unremovable component cannot be turned off or deleted, and showing
	 * a button that would do nothing is worse than not showing it. */
	ConflictChoice askAboutConflict(QWidget* parent, const QString& incoming,
									const QString& existing, bool canDisable,
									bool canUninstall)
	{
		QMessageBox box(parent);
		box.setIcon(QMessageBox::Warning);
		box.setWindowTitle(QObject::tr("Two mod loaders"));
		box.setText(
			QObject::tr(
				"%1 is already installed and switched on here. It and %2 "
				"hook into the same parts of the game, and an instance "
				"carrying both will usually fail to start.\n\nWhat should "
				"happen to %1?")
				.arg(existing, incoming));
		box.setTextInteractionFlags(Qt::TextBrowserInteraction);

		QAbstractButton* keep = box.addButton(QObject::tr("Leave it alone"),
											  QMessageBox::AcceptRole);
		QAbstractButton* disable =
			canDisable ? box.addButton(QObject::tr("Turn it off"),
									   QMessageBox::ActionRole)
					   : nullptr;
		QAbstractButton* uninstall =
			canUninstall ? box.addButton(QObject::tr("Remove it"),
										 QMessageBox::DestructiveRole)
						 : nullptr;
		box.addButton(QMessageBox::Cancel);

		box.exec();

		QAbstractButton* answer = box.clickedButton();
		if (answer == keep) {
			return ConflictChoice::Keep;
		}
		if (disable != nullptr && answer == disable) {
			return ConflictChoice::Disable;
		}
		if (uninstall != nullptr && answer == uninstall) {
			return ConflictChoice::Uninstall;
		}
		/* Cancel, or the window was closed without an answer. */
		return ConflictChoice::Abort;
	}

	/* "Forge 47.2.0" - what the conflict question calls a loader. */
	QString describe(const QString& brand, const QString& version)
	{
		return version.isEmpty() ? brand
								 : QStringLiteral("%1 %2").arg(brand, version);
	}
} // namespace

InstallLoaderDialog::InstallLoaderDialog(PackProfile* profile,
										 const QString& initialUid,
										 QWidget* parent)
	: QDialog(parent), m_profile(profile),
	  m_buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
	  m_layout(this)
{
	setObjectName(QStringLiteral("InstallLoaderDialog"));
	setWindowTitle(dialogTitle());
	setWindowModality(Qt::WindowModal);
	resize(560, 420);

	buildPages(initialUid);

	updateOkButton();
}

InstallLoaderDialog::~InstallLoaderDialog() {}

void InstallLoaderDialog::buildPages(const QString& initialUid)
{
	/* One page per row of the table, in the table's order, so the loader
	 * list on screen and the loader list in the code cannot drift. */
	for (const ModLoaderInfo& loader : knownModLoaders()) {
		m_pages.append(new LoaderVersionPage(loader, m_profile));
	}

	m_layout.setContentsMargins(0, 0, 0, 0);

	/* Handing the uid straight to the container means the page it names
	 * is the one built and shown first; an unknown or empty uid falls
	 * back to the first page on its own. */
	m_container = new PageContainer(this, initialUid, this);
	m_container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	m_container->layout()->setContentsMargins(0, 0, 0, 0);
	m_layout.addWidget(m_container);

	/* The search box belongs to the dialog, not to the version widget,
	 * so the other pickers built on that widget - the Java version
	 * wizard, the vanilla version list - are left exactly as they were.
	 *
	 * addButtons() is the container's only injection point and drops
	 * what it is given below the page area, so the box and the buttons
	 * go in together, stacked. That puts the filter directly under the
	 * list it filters, which is where the version page keeps its own. */
	m_search = new QLineEdit(this);
	m_search->setPlaceholderText(tr("Filter builds"));
	m_search->setClearButtonEnabled(true);

	m_buttons.setContentsMargins(0, 0, 6, 6);
	QPushButton* refresh =
		m_buttons.addButton(tr("&Refresh"), QDialogButtonBox::ResetRole);
	refresh->setToolTip(tr("Download this loader's version list again."));
	refresh->setAutoDefault(false);

	QPushButton* ok = m_buttons.button(QDialogButtonBox::Ok);
	ok->setText(tr("Install"));
	ok->setDefault(true);

	auto* bottom = new QVBoxLayout();
	bottom->setContentsMargins(6, 0, 0, 0);
	bottom->addWidget(m_search);
	bottom->addWidget(&m_buttons);
	m_container->addButtons(bottom);

	for (auto* page : m_pages) {
		connect(page, &VersionSelectWidget::selectedVersionChanged, this,
				&InstallLoaderDialog::onSelectionChanged);
		/* Double click is "this one, install it" - the shortcut the
		 * launcher's other version pickers offer. */
		connect(page->view(), &QAbstractItemView::doubleClicked, this,
				&InstallLoaderDialog::accept);
	}

	connect(m_container, &PageContainer::selectedPageChanged, this,
			&InstallLoaderDialog::onPageChanged);
	connect(m_search, &QLineEdit::textChanged, this,
			&InstallLoaderDialog::onSearchChanged);
	connect(refresh, &QPushButton::clicked, this,
			&InstallLoaderDialog::onRefreshClicked);
	connect(&m_buttons, &QDialogButtonBox::accepted, this,
			&InstallLoaderDialog::accept);
	connect(&m_buttons, &QDialogButtonBox::rejected, this,
			&InstallLoaderDialog::reject);

	m_search->setFocus();
}

QList<BasePage*> InstallLoaderDialog::getPages()
{
	QList<BasePage*> pages;
	pages.reserve(m_pages.size());
	for (auto* page : m_pages) {
		pages.append(page);
	}
	return pages;
}

QString InstallLoaderDialog::dialogTitle()
{
	return tr("Install Loader");
}

LoaderVersionPage* InstallLoaderDialog::currentPage() const
{
	if (m_container == nullptr) {
		return nullptr;
	}
	return dynamic_cast<LoaderVersionPage*>(m_container->selectedPage());
}

void InstallLoaderDialog::onPageChanged(BasePage* previous, BasePage* selected)
{
	/* The term is dropped on the way between pages, which is the
	 * opposite of what DownloadContentDialog does with its providers -
	 * deliberately. There the two pages are two catalogues of the same
	 * thing and a search means the same on both. Here a Forge build
	 * number is meaningless on the Fabric page, so carrying it across
	 * would only present an empty list and a reason that is not
	 * obvious. */
	if (auto* previousPage = dynamic_cast<LoaderVersionPage*>(previous)) {
		previousPage->setSearchTerm(QString());
	}
	if (auto* selectedPage = dynamic_cast<LoaderVersionPage*>(selected)) {
		selectedPage->setSearchTerm(QString());
	}
	if (m_search != nullptr) {
		QSignalBlocker blocked(m_search);
		m_search->clear();
	}

	updateOkButton();
}

void InstallLoaderDialog::onSelectionChanged()
{
	/* Lists load in the background, so a page the user has already left
	 * can still settle on a version and report it. Only what is on
	 * screen decides whether Install is available. */
	if (sender() == currentPage()) {
		updateOkButton();
	}
}

void InstallLoaderDialog::onSearchChanged(const QString& term)
{
	if (auto* page = currentPage()) {
		page->setSearchTerm(term);
	}
	/* Filtering can hide the row that was selected, and a hidden row is
	 * still what would be installed. */
	updateOkButton();
}

void InstallLoaderDialog::onRefreshClicked()
{
	if (auto* page = currentPage()) {
		page->reload();
	}
}

void InstallLoaderDialog::updateOkButton()
{
	auto* page = currentPage();
	m_buttons.button(QDialogButtonBox::Ok)
		->setEnabled(page != nullptr && page->selectedVersion() != nullptr);
}

bool InstallLoaderDialog::settleConflicts(const LoaderVersionPage* page)
{
	const ModLoaderInfo& loader = page->loader();
	const QString incoming =
		describe(loader.brandName, page->selectedVersion()->descriptor());

	/* Uids, looked up one at a time as their turn comes. Answering
	 * "remove it" for the first conflict drops that component and
	 * re-resolves the profile; PackProfile::getComponent() hands out raw
	 * pointers into the list it owns, so a batch of them collected
	 * beforehand would be worth nothing by the second iteration. */
	for (const QString& conflictUid : loader.conflictsWith) {
		Component* conflict = m_profile->getComponent(conflictUid);
		if (conflict == nullptr || !conflict->isEnabled()) {
			/* Absent, or switched off and therefore already harmless. */
			continue;
		}
		if (conflict->isCustom()) {
			/* A customised component is the user's own JSON. Offering to
			 * delete it from here would throw away edits that the
			 * version page is the right place to manage. */
			continue;
		}

		const QString existing =
			describe(conflict->getName(), conflict->getVersion());
		const ConflictChoice choice = askAboutConflict(
			this, incoming, existing, conflict->canBeDisabled(),
			conflict->isRemovable());

		switch (choice) {
			case ConflictChoice::Keep:
				break;
			case ConflictChoice::Disable:
				conflict->setEnabled(false);
				m_profile->resolve(Net::Mode::Online);
				break;
			case ConflictChoice::Uninstall:
				/* `conflict` dangles from here on; conflictUid is the
				 * copy that outlives it. */
				m_profile->remove(conflictUid);
				m_profile->resolve(Net::Mode::Online);
				break;
			case ConflictChoice::Abort:
				return false;
		}
	}

	return true;
}

void InstallLoaderDialog::applySelection(const LoaderVersionPage* page)
{
	const QString uid = page->loader().uid;

	m_profile->setComponentVersion(uid, page->selectedVersion()->descriptor());

	/* Reinstalling a loader that is present but switched off has to
	 * switch it back on, or the install looks like it did nothing.
	 * setComponentVersion() creates the component when it was missing,
	 * so this has to come after it. */
	if (Component* component = m_profile->getComponent(uid)) {
		component->setEnabled(true);
	}

	m_profile->resolve(Net::Mode::Online);
}

void InstallLoaderDialog::accept()
{
	auto* page = currentPage();
	if (page == nullptr || !page->selectedVersion()) {
		/* Reachable by double clicking empty space in the list. */
		return;
	}

	if (!settleConflicts(page)) {
		/* The user backed out of a conflict question. Stay open on the
		 * version they had picked rather than closing on a half-applied
		 * decision. */
		return;
	}

	applySelection(page);
	QDialog::accept();
}

#include "InstallLoaderDialog.moc"
