/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * LogIngester — pulls the latest log + crash-report text out of an
 * instance for the analysis engine to chew on.
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"

class LogIngester
{
  public:
	struct Bundle {
		QString combinedText;
		QStringList sources; // file paths we read
		bool fromLatestLog = false;
		bool fromCrashReport = false;
	};

	Bundle ingestForInstance(const QString& instancePath) const;

	/* For testing — pull from an arbitrary path or directory. */
	Bundle ingestFromPath(const QString& path) const;
};
