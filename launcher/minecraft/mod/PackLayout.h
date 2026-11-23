/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QFileInfo>

namespace PackLayout
{
	bool isShaderPack(const QFileInfo& file);
	bool isDataPack(const QFileInfo& file);
} // namespace PackLayout
