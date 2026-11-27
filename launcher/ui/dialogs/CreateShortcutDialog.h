/*
 * Copyright 2022 arthomnix
 * Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QDialog>
#include "BaseInstance.h"

namespace Ui
{
    class CreateShortcutDialog;
}

class CreateShortcutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateShortcutDialog(QWidget *parent = nullptr, InstancePtr instance = nullptr);
    ~CreateShortcutDialog() override;

private
slots:
    void on_shortcutPathBrowse_clicked();
    void updateDialogState();
    void accept() override;

private:
    Ui::CreateShortcutDialog *ui;
    InstancePtr m_instance;

    QString m_iconKey;

    QString shortcutSuffix() const;
    void browseShortcutPath(const QString& directory);
    QStringList launchArguments(const QString& dataDir) const;
    bool createShortcut(const QString& path, const QString& suffix);
};
