/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * UpdateApplier — pure-data planner for "apply a newer pack
 * version to this instance".
 *
 * Lives one layer below ApplyProgressDialog: the dialog drives the
 * IO (HTTP, zip extract, S05 mod calls), this module just produces
 * a plan it can execute step by step.
 *
 * Separation rationale: the diff is the *only* part of the apply
 * flow that benefits from being unit-testable. Network and S05
 * calls have to happen against a real instance; the diff against
 * sidecars + a parsed manifest does not. Keeping it here means we
 * can later add tests without instantiating any Qt UI.
 *
 * Provider scope: Modrinth + CurseForge. Modrinth packs ship
 * everything we need inside `modrinth.index.json` (direct CDN
 * URLs + hashes). CurseForge packs only ship `projectID`/`fileID`
 * pairs in `manifest.json`, so we resolve each pair through the
 * CF v1 API (`/mods/{modId}/files/{fileId}`) before we have a
 * full ParsedManifest. That resolution lives entirely inside
 * fetchAndParseManifest so the rest of the pipeline stays
 * provider-agnostic.
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"
#include "PackMetadata.h"

namespace pack_updater
{

	/* Parsed mrpack file entry — just the bits the planner needs.
	 * Path is the instance-relative location ("mods/sodium.jar"
	 * etc.); hashes come straight from the manifest. */
	struct ManifestFile {
		QString path;
		QString sha1;
		QString sha512;
		QUrl downloadUrl;
		qint64 size = 0;
	};

	/* Versions the new manifest expects of the launcher's
	 * components. Empty string = "don't change this component".
	 * Maps onto PackProfile uids inside the applier. */
	struct ManifestComponents {
		QString minecraftVersion;
		QString fabricLoaderVersion;
		QString forgeVersion;
		QString neoForgeVersion;
		QString quiltLoaderVersion;
	};

	struct ParsedManifest {
		QString name;
		QString versionId;
		ManifestComponents components;
		QVector<ManifestFile> files;
		/* The on-the-wire bytes, so the apply step can re-extract
		 * `overrides/` from the original mrpack zip instead of
		 * parsing it twice. */
		QString downloadedZipPath;
	};

	/* A single mod-folder mutation the apply step will perform. */
	struct FileAction {
		enum Kind { Add, Replace, Remove };
		Kind kind = Add;
		QString instanceRelativePath; /* "mods/sodium.jar"   */
		QString fileName;			  /* "sodium.jar"        */
		QString folder;				  /* "mods"              */
		QUrl downloadUrl;			  /* for Add / Replace   */
		QString sha1;
		QString sha512;
		qint64 size = 0;
	};

	/* What changed about the loader / Minecraft stack. Each pair
	 * is { uid, newVersion }; the dialog renders these as
	 * "Fabric Loader: 0.15.7 → 0.16.2" for user approval before we
	 * touch instance_component_set_version. */
	struct ComponentChange {
		QString uid;
		QString currentVersion; /* empty = component not installed */
		QString newVersion;
	};

	struct UpdatePlan {
		bool ok = false;
		QString errorMessage;

		PackRecord installed;  /* what we found in instance.cfg */
		ParsedManifest target; /* what the new pack wants */
		QVector<FileAction> files;
		QVector<ComponentChange> components;

		/* Convenience predicates for the dialog. */
		bool hasFileChanges() const
		{
			return !files.isEmpty();
		}
		bool hasComponentChanges() const
		{
			return !components.isEmpty();
		}
	};

	/* Step 1 of the apply flow: download the pack archive pointed
	 * at by `packUrl`, parse its manifest, return a ParsedManifest.
	 * Modrinth packs read `modrinth.index.json` directly; CurseForge
	 * packs read `manifest.json` and then resolve each
	 * projectID/fileID via the CF v1 API to materialise download
	 * URLs. The archive stays on disk at `downloadedZipPath` for
	 * the overrides step that follows. Async — callback fires on
	 * the GUI thread. */
	using ManifestCallback = std::function<void(ParsedManifest)>;
	void fetchAndParseManifest(MMCOContext* ctx, Provider provider,
							   const QUrl& packUrl, const QString& scratchDir,
							   ManifestCallback cb);

	/* Step 2: build a plan by diffing the new manifest against the
	 * instance's existing mod sidecars + component list. Pure
	 * function; safe to test in isolation. */
	UpdatePlan diffAgainstInstance(MMCOContext* ctx, const QString& instanceId,
								   const QString& instanceRoot,
								   const PackRecord& installed,
								   ParsedManifest manifest);

} /* namespace pack_updater */
