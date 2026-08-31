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

#include "sys.h"

#include <windows.h>

#include <QOperatingSystemVersion>

Sys::KernelInfo Sys::getKernelInfo()
{
	Sys::KernelInfo out;
	out.kernelType = KernelType::Windows;
	out.kernelName = "Windows";
	const auto osVersion = QOperatingSystemVersion::current();
	out.kernelMajor = osVersion.majorVersion();
	out.kernelMinor = osVersion.minorVersion();
	out.kernelPatch = osVersion.microVersion();
	out.kernelVersion = QString("%1.%2.%3")
							.arg(out.kernelMajor)
							.arg(out.kernelMinor)
							.arg(out.kernelPatch);
	return out;
}

uint64_t Sys::getSystemRam()
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	// bytes
	return (uint64_t)status.ullTotalPhys;
}

bool Sys::isSystem64bit()
{
#if defined(_WIN64)
	return true;
#elif defined(_WIN32)
	BOOL f64 = false;
	return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#else
	// it's some other kind of system...
	return false;
#endif
}

bool Sys::isCPU64bit()
{
	SYSTEM_INFO info;
	ZeroMemory(&info, sizeof(SYSTEM_INFO));
	GetNativeSystemInfo(&info);
	auto arch = info.wProcessorArchitecture;
	return arch == PROCESSOR_ARCHITECTURE_AMD64 ||
		   arch == PROCESSOR_ARCHITECTURE_IA64;
}

Sys::DistributionInfo Sys::getDistributionInfo()
{
	DistributionInfo result;
	return result;
}
