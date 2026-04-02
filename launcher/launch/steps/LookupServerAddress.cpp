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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "LookupServerAddress.h"

#include <launch/LaunchTask.h>

LookupServerAddress::LookupServerAddress(LaunchTask* parent)
	: LaunchStep(parent), m_dnsLookup(new QDnsLookup(this))
{
	connect(m_dnsLookup, &QDnsLookup::finished, this,
			&LookupServerAddress::on_dnsLookupFinished);

	m_dnsLookup->setType(QDnsLookup::SRV);
}

void LookupServerAddress::setLookupAddress(const QString& lookupAddress)
{
	m_lookupAddress = lookupAddress;
	m_dnsLookup->setName(QString("_minecraft._tcp.%1").arg(lookupAddress));
}

void LookupServerAddress::setOutputAddressPtr(MinecraftServerTargetPtr output)
{
	m_output = std::move(output);
}

bool LookupServerAddress::abort()
{
	m_dnsLookup->abort();
	emitFailed("Aborted");
	return true;
}

void LookupServerAddress::executeTask()
{
	m_dnsLookup->lookup();
}

void LookupServerAddress::on_dnsLookupFinished()
{
	if (isFinished()) {
		// Aborted
		return;
	}

	if (m_dnsLookup->error() != QDnsLookup::NoError) {
		emit logLine(QString("Failed to resolve server address (this is NOT an "
							 "error!) %1: %2\n")
						 .arg(m_dnsLookup->name(), m_dnsLookup->errorString()),
					 MessageLevel::MeshMC);
		resolve(m_lookupAddress,
				25565); // Technically the task failed, however, we don't abort
						// the launch and leave it up to minecraft to fail (or
						// maybe not) when connecting
		return;
	}

	const auto records = m_dnsLookup->serviceRecords();
	if (records.empty()) {
		emit logLine(
			QString("Failed to resolve server address %1: the DNS lookup "
					"succeeded, but no records were returned.\n")
				.arg(m_dnsLookup->name()),
			MessageLevel::Warning);
		resolve(m_lookupAddress,
				25565); // Technically the task failed, however, we don't abort
						// the launch and leave it up to minecraft to fail (or
						// maybe not) when connecting
		return;
	}

	const auto& firstRecord = records.at(0);
	quint16 port = firstRecord.port();

	emit logLine(QString("Resolved server address %1 to %2 with port %3\n")
					 .arg(m_dnsLookup->name(), firstRecord.target(),
						  QString::number(port)),
				 MessageLevel::MeshMC);
	resolve(firstRecord.target(), port);
}

void LookupServerAddress::resolve(const QString& address, quint16 port)
{
	m_output->address = address;
	m_output->port = port;

	emitSucceeded();
	m_dnsLookup->deleteLater();
}
