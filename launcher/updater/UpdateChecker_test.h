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
	void tst_ParseFeedItems_VersionChannelAndNotes();
	void tst_ParseFeedItems_IgnoresAssets();
	void tst_ParseFeedItems_MissingChannelDefaultsToStable();
	void tst_ParseFeedItems_SkipsEntryWithoutVersion();
	void tst_ParseFeedItems_ReportsMalformedXml();

	// Channel policy
	void tst_IsChannelAccepted_data();
	void tst_IsChannelAccepted();

	// Entry selection
	void tst_PickBestItemIndex_StableBuildIgnoresBeta();
	void tst_PickBestItemIndex_BetaBuildTakesNewestOfBoth();
	void tst_PickBestItemIndex_IgnoresFeedOrder();
	void tst_PickBestItemIndex_NoAcceptableEntry();

	// GitHub release resolution
	void tst_ReleaseTag();
	void tst_ReleaseAssetName_data();
	void tst_ReleaseAssetName();
	void tst_MakeGithubDownloadUrl();
	void tst_MakeGithubDownloadUrl_UnknownArtifactYieldsNothing();

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
