/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "AnalysisPage.h"
#include "LogIngester.h"
#include "LearningStore.h"

#include <QSplitter>
#include <QTextEdit>

namespace
{
const char* severityName(Severity s)
{
	switch (s) {
		case Severity::High:
			return "high";
		case Severity::Medium:
			return "medium";
		case Severity::Low:
		default:
			return "low";
	}
}
} // namespace

AnalysisPage::AnalysisPage(InstancePtr instance, RuleEngine* engine,
						   LearningStore* learning, QWidget* parent)
	: QWidget(parent), m_instance(instance), m_engine(engine),
	  m_learning(learning)
{
	buildUi();
	runAnalysis();
}

void AnalysisPage::buildUi()
{
	auto* root = new QVBoxLayout(this);

	m_summaryLabel = new QLabel(this);
	m_summaryLabel->setWordWrap(true);
	root->addWidget(m_summaryLabel);

	auto* splitter = new QSplitter(Qt::Horizontal, this);

	m_tree = new QTreeWidget(splitter);
	m_tree->setHeaderLabels(
		{tr("Severity"), tr("Title"), tr("Line"), tr("Score")});
	m_tree->setRootIsDecorated(false);
	m_tree->setAlternatingRowColors(true);
	connect(m_tree, &QTreeWidget::itemSelectionChanged, this,
			&AnalysisPage::onSelectionChanged);
	splitter->addWidget(m_tree);

	m_adviceView = new QTextEdit(splitter);
	m_adviceView->setReadOnly(true);
	splitter->addWidget(m_adviceView);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 2);

	root->addWidget(splitter, 1);

	auto* btnRow = new QHBoxLayout();

	auto* reanalyse = new QPushButton(tr("Re-analyse"), this);
	connect(reanalyse, &QPushButton::clicked, this, &AnalysisPage::onReanalyse);
	btnRow->addWidget(reanalyse);

	btnRow->addStretch();

	m_helpedBtn = new QPushButton(tr("✓ This fixed it"), this);
	m_helpedBtn->setEnabled(false);
	connect(m_helpedBtn, &QPushButton::clicked, this, &AnalysisPage::onHelped);
	btnRow->addWidget(m_helpedBtn);

	m_didntBtn = new QPushButton(tr("✗ Didn't help"), this);
	m_didntBtn->setEnabled(false);
	connect(m_didntBtn, &QPushButton::clicked, this,
			&AnalysisPage::onDidNotHelp);
	btnRow->addWidget(m_didntBtn);

	m_promoteBtn = new QPushButton(tr("Promote unknown error → rule…"), this);
	m_promoteBtn->setEnabled(false);
	m_promoteBtn->setToolTip(
		tr("If no rule matched but the same crash signature has appeared "
		   "more than once, you can teach ErrorOracle about it by writing "
		   "a custom title + advice."));
	connect(m_promoteBtn, &QPushButton::clicked, this,
			&AnalysisPage::onPromoteNovel);
	btnRow->addWidget(m_promoteBtn);

	root->addLayout(btnRow);
}

void AnalysisPage::runAnalysis()
{
	LogIngester ing;
	auto bundle = ing.ingestForInstance(m_instance->instanceRoot());

	m_matches = m_engine->analyse(bundle.combinedText);

	// Score each rule via LearningStore.
	for (auto& m : m_matches) {
		m.score = m_learning->scoreFor(m.ruleId, m_instance->id());
		m_learning->recordSeen(m.ruleId, m_instance->id());
	}
	std::sort(m_matches.begin(), m_matches.end(),
			  [](const Match& a, const Match& b) {
				  if (a.severity != b.severity)
					  return int(a.severity) > int(b.severity);
				  return a.score > b.score;
			  });

	// Compute & remember a fingerprint of the failure for the
	// "promote to rule" affordance.
	m_currentFingerprint = LearningStore::fingerprint(bundle.combinedText);
	m_currentSampleLine.clear();

	// Pull a representative line for the novel promotion UI:
	// first matching `Exception in thread` or first `at <class>.<method>`.
	QRegularExpression re(QStringLiteral("(Exception in thread.*|at\\s+[A-Za-z][\\w.$]+)"));
	auto it = re.match(bundle.combinedText);
	if (it.hasMatch())
		m_currentSampleLine = it.captured(1).left(160);

	m_tree->clear();
	for (const auto& m : m_matches) {
		auto* item = new QTreeWidgetItem(m_tree);
		item->setText(0, QString::fromLatin1(severityName(m.severity)));
		item->setText(1, m.ruleTitle);
		item->setText(2, m.line >= 0 ? QString::number(m.line) : QString("?"));
		item->setText(3, QString::number(m.score, 'f', 2));
		item->setData(0, Qt::UserRole, m.ruleId);
	}
	m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	// Record novel fingerprint if no rule fired but we have a signature.
	if (m_matches.isEmpty() && !m_currentFingerprint.isEmpty()) {
		m_learning->recordNovel(m_currentFingerprint, m_currentSampleLine,
								m_instance->id());
		m_learning->save();
		m_promoteBtn->setEnabled(true);
	} else {
		m_promoteBtn->setEnabled(false);
	}

	// Summary text
	QString src = bundle.sources.isEmpty()
					  ? tr("(no logs found)")
					  : bundle.sources.join(QStringLiteral(", "));
	m_summaryLabel->setText(
		tr("Analysed: <b>%1</b>. %2 rule match(es). Crash signature: "
		   "<code>%3</code>")
			.arg(src.toHtmlEscaped())
			.arg(m_matches.size())
			.arg(m_currentFingerprint.isEmpty()
					 ? tr("(no stack trace)")
					 : m_currentFingerprint));

	if (m_matches.isEmpty()) {
		m_adviceView->setMarkdown(
			tr("### No matching rules\n\n"
			   "Either the instance ran cleanly or the crash isn't in "
			   "ErrorOracle's rule pack yet. "
			   "If the same signature shows up more than once you can "
			   "promote it into a rule using the button below.\n"));
	}
}

Match AnalysisPage::selectedMatch() const
{
	auto items = m_tree->selectedItems();
	if (items.isEmpty())
		return {};
	QString id = items.first()->data(0, Qt::UserRole).toString();
	for (const auto& m : m_matches)
		if (m.ruleId == id)
			return m;
	return {};
}

void AnalysisPage::onSelectionChanged()
{
	auto m = selectedMatch();
	bool enable = !m.ruleId.isEmpty();
	m_helpedBtn->setEnabled(enable);
	m_didntBtn->setEnabled(enable);
	if (!enable)
		return;
	QString matchLine = m.matchedLine.toHtmlEscaped();
	QString md = QStringLiteral("### %1\n\n").arg(m.ruleTitle) +
				 m.advice + QStringLiteral("\n\n---\n\n**Matched line:** `") +
				 m.matchedLine + QStringLiteral("`\n");
	m_adviceView->setMarkdown(md);
}

void AnalysisPage::onReanalyse()
{
	runAnalysis();
}

void AnalysisPage::onHelped()
{
	auto m = selectedMatch();
	if (m.ruleId.isEmpty())
		return;
	m_learning->recordHelped(m.ruleId, m_instance->id());
	m_learning->save();
	m_summaryLabel->setText(tr("Recorded: rule <b>%1</b> helped on this "
							   "instance.")
								.arg(m.ruleTitle));
}

void AnalysisPage::onDidNotHelp()
{
	auto m = selectedMatch();
	if (m.ruleId.isEmpty())
		return;
	m_learning->recordDidNotHelp(m.ruleId, m_instance->id());
	m_learning->save();
	m_summaryLabel->setText(tr("Recorded: rule <b>%1</b> did not help.")
								.arg(m.ruleTitle));
}

void AnalysisPage::onPromoteNovel()
{
	if (m_currentFingerprint.isEmpty())
		return;

	QDialog dlg(this);
	dlg.setWindowTitle(tr("Promote crash signature to a user rule"));
	auto* v = new QVBoxLayout(&dlg);
	v->addWidget(new QLabel(tr("Signature: <code>%1</code>").arg(m_currentFingerprint)));
	v->addWidget(new QLabel(tr("Sample: <code>%1</code>")
								.arg(m_currentSampleLine.toHtmlEscaped())));

	auto* titleEdit = new QLineEdit(&dlg);
	titleEdit->setPlaceholderText(tr("Short title (one sentence)"));
	v->addWidget(titleEdit);

	auto* patternEdit = new QLineEdit(&dlg);
	patternEdit->setPlaceholderText(
		tr("Regex pattern (leave blank to derive from the sample line)"));
	v->addWidget(patternEdit);

	auto* adviceEdit = new QTextEdit(&dlg);
	adviceEdit->setPlaceholderText(tr("Remediation advice in Markdown"));
	v->addWidget(adviceEdit);

	auto* btnBox = new QHBoxLayout();
	btnBox->addStretch();
	auto* okBtn = new QPushButton(tr("Save"), &dlg);
	auto* cancelBtn = new QPushButton(tr("Cancel"), &dlg);
	btnBox->addWidget(okBtn);
	btnBox->addWidget(cancelBtn);
	v->addLayout(btnBox);
	connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
	connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

	if (dlg.exec() != QDialog::Accepted)
		return;

	QString title = titleEdit->text().trimmed();
	QString pattern = patternEdit->text().trimmed();
	QString advice = adviceEdit->toPlainText().trimmed();
	if (title.isEmpty() || advice.isEmpty())
		return;

	// Derive a pattern from the sample line if not provided.
	if (pattern.isEmpty() && !m_currentSampleLine.isEmpty()) {
		QString esc = QRegularExpression::escape(m_currentSampleLine);
		pattern = esc.left(120); // pin the prefix
	}
	if (pattern.isEmpty())
		return;

	// Persist into a user rules pack we control.
	QString userRulesDir =
		QString::fromUtf8(/* plugin will fill this in via ctx */
						  qgetenv("MESHMC_USER_RULES_DIR"));
	if (userRulesDir.isEmpty())
		userRulesDir = QDir::homePath() + "/.local/share/meshmc/errororacle/userrules";
	QDir().mkpath(userRulesDir);

	QString fileName = "promoted-" + m_currentFingerprint + ".json";
	QFile out(QDir(userRulesDir).filePath(fileName));
	if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QMessageBox::warning(this, tr("ErrorOracle"),
							 tr("Could not write user rule file."));
		return;
	}
	QJsonObject root;
	root["schema"] = 1;
	root["name"] = "User-promoted rules";
	root["version"] = "1.0.0";
	QJsonArray rules;
	QJsonObject rule;
	rule["id"] = "user." + m_currentFingerprint;
	rule["title"] = title;
	rule["severity"] = "medium";
	QJsonArray patterns;
	patterns.append(pattern);
	rule["patterns"] = patterns;
	rule["advice"] = advice;
	QJsonArray tags;
	tags.append("user-promoted");
	rule["tags"] = tags;
	rules.append(rule);
	root["rules"] = rules;
	out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));

	m_learning->forgetNovel(m_currentFingerprint);
	m_learning->save();

	QMessageBox::information(
		this, tr("ErrorOracle"),
		tr("Saved user rule. Click <b>Re-analyse</b> to load it."));
}
