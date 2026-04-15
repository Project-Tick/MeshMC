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

#include "MSA.h"

#include "minecraft/auth/steps/MSAStep.h"
#include "minecraft/auth/steps/XboxUserStep.h"
#include "minecraft/auth/steps/XboxAuthorizationStep.h"
#include "minecraft/auth/steps/MeshMCLoginStep.h"
#include "minecraft/auth/steps/XboxProfileStep.h"
#include "minecraft/auth/steps/EntitlementsStep.h"
#include "minecraft/auth/steps/MinecraftProfileStep.h"
#include "minecraft/auth/steps/GetSkinStep.h"

MSASilent::MSASilent(AccountData* data, QObject* parent)
	: AuthFlow(data, parent)
{
	m_steps.append(new MSAStep(m_data, MSAStep::Action::Refresh));
	m_steps.append(new XboxUserStep(m_data));
	m_steps.append(new XboxAuthorizationStep(m_data, &m_data->xboxApiToken,
											 "http://xboxlive.com", "Xbox"));
	m_steps.append(
		new XboxAuthorizationStep(m_data, &m_data->mojangservicesToken,
								  "rp://api.minecraftservices.com/", "Mojang"));
	m_steps.append(new MeshMCLoginStep(m_data));
	m_steps.append(new XboxProfileStep(m_data));
	m_steps.append(new EntitlementsStep(m_data));
	m_steps.append(new MinecraftProfileStep(m_data));
	m_steps.append(new GetSkinStep(m_data));
}

MSAInteractive::MSAInteractive(AccountData* data, QObject* parent)
	: AuthFlow(data, parent)
{
	m_steps.append(new MSAStep(m_data, MSAStep::Action::Login));
	m_steps.append(new XboxUserStep(m_data));
	m_steps.append(new XboxAuthorizationStep(m_data, &m_data->xboxApiToken,
											 "http://xboxlive.com", "Xbox"));
	m_steps.append(
		new XboxAuthorizationStep(m_data, &m_data->mojangservicesToken,
								  "rp://api.minecraftservices.com/", "Mojang"));
	m_steps.append(new MeshMCLoginStep(m_data));
	m_steps.append(new XboxProfileStep(m_data));
	m_steps.append(new EntitlementsStep(m_data));
	m_steps.append(new MinecraftProfileStep(m_data));
	m_steps.append(new GetSkinStep(m_data));
}
