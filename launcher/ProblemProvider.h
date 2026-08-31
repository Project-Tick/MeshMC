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
