// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

/*
 * A launch's progress: 0..1, or busy without a figure when the step cannot
 * say how far it is (progress below 0).
 *
 * The desktop style draws the bar with the platform's widget style, which
 * on macOS asks for no height of its own, leaving a bar of none; the height
 * below is a floor for that, and stays below Breeze's own.
 */
QQC2.ProgressBar {
    property real progress: -1

    from: 0
    to: 1
    value: Math.max(0, progress)
    indeterminate: progress < 0
    implicitHeight: Math.max(implicitBackgroundHeight, Kirigami.Units.smallSpacing * 2) + topInset + bottomInset
}
