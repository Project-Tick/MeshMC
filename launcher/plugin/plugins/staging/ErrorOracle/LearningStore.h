/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * LearningStore — persisted (rule, instance, signature) -> outcome
 * table. The plugin uses this to:
 *
 *   1) Score known rule hits so the most-useful ones float to the top
 *      of the suggestion list.
 *
 *   2) Detect novel error fingerprints by computing a stable
 *      *signature* of each crash report — the sequence of top-of-stack
 *      class names, lower-cased, with mod-version-specific suffixes
 *      stripped — and remembering them in a separate "novel" table.
 *      The next time the same signature appears, the user is offered
 *      the chance to *promote* it into a user rule, providing a custom
 *      title and advice text. That's the "learning" part — the plugin
 *      builds new rules from real-world crashes without any LLM.
 *
 * Storage: a JSON file under <plugin_data>/learn.json, written
 * atomically.
 */

#pragma once

#include "plugin/sdk/mmco_cxx_sdk.hpp"

struct OutcomeStats {
	int helped = 0;
	int didNotHelp = 0;
	int seen = 0;
	qint64 lastSeenSecs = 0;
};

struct NovelFingerprint {
	QString signature;
	QString sampleLine; // the line that produced the signature
	QString sampleInstance;
	int occurrences = 0;
	qint64 firstSeenSecs = 0;
	qint64 lastSeenSecs = 0;
};

class LearningStore
{
  public:
	bool open(const QString& filePath);
	bool save();

	double scoreFor(const QString& ruleId, const QString& instanceId) const;

	void recordSeen(const QString& ruleId, const QString& instanceId);
	void recordHelped(const QString& ruleId, const QString& instanceId);
	void recordDidNotHelp(const QString& ruleId, const QString& instanceId);

	/* Compute a deterministic crash signature from log text. The
	 * signature lives across launches and is portable between
	 * machines, so it's a useful hash for shared crash reports. */
	static QString fingerprint(const QString& logText);

	void recordNovel(const QString& signature, const QString& sampleLine,
					 const QString& instanceId);
	QList<NovelFingerprint> novelFingerprints() const;

	/* Forget a signature — typically after the user promotes it to a
	 * rule. */
	void forgetNovel(const QString& signature);

  private:
	struct Key {
		QString ruleId;
		QString instanceId;
		bool operator==(const Key& o) const
		{
			return ruleId == o.ruleId && instanceId == o.instanceId;
		}
	};
	friend uint qHash(const LearningStore::Key&, uint seed) noexcept;

	QHash<Key, OutcomeStats> m_outcomes;
	QHash<QString, NovelFingerprint> m_novel;
	QString m_path;
};

uint qHash(const LearningStore::Key& k, uint seed = 0) noexcept;
