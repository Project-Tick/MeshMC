/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * PackMetadata — backed by S24 per-instance settings.
 *
 * Reads/writes the Pack* keys that BaseInstance pre-registers and
 * InstanceImportTask seeds on import. PackUpdater never owns its
 * own sidecar file.
 *
 * Key names mirror what BaseInstance.cpp registers — do NOT rename
 * one without updating the other.
 */

#include "PackMetadata.h"

namespace pack_updater
{

	namespace
	{
		/* Centralised key names so a typo here is a one-line fix
		 * rather than three. */
		constexpr const char kProvider[] = "PackProvider";
		constexpr const char kPackId[] = "PackId";
		constexpr const char kPackSlug[] = "PackSlug";
		constexpr const char kVersionId[] = "PackVersionId";
		constexpr const char kVersionLabel[] = "PackVersionLabel";
		constexpr const char kIconUrl[] = "PackIconUrl";
		constexpr const char kSourceUrl[] = "PackSourceUrl";
		constexpr const char kInstalledAt[] = "PackInstalledAt";
		constexpr const char kManifestSha512[] = "PackManifestSha512";

		/* S24 returns a `tempString` pointer that's invalidated by the
		 * very next API call. This helper copies into a QString
		 * immediately so callers can chain reads safely. Returns an
		 * empty QString when the value is missing or the read fails. */
		QString readKey(MMCOContext* ctx, const QString& instanceId,
						const char* key)
		{
			if (!ctx)
				return {};
			const QByteArray idUtf8 = instanceId.toUtf8();
			const char* v = ctx->instance_setting_get(ctx->module_handle,
													  idUtf8.constData(), key);
			return v ? QString::fromUtf8(v) : QString();
		}

		bool writeKey(MMCOContext* ctx, const QString& instanceId,
					  const char* key, const QString& value)
		{
			if (!ctx)
				return false;
			const QByteArray idUtf8 = instanceId.toUtf8();
			const QByteArray valUtf8 = value.toUtf8();
			return ctx->instance_setting_set(ctx->module_handle,
											 idUtf8.constData(), key,
											 valUtf8.constData()) == 0;
		}
	} /* namespace */

	const char* providerToString(Provider p)
	{
		switch (p) {
			case Provider::Modrinth:
				return "modrinth";
			case Provider::CurseForge:
				return "curseforge";
			case Provider::MultiMC:
				return "multimc";
			case Provider::Unknown:
			default:
				return "unknown";
		}
	}

	Provider providerFromString(const QString& s)
	{
		const QString n = s.trimmed().toLower();
		if (n == QLatin1String("modrinth"))
			return Provider::Modrinth;
		if (n == QLatin1String("curseforge"))
			return Provider::CurseForge;
		if (n == QLatin1String("multimc"))
			return Provider::MultiMC;
		return Provider::Unknown;
	}

	bool exists(MMCOContext* ctx, const QString& instanceId)
	{
		return !readKey(ctx, instanceId, kProvider).isEmpty();
	}

	std::optional<PackRecord> load(MMCOContext* ctx, const QString& instanceId)
	{
		const QString providerStr = readKey(ctx, instanceId, kProvider);
		if (providerStr.isEmpty())
			return std::nullopt;

		PackRecord rec;
		rec.provider = providerFromString(providerStr);
		rec.packId = readKey(ctx, instanceId, kPackId);
		rec.packSlug = readKey(ctx, instanceId, kPackSlug);
		rec.installedVersionId = readKey(ctx, instanceId, kVersionId);
		rec.installedVersionLabel = readKey(ctx, instanceId, kVersionLabel);
		rec.iconUrl = readKey(ctx, instanceId, kIconUrl);
		rec.sourceUrl = readKey(ctx, instanceId, kSourceUrl);
		rec.installedAtIso8601 = readKey(ctx, instanceId, kInstalledAt);
		rec.manifestSha512 = readKey(ctx, instanceId, kManifestSha512);
		return rec;
	}

	bool save(MMCOContext* ctx, const QString& instanceId,
			  const PackRecord& rec)
	{
		bool ok = true;
		ok &= writeKey(ctx, instanceId, kProvider,
					   QString::fromLatin1(providerToString(rec.provider)));
		ok &= writeKey(ctx, instanceId, kPackId, rec.packId);
		ok &= writeKey(ctx, instanceId, kPackSlug, rec.packSlug);
		ok &= writeKey(ctx, instanceId, kVersionId, rec.installedVersionId);
		ok &=
			writeKey(ctx, instanceId, kVersionLabel, rec.installedVersionLabel);
		ok &= writeKey(ctx, instanceId, kIconUrl, rec.iconUrl);
		ok &= writeKey(ctx, instanceId, kSourceUrl, rec.sourceUrl);
		ok &= writeKey(ctx, instanceId, kInstalledAt, rec.installedAtIso8601);
		ok &= writeKey(ctx, instanceId, kManifestSha512, rec.manifestSha512);
		return ok;
	}

	bool clear(MMCOContext* ctx, const QString& instanceId)
	{
		/* Empty strings are the agreed "absent" marker — keys stay
		 * registered, but every reader treats empty PackProvider as
		 * "not pack-managed". */
		PackRecord blank;
		return save(ctx, instanceId, blank);
	}

} /* namespace pack_updater */
