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

#include <QString>
#include <QDateTime>
#include <QMap>
#include <QVariantMap>

namespace Katabasis
{
	enum class Activity {
		Idle,
		LoggingIn,
		LoggingOut,
		Refreshing,
		FailedSoft, //!< soft failure. this generally means the user auth
					//!< details haven't been invalidated
		FailedHard, //!< hard failure. auth is invalid
		FailedGone, //!< hard failure. auth is invalid, and the account no
					//!< longer exists
		Succeeded
	};

	enum class Validity { None, Assumed, Certain };

	struct Token {
		QDateTime issueInstant;
		QDateTime notAfter;
		QString token;
		QString refresh_token;
		QVariantMap extra;

		Validity validity = Validity::None;
		bool persistent = true;
	};

} // namespace Katabasis
