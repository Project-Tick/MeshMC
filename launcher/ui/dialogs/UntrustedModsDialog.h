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

#include <QDialog>
#include <QStringList>

namespace Ui
{
	class UntrustedModsDialog;
}

/* Consent prompt for installing code the launcher cannot vouch for.
 *
 * A modpack may name mod downloads on any host it likes, and it may
 * simply carry jars inside the archive. Either way the game will load
 * them as code, with the user's permissions. When the pack did not come
 * from one of the catalogue browsers, that is a decision only the user
 * can make, so it is put to them.
 *
 * This is a dialog of its own rather than a message box with the list
 * hidden behind "Show Details" for one reason: the list is the
 * information. A prompt whose evidence is one click away, and whose
 * accept button can be hit before the text has been read, measures
 * whether the user can dismiss a dialog rather than whether they
 * consent. So the files are visible, and accepting takes a deliberate
 * second action - ticking a box that does not even become available for
 * the first few seconds.
 */
class UntrustedModsDialog : public QDialog
{
	Q_OBJECT
  public:
	/* @p paths are the files in question, as instance-relative paths. */
	explicit UntrustedModsDialog(const QStringList& paths,
								 QWidget* parent = nullptr);
	~UntrustedModsDialog() override;

  private:
	Ui::UntrustedModsDialog* m_ui;
};
