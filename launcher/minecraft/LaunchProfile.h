/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <QString>
#include "Library.h"
#include <ProblemProvider.h>

class LaunchProfile : public ProblemProvider
{
  public:
	virtual ~LaunchProfile() {};

  public: /* application of profile variables from patches */
	void applyMinecraftVersion(const QString& id);
	void applyMainClass(const QString& mainClass);
	void applyAppletClass(const QString& appletClass);
	void applyMinecraftArguments(const QString& minecraftArguments);
	void applyMinecraftVersionType(const QString& type);
	void applyMinecraftAssets(MojangAssetIndexInfo::Ptr assets);
	void applyTraits(const QSet<QString>& traits);
	void applyTweakers(const QStringList& tweakers);
	void applyJarMods(const QList<LibraryPtr>& jarMods);
	void applyMods(const QList<LibraryPtr>& jarMods);
	void applyLibrary(LibraryPtr library);
	void applyMavenFile(LibraryPtr library);
	void applyMainJar(LibraryPtr jar);
	void applyProblemSeverity(ProblemSeverity severity);
	/// clear the profile
	void clear();

  public: /* getters for profile variables */
	QString getMinecraftVersion() const;
	QString getMainClass() const;
	QString getAppletClass() const;
	QString getMinecraftVersionType() const;
	MojangAssetIndexInfo::Ptr getMinecraftAssets() const;
	QString getMinecraftArguments() const;
	const QSet<QString>& getTraits() const;
	const QStringList& getTweakers() const;
	const QList<LibraryPtr>& getJarMods() const;
	const QList<LibraryPtr>& getLibraries() const;
	const QList<LibraryPtr>& getNativeLibraries() const;
	const QList<LibraryPtr>& getMavenFiles() const;
	const LibraryPtr getMainJar() const;
	void getLibraryFiles(const QString& architecture, QStringList& jars,
						 QStringList& nativeJars, const QString& overridePath,
						 const QString& tempPath) const;
	bool hasTrait(const QString& trait) const;
	ProblemSeverity getProblemSeverity() const override;
	const QList<PatchProblem> getProblems() const override;

  private:
	/// the version of Minecraft - jar to use
	QString m_minecraftVersion;

	/// Release type - "release" or "snapshot"
	QString m_minecraftVersionType;

	/// Assets type - "legacy" or a version ID
	MojangAssetIndexInfo::Ptr m_minecraftAssets;

	/**
	 * arguments that should be used for launching minecraft
	 *
	 * ex: "--username ${auth_player_name} --session ${auth_session}
	 *      --version ${version_name} --gameDir ${game_directory} --assetsDir
	 * ${game_assets}"
	 */
	QString m_minecraftArguments;

	/// A list of all tweaker classes
	QStringList m_tweakers;

	/// The main class to load first
	QString m_mainClass;

	/// The applet class, for some very old minecraft releases
	QString m_appletClass;

	/// the list of libraries
	QList<LibraryPtr> m_libraries;

	/// the list of maven files to be placed in the libraries folder, but not
	/// acted upon
	QList<LibraryPtr> m_mavenFiles;

	/// the main jar
	LibraryPtr m_mainJar;

	/// the list of native libraries
	QList<LibraryPtr> m_nativeLibraries;

	/// traits, collected from all the version files (version files can only
	/// add)
	QSet<QString> m_traits;

	/// A list of jar mods. version files can add those.
	QList<LibraryPtr> m_jarMods;

	/// the list of mods
	QList<LibraryPtr> m_mods;

	ProblemSeverity m_problemSeverity = ProblemSeverity::None;
};
