/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 */

#pragma once

#include <QObject>

/*
 * Header companion for UpdateChecker_test.cpp.
 *
 * The test fixture is declared here so AutoMoc can find a class with
 * Q_OBJECT outside of any C++ raw string literals — the implementation in
 * the .cpp file contains a number of multiline raw strings that confuse
 * AutoMoc's source scanner if the fixture is defined inline.
 */
class UpdateCheckerTest : public QObject
{
	Q_OBJECT
  private slots:
	// Feed parsing
	void tst_ParseStableFeedItem_StructuredMatch();
	void tst_ParseStableFeedItem_StructuredArchSelectsCorrectAsset();
	void tst_ParseStableFeedItem_LegacyArtifactFallback();
	void tst_ParseStableFeedItem_NoMatchingAssetForBuild();
	void tst_ParseStableFeedItem_ReportsMalformedXml();

	// latest.json parsing
	void tst_ParseLatestJsonVersion_Stable();
	void tst_ParseLatestJsonVersion_MissingProduct();
	void tst_ParseLatestJsonVersion_Malformed();

	// Version helpers
	void tst_NormalizeVersion_data();
	void tst_NormalizeVersion();
	void tst_CompareVersions_data();
	void tst_CompareVersions();
};
