/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QString>

namespace InstanceShortcuts
{
bool record(const QString& instanceRoot, const QString& shortcutPath);
int countOwned(const QString& instanceRoot);
void removeOwned(const QString& instanceRoot);
}
