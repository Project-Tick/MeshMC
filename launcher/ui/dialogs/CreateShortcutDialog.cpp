/*
 * Copyright 2022-2023 arthomnix
 * Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include <QFileDialog>
#include <BuildConfig.h>
#include <QBuffer>
#include <QDataStream>
#include <QMessageBox>
#include <QPainter>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QUuid>
#include <QXmlStreamWriter>
#include "CreateShortcutDialog.h"
#include "ui_CreateShortcutDialog.h"
#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/WorldList.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/PackProfile.h"
#include "icons/IconList.h"
#include "icons/IconUtils.h"
#include "FileSystem.h"
#include "InstanceShortcuts.h"
#include "ui/dialogs/IconPickerDialog.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shobjidl.h>
#include <shlguid.h>
#endif

namespace
{
bool writeShortcutFile(const QString& path, const QByteArray& contents, bool executable = false)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size())
        return false;
    if (executable && !file.setPermissions(file.permissions() | QFileDevice::ExeOwner))
        return false;
    return file.commit();
}

#ifdef Q_OS_UNIX
QString shellArgument(QString value)
{
    value.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + value + QLatin1Char('\'');
}

QByteArray shellScript(const QStringList& command)
{
    QStringList words;
    for (const auto& word : command)
        words.append(shellArgument(word));
    return (QStringLiteral("#!/bin/sh\nexec ") + words.join(QLatin1Char(' ')) + QLatin1Char('\n')).toUtf8();
}
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
bool isFlatpak()
{
    return QFileInfo::exists(QStringLiteral("/.flatpak-info"));
}

QString desktopString(QString value)
{
    return value.replace("\\", "\\\\").replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t");
}

QString desktopArgument(QString value)
{
    value.replace("%", "%%").replace("\\", "\\\\").replace("\"", "\\\"")
        .replace("$", "\\$").replace("`", "\\`");
    return desktopString(QLatin1Char('"') + value + QLatin1Char('"'));
}
#endif

#ifdef Q_OS_MACOS
bool writeMacApp(const QString& path, const QString& name, const QStringList& command, const QPixmap& icon)
{
    QTemporaryDir staging(QFileInfo(path).absolutePath() + "/.meshmc-shortcut-XXXXXX");
    if (!staging.isValid())
        return false;

    QDir bundle(staging.path());
    if (!bundle.mkpath("Contents/MacOS") || !bundle.mkpath("Contents/Resources"))
        return false;

    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !icon.save(&buffer, "PNG"))
        return false;

    // Finder accepts a 128-pixel PNG in an ICNS container.
    QByteArray icns;
    QDataStream stream(&icns, QIODevice::WriteOnly);
    stream.writeRawData("icns", 4);
    stream << quint32(png.size() + 16);
    stream.writeRawData("ic07", 4);
    stream << quint32(png.size() + 8);
    icns.append(png);

    QByteArray plist;
    QXmlStreamWriter xml(&plist);
    xml.writeStartDocument();
    xml.writeDTD(QStringLiteral("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"));
    xml.writeStartElement("plist");
    xml.writeAttribute("version", "1.0");
    xml.writeStartElement("dict");
    xml.writeTextElement("key", "CFBundleExecutable");
    xml.writeTextElement("string", "Launch");
    xml.writeTextElement("key", "CFBundlePackageType");
    xml.writeTextElement("string", "APPL");
    xml.writeTextElement("key", "CFBundleName");
    xml.writeTextElement("string", name);
    xml.writeTextElement("key", "CFBundleIconFile");
    xml.writeTextElement("string", "Icon.icns");
    xml.writeEndDocument();

    return writeShortcutFile(bundle.filePath("Contents/Info.plist"), plist)
        && writeShortcutFile(bundle.filePath("Contents/Resources/Icon.icns"), icns)
        && writeShortcutFile(bundle.filePath("Contents/MacOS/Launch"), shellScript(command), true)
        && QDir().rename(bundle.absolutePath(), path);
}
#endif

#ifdef Q_OS_WIN
QString windowsArgument(const QString& argument)
{
    QString quoted = QStringLiteral("\"");
    int backslashes = 0;
    for (const auto character : argument) {
        if (character == QLatin1Char('\\')) {
            ++backslashes;
            continue;
        }
        quoted += QString(character == QLatin1Char('"') ? backslashes * 2 + 1 : backslashes, QLatin1Char('\\'));
        quoted += character;
        backslashes = 0;
    }
    return quoted + QString(backslashes * 2, QLatin1Char('\\')) + QLatin1Char('"');
}

bool writeWindowsLink(const QString& path, const QString& program, const QString& dataDir,
                      const QStringList& arguments, const QString& name, const QString& icon)
{
    const auto initialized = CoInitialize(nullptr);
    IShellLinkW* link = nullptr;
    auto result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW, reinterpret_cast<void**>(&link));
    if (SUCCEEDED(result)) {
        QStringList words;
        for (const auto& argument : arguments)
            words.append(windowsArgument(argument));

        const auto target = QDir::toNativeSeparators(program);
        const auto dir = QDir::toNativeSeparators(dataDir);
        const auto args = words.join(QLatin1Char(' '));
        const auto iconPath = QDir::toNativeSeparators(icon);
        IPersistFile* file = nullptr;
        result = link->SetPath(reinterpret_cast<LPCWSTR>(target.utf16()));
        if (SUCCEEDED(result))
            result = link->SetWorkingDirectory(reinterpret_cast<LPCWSTR>(dir.utf16()));
        if (SUCCEEDED(result))
            result = link->SetArguments(reinterpret_cast<LPCWSTR>(args.utf16()));
        if (SUCCEEDED(result))
            result = link->SetDescription(reinterpret_cast<LPCWSTR>(name.utf16()));
        if (SUCCEEDED(result))
            result = link->SetIconLocation(reinterpret_cast<LPCWSTR>(iconPath.utf16()), 0);
        if (SUCCEEDED(result))
            result = link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&file));
        if (SUCCEEDED(result)) {
            QTemporaryDir staging(QFileInfo(path).absolutePath() + "/.meshmc-shortcut-XXXXXX");
            if (staging.isValid()) {
                const auto staged = QDir::toNativeSeparators(QDir(staging.path()).filePath("shortcut.lnk"));
                result = file->Save(reinterpret_cast<LPCOLESTR>(staged.utf16()), TRUE);
                if (SUCCEEDED(result)) {
                    const auto destination = QDir::toNativeSeparators(path);
                    if (!MoveFileExW(reinterpret_cast<LPCWSTR>(staged.utf16()),
                                     reinterpret_cast<LPCWSTR>(destination.utf16()), MOVEFILE_REPLACE_EXISTING)) {
                        result = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            } else {
                result = E_FAIL;
            }
            file->Release();
        }
        link->Release();
    }
    if (SUCCEEDED(initialized))
        CoUninitialize();
    return SUCCEEDED(result);
}
#endif
}

CreateShortcutDialog::CreateShortcutDialog(QWidget *parent, InstancePtr instance)
    :QDialog(parent), ui(new Ui::CreateShortcutDialog), m_instance(instance)
{
    ui->setupUi(this);
    m_iconKey = m_instance->iconKey();
    const auto refreshIcon = [this] {
        ui->shortcutIconButton->setIcon(APPLICATION->icons()->getIcon(m_iconKey));
    };
    refreshIcon();
    connect(ui->shortcutIconButton, &QPushButton::clicked, this, [this, refreshIcon] {
        IconPickerDialog picker(this);
        if (picker.execWithSelection(m_iconKey) == QDialog::Accepted) {
            m_iconKey = picker.selectedIconKey;
            refreshIcon();
        }
    });
    connect(ui->createScriptCheckBox, &QCheckBox::toggled, this, [this](bool script) {
        ui->shortcutIconButton->setEnabled(!script);
        const auto path = ui->shortcutPath->text();
        for (const auto& suffix : { QStringLiteral(".app"), QStringLiteral(".command"),
                                   QStringLiteral(".desktop"), QStringLiteral(".sh"),
                                   QStringLiteral(".lnk"), QStringLiteral(".bat") }) {
            if (path.endsWith(suffix, Qt::CaseInsensitive)) {
                ui->shortcutPath->setText(path.left(path.size() - suffix.size()) + '.' + shortcutSuffix());
                break;
            }
        }
    });

#if defined(Q_OS_MACOS)
    ui->createScriptCheckBox->setText(tr("Create a .command script instead of an app"));
#elif defined(Q_OS_UNIX)
    const auto applications = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (!isFlatpak() && !applications.isEmpty()) {
        auto* button = new QPushButton(tr("Applications folder…"), this);
        ui->gridLayout->addWidget(button, 0, 3);
        connect(button, &QPushButton::clicked, this, [this, applications] {
            if (QDir().mkpath(applications))
                browseShortcutPath(applications);
            else
                QMessageBox::warning(this, tr("Create Shortcut"), tr("Could not open %1.").arg(applications));
        });
    }
#endif

    QStringList accountNameList;
    auto accounts = APPLICATION->accounts();

    for (int i = 0; i < accounts->count(); i++)
    {
        auto entry = accounts->at(i);
        if(!entry.isAccount)
        {
            continue;
        }
        accountNameList.append(entry.account->profileName());
    }

    ui->profileComboBox->addItems(accountNameList);
    ui->useProfileCheckBox->setEnabled(!accountNameList.isEmpty());

    if (accounts->defaultAccount())
    {
        ui->profileComboBox->setCurrentText(accounts->defaultAccount()->profileName());
    }

    // TODO: check if version is affected by crashing when joining servers on launch, ideally in meta

    bool instanceSupportsQuickPlay = false;

    auto mcInstance = std::dynamic_pointer_cast<MinecraftInstance>(instance);
    if (mcInstance)
    {
        mcInstance->getPackProfile()->reload(Net::Mode::Online);

        const auto minecraft = mcInstance->getPackProfile()->getComponent("net.minecraft");
        if (minecraft && minecraft->getReleaseDateTime() >= g_VersionFilterData.quickPlayBeginsDate)
        {
            mcInstance->worldList()->update();
            for (const auto &world : mcInstance->worldList()->allWorlds())
            {
                ui->joinSingleplayer->addItem(world.name().isEmpty() ? world.folderName() : world.name(), world.folderName());
            }
            instanceSupportsQuickPlay = ui->joinSingleplayer->count() > 0;
        }
    }

    ui->joinServerRadioButton->setChecked(true);
    if (!instanceSupportsQuickPlay)
    {
        ui->joinSingleplayerRadioButton->setVisible(false);
        ui->joinSingleplayer->setVisible(false);
    }

    connect(ui->joinWorldCheckBox, &QCheckBox::toggled, this, &CreateShortcutDialog::updateDialogState);

    updateDialogState();
}

CreateShortcutDialog::~CreateShortcutDialog()
{
    delete ui;
}

QString CreateShortcutDialog::shortcutSuffix() const
{
#if defined(Q_OS_MACOS)
    return ui->createScriptCheckBox->isChecked() ? "command" : "app";
#elif defined(Q_OS_WIN)
    return ui->createScriptCheckBox->isChecked() ? "bat" : "lnk";
#else
    return ui->createScriptCheckBox->isChecked() ? "sh" : "desktop";
#endif
}

void CreateShortcutDialog::on_shortcutPathBrowse_clicked()
{
    auto directory = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (isFlatpak() || directory.isEmpty())
        directory = QDir::homePath();
#endif
    browseShortcutPath(directory);
}

void CreateShortcutDialog::browseShortcutPath(const QString& directory)
{
    QFileDialog dialog(this, tr("Select shortcut path"), directory);
    dialog.setDefaultSuffix(shortcutSuffix());
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setOption(QFileDialog::DontConfirmOverwrite);
    dialog.selectFile(FS::RemoveInvalidFilenameChars(m_instance->name() + " - " + BuildConfig.LAUNCHER_DISPLAYNAME)
                      + "." + shortcutSuffix());
    if (dialog.exec() == QDialog::Accepted)
        ui->shortcutPath->setText(dialog.selectedFiles().value(0));
}

void CreateShortcutDialog::accept()
{
    const auto suffix = shortcutSuffix();
    auto path = ui->shortcutPath->text().trimmed();
    if (path.isEmpty())
        return;
    if (!path.endsWith('.' + suffix, Qt::CaseInsensitive))
        path += '.' + suffix;
    path = QFileInfo(path).absoluteFilePath();

    const QFileInfo existing(path);
    if (existing.isDir() || existing.isSymLink()) {
        QMessageBox::warning(this, tr("Create Shortcut"), tr("A folder or symbolic link already exists at %1.").arg(path));
        return;
    }
    if (existing.exists() && QMessageBox::question(this, tr("Replace Shortcut"),
            tr("Replace the existing file at %1?").arg(path)) != QMessageBox::Yes) {
        return;
    }

    if (!createShortcut(path, suffix))
        return;
    if (!InstanceShortcuts::record(m_instance->instanceRoot(), path)) {
        QMessageBox::warning(this, tr("Create Shortcut"),
            tr("The shortcut was created, but it could not be registered for automatic removal."));
    }
    QDialog::accept();
}

void CreateShortcutDialog::updateDialogState()
{
    if (!ui->launchOfflineCheckBox->isChecked())
        ui->offlineUsernameCheckBox->setChecked(false);

    const bool useTarget = ui->joinWorldCheckBox->isChecked();
    const bool server = ui->joinServerRadioButton->isChecked();
    const bool world = ui->joinSingleplayerRadioButton->isChecked();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
        !ui->shortcutPath->text().trimmed().isEmpty()
        && (!useTarget || (server && !ui->joinServer->text().trimmed().isEmpty())
                       || (world && ui->joinSingleplayer->currentIndex() >= 0))
        && (!ui->offlineUsernameCheckBox->isChecked() || !ui->offlineUsername->text().trimmed().isEmpty())
        && (!ui->useProfileCheckBox->isChecked() || !ui->profileComboBox->currentText().isEmpty()));
    ui->joinServer->setEnabled(useTarget && server);
    ui->joinSingleplayer->setEnabled(useTarget && world);
    ui->offlineUsername->setEnabled(ui->offlineUsernameCheckBox->isChecked());
}

QStringList CreateShortcutDialog::launchArguments(const QString& dataDir) const
{
    QStringList args{ "--dir", dataDir, "--launch", m_instance->id() };
    if (ui->joinWorldCheckBox->isChecked()) {
        if (ui->joinServerRadioButton->isChecked())
            args << "--server" << ui->joinServer->text().trimmed();
        else if (ui->joinSingleplayerRadioButton->isChecked())
            args << "--world" << ui->joinSingleplayer->currentData().toString();
    }
    if (ui->useProfileCheckBox->isChecked())
        args << "--profile" << ui->profileComboBox->currentText();
    if (ui->launchOfflineCheckBox->isChecked()) {
        args << "--offline";
        if (ui->offlineUsernameCheckBox->isChecked())
            args << "--name" << ui->offlineUsername->text();
    }
    return args;
}

bool CreateShortcutDialog::createShortcut(const QString& path, const QString& suffix)
{
    auto program = QCoreApplication::applicationFilePath();
    auto dataDir = QDir::currentPath();
#ifdef Q_OS_WIN
    program = QDir::toNativeSeparators(program);
    dataDir = QDir::toNativeSeparators(dataDir);
#endif
    auto args = launchArguments(dataDir);
#if defined(Q_OS_MACOS)
    if (program.startsWith("/private/var/") || program.startsWith("/Volumes/")
        || program.startsWith(QDir::tempPath() + '/')) {
        QMessageBox::warning(this, tr("Create Shortcut"),
            tr("MeshMC is running from a temporary location or disk image. Move it to Applications first."));
        return false;
    }
#elif defined(Q_OS_UNIX)
    if (isFlatpak()) {
        auto appId = qgetenv("FLATPAK_ID");
        if (appId.isEmpty())
            appId = "org.projecttick.MeshMC";
        program = QStringLiteral("flatpak");
        args = QStringList{ "run", QString::fromUtf8(appId) } + args;
    } else if (program.startsWith("/tmp/.mount_") || !qgetenv("APPIMAGE").isEmpty()) {
        program = QString::fromLocal8Bit(qgetenv("APPIMAGE"));
        if (!QFileInfo(program).isExecutable()) {
            QMessageBox::warning(this, tr("Create Shortcut"), tr("Could not locate the original AppImage."));
            return false;
        }
    } else {
        const QDir bin(QFileInfo(program).absolutePath());
        const QFileInfo wrapper(bin.absoluteFilePath("../" + BuildConfig.LAUNCHER_NAME));
        if (bin.dirName() == "bin" && wrapper.isFile() && wrapper.isExecutable())
            program = wrapper.absoluteFilePath();
    }
#endif
    if (!FS::ensureFilePathExists(path)) {
        QMessageBox::warning(this, tr("Create Shortcut"), tr("Could not create the folder for %1.").arg(path));
        return false;
    }

    const QStringList command = QStringList{ program } + args;
    const auto name = QFileInfo(path).completeBaseName();
    bool created = false;
    QString savedIcon;
    const bool usesIcon = suffix == "app" || suffix == "lnk" || suffix == "desktop";
    QPixmap icon;
    if (usesIcon) {
        const QSize size(128, 128);
        const auto fitted = IconUtils::fitted(APPLICATION->icons()->getIcon(m_iconKey), size, 1.0);
        if (!fitted.isNull()) {
            icon = QPixmap(size);
            icon.fill(Qt::transparent);
            QPainter painter(&icon);
            painter.drawPixmap((size.width() - fitted.width()) / 2, (size.height() - fitted.height()) / 2, fitted);
        }
    }

#if defined(Q_OS_MACOS)
    created = suffix == "app" ? writeMacApp(path, name, command, icon)
                              : writeShortcutFile(path, shellScript(command), true);
#elif defined(Q_OS_WIN)
    if (suffix == "bat") {
        QStringList words;
        for (const auto& argument : command) {
            if (argument.contains('"') || argument.contains('\r') || argument.contains('\n')) {
                QMessageBox::warning(this, tr("Create Shortcut"),
                    tr("The command contains characters a batch file cannot represent. Use a Windows shortcut instead."));
                return false;
            }
            auto escaped = argument;
            words.append(windowsArgument(escaped.replace("%", "%%")));
        }
        const auto script = QStringLiteral("@ECHO OFF\r\nSETLOCAL DisableDelayedExpansion\r\n")
            + words.join(' ') + QStringLiteral("\r\n");
        const auto bytes = script.toLocal8Bit();
        created = QString::fromLocal8Bit(bytes) == script && writeShortcutFile(path, bytes);
    } else if (!icon.isNull()) {
        savedIcon = FS::PathCombine(m_instance->instanceRoot(), "shortcut-icons", QUuid::createUuid().toString() + ".ico");
        created = FS::ensureFilePathExists(savedIcon) && icon.save(savedIcon, "ICO")
            && writeWindowsLink(path, program, dataDir, args, name, savedIcon);
    }
#elif defined(Q_OS_UNIX)
    if (suffix == "sh") {
        created = writeShortcutFile(path, shellScript(command), true);
    } else if (!icon.isNull()) {
        savedIcon = FS::PathCombine(m_instance->instanceRoot(), "shortcut-icons", QUuid::createUuid().toString() + ".png");
        QStringList words;
        for (const auto& argument : command)
            words.append(desktopArgument(argument));
        const auto entry = QStringLiteral("[Desktop Entry]\nType=Application\nCategories=Game;\nName=")
            + desktopString(name) + "\nExec=" + words.join(' ') + "\nPath=" + desktopString(dataDir)
            + "\nIcon=" + desktopString(savedIcon) + '\n';
        created = FS::ensureFilePathExists(savedIcon) && icon.save(savedIcon, "PNG")
            && writeShortcutFile(path, entry.toUtf8(), true);
    }
#endif

    if (!created) {
        if (!savedIcon.isEmpty())
            QFile::remove(savedIcon);
        QMessageBox::critical(this, tr("Create Shortcut"), tr("Could not write the shortcut to %1.").arg(path));
    }
    return created;
}
