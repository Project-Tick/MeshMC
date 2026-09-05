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

namespace Sys
{
	const uint64_t mebibyte = 1024ull * 1024ull;

	enum class KernelType { Undetermined, Windows, Darwin, Linux };

	struct KernelInfo {
		QString kernelName;
		QString kernelVersion;

		KernelType kernelType = KernelType::Undetermined;
		int kernelMajor = 0;
		int kernelMinor = 0;
		int kernelPatch = 0;
		bool isCursed = false;
	};

	KernelInfo getKernelInfo();

	struct DistributionInfo {
		DistributionInfo operator+(const DistributionInfo& rhs) const
		{
			DistributionInfo out;
			if (!distributionName.isEmpty()) {
				out.distributionName = distributionName;
			} else {
				out.distributionName = rhs.distributionName;
			}
			if (!distributionVersion.isEmpty()) {
				out.distributionVersion = distributionVersion;
			} else {
				out.distributionVersion = rhs.distributionVersion;
			}
			return out;
		}
		QString distributionName;
		QString distributionVersion;
	};

	DistributionInfo getDistributionInfo();

	uint64_t getSystemRam();

	bool isSystem64bit();

	bool isCPU64bit();

	struct LsbInfo {
		QString distributor;
		QString version;
		QString description;
		QString codename;
	};

	bool main_lsb_info(LsbInfo& out);
	bool fallback_lsb_info(Sys::LsbInfo& out);
	void lsb_postprocess(Sys::LsbInfo& lsb, Sys::DistributionInfo& out);
	Sys::DistributionInfo read_lsb_release();

	QString _extract_distribution(const QString& x);
	QString _extract_version(const QString& x);
	Sys::DistributionInfo read_legacy_release();

	Sys::DistributionInfo read_os_release();
} // namespace Sys
