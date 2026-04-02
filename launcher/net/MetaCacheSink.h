/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "FileSink.h"
#include "ChecksumValidator.h"
#include "net/HttpMetaCache.h"

namespace Net
{
	class MetaCacheSink : public FileSink
	{
	  public: /* con/des */
		MetaCacheSink(MetaEntryPtr entry, ChecksumValidator* md5sum);
		virtual ~MetaCacheSink();
		bool hasLocalData() override;

	  protected: /* methods */
		JobStatus initCache(QNetworkRequest& request) override;
		JobStatus finalizeCache(QNetworkReply& reply) override;

	  private: /* data */
		MetaEntryPtr m_entry;
		ChecksumValidator* m_md5Node;
	};
} // namespace Net
