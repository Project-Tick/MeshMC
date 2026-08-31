/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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