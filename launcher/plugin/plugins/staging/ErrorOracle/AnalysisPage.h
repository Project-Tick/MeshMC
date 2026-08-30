/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * AnalysisPage — instance page that runs the rule engine over the
 * instance's most recent log, shows ranked suggestions, and lets the
 * user mark which suggestion actually fixed the problem so the
 * LearningStore can re-rank future suggestions.
 */

#pragma once

#include "plugin/sdk/mmco_cxx_sdk.hpp"
#include "RuleEngine.h"

class QTextEdit;

class AnalysisPage : public QWidget, public BasePage
{
	Q_OBJECT
  public:
	AnalysisPage(const QString& instanceId, const QString& instanceRoot,
				 RuleEngine* engine, class LearningStore* learning,
				 QWidget* parent = nullptr);

	QString id() const override
	{
		return QStringLiteral("error-oracle");
	}
	QString displayName() const override
	{
		return QObject::tr("Error Analysis");
	}
	QIcon icon() const override
	{
		return QIcon::fromTheme(QStringLiteral("status-bad"));
	}

  private slots:
	void onReanalyse();
	void onHelped();
	void onDidNotHelp();
	void onPromoteNovel();
	void onSelectionChanged();

  private:
	void buildUi();
	void runAnalysis();
	Match selectedMatch() const;

	QString m_instanceId;
	QString m_instanceRoot;
	RuleEngine* m_engine = nullptr;
	class LearningStore* m_learning = nullptr;
	QList<Match> m_matches;
	QString m_currentFingerprint;
	QString m_currentSampleLine;

	QTreeWidget* m_tree = nullptr;
	QTextEdit* m_adviceView = nullptr;
	QLabel* m_summaryLabel = nullptr;
	QPushButton* m_helpedBtn = nullptr;
	QPushButton* m_didntBtn = nullptr;
	QPushButton* m_promoteBtn = nullptr;
};
