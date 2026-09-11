/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SkinManageDialog.h"
#include "ui_SkinManageDialog.h"

#include <QAction>
#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMimeDatabase>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QRect>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QUrl>

#include "Application.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "QObjectPtr.h"
#include "settings/SettingsObject.h"

/* AccountTask, not just MinecraftAccount: refresh() returns a
 * shared_qobject_ptr<AccountTask>, and converting that to a Task::Ptr needs
 * AccountTask to be a complete type here. MinecraftAccount.h only
 * forward-declares it, which is enough to *name* the return type but not to
 * prove the derivation to std::shared_ptr. */
#include "minecraft/auth/AccountTask.h"
#include "minecraft/services/CapeChange.h"
#include "minecraft/services/SkinDelete.h"
#include "minecraft/services/SkinUpload.h"
#include "minecraft/skins/ProfileSkinImport.h"

#include "net/Download.h"
#include "net/NetJob.h"
#include "tasks/SequentialTask.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/instanceview/InstanceDelegate.h"

namespace
{
	/* Cape textures are cached next to the skins, in their own subdirectory
	 * so they never turn up in the skin list. */
	const char* const kCapeCacheDirName = "capes";

	/* Thumbnail geometry. The cape strip is 10x16 at (1, 1) of the cape
	 * texture; an elytra wing is 12x20 at (34, 2). Both are format facts, not
	 * choices. */
	const QRect kCapeFrontRegion(1, 1, 10, 16);
	const QRect kElytraWingRegion(34, 2, 12, 20);
	const QSize kCapeThumbnailSize(80, 128);
	const QSize kElytraThumbnailSize(84, 128);

	/* Render the little cape picture shown in the combo and under it.
	 *
	 * The cape form is stretched to fill its box -- a 10x16 strip is far
	 * narrower than the space available and letterboxing it reads worse than
	 * the distortion does. The elytra form keeps its aspect ratio, because
	 * two wings side by side already fill the width. */
	QPixmap renderCapeThumbnail(const QImage& cape, bool asElytra)
	{
		if (cape.isNull()) {
			return QPixmap();
		}

		if (!asElytra) {
			return QPixmap::fromImage(
				cape.copy(kCapeFrontRegion)
					.scaled(kCapeThumbnailSize, Qt::IgnoreAspectRatio,
							Qt::FastTransformation));
		}

		const QImage wing = cape.copy(kElytraWingRegion);
		const QImage mirrored = wing.mirrored(true, false);

		/* One pixel of gap between the wings, and headroom above so the pair
		 * sits where it would on the model rather than flush to the top. */
		QImage pair(wing.width() * 2 + 1, wing.height() + 14,
					QImage::Format_ARGB32);
		pair.fill(Qt::transparent);
		{
			QPainter painter(&pair);
			painter.drawImage(0, 7, wing);
			painter.drawImage(wing.width() + 1, 7, mirrored);
		}
		return QPixmap::fromImage(pair.scaled(kElytraThumbnailSize,
											  Qt::KeepAspectRatio,
											  Qt::FastTransformation));
	}
} // namespace

SkinManageDialog::SkinManageDialog(QWidget* parent,
								   MinecraftAccountPtr account)
	: QDialog(parent), m_account(account), m_ui(new Ui::SkinManageDialog),
	  m_library(this, APPLICATION->settings()->get("SkinsDir").toString(),
				account)
{
	m_ui->setupUi(this);

	/* Window-modal rather than application-modal: uploading a skin should
	 * not stop an instance from being launched in another window. */
	setWindowModality(Qt::WindowModal);

	if (skinrender::SkinPreviewSurface::isAvailable()) {
		/* The chequerboard is derived from the dialog's own base colour, so
		 * the preview belongs to whatever theme is active. */
		m_preview = new skinrender::SkinPreviewSurface(
			this, palette().color(QPalette::Normal, QPalette::Base));
	} else {
		/* No GL: fall back to the flat front-and-back sprite. Same
		 * information, no rotation. */
		m_flatPreview = new QLabel(this);
		m_flatPreview->setSizePolicy(QSizePolicy::Expanding,
									 QSizePolicy::Expanding);
		m_flatPreview->setAlignment(Qt::AlignCenter);
	}

	setUpSkinList();
	loadCapes();

	/* Open on the skin the account is actually wearing. An invalid index
	 * (nothing matched) simply leaves the list unselected. */
	m_ui->skinList->setCurrentIndex(
		m_library.index(m_library.indexOfAccountSkin()));

	m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

	/* Added last so the layout's stretch factors apply to a preview that is
	 * already sized, and so nothing above can call into a half-built
	 * widget. */
	if (m_preview) {
		m_previewContainer = QWidget::createWindowContainer(m_preview, this);
		m_ui->previewLayout->insertWidget(0, m_previewContainer);
	} else {
		m_ui->previewLayout->insertWidget(0, m_flatPreview);
	}
}

SkinManageDialog::~SkinManageDialog()
{
	/* The container owns the preview window and destroys it properly (a GL
	 * context has to be current for that). Doing it here, rather than letting
	 * QWidget teardown get to it, means the platform window is still alive
	 * when the preview releases its textures and shaders. */
	if (m_previewContainer) {
		delete m_previewContainer;
		m_previewContainer = nullptr;
	}
	m_preview = nullptr;

	delete m_ui;
}

void SkinManageDialog::setUpSkinList()
{
	QListView* view = m_ui->skinList;

	/* A grid of thumbnails that reflows with the dialog, one selection at a
	 * time. Uniform item sizes let the view skip measuring every row, which
	 * matters once a library has a few hundred skins in it. */
	view->setViewMode(QListView::IconMode);
	view->setFlow(QListView::LeftToRight);
	view->setIconSize(QSize(48, 48));
	view->setMovement(QListView::Static);
	view->setResizeMode(QListView::Adjust);
	view->setSelectionMode(QAbstractItemView::SingleSelection);
	view->setSpacing(5);
	view->setWordWrap(false);
	view->setWrapping(true);
	view->setUniformItemSizes(true);
	view->setTextElideMode(Qt::ElideRight);
	view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setItemDelegate(new ListViewDelegate(this));

	/* Drops only, and always a copy: a PNG dragged in from a file manager is
	 * imported into the library and left where it was. */
	view->setAcceptDrops(true);
	view->setDropIndicatorShown(true);
	view->viewport()->setAcceptDrops(true);
	view->setDragDropMode(QAbstractItemView::DropOnly);
	view->setDefaultDropAction(Qt::CopyAction);

	/* Del and F2 are handled here rather than through the actions' shortcuts:
	 * the actions only ever live in the context menu, and a shortcut on a
	 * menu that is not open would fire from anywhere in the dialog --
	 * including while the user is typing a URL. */
	view->installEventFilter(this);

	view->setModel(&m_library);

	connect(view, &QAbstractItemView::doubleClicked, this,
			&SkinManageDialog::onSkinActivated);
	connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
			this, &SkinManageDialog::onSelectionChanged);
	connect(view, &QListView::customContextMenuRequested, this,
			&SkinManageDialog::showContextMenu);
	connect(m_ui->elytraCheck, &QCheckBox::toggled, this,
			&SkinManageDialog::onElytraToggled);
}

void SkinManageDialog::loadCapes()
{
	if (!m_account || !m_account->accountData()) {
		return;
	}
	const MinecraftProfile& profile =
		m_account->accountData()->minecraftProfile;

	/* Row 0, and the only entry that is not a cape. Its data is a null
	 * QVariant, which is what CapeChange reads as "take the cape off". */
	m_ui->capeCombo->addItem(tr("No Cape"), QVariant());
	if (profile.currentCape.isEmpty()) {
		m_ui->capeCombo->setCurrentIndex(0);
	}

	const QString capeCacheDir =
		FS::PathCombine(m_library.directory(), QLatin1String(kCapeCacheDirName));
	FS::ensureFolderPathExists(capeCacheDir);

	NetJob::Ptr job(new NetJob(tr("Download capes"), APPLICATION->network()));
	bool needsDownload = false;

	for (const Cape& cape : profile.capes) {
		const QString path =
			FS::PathCombine(capeCacheDir, cape.id + QStringLiteral(".png"));

		/* The account sometimes carries the cape bytes inline. Then there is
		 * nothing to fetch -- but still write them out, so the next run does
		 * not depend on the account being refreshed first. */
		if (!cape.data.isEmpty()) {
			QImage inline_;
			if (inline_.loadFromData(cape.data, "PNG")) {
				m_capes.insert(cape.id, inline_);
				inline_.save(path, "PNG");
				continue;
			}
		}
		if (QFileInfo::exists(path)) {
			continue;
		}
		if (!cape.url.isEmpty()) {
			needsDownload = true;
			job->addNetAction(Net::Download::makeFile(QUrl(cape.url), path));
		}
	}

	if (needsDownload) {
		ProgressDialog progress(this);
		progress.execWithTask(job.get());
	}

	int row = 0;
	for (const Cape& cape : profile.capes) {
		++row;

		if (!m_capes.contains(cape.id)) {
			const QString path = FS::PathCombine(
				capeCacheDir, cape.id + QStringLiteral(".png"));
			QImage cached;
			if (QFileInfo::exists(path) && cached.load(path)) {
				m_capes.insert(cape.id, cached);
			}
		}

		/* A cape whose texture could not be had is still listed -- it can be
		 * equipped, it just has no picture. Dropping it would make the cape
		 * unreachable because of a failed download. */
		const QPixmap thumbnail = renderCapeThumbnail(
			m_capes.value(cape.id), m_ui->elytraCheck->isChecked());
		if (thumbnail.isNull()) {
			m_ui->capeCombo->addItem(cape.alias, cape.id);
		} else {
			m_ui->capeCombo->addItem(QIcon(thumbnail), cape.alias, cape.id);
		}

		m_capeRows.insert(cape.id, row);
	}
}

QString SkinManageDialog::currentCapeId() const
{
	return m_ui->capeCombo->currentData().toString();
}

const SkinEntry* SkinManageDialog::previewSkin() const
{
	const SkinEntry* entry = m_library.entry(m_selectedName);
	return (entry && entry->isUsable()) ? entry : nullptr;
}

QImage SkinManageDialog::previewCape(const QString& capeId) const
{
	return m_capes.value(capeId);
}

SkinEntry* SkinManageDialog::selectedSkin()
{
	SkinEntry* entry = m_library.entry(m_selectedName);
	return (entry && entry->isUsable()) ? entry : nullptr;
}

void SkinManageDialog::refreshPreview()
{
	SkinEntry* skin = selectedSkin();
	if (!skin) {
		return;
	}

	if (m_preview) {
		m_preview->showSkin(skin);
	} else if (m_flatPreview) {
		m_flatPreview->setPixmap(
			QPixmap::fromImage(skin->thumbnail())
				.scaled(m_flatPreview->size(), Qt::KeepAspectRatio,
						Qt::FastTransformation));
	}
}

void SkinManageDialog::refreshCapePreview()
{
	const QImage cape = m_capes.value(currentCapeId());
	if (cape.isNull()) {
		m_ui->capePreview->clear();
		return;
	}

	/* A third of the dialog: big enough to judge the cape by, small enough
	 * that it never pushes the controls around. */
	const QSize target = size() * (1.0 / 3.0);
	m_ui->capePreview->setPixmap(
		renderCapeThumbnail(cape, m_ui->elytraCheck->isChecked())
			.scaled(target, Qt::KeepAspectRatio, Qt::FastTransformation));
}

void SkinManageDialog::onSelectionChanged(const QItemSelection& selected,
										  const QItemSelection& deselected)
{
	Q_UNUSED(deselected)

	if (selected.isEmpty()) {
		return;
	}
	const QModelIndexList indexes = selected.first().indexes();
	if (indexes.isEmpty()) {
		return;
	}
	const QString name = indexes.first().data(Qt::UserRole).toString();
	if (name.isEmpty()) {
		return;
	}

	m_selectedName = name;
	SkinEntry* skin = selectedSkin();
	if (!skin) {
		return;
	}

	refreshPreview();

	/* The controls follow the selection, not the other way round: each entry
	 * remembers its own cape and arm width. value() gives 0 -- "No Cape" --
	 * for an entry whose cape the account no longer owns. */
	m_ui->capeCombo->setCurrentIndex(m_capeRows.value(skin->capeId()));
	m_ui->classicRadio->setChecked(skin->arms() == SkinEntry::Arms::Classic);
	m_ui->slimRadio->setChecked(skin->arms() == SkinEntry::Arms::Slim);
}

void SkinManageDialog::onSkinActivated(const QModelIndex& index)
{
	/* Double-click means "wear this one" -- take the entry under the cursor
	 * rather than the selection, which has not necessarily caught up. */
	m_selectedName = index.data(Qt::UserRole).toString();
	accept();
}

void SkinManageDialog::onElytraToggled(bool checked)
{
	if (m_preview) {
		m_preview->setElytraVisible(checked);
	}
	/* Same code path as picking a cape: it re-renders the thumbnail and
	 * pushes the texture at the preview, both of which depend on this flag. */
	on_capeCombo_currentIndexChanged(m_ui->capeCombo->currentIndex());
}

void SkinManageDialog::on_capeCombo_currentIndexChanged(int index)
{
	Q_UNUSED(index)

	refreshCapePreview();

	const QString capeId = currentCapeId();
	if (m_preview) {
		m_preview->showCape(m_capes.value(capeId));
	}

	/* Picking a cape edits the selected entry, so the choice survives closing
	 * the dialog even if nothing is uploaded. */
	if (SkinEntry* skin = selectedSkin()) {
		skin->setCapeId(capeId);
		refreshPreview();
	}
}

void SkinManageDialog::on_classicRadio_toggled(bool checked)
{
	/* Only this radio is connected: the two share a group, so slim toggling
	 * emits this as well, with checked == false. */
	if (SkinEntry* skin = selectedSkin()) {
		skin->setArms(checked ? SkinEntry::Arms::Classic
							  : SkinEntry::Arms::Slim);
		refreshPreview();
	}
}

void SkinManageDialog::on_openFolderButton_clicked()
{
	DesktopServices::openDirectory(m_library.directory(), true);
}

void SkinManageDialog::on_importFileButton_clicked()
{
	const QString filter =
		QMimeDatabase().mimeTypeForName(QStringLiteral("image/png"))
			.filterString();
	const QString chosen = QFileDialog::getOpenFileName(
		this, tr("Select Skin Texture"), QString(), filter);
	if (chosen.isNull()) {
		return;
	}

	/* The library copies the file in; the directory watcher is what puts it
	 * in the list. */
	const QString problem = m_library.importFile(chosen);
	if (!problem.isEmpty()) {
		CustomMessageBox::selectable(this,
									 tr("Selected file is not a valid skin"),
									 problem, QMessageBox::Critical)
			->show();
	}
}

void SkinManageDialog::on_importUrlButton_clicked()
{
	const QUrl url(m_ui->sourceEdit->text());
	if (!url.isValid()) {
		CustomMessageBox::selectable(this, tr("Invalid url"), tr("Invalid url"),
									 QMessageBox::Critical)
			->show();
		return;
	}

	const QString path =
		FS::PathCombine(m_library.directory(), url.fileName());

	NetJob::Ptr job(new NetJob(tr("Download skin"), APPLICATION->network()));
	job->addNetAction(Net::Download::makeFile(url, path));
	ProgressDialog progress(this);
	progress.execWithTask(job.get());

	/* Judge it by what actually landed on disk rather than by the job's exit
	 * code: a server can answer 200 with an HTML error page. */
	if (!SkinEntry(path).isUsable()) {
		CustomMessageBox::selectable(
			this, tr("URL is not a valid skin"),
			QFileInfo::exists(path)
				? tr("Skin images must be 64x64 or 64x32 pixel PNG files.")
				: tr("Unable to download the skin: '%1'.")
					  .arg(m_ui->sourceEdit->text()),
			QMessageBox::Critical)
			->show();
		QFile::remove(path);
		return;
	}

	m_ui->sourceEdit->clear();
	/* A URL ending in a bare name gives a file the library would not pick up,
	 * since it only tracks ".png". */
	if (QFileInfo(path).suffix().isEmpty()) {
		QFile::rename(path, path + QStringLiteral(".png"));
	}
}

void SkinManageDialog::on_importUserButton_clicked()
{
	const QString username = m_ui->sourceEdit->text();
	if (username.isEmpty()) {
		return;
	}

	const QString path = FS::PathCombine(
		m_library.directory(), username + QStringLiteral(".png"));

	shared_qobject_ptr<ProfileSkinImport> import(
		new ProfileSkinImport(nullptr, username, path));

	ProgressDialog progress(this);
	progress.execWithTask(import.get());

	SkinEntry imported(path);
	if (!imported.isUsable()) {
		/* Read the reason off the task rather than catching failed() into a
		 * local: shared_qobject_ptr destroys through deleteLater(), so the
		 * task outlives this scope and a connection capturing a local by
		 * reference would be left pointing at dead stack. */
		QString failure = import->failReason();
		if (failure.isEmpty()) {
			failure = tr("the skin is invalid");
		}
		CustomMessageBox::selectable(
			this, tr("Username not found"),
			tr("Unable to find the skin for '%1'\n because: %2.")
				.arg(username, failure),
			QMessageBox::Critical)
			->show();
		QFile::remove(path);
		return;
	}

	m_ui->sourceEdit->clear();

	/* The profile knows things the PNG does not, so they are carried over
	 * before the entry is folded into the library. */
	imported.setArms(import->arms());
	imported.setTextureUrl(import->textureUrl());
	if (m_capes.contains(import->capeId())) {
		/* Only if this account owns the same cape -- the session server
		 * publishes no usable cape ids, so this almost never matches, and
		 * equipping a cape we do not own would fail at upload time. */
		imported.setCapeId(import->capeId());
	}
	m_library.mergeEntry(imported);
}

void SkinManageDialog::accept()
{
	SkinEntry* skin = m_library.entry(m_selectedName);
	if (!skin) {
		/* Nothing selected -- OK cannot mean anything, so treat it as
		 * cancelling rather than uploading whatever happens to be first. */
		reject();
		return;
	}

	const QString path = skin->path();
	QFile file(path);
	if (!QFile::exists(path) || !file.open(QIODevice::ReadOnly)) {
		CustomMessageBox::selectable(this, tr("Skin Upload"),
									 tr("Skin file does not exist!"),
									 QMessageBox::Warning)
			->exec();
		reject();
		return;
	}
	const QByteArray texture = file.readAll();
	file.close();

	/* Sequential, not concurrent: the cape change and the profile refresh
	 * only make sense once the upload went through, and a refresh that
	 * overlaps the upload can report the old skin back. */
	shared_qobject_ptr<SequentialTask> job(
		new SequentialTask(nullptr, tr("Change skin")));

	job->addTask(Task::Ptr(new SkinUpload(
		nullptr, m_account->accessToken(), texture,
		skin->arms() == SkinEntry::Arms::Slim ? SkinUpload::ALEX
											  : SkinUpload::STEVE)));

	const QString capeId = skin->capeId();
	if (capeId != m_account->accountData()->minecraftProfile.currentCape) {
		job->addTask(Task::Ptr(
			new CapeChange(nullptr, m_account->accessToken(), capeId)));
	}

	/* Refresh last: it is what tells us the URL the skin ended up at, which
	 * is how this entry is recognised as the worn one from now on. */
	job->addTask(m_account->refresh());

	ProgressDialog progress(this);
	if (progress.execWithTask(job.get()) != QDialog::Accepted) {
		CustomMessageBox::selectable(this, tr("Skin Upload"),
									 tr("Failed to upload skin!"),
									 QMessageBox::Warning)
			->exec();
		reject();
		return;
	}

	skin->setTextureUrl(
		m_account->accountData()->minecraftProfile.skin.url);
	m_library.save();

	QDialog::accept();
}

void SkinManageDialog::on_resetSkinButton_clicked()
{
	shared_qobject_ptr<SequentialTask> job(
		new SequentialTask(nullptr, tr("Reset skin")));
	job->addTask(
		Task::Ptr(new SkinDelete(nullptr, m_account->accessToken())));
	job->addTask(m_account->refresh());

	ProgressDialog progress(this);
	if (progress.execWithTask(job.get()) != QDialog::Accepted) {
		CustomMessageBox::selectable(this, tr("Skin Delete"),
									 tr("Failed to delete current skin!"),
									 QMessageBox::Warning)
			->exec();
		reject();
		return;
	}
	QDialog::accept();
}

void SkinManageDialog::showContextMenu(const QPoint& position)
{
	QMenu menu(tr("Context menu"), this);
	menu.addAction(m_ui->actionRenameSkin);
	menu.addAction(m_ui->actionDeleteSkin);
	menu.exec(m_ui->skinList->mapToGlobal(position));
}

bool SkinManageDialog::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == m_ui->skinList && event->type() == QEvent::KeyPress) {
		switch (static_cast<QKeyEvent*>(event)->key()) {
			case Qt::Key_Delete:
				on_actionDeleteSkin_triggered();
				return true;
			case Qt::Key_F2:
				on_actionRenameSkin_triggered();
				return true;
			default:
				break;
		}
	}
	return QDialog::eventFilter(watched, event);
}

void SkinManageDialog::on_actionRenameSkin_triggered()
{
	if (m_selectedName.isEmpty()) {
		return;
	}
	/* Renaming is editing the item in place; the model turns that into a file
	 * rename. */
	m_ui->skinList->edit(m_ui->skinList->currentIndex());
}

void SkinManageDialog::on_actionDeleteSkin_triggered()
{
	if (m_selectedName.isEmpty()) {
		return;
	}

	/* Deleting the skin the account is wearing would leave the library unable
	 * to say what is currently worn, and the file is the only copy. */
	if (m_library.indexOfName(m_selectedName) ==
		m_library.indexOfAccountSkin()) {
		CustomMessageBox::selectable(this, tr("Delete error"),
									 tr("Can not delete skin that is in use."),
									 QMessageBox::Warning)
			->exec();
		return;
	}

	const SkinEntry* skin = m_library.entry(m_selectedName);
	if (!skin) {
		return;
	}
	/* Copied out before anything can invalidate the entry. */
	const QString name = skin->name();

	const int answer =
		CustomMessageBox::selectable(this, tr("Confirm Deletion"),
									 tr("You are about to delete \"%1\".\n"
										"Are you sure?")
										 .arg(name),
									 QMessageBox::Warning,
									 QMessageBox::Yes | QMessageBox::No,
									 QMessageBox::No)
			->exec();
	if (answer != QMessageBox::Yes) {
		return;
	}

	/* Trash first so it stays recoverable; fall back to an outright delete
	 * where there is no trash to put it in. */
	if (!m_library.remove(m_selectedName, true)) {
		m_library.remove(m_selectedName, false);
	}
}

void SkinManageDialog::resizeEvent(QResizeEvent* event)
{
	QDialog::resizeEvent(event);

	/* Both previews are sized in terms of the dialog, so both have to be
	 * redrawn when it changes. */
	refreshCapePreview();
	if (m_flatPreview) {
		refreshPreview();
	}
}
