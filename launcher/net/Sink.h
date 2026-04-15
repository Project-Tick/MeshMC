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

#include "net/NetAction.h"

#include "Validator.h"

namespace Net
{
	class Sink
	{
	  public: /* con/des */
		Sink() {};
		virtual ~Sink() {};

	  public: /* methods */
		virtual JobStatus init(QNetworkRequest& request) = 0;
		virtual JobStatus write(QByteArray& data) = 0;
		virtual JobStatus abort() = 0;
		virtual JobStatus finalize(QNetworkReply& reply) = 0;
		virtual bool hasLocalData() = 0;

		void addValidator(Validator* validator)
		{
			if (validator) {
				validators.push_back(std::shared_ptr<Validator>(validator));
			}
		}

	  protected: /* methods */
		bool finalizeAllValidators(QNetworkReply& reply)
		{
			for (auto& validator : validators) {
				if (!validator->validate(reply))
					return false;
			}
			return true;
		}
		bool failAllValidators()
		{
			bool success = true;
			for (auto& validator : validators) {
				success &= validator->abort();
			}
			return success;
		}
		bool initAllValidators(QNetworkRequest& request)
		{
			for (auto& validator : validators) {
				if (!validator->init(request))
					return false;
			}
			return true;
		}
		bool writeAllValidators(QByteArray& data)
		{
			for (auto& validator : validators) {
				if (!validator->write(data))
					return false;
			}
			return true;
		}

	  protected: /* data */
		std::vector<std::shared_ptr<Validator>> validators;
	};
} // namespace Net
