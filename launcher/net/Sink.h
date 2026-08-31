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
