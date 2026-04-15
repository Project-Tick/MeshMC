/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#pragma once

#include "Validator.h"
#include <QCryptographicHash>
#include <memory>
#include <QFile>

namespace Net
{
	class ChecksumValidator : public Validator
	{
	  public: /* con/des */
		ChecksumValidator(QCryptographicHash::Algorithm algorithm,
						  QByteArray expected = QByteArray())
			: m_checksum(algorithm), m_expected(expected) {};
		virtual ~ChecksumValidator() {};

	  public: /* methods */
		bool init(QNetworkRequest&) override
		{
			m_checksum.reset();
			return true;
		}
		bool write(QByteArray& data) override
		{
			m_checksum.addData(data);
			return true;
		}
		bool abort() override
		{
			return true;
		}
		bool validate(QNetworkReply&) override
		{
			if (m_expected.size() && m_expected != hash()) {
				qWarning() << "Checksum mismatch, download is bad.";
				return false;
			}
			return true;
		}
		QByteArray hash()
		{
			return m_checksum.result();
		}
		void setExpected(QByteArray expected)
		{
			m_expected = expected;
		}

	  private: /* data */
		QCryptographicHash m_checksum;
		QByteArray m_expected;
	};
} // namespace Net