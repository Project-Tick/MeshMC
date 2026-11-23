/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
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
