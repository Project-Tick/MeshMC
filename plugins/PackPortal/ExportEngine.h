/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ExportEngine — the moving parts that turn a MeshMC instance into a
 * .mrpack / CurseForge zip / MultiMC zip. Each format adapter only
 * deals with the manifest dialect; the engine handles:
 *
 *   • walking the instance directory and applying the format's
 *     include/exclude rules,
 *   • computing per-file hashes (SHA-1 + SHA-256 + size),
 *   • cross-referencing mods that have CurseForge/Modrinth identity
 *     in the per-instance metadata index,
 *   • populating a staging directory, and
 *   • finally packing the staging directory into the target zip via
 *     MMCZip::compressDir().
 */

#pragma once

#include "PackFormat.h"

namespace pack
{

class ExportEngine
{
  public:
	/* `ctx` is the MMCOContext owned by the plugin. The engine drives
	 * every host-side query through it (instance list walks, component
	 * versions, icon key, zip compression). */
	explicit ExportEngine(MMCOContext* ctx = nullptr) : m_ctx(ctx) {}

	struct Options {
		Format format = Format::MrPack;

		/* Whether to embed every mod into overrides/, or to leave the
		 * referenced ones out of the archive and rely on the manifest
		 * download links. Always true for MultiMC. */
		bool embedAllMods = false;

		/* Skip world saves to keep archive size sane. */
		bool skipWorlds = true;

		/* Skip log files, crash reports, screenshots. */
		bool skipVolatile = true;

		QString name;
		QString version;
		QString author;
		QString summary;
	};

	struct Result {
		bool ok = false;
		QString outputPath;
		QString errorMsg;
		QStringList warnings;
		int filesEmbedded = 0;
		int filesReferenced = 0;
	};

	/* Synchronous; does its own IO. Designed to be called from a
	 * worker thread or QtConcurrent if the caller cares about
	 * responsiveness. */
	Result exportInstance(const QString& instanceId, const Options& opts,
						  const QString& outputPath);

  private:
	bool stageOverrides(ExportStage& stage, const Options& opts,
						QStringList& warnings);
	bool hashFiles(ExportStage& stage);
	bool collectComponents(const QString& instanceId, ComponentSet& out);
	bool collectFiles(const QString& instanceId, const QString& root,
					  const Options& opts, QList<PackFile>& out,
					  QStringList& warnings);
	void resolveExternalIdentity(QList<PackFile>& files,
								 const QString& instanceRoot);

	bool writeFile(const QString& abs, const QByteArray& bytes);

	MMCOContext* m_ctx = nullptr;
};

} // namespace pack
