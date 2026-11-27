#include "LaunchController.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/MinecraftInstance.h"
#include "Application.h"
#include "BuildConfig.h"

#include "ui/MainWindow.h"
#include "ui/InstanceWindow.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProfileSelectDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/OfflineNameDialog.h"

#include <QLineEdit>
#include <QInputDialog>
#include <QStringList>
#include <QHostInfo>
#include <QList>
#include <QHostAddress>
#include <QPushButton>
#include <QRegularExpression>

#include "BuildConfig.h"
#include "JavaCommon.h"
#include "tasks/Task.h"
#include "minecraft/auth/AccountTask.h"
#include "launch/steps/TextPrint.h"
#include "ui/dialogs/AccountsDialog.h"

LaunchController::LaunchController(QObject *parent) : Task(parent)
{
}

void LaunchController::executeTask()
{
    if (!m_instance)
    {
        emitFailed(tr("No instance specified!"));
        return;
    }

    if(!JavaCommon::checkJVMArgs(m_instance->settings()->get("JvmArgs").toString(), m_parentWidget)) {
        emitFailed(tr("Invalid Java arguments specified. Please fix this first."));
        return;
    }

    login();
}

void LaunchController::decideAccount()
{
    if(m_accountToUse) {
        return;
    }

    auto accounts = APPLICATION->accounts();
    const auto hasAccount = [&accounts] {
        for (int row = 0; row < accounts->count(); ++row) {
            const auto& entry = accounts->at(row);
            if (entry.isAccount && entry.account)
                return true;
        }
        return false;
    };
    if (!hasAccount()) {
        QMessageBox box(QMessageBox::Question, tr("No Accounts"),
            tr("Add a Minecraft account to play the full game. You can try the demo on supported versions."),
            QMessageBox::Cancel, m_parentWidget);
        auto* add = box.addButton(tr("Add Account"), QMessageBox::AcceptRole);
        auto* demo = supportsDemo() ? box.addButton(tr("Play Demo"), QMessageBox::ActionRole) : nullptr;
        box.setDefaultButton(add);
        box.exec();

        if (demo && box.clickedButton() == demo) {
            m_demoRequested = true;
            return;
        }
        if (box.clickedButton() != add)
            return;
        APPLICATION->ShowAccountsDialog(m_parentWidget);
        if (!hasAccount())
            return;
    }

    m_accountToUse = accounts->defaultAccount();
    if (!m_accountToUse)
    {
        // If no default account is set, ask the user which one to use.
        ProfileSelectDialog selectDialog(
            tr("Which account would you like to use?"),
            ProfileSelectDialog::GlobalDefaultCheckbox,
            m_parentWidget
        );

        selectDialog.exec();

        // Launch the instance with the selected account.
        m_accountToUse = selectDialog.selectedAccount();

        // If the user said to use the account as default, do that.
        if (selectDialog.useAsGlobalDefault() && m_accountToUse) {
            accounts->setDefaultAccount(m_accountToUse);
        }
    }
}


bool LaunchController::supportsDemo() const
{
    const auto instance = std::dynamic_pointer_cast<MinecraftInstance>(m_instance);
    return instance && instance->supportsDemo();
}

void LaunchController::launchDemo()
{
    if (!supportsDemo()) {
        const auto message = tr("This Minecraft version does not support demo mode.");
        QMessageBox::information(m_parentWidget, tr("Play Demo"), message);
        emitFailed(message);
        return;
    }

    const auto preferredAccount = m_accountToUse ? m_accountToUse : APPLICATION->accounts()->defaultAccount();
    static const QRegularExpression playerName(QStringLiteral("^[A-Za-z0-9_]{1,16}$"));
    QString name = preferredAccount ? preferredAccount->profileName() : QString();
    if (!playerName.match(name).hasMatch())
        name = QStringLiteral("Player");
    for (;;) {
        bool accepted = false;
        name = QInputDialog::getText(m_parentWidget, tr("Demo Player Name"),
            tr("Enter a player name using 1 to 16 letters, numbers or underscores."),
            QLineEdit::Normal, name, &accepted).trimmed();
        if (!accepted) {
            emitAborted();
            return;
        }
        if (playerName.match(name).hasMatch())
            break;
    }

    m_session = std::make_shared<AuthSession>();
    m_session->MakeDemo(name);
    launchInstance();
}

void LaunchController::login() {
    if (!m_demoRequested)
        decideAccount();
    if (m_demoRequested) {
        launchDemo();
        return;
    }

    // if no account is selected, we bail
    if (!m_accountToUse)
    {
        emitFailed(tr("No account selected for launch."));
        return;
    }

    // we loop until the user succeeds in logging in or gives up
    bool tryagain = true;

    while (tryagain)
    {
        m_session = std::make_shared<AuthSession>();
        m_session->wants_online = m_userWantsOnline;
        m_accountToUse->fillSession(m_session);
        switch(m_accountToUse->accountState()) {
            case AccountState::Offline: {
                m_session->wants_online = false;
                // NOTE: fallthrough is intentional
            }
            case AccountState::Online: {
                if(!m_session->wants_online) {
                    QString usedname;
                    if(m_offlineName.isEmpty()) {
                        // we ask the user for a player name
                        QString lastOfflinePlayerName = APPLICATION->settings()->get("LastOfflinePlayerName").toString();
                        usedname = lastOfflinePlayerName.isEmpty() ? m_session->player_name : lastOfflinePlayerName;
                        OfflineNameDialog dialog(usedname, m_parentWidget);
                        int result = dialog.exec();
                        if(result == OfflineNameDialog::Accepted)
                        {
                            usedname = dialog.textValue();
                            APPLICATION->settings()->set("LastOfflinePlayerName", usedname);
                        }
                        else if (result == OfflineNameDialog::Rejected)
                        {
                            tryagain = false;
                            break;
                        }
                        else if(result == OfflineNameDialog::OnlineRequested)
                        {
                            m_userWantsOnline = true;
                            if(m_accountToUse->accountState() == AccountState::Offline)
                            {
                                auto task = m_accountToUse->refresh();
                                if(task)
                                {
                                    task->start();
                                }
                            }
                            break;
                        }
                    }
                    else {
                        usedname = m_offlineName;
                    }

                    m_session->MakeOffline(usedname);
                    // offline flavored game from here :3
                }
                if(m_accountToUse->ownsMinecraft()) {
                    if(!m_accountToUse->hasProfile()) {
                        QMessageBox box(m_parentWidget);
                        box.setWindowTitle(tr("Account doesn't have a Minecraft profile"));
                        box.setText(tr("The account doesn't have a Minecraft profile yet.\nYou need to create a profile first to play.\n\nDo you want to open the Accounts dialog?"));
                        box.setIcon(QMessageBox::Warning);
                        auto accountsButton = box.addButton(tr("Open Accounts"), QMessageBox::ButtonRole::YesRole);
                        box.addButton(tr("Cancel"), QMessageBox::ButtonRole::NoRole);
                        box.setDefaultButton(accountsButton);

                        box.exec();
                        if(box.clickedButton() == accountsButton) {
                            AccountsDialog dialog(m_parentWidget, m_accountToUse->internalId());
                            dialog.exec();
                            tryagain = true;
                            continue;
                        }
                        else {
                            emitFailed(tr("Launch cancelled - account does not own Minecraft."));
                            return;
                        }
                    }
                    // we own Minecraft, there is a profile, it's all ready to go!
                    launchInstance();
                    return;
                }
                else {
                    // play demo ?
                    QMessageBox box(m_parentWidget);
                    box.setWindowTitle(tr("Play demo?"));
                    box.setText(tr("This account does not own Minecraft.\nYou need to purchase the game first to play it.\n\nDo you want to play the demo?"));
                    box.setIcon(QMessageBox::Warning);
                    auto* demoButton = supportsDemo() ? box.addButton(tr("Play Demo"), QMessageBox::YesRole) : nullptr;
                    if (!demoButton)
                        box.setText(tr("This account does not own Minecraft and this version does not support demo mode."));
                    auto cancelButton = box.addButton(tr("Cancel"), QMessageBox::ButtonRole::NoRole);
                    box.setDefaultButton(cancelButton);

                    box.exec();
                    if (demoButton && box.clickedButton() == demoButton) {
                        launchDemo();
                    }
                    else {
                        emitFailed(tr("Launch cancelled - account does not own Minecraft."));
                    }
                }
                return;
            }
            case AccountState::Errored:
                // This means some sort of soft error that we can fix with a refresh ... so let's refresh.
            case AccountState::Unchecked: {
                m_accountToUse->refresh();
                // NOTE: fallthrough intentional
            }
            case AccountState::Working: {
                // refresh is in progress, we need to wait for it to finish to proceed.
                ProgressDialog progDialog(m_parentWidget);
                if (m_userWantsOnline)
                {
                    progDialog.setSkipButton(true, tr("Play Offline"));
                }
                auto task = m_accountToUse->currentTask();
                progDialog.execWithTask(task.get());
                continue;
            }
            // FIXME: this is missing - the meaning is that the account is queued for refresh and we should wait for that
            /*
            case AccountState::Queued: {
                return;
            }
            */
            case AccountState::Expired: {
                auto errorString = tr("The account has expired and needs to be logged into manually. Press OK to open the accounts window.");
                auto button = QMessageBox::warning(
                    m_parentWidget,
                    tr("Account refresh failed"),
                    errorString,
                    QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel,
                    QMessageBox::StandardButton::Ok
                );
                if (button == QMessageBox::StandardButton::Ok) {
                    auto accounts = APPLICATION->accounts();
                    accounts->removeAccount(m_accountToUse->internalId());
                    AccountsDialog accountsDialog;
                    accountsDialog.exec();
                    emitFailed("The account has expired.");
                    return;
                } else {
                    emitFailed(errorString);
                    return;
                }
            }
            case AccountState::Gone: {
                auto errorString = tr("The account no longer exists on the servers. It may have been migrated, in which case please add the new account you migrated this one to.");
                QMessageBox::warning(
                    m_parentWidget,
                    tr("Account gone"),
                    errorString,
                    QMessageBox::StandardButton::Ok,
                    QMessageBox::StandardButton::Ok
                );
                emitFailed(errorString);
                return;
            }
            case AccountState::MustMigrate: {
                auto errorString = tr("The account must be migrated to a Microsoft account.");
                QMessageBox::warning(
                    m_parentWidget,
                    tr("Account requires migration"),
                    errorString,
                    QMessageBox::StandardButton::Ok,
                    QMessageBox::StandardButton::Ok
                );
                emitFailed(errorString);
                return;
            }
        }
    }
    emitFailed(tr("Failed to launch."));
}

void LaunchController::launchInstance()
{
    Q_ASSERT_X(m_instance != NULL, "launchInstance", "instance is NULL");
    Q_ASSERT_X(m_session.get() != nullptr, "launchInstance", "session is NULL");

    if(!m_instance->reloadSettings())
    {
        QMessageBox::critical(m_parentWidget, tr("Error!"), tr("Couldn't load the instance profile."));
        emitFailed(tr("Couldn't load the instance profile."));
        return;
    }

    m_launcher = m_instance->createLaunchTask(m_session, m_quickPlayTarget);
    if (!m_launcher)
    {
        emitFailed(tr("Couldn't instantiate a launcher."));
        return;
    }

    auto console = qobject_cast<InstanceWindow *>(m_parentWidget);
    auto showConsole = m_instance->settings()->get("ShowConsole").toBool();
    if(!console && showConsole)
    {
        APPLICATION->showInstanceWindow(m_instance);
    }
    connect(m_launcher.get(), &LaunchTask::readyForLaunch, this, &LaunchController::readyForLaunch);
    connect(m_launcher.get(), &LaunchTask::succeeded, this, &LaunchController::onSucceeded);
    connect(m_launcher.get(), &LaunchTask::failed, this,  &LaunchController::onFailed);
    connect(m_launcher.get(), &LaunchTask::requestProgress, this, &LaunchController::onProgressRequested);

    // Prepend Online and Auth Status
    QString online_mode;
    if (m_session->demo) {
        online_mode = "demo";
    } else if(m_session->wants_online) {
        online_mode = "online";

        // Prepend Server Status
        QStringList servers = {QUrl(BuildConfig.SESSION_BASE).host(), QUrl(BuildConfig.TEXTURE_BASE).host(), QUrl(BuildConfig.API_BASE).host()};
        QString resolved_servers = "";
        QHostInfo host_info;

        for(QString server : servers) {
            host_info = QHostInfo::fromName(server);
            resolved_servers = resolved_servers + server + " resolves to:\n    [";
            if(!host_info.addresses().isEmpty()) {
                for(QHostAddress address : host_info.addresses()) {
                    resolved_servers = resolved_servers + address.toString();
                    if(!host_info.addresses().endsWith(address)) {
                        resolved_servers = resolved_servers + ", ";
                    }
                }
            } else {
                resolved_servers = resolved_servers + "N/A";
            }
            resolved_servers = resolved_servers + "]\n\n";
        }
        m_launcher->prependStep(new TextPrint(m_launcher.get(), resolved_servers, MessageLevel::Launcher));
    } else {
        online_mode = "offline";
    }

    m_launcher->prependStep(new TextPrint(m_launcher.get(), "Launched instance in " + online_mode + " mode\n", MessageLevel::Launcher));

    // Prepend Version
    m_launcher->prependStep(new TextPrint(m_launcher.get(), BuildConfig.LAUNCHER_NAME + " version: " + BuildConfig.printableVersionString() + "\n\n", MessageLevel::Launcher));
    m_launcher->start();
}

void LaunchController::readyForLaunch()
{
    if (!m_profiler)
    {
        m_launcher->proceed();
        return;
    }

    QString error;
    if (!m_profiler->check(&error))
    {
        m_launcher->abort();
        QMessageBox::critical(m_parentWidget, tr("Error!"), tr("Couldn't start profiler: %1").arg(error));
        emitFailed("Profiler startup failed!");
        return;
    }
    BaseProfiler *profilerInstance = m_profiler->createProfiler(m_launcher->instance(), this);

    connect(profilerInstance, &BaseProfiler::readyToLaunch, [this](const QString & message)
    {
        QMessageBox msg;
        msg.setText(tr("The game launch is delayed until you press the "
                        "button. This is the right time to setup the profiler, as the "
                        "profiler server is running now.\n\n%1").arg(message));
        msg.setWindowTitle(tr("Waiting."));
        msg.setIcon(QMessageBox::Information);
        msg.addButton(tr("Launch"), QMessageBox::AcceptRole);
        msg.setModal(true);
        msg.exec();
        m_launcher->proceed();
    });
    connect(profilerInstance, &BaseProfiler::abortLaunch, [this](const QString & message)
    {
        QMessageBox msg;
        msg.setText(tr("Couldn't start the profiler: %1").arg(message));
        msg.setWindowTitle(tr("Error"));
        msg.setIcon(QMessageBox::Critical);
        msg.addButton(QMessageBox::Ok);
        msg.setModal(true);
        msg.exec();
        m_launcher->abort();
        emitFailed("Profiler startup failed!");
    });
    profilerInstance->beginProfiling(m_launcher);
}

void LaunchController::onSucceeded()
{
    emitSucceeded();
}

void LaunchController::onFailed(QString reason)
{
    if(m_instance->settings()->get("ShowConsoleOnError").toBool())
    {
        APPLICATION->showInstanceWindow(m_instance, "console");
    }
    emitFailed(reason);
}

void LaunchController::onProgressRequested(Task* task)
{
    ProgressDialog progDialog(m_parentWidget);
    progDialog.setSkipButton(true, tr("Stop"));
    m_launcher->proceed();
    progDialog.execWithTask(task);
}

bool LaunchController::abort()
{
    if(!m_launcher)
    {
        return true;
    }
    if(!m_launcher->canAbort())
    {
        return false;
    }
    auto response = CustomMessageBox::selectable(
            m_parentWidget, tr("Kill Minecraft?"),
            tr("This can cause the instance to get corrupted and should only be used if Minecraft "
            "is frozen for some reason"),
            QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)->exec();
    if (response == QMessageBox::Yes)
    {
        return m_launcher->abort();
    }
    return false;
}
