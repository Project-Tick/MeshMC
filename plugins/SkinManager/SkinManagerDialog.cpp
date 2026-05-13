/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 *  SkinManagerDialog implementation.  See the header comment for the
 *  high-level shape of the dialog.
 */

#include "SkinManagerDialog.h"
#include "SkinViewerWidget.h"
#include "ui_SkinManagerDialog.h"

#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/AccountData.h"
#include "minecraft/services/SkinUpload.h"
#include "minecraft/services/SkinDelete.h"
#include "minecraft/services/CapeChange.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "tasks/SequentialTask.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QPainter>
#include <QPushButton>
#include <QStandardPaths>

using SkinManagerNS::ModelVariant;
using SkinManagerNS::SkinViewerWidget;

namespace
{

	/* Same heuristic as the global page used earlier — kept self-contained
	 * so the two files don't share state. */
	ModelVariant detectVariant(const QImage& image)
	{
		if (image.isNull())
			return ModelVariant::Classic;
		QImage img =
			image.scaled(64, 64, Qt::IgnoreAspectRatio, Qt::FastTransformation);
		img = img.convertToFormat(QImage::Format_ARGB32);
		const QRgb px = img.pixel(54, 20);
		return (qAlpha(px) > 0) ? ModelVariant::Classic : ModelVariant::Slim;
	}

	QImage normaliseSkin(const QImage& src)
	{
		if (src.isNull())
			return src;
		if (src.width() == 64 && src.height() == 64)
			return src.convertToFormat(QImage::Format_RGBA8888);

		/* Legacy 64×32 skin → mirror right limbs onto the bottom half. */
		QImage upgraded(64, 64, QImage::Format_RGBA8888);
		upgraded.fill(Qt::transparent);
		const QImage base =
			src.scaled(64, 32, Qt::IgnoreAspectRatio, Qt::FastTransformation)
				.convertToFormat(QImage::Format_RGBA8888);
		for (int y = 0; y < 32; ++y) {
			memcpy(upgraded.scanLine(y), base.constScanLine(y),
				   base.bytesPerLine());
		}
		for (int y = 0; y < 16; ++y) {
			for (int x = 0; x < 16; ++x) {
				upgraded.setPixelColor(32 + x, 48 + y,
									   base.pixelColor(40 + (15 - x), 16 + y));
				upgraded.setPixelColor(16 + x, 48 + y,
									   base.pixelColor(0 + (15 - x), 16 + y));
			}
		}
		return upgraded;
	}

} // namespace

SkinManagerDialog::SkinManagerDialog(MinecraftAccountPtr account,
									 QWidget* parent)
	: QDialog(parent), ui(new Ui::SkinManagerDialog), m_account(account)
{
	ui->setupUi(this);
	setWindowTitle(
		tr("Skin Upload — %1")
			.arg(account ? account->profileName() : tr("(no account)")));

	/* Replace the placeholder viewerHost with a real OpenGL viewer. */
	m_viewer = new SkinViewerWidget(ui->viewerHost);
	ui->viewerHostLayout->addWidget(m_viewer);

	QObject::connect(m_viewer, &SkinViewerWidget::skinFileDropped, this,
					 &SkinManagerDialog::onSkinFileDropped);
	QObject::connect(ui->btnBrowse, &QPushButton::clicked, this,
					 &SkinManagerDialog::onBrowseClicked);
	QObject::connect(ui->btnReset, &QPushButton::clicked, this,
					 &SkinManagerDialog::onResetClicked);
	QObject::connect(ui->rdoClassic, &QRadioButton::toggled, this,
					 &SkinManagerDialog::onVariantToggled);
	QObject::connect(ui->rdoSlim, &QRadioButton::toggled, this,
					 &SkinManagerDialog::onVariantToggled);
	QObject::connect(ui->chkOverlay, &QCheckBox::toggled, this,
					 &SkinManagerDialog::onOverlayToggled);
	QObject::connect(ui->chkAutoRotate, &QCheckBox::toggled, this,
					 &SkinManagerDialog::onAutoRotateToggled);
	QObject::connect(ui->capeCombo,
					 QOverload<int>::of(&QComboBox::currentIndexChanged), this,
					 &SkinManagerDialog::onCapeChanged);
	QObject::connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
					 &SkinManagerDialog::onAccept);
	QObject::connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
					 &QDialog::reject);

	loadAccountState();
}

SkinManagerDialog::~SkinManagerDialog()
{
	delete ui;
}

/* ── load: render whatever the account currently has ─────────────── */

void SkinManagerDialog::loadAccountState()
{
	if (!m_account || !m_account->accountData())
		return;

	const auto& data = *m_account->accountData();
	ui->accountLabel->setText(tr("Account: %1").arg(m_account->profileName()));

	/* Skin */
	QImage skinImg;
	if (!data.minecraftProfile.skin.data.isEmpty()) {
		skinImg.loadFromData(data.minecraftProfile.skin.data, "PNG");
		skinImg = normaliseSkin(skinImg);
	}
	ModelVariant variant = ModelVariant::Classic;
	if (data.minecraftProfile.skin.variant.compare(QStringLiteral("SLIM"),
												   Qt::CaseInsensitive) == 0) {
		variant = ModelVariant::Slim;
	} else if (!skinImg.isNull()) {
		variant = detectVariant(skinImg);
	}
	if (variant == ModelVariant::Slim) {
		ui->rdoSlim->setChecked(true);
	} else {
		ui->rdoClassic->setChecked(true);
	}
	m_viewer->setSkin(skinImg, variant);

	/* Capes */
	ui->capeCombo->blockSignals(true);
	ui->capeCombo->clear();
	ui->capeCombo->addItem(tr("No Cape"), QString());
	int activeRow = 0;
	int row = 1;
	for (auto it = data.minecraftProfile.capes.cbegin();
		 it != data.minecraftProfile.capes.cend(); ++it, ++row) {
		const Cape& c = it.value();
		QPixmap preview;
		if (!c.data.isEmpty()) {
			QPixmap pix;
			if (pix.loadFromData(c.data, "PNG")) {
				preview = pix.copy(1, 1, 10, 16);
			}
		}
		QString label = c.alias.isEmpty() ? c.id : c.alias;
		if (preview.isNull())
			ui->capeCombo->addItem(label, c.id);
		else
			ui->capeCombo->addItem(QIcon(preview), label, c.id);
		if (c.id == data.minecraftProfile.currentCape)
			activeRow = row;
	}
	ui->capeCombo->setCurrentIndex(activeRow);
	ui->capeCombo->blockSignals(false);

	/* Push cape preview into the 3D viewer. */
	onCapeChanged(activeRow);
}

/* ── browse / drop handlers ──────────────────────────────────────── */

void SkinManagerDialog::onBrowseClicked()
{
	const QString picturesDir =
		QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	const QString chosen =
		QFileDialog::getOpenFileName(this, tr("Select Skin Texture"),
									 picturesDir, tr("Minecraft skin (*.png)"));
	if (chosen.isEmpty())
		return;
	loadSkinFile(chosen);
}

void SkinManagerDialog::onSkinFileDropped(const QString& path)
{
	loadSkinFile(path);
}

void SkinManagerDialog::loadSkinFile(const QString& path)
{
	QImage img(path);
	if (img.isNull() || img.width() != 64 ||
		(img.height() != 64 && img.height() != 32)) {
		setStatus(tr("Skin PNGs must be 64×64 (modern) or 64×32 (legacy). "
					 "The chosen file is %1×%2.")
					  .arg(img.width())
					  .arg(img.height()),
				  /*error=*/true);
		return;
	}

	m_chosenSkinPath = path;
	m_chosenSkinImage = normaliseSkin(img);
	ui->skinPathText->setText(path);

	/* Auto-detect variant from the new file unless the user has
	 * manually clicked one of the radios since opening the dialog —
	 * detect-once heuristic. */
	const ModelVariant detected = detectVariant(m_chosenSkinImage);
	if (detected == ModelVariant::Slim)
		ui->rdoSlim->setChecked(true);
	else
		ui->rdoClassic->setChecked(true);

	m_viewer->setSkin(m_chosenSkinImage, detected);
	setStatus(QString());
}

void SkinManagerDialog::onResetClicked()
{
	if (!m_account)
		return;

	const int rc = QMessageBox::question(
		this, tr("Reset skin"),
		tr("Remove the active custom skin for %1?\n\n"
		   "Mojang will revert the account to the "
		   "default Steve or Alex skin.")
			.arg(m_account->profileName()),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (rc != QMessageBox::Yes)
		return;

	ProgressDialog prog(this);
	auto task = std::make_shared<SkinDelete>(this, m_account->accessToken());
	if (prog.execWithTask(task.get()) != QDialog::Accepted) {
		CustomMessageBox::selectable(this, tr("Skin reset failed"),
									 tr("Mojang did not process the request."),
									 QMessageBox::Warning)
			->exec();
		return;
	}

	/* Drop the cached skin so the next refresh re-fetches the default. */
	m_account->accountData()->minecraftProfile.skin = Skin{};
	m_viewer->clearSkin();
	setStatus(tr("Skin reset to the Mojang default."));
}

/* ── viewer toggles ───────────────────────────────────────────────── */

void SkinManagerDialog::onVariantToggled()
{
	const ModelVariant v =
		ui->rdoSlim->isChecked() ? ModelVariant::Slim : ModelVariant::Classic;
	QImage current;
	if (!m_chosenSkinImage.isNull()) {
		current = m_chosenSkinImage;
	} else if (m_account && !m_account->accountData()
								 ->minecraftProfile.skin.data.isEmpty()) {
		current.loadFromData(
			m_account->accountData()->minecraftProfile.skin.data, "PNG");
		current = normaliseSkin(current);
	}
	m_viewer->setSkin(current, v);
}

void SkinManagerDialog::onOverlayToggled(bool on)
{
	m_viewer->setShowOverlay(on);
}

void SkinManagerDialog::onAutoRotateToggled(bool on)
{
	m_viewer->setAutoRotate(on);
}

void SkinManagerDialog::onCapeChanged(int row)
{
	if (!m_account)
		return;
	const QString capeId = ui->capeCombo->itemData(row).toString();
	if (capeId.isEmpty()) {
		m_viewer->setCape(QImage());
		return;
	}
	const Cape& c =
		m_account->accountData()->minecraftProfile.capes.value(capeId);
	QImage capeImg;
	if (!c.data.isEmpty())
		capeImg.loadFromData(c.data, "PNG");
	m_viewer->setCape(capeImg);
}

/* ── commit ───────────────────────────────────────────────────────── */

void SkinManagerDialog::onAccept()
{
	if (!m_account) {
		reject();
		return;
	}

	/* Build a SequentialTask matching the stock SkinUploadDialog
	 * behaviour: optional SkinUpload + optional CapeChange. */
	SequentialTask seq;
	bool anything = false;

	if (!m_chosenSkinPath.isEmpty()) {
		QFile f(m_chosenSkinPath);
		if (!f.open(QIODevice::ReadOnly)) {
			setStatus(tr("Could not read %1: %2")
						  .arg(m_chosenSkinPath, f.errorString()),
					  /*error=*/true);
			return;
		}
		const QByteArray bytes = f.readAll();
		f.close();
		const SkinUpload::Model model =
			ui->rdoSlim->isChecked() ? SkinUpload::ALEX : SkinUpload::STEVE;
		seq.addTask(shared_qobject_ptr<SkinUpload>(
			new SkinUpload(this, m_account->accessToken(), bytes, model)));
		anything = true;
	}

	const QString chosenCape = ui->capeCombo->currentData().toString();
	if (chosenCape != m_account->accountData()->minecraftProfile.currentCape) {
		seq.addTask(shared_qobject_ptr<CapeChange>(
			new CapeChange(this, m_account->accessToken(), chosenCape)));
		anything = true;
	}

	if (!anything) {
		/* Nothing to apply — just close the dialog quietly. */
		accept();
		return;
	}

	ProgressDialog prog(this);
	if (prog.execWithTask(&seq) != QDialog::Accepted) {
		CustomMessageBox::selectable(this, tr("Skin Upload"),
									 tr("Failed to apply skin changes."),
									 QMessageBox::Warning)
			->exec();
		return;
	}

	/* Update in-memory cache so the launcher's account list re-renders
	 * immediately. */
	if (!m_chosenSkinPath.isEmpty()) {
		QFile f(m_chosenSkinPath);
		if (f.open(QIODevice::ReadOnly)) {
			m_account->accountData()->minecraftProfile.skin.data = f.readAll();
			f.close();
		}
		m_account->accountData()->minecraftProfile.skin.variant =
			ui->rdoSlim->isChecked() ? QStringLiteral("SLIM")
									 : QStringLiteral("CLASSIC");
	}
	if (chosenCape != m_account->accountData()->minecraftProfile.currentCape)
		m_account->accountData()->minecraftProfile.currentCape = chosenCape;

	CustomMessageBox::selectable(this, tr("Skin Upload"),
								 tr("Successfully applied skin changes."),
								 QMessageBox::Information)
		->exec();
	accept();
}

/* ── status line ──────────────────────────────────────────────────── */

void SkinManagerDialog::setStatus(const QString& text, bool error)
{
	ui->statusLabel->setText(text);
	ui->statusLabel->setStyleSheet(
		error ? QStringLiteral("color: #d33; font-size: 11px;")
			  : QStringLiteral("color: palette(mid); font-size: 11px;"));
}
