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

enum class ProblemSeverity { None, Warning, Error };

struct PatchProblem {
	ProblemSeverity m_severity;
	QString m_description;
};

class ProblemProvider
{
  public:
	virtual ~ProblemProvider() {};
	virtual const QList<PatchProblem> getProblems() const = 0;
	virtual ProblemSeverity getProblemSeverity() const = 0;
};

class ProblemContainer : public ProblemProvider
{
  public:
	const QList<PatchProblem> getProblems() const override
	{
		return m_problems;
	}
	ProblemSeverity getProblemSeverity() const override
	{
		return m_problemSeverity;
	}
	virtual void addProblem(ProblemSeverity severity,
							const QString& description)
	{
		if (severity > m_problemSeverity) {
			m_problemSeverity = severity;
		}
		m_problems.append({severity, description});
	}

  private:
	QList<PatchProblem> m_problems;
	ProblemSeverity m_problemSeverity = ProblemSeverity::None;
};
