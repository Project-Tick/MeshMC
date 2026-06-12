/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * CurseForgeSource — implements UpdateSource for CurseForge packs.
 *
 * Endpoint: GET https://api.curseforge.com/v1/mods/{modId}/files
 *   - requires `x-api-key` header (BuildConfig.CURSEFORGE_API_KEY,
 *     baked into the plugin at compile time).
 *   - response wraps the file list under `data`.
 *
 * Lookup key is the numeric `pack_id` we recorded from
 * `manifest.json`'s `projectID`. CurseForge zips published before
 * `projectID` was canonical omit the field; in that case the
 * record will have an empty `packId` and this adapter reports a
 * helpful error rather than guessing.
 *
 * Why this plugin gets to see the API key directly: BuildConfig
 * symbols are linked into every .mmco at compile time (the in-tree
 * SDK exports `buildconfig/BuildConfig.h` as a public include
 * path, and the BuildConfig static lib goes through the SDK target
 * for plugins that need it). Pulling the key from BuildConfig
 * keeps it on a single channel and avoids exposing it through a
 * generic plugin API.
 */

#include "UpdateSource.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace pack_updater
{

	namespace
	{
		struct CurseForgeCtx {
			LatestVersionCallback cb;
		};

		void onHttp(void* user_data, int status, const void* body,
					size_t body_size)
		{
			std::unique_ptr<CurseForgeCtx> self(
				static_cast<CurseForgeCtx*>(user_data));

			LatestVersion out;
			if (status < 200 || status >= 300) {
				out.errorMessage =
					QObject::tr("CurseForge API returned HTTP %1").arg(status);
				self->cb(out);
				return;
			}

			const QByteArray bytes(static_cast<const char*>(body),
								   static_cast<int>(body_size));
			QJsonParseError err{};
			const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
			if (err.error != QJsonParseError::NoError || !doc.isObject()) {
				out.errorMessage =
					QObject::tr("CurseForge response was not a JSON object");
				self->cb(out);
				return;
			}

			const QJsonArray arr =
				doc.object().value(QStringLiteral("data")).toArray();
			if (arr.isEmpty()) {
				out.errorMessage =
					QObject::tr("CurseForge lists no files for this pack");
				self->cb(out);
				return;
			}

			/* CF response is reverse-chronological — pick the newest
			 * `fileDate` directly (same defensive tie-breaker as the
			 * Modrinth adapter). */
			QJsonObject best;
			QString bestDate;
			for (const auto& v : arr) {
				const QJsonObject o = v.toObject();
				const QString date =
					o.value(QStringLiteral("fileDate")).toString();
				if (date > bestDate) {
					bestDate = date;
					best = o;
				}
			}
			if (best.isEmpty())
				best = arr.first().toObject();

			out.ok = true;
			out.versionId =
				QString::number(best.value(QStringLiteral("id")).toInteger());
			out.versionLabel =
				best.value(QStringLiteral("displayName")).toString();
			out.manifestUrl =
				best.value(QStringLiteral("downloadUrl")).toString();

			self->cb(out);
		}

		class CurseForgeSource : public UpdateSource
		{
		  public:
			void fetchLatest(MMCOContext* ctx, const PackRecord& rec,
							 LatestVersionCallback cb) override
			{
				if (!ctx) {
					LatestVersion bad;
					bad.errorMessage =
						QObject::tr("Plugin context unavailable");
					cb(bad);
					return;
				}
				if (rec.packId.isEmpty()) {
					LatestVersion bad;
					bad.errorMessage = QObject::tr(
						"This CurseForge record has no project id — the "
						"original `manifest.json` didn't carry one. Use "
						"Detach + re-attach once we wire the attach UI.");
					cb(bad);
					return;
				}
				const QString apiKey = QString::fromUtf8(
					ctx->app_setting_get
						? ctx->app_setting_get(ctx->module_handle,
											   "CurseForgeAPIKey")
						: nullptr);
				if (apiKey.isEmpty()) {
					LatestVersion bad;
					bad.errorMessage = QObject::tr(
						"This build of MeshMC has no CurseForge API key "
						"configured (CURSEFORGE_API_KEY is empty).");
					cb(bad);
					return;
				}
				if (!ctx->http_get_with_headers) {
					LatestVersion bad;
					bad.errorMessage = QObject::tr(
						"Host launcher is too old: http_get_with_headers "
						"(ABI 3) is required for CurseForge support.");
					cb(bad);
					return;
				}

				const QString url =
					QStringLiteral(
						"https://api.curseforge.com/v1/mods/%1/files")
						.arg(rec.packId);

				/* `x-api-key: <BUILDCONFIG_KEY>` — the only header CF
				 * mandates for v1. Accept header is JSON by default
				 * for the v1 endpoint, no need to set it. */
				const QByteArray headerLine =
					QStringLiteral("x-api-key: %1").arg(apiKey).toUtf8();
				const char* headers[] = {headerLine.constData()};

				auto* heap = new CurseForgeCtx{std::move(cb)};
				const QByteArray urlUtf8 = url.toUtf8();
				int rc = ctx->http_get_with_headers(ctx->module_handle,
													urlUtf8.constData(),
													headers, 1, &onHttp, heap);
				if (rc != 0) {
					delete heap;
					/* cb already moved — no clean way to surface this
					 * failure. Launcher logs the queue error. */
				}
			}
		};

	} /* namespace */

	UpdateSource* makeCurseForgeSource_internal()
	{
		return new CurseForgeSource();
	}

} /* namespace pack_updater */
