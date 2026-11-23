/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "JavaDownloadDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "net/ChecksumValidator.h"
#include <QRegularExpression>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QHeaderView>
#include <QSplitter>

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "Json.h"
#include "java/JavaUtils.h"
#include "net/Download.h"

JavaDownloadDialog::JavaDownloadDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Download Java"));
	setMinimumSize(700, 450);
	resize(800, 500);
	m_providers = JavaDownload::JavaProviderInfo::availableProviders();
	setupUi();
}

void JavaDownloadDialog::setupUi()
{
	auto* mainLayout = new QVBoxLayout(this);

	// --- Three-column selection area ---
	auto* columnsLayout = new QHBoxLayout();

	// Left: Provider list
	auto* providerGroup = new QGroupBox(tr("Provider"), this);
	auto* providerLayout = new QVBoxLayout(providerGroup);
	m_providerList = new QListWidget(this);
	m_providerList->setIconSize(QSize(24, 24));
	for (const auto& provider : m_providers) {
		auto* item = new QListWidgetItem(
			APPLICATION->getThemedIcon(provider.iconName), provider.name);
		m_providerList->addItem(item);
	}
	providerLayout->addWidget(m_providerList);
	columnsLayout->addWidget(providerGroup, 1);

	// Center: Major version list
	auto* versionGroup = new QGroupBox(tr("Version"), this);
	auto* versionLayout = new QVBoxLayout(versionGroup);
	m_versionList = new QListWidget(this);
	versionLayout->addWidget(m_versionList);
	columnsLayout->addWidget(versionGroup, 1);

	// Right: Sub-version / build list
	auto* subVersionGroup = new QGroupBox(tr("Build"), this);
	auto* subVersionLayout = new QVBoxLayout(subVersionGroup);
	m_subVersionList = new QListWidget(this);
	subVersionLayout->addWidget(m_subVersionList);
	columnsLayout->addWidget(subVersionGroup, 1);

	mainLayout->addLayout(columnsLayout, 1);

	// Info label
	m_infoLabel = new QLabel(this);
	m_infoLabel->setWordWrap(true);
	m_infoLabel->setText(
		tr("Select a Java provider, version, and build to download."));
	mainLayout->addWidget(m_infoLabel);


	// Buttons
	auto* buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();

	m_downloadBtn = new QPushButton(tr("Download"), this);
	m_downloadBtn->setEnabled(false);
	buttonLayout->addWidget(m_downloadBtn);

	m_cancelBtn = new QPushButton(tr("Cancel"), this);
	buttonLayout->addWidget(m_cancelBtn);

	mainLayout->addLayout(buttonLayout);

	// Connections
	connect(m_providerList, &QListWidget::currentRowChanged, this,
			&JavaDownloadDialog::providerChanged);
	connect(m_versionList, &QListWidget::currentRowChanged, this,
			&JavaDownloadDialog::majorVersionChanged);
	connect(m_subVersionList, &QListWidget::currentRowChanged, this,
			&JavaDownloadDialog::subVersionChanged);
	connect(m_downloadBtn, &QPushButton::clicked, this,
			&JavaDownloadDialog::onDownloadClicked);
	connect(m_cancelBtn, &QPushButton::clicked, this,
			&JavaDownloadDialog::onCancelClicked);

	// Select first provider
	if (m_providerList->count() > 0) {
		m_providerList->setCurrentRow(0);
	}
}

void JavaDownloadDialog::providerChanged(int index)
{
	if (index < 0 || index >= m_providers.size())
		return;

	m_versionList->clear();
	m_subVersionList->clear();
	m_downloadBtn->setEnabled(false);
	m_versions.clear();
	m_runtimes.clear();

	const auto& provider = m_providers[index];
	m_infoLabel->setText(tr("Loading versions for %1...").arg(provider.name));

	fetchVersionList(provider.uid);
}

void JavaDownloadDialog::majorVersionChanged(int index)
{
	if (index < 0 || index >= m_versions.size())
		return;

	m_subVersionList->clear();
	m_downloadBtn->setEnabled(false);
	m_runtimes.clear();

	const auto& version = m_versions[index];
	m_infoLabel->setText(tr("Loading builds for %1...").arg(version.versionId));

	fetchRuntimes(version.uid, version.versionId);
}

void JavaDownloadDialog::subVersionChanged(int index)
{
	if (index < 0 || index >= m_runtimes.size()) {
		m_downloadBtn->setEnabled(false);
		return;
	}

	const auto& rt = m_runtimes[index];
	m_infoLabel->setText(tr("Ready to download: %1\n"
							"Version: %2\n"
							"Platform: %3\n"
							"Checksum: %4")
							 .arg(rt.name, rt.version.toString(), rt.runtimeOS,
								  rt.checksumHash.isEmpty()
									  ? tr("None")
									  : tr("Yes (%1)").arg(rt.checksumType)));
	m_downloadBtn->setEnabled(true);
}

void JavaDownloadDialog::fetchVersionList(const QString& uid)
{
    if (m_fetchJob) {
        disconnect(m_fetchJob.get(), nullptr, this, nullptr);
        m_fetchJob->abort();
        m_fetchJob.reset();
    }
    const auto data = std::make_shared<QByteArray>();

	QString url = QString("%1%2/index.json").arg(BuildConfig.META_URL, uid);
	m_fetchJob = new NetJob(tr("Fetch Java versions"), APPLICATION->network());
	auto dl = Net::Download::makeByteArray(QUrl(url), data.get());
    connect(dl.get(), &QObject::destroyed, [data] {});
	m_fetchJob->addNetAction(dl);

	connect(m_fetchJob.get(), &NetJob::succeeded, this, [this, uid, data]() {
		m_fetchJob.reset();

		QJsonDocument doc;
		try {
			doc = Json::requireDocument(*data);
		} catch (const Exception& e) {
			m_infoLabel->setText(
				tr("Failed to parse version list: %1").arg(e.cause()));
			return;
		}
		if (!doc.isObject()) {
			m_infoLabel->setText(tr("Failed to parse version list."));
			return;
		}

		m_versions = JavaDownload::parseVersionIndex(doc.object(), uid);
		m_versionList->clear();

		for (const auto& ver : m_versions) {
			QString displayName = ver.versionId;
			if (displayName.startsWith("java")) {
				displayName = "Java " + displayName.mid(4);
			}
			m_versionList->addItem(displayName);
		}

		if (m_versions.size() > 0) {
			m_infoLabel->setText(tr("Select a version."));
		} else {
			m_infoLabel->setText(
				tr("No versions available for this provider."));
		}
	});

	connect(m_fetchJob.get(), &NetJob::failed, this, [this](QString reason) {
		m_fetchJob.reset();
		m_infoLabel->setText(tr("Failed to load versions: %1").arg(reason));
	});

	m_fetchJob->start();
}

void JavaDownloadDialog::fetchRuntimes(const QString& uid,
									   const QString& versionId)
{
    if (m_fetchJob) {
        disconnect(m_fetchJob.get(), nullptr, this, nullptr);
        m_fetchJob->abort();
        m_fetchJob.reset();
    }
    const auto data = std::make_shared<QByteArray>();
    if (!QRegularExpression("^[A-Za-z0-9._+-]+$").match(versionId).hasMatch() ||
        versionId == "." || versionId == "..") {
        m_infoLabel->setText(tr("Invalid version in Java metadata."));
        return;
    }
	QString url =
		QString("%1%2/%3.json").arg(BuildConfig.META_URL, uid, versionId);
	m_fetchJob =
		new NetJob(tr("Fetch Java runtime details"), APPLICATION->network());
	auto dl = Net::Download::makeByteArray(QUrl(url), data.get());
    connect(dl.get(), &QObject::destroyed, [data] {});
    for (const auto& version : m_versions) {
        if (version.versionId == versionId && version.uid == uid && !version.sha256.isEmpty()) {
            if (!QRegularExpression("^[A-Fa-f0-9]{64}$").match(version.sha256).hasMatch()) {
                m_fetchJob.reset();
                m_infoLabel->setText(tr("Invalid checksum in Java metadata."));
                return;
            }
            dl->addValidator(new Net::ChecksumValidator(
                QCryptographicHash::Sha256, QByteArray::fromHex(version.sha256.toLatin1())));
            break;
        }
    }
	m_fetchJob->addNetAction(dl);

	connect(m_fetchJob.get(), &NetJob::succeeded, this, [this, data]() {
		m_fetchJob.reset();

		QJsonDocument doc;
		try {
			doc = Json::requireDocument(*data);
		} catch (const Exception& e) {
			m_infoLabel->setText(
				tr("Failed to parse runtime details: %1").arg(e.cause()));
			return;
		}
		if (!doc.isObject()) {
			m_infoLabel->setText(tr("Failed to parse runtime details."));
			return;
		}

		auto allRuntimes = JavaDownload::parseRuntimes(doc.object());
		QString myOS = JavaDownload::currentRuntimeOS();

		m_runtimes.clear();
		m_subVersionList->clear();
		for (const auto& rt : allRuntimes) {
			if (rt.runtimeOS == myOS) {
				m_runtimes.append(rt);
				m_subVersionList->addItem(rt.version.toString());
			}
		}

		if (m_runtimes.isEmpty()) {
			m_infoLabel->setText(
				tr("No builds available for your platform (%1).").arg(myOS));
			m_downloadBtn->setEnabled(false);
		} else {
			m_infoLabel->setText(tr("Select a build to download."));
			m_subVersionList->setCurrentRow(0);
		}
	});

	connect(m_fetchJob.get(), &NetJob::failed, this, [this](QString reason) {
		m_fetchJob.reset();
		m_infoLabel->setText(
			tr("Failed to load runtime details: %1").arg(reason));
	});

	m_fetchJob->start();
}

QString JavaDownloadDialog::javaInstallDir() const
{
	return JavaUtils::managedJavaRoot();
}

void JavaDownloadDialog::onDownloadClicked()
{
	int idx = m_subVersionList->currentRow();
	if (idx < 0 || idx >= m_runtimes.size())
		return;

	const auto& runtime = m_runtimes[idx];

    const QRegularExpression safeName("^[A-Za-z0-9][A-Za-z0-9._+-]*$");
    if (!safeName.match(runtime.vendor).hasMatch() || !safeName.match(runtime.name).hasMatch()) {
        QMessageBox::warning(this, tr("Java Download"), tr("Invalid Java runtime name or vendor."));
        return;
    }
    const auto targetDir = FS::PathCombine(javaInstallDir(), runtime.vendor,
        runtime.name + '-' + runtime.version.toString());
    if (QFileInfo::exists(targetDir) &&
        QMessageBox::question(this, tr("Already Installed"), tr("Reinstall the Java runtime at %1?").arg(targetDir),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    JavaDownloadTask task(runtime, targetDir);
    ProgressDialog progress(this);
    progress.setSkipButton(true, tr("Stop"));
    if (progress.execWithTask(&task) != QDialog::Accepted) {
        if (!task.wasAborted())
            QMessageBox::warning(this, tr("Java Download Failed"), task.failReason());
        return;
    }
    m_installedJavaPath = task.installedJavaPath();
    accept();
}

void JavaDownloadDialog::onCancelClicked()
{
	if (m_fetchJob) {
        disconnect(m_fetchJob.get(), nullptr, this, nullptr);
		m_fetchJob->abort();
		m_fetchJob.reset();
	}
	reject();
}
