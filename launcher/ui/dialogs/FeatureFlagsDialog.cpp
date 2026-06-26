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
 */

#include "FeatureFlagsDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "featureflags/FeatureFlags.h"

FeatureFlagsDialog::FeatureFlagsDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Feature Flags"));
	resize(640, 420);

	auto* layout = new QVBoxLayout(this);

	m_endpointLabel = new QLabel(this);
	m_endpointLabel->setTextFormat(Qt::RichText);
	m_endpointLabel->setOpenExternalLinks(true);
	m_endpointLabel->setWordWrap(true);
	layout->addWidget(m_endpointLabel);

	m_statusLabel = new QLabel(this);
	m_statusLabel->setWordWrap(true);
	layout->addWidget(m_statusLabel);

	m_table = new QTableWidget(this);
	m_table->setColumnCount(5);
	m_table->setHorizontalHeaderLabels(
		{ tr("Flag"), tr("Toggle"), tr("Effective"), tr("Strategies"),
		  tr("Override") });
	m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
	m_table->verticalHeader()->setVisible(false);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setShowGrid(false);
	m_table->setFocusPolicy(Qt::NoFocus);
	layout->addWidget(m_table, 1);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	m_refreshButton =
		m_buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
	layout->addWidget(m_buttons);

	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
	connect(m_refreshButton, &QPushButton::clicked, this,
			&FeatureFlagsDialog::requestRefresh);

	// Re-render whenever a refresh lands while the dialog is open.
	if (auto* ff = FeatureFlags::instance()) {
		// Queued so a reload triggered from inside a cell-widget callback
		// (e.g. changing an override combo, which emits updated()) runs after
		// that callback returns, never deleting the combo mid-signal.
		connect(ff, &FeatureFlags::updated, this,
				&FeatureFlagsDialog::reloadTable, Qt::QueuedConnection);
	}

	reloadTable();
}

void FeatureFlagsDialog::requestRefresh()
{
	if (auto* ff = FeatureFlags::instance()) {
		ff->refresh();
	}
	// reloadTable() will be triggered by the updated() signal on success; we
	// also refresh the status line immediately to reflect the attempt.
	reloadTable();
}

void FeatureFlagsDialog::reloadTable()
{
	auto* ff = FeatureFlags::instance();

	if (!ff || !ff->isConfigured()) {
		m_endpointLabel->setText(
			tr("Feature flags are <b>not configured</b> for this build "
			   "(no Unleash instance id). All flags fall back to their "
			   "compiled-in defaults."));
		m_statusLabel->clear();
		m_table->setRowCount(0);
		m_refreshButton->setEnabled(false);
		return;
	}

	m_refreshButton->setEnabled(true);

	const QString url = ff->endpointUrl();
	m_endpointLabel->setText(
		tr("Endpoint: <a href=\"%1\">%1</a><br>Application: <b>%2</b>")
			.arg(url.toHtmlEscaped(), ff->appName().toHtmlEscaped()));

	const QList<FeatureFlagState> flags = ff->allFlags();

	if (!ff->hasData()) {
		m_statusLabel->setText(
			tr("No feature document loaded yet (offline, or not fetched). "
			   "Flags use their compiled-in defaults. Press Refresh to try "
			   "fetching."));
	} else {
		const QDateTime updated = ff->lastUpdated();
		m_statusLabel->setText(
			tr("%n flag(s) loaded. Last updated: %1", "", flags.size())
				.arg(updated.isValid()
						 ? updated.toString(Qt::TextDate)
						 : tr("unknown")));
	}

	m_table->setRowCount(flags.size());
	for (int row = 0; row < flags.size(); ++row) {
		const FeatureFlagState& f = flags.at(row);

		auto* nameItem = new QTableWidgetItem(f.name);
		auto* toggleItem = new QTableWidgetItem(
			f.toggleEnabled ? tr("enabled") : tr("disabled"));
		auto* effItem = new QTableWidgetItem(
			f.effective ? tr("ON") : tr("OFF"));
		auto* stratItem = new QTableWidgetItem(f.strategies);

		// Tint the effective column so on/off is scannable at a glance.
		effItem->setForeground(f.effective ? QColor(0x2e, 0x7d, 0x32)
											: QColor(0xc6, 0x28, 0x28));
		// When a local override is forcing the value, mark the effective cell
		// so it is clear the value did not come straight from the backend.
		if (f.override != FlagOverride::Auto) {
			QFont ef = effItem->font();
			ef.setBold(true);
			effItem->setFont(ef);
			effItem->setToolTip(tr("Forced by a local override."));
		}

		m_table->setItem(row, 0, nameItem);
		m_table->setItem(row, 1, toggleItem);
		m_table->setItem(row, 2, effItem);
		m_table->setItem(row, 3, stratItem);

		// Override selector: Auto / Force ON / Force OFF.
		auto* combo = new QComboBox(m_table);
		combo->addItem(tr("Auto"), static_cast<int>(FlagOverride::Auto));
		combo->addItem(tr("Force ON"), static_cast<int>(FlagOverride::ForceOn));
		combo->addItem(tr("Force OFF"), static_cast<int>(FlagOverride::ForceOff));
		combo->setCurrentIndex(static_cast<int>(f.override));
		const QString flagName = f.name;
		connect(combo, &QComboBox::currentIndexChanged, this,
				[flagName](int index) {
					if (auto* ff = FeatureFlags::instance()) {
						ff->setOverride(flagName,
										static_cast<FlagOverride>(index));
					}
				});
		m_table->setCellWidget(row, 4, combo);
	}
}
