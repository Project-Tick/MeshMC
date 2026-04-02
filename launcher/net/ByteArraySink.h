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

#include "Sink.h"

namespace Net
{
	/*
	 * Sink object for downloads that uses an external QByteArray it doesn't own
	 * as a target.
	 */
	class ByteArraySink : public Sink
	{
	  public:
		ByteArraySink(QByteArray* output)
			: m_output(output) {
				  // nil
			  };

		virtual ~ByteArraySink()
		{
			// nil
		}

	  public:
		JobStatus init(QNetworkRequest& request) override
		{
			m_output->clear();
			if (initAllValidators(request))
				return Job_InProgress;
			return Job_Failed;
		};

		JobStatus write(QByteArray& data) override
		{
			m_output->append(data);
			if (writeAllValidators(data))
				return Job_InProgress;
			return Job_Failed;
		}

		JobStatus abort() override
		{
			m_output->clear();
			failAllValidators();
			return Job_Failed;
		}

		JobStatus finalize(QNetworkReply& reply) override
		{
			if (finalizeAllValidators(reply))
				return Job_Finished;
			return Job_Failed;
		}

		bool hasLocalData() override
		{
			return false;
		}

	  private:
		QByteArray* m_output;
	};
} // namespace Net
