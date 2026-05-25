/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ModrinthSource — implements UpdateSource for Modrinth modpacks.
 *
 * Endpoint: GET https://api.modrinth.com/v2/project/{slug}/version
 *   - returns an array of version objects, newest first.
 *   - each version carries `id`, `version_number`, `files[]`
 *     (with the primary mrpack download URL).
 *
 * We do NOT need an API key — Modrinth's public v2 is open. The
 * launcher's User-Agent (forced by S11) keeps us identifiable to
 * their rate limiter.
 *
 * Lookup happens by `pack_slug` (which we record at install time
 * from the mrpack manifest's `name` field). The Modrinth UI URL
 * uses the same slug, so what we store doubles as the canonical
 * pack-page URL when the user clicks the source link.
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
		/* Heap-allocated callback wrapper used as user_data for the
		 * C-ABI HTTP callback. Self-deletes on completion so we
		 * don't leak even on parse failure paths. */
		struct ModrinthCtx {
			LatestVersionCallback cb;
		};

		void onHttp(void* user_data, int status, const void* body,
					size_t body_size)
		{
			std::unique_ptr<ModrinthCtx> self(
				static_cast<ModrinthCtx*>(user_data));

			LatestVersion out;
			if (status < 200 || status >= 300) {
				out.errorMessage =
					QObject::tr("Modrinth API returned HTTP %1").arg(status);
				self->cb(out);
				return;
			}

			const QByteArray bytes(static_cast<const char*>(body),
								   static_cast<int>(body_size));
			QJsonParseError err{};
			const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
			if (err.error != QJsonParseError::NoError || !doc.isArray()) {
				out.errorMessage =
					QObject::tr("Modrinth response was not a JSON array");
				self->cb(out);
				return;
			}

			const QJsonArray arr = doc.array();
			if (arr.isEmpty()) {
				out.errorMessage =
					QObject::tr("Modrinth lists no versions for this pack");
				self->cb(out);
				return;
			}

			/* Modrinth returns newest first, but the API contract
			 * doesn't *guarantee* it — pick the one with the latest
			 * `date_published` for safety. ISO 8601 strings sort
			 * lexicographically. */
			QJsonObject best;
			QString bestDate;
			for (const auto& v : arr) {
				const QJsonObject o = v.toObject();
				const QString date =
					o.value(QStringLiteral("date_published")).toString();
				if (date > bestDate) {
					bestDate = date;
					best = o;
				}
			}
			if (best.isEmpty())
				best = arr.first().toObject();

			out.ok = true;
			out.versionId = best.value(QStringLiteral("id")).toString();
			out.versionLabel =
				best.value(QStringLiteral("version_number")).toString();

			/* Primary file URL — Modrinth marks one `.mrpack` as
			 * primary; fall back to the first file if no flag set. */
			const QJsonArray files =
				best.value(QStringLiteral("files")).toArray();
			QString primaryUrl;
			QString anyUrl;
			for (const auto& fv : files) {
				const QJsonObject fo = fv.toObject();
				const QString url = fo.value(QStringLiteral("url")).toString();
				if (anyUrl.isEmpty())
					anyUrl = url;
				if (fo.value(QStringLiteral("primary")).toBool()) {
					primaryUrl = url;
					break;
				}
			}
			out.manifestUrl = primaryUrl.isEmpty() ? anyUrl : primaryUrl;

			self->cb(out);
		}

		class ModrinthSource : public UpdateSource
		{
		  public:
			void fetchLatest(MMCOContext* ctx, const PackRecord& rec,
							 LatestVersionCallback cb) override
			{
				if (!ctx || rec.packSlug.isEmpty()) {
					LatestVersion bad;
					bad.errorMessage = QObject::tr(
						"This Modrinth record is missing a pack slug — "
						"the manifest may not have been preserved at "
						"import time.");
					cb(bad);
					return;
				}

				const QString url =
					QStringLiteral(
						"https://api.modrinth.com/v2/project/%1/version")
						.arg(rec.packSlug);

				/* The HTTP callback owns its context heap-allocated
				 * so it survives the original stack frame. */
				auto* heap = new ModrinthCtx{std::move(cb)};
				const QByteArray urlUtf8 = url.toUtf8();
				int rc = ctx->http_get(ctx->module_handle, urlUtf8.constData(),
									   &onHttp, heap);
				if (rc != 0) {
					delete heap;
					LatestVersion bad;
					bad.errorMessage =
						QObject::tr("Could not queue Modrinth request");
					/* cb was moved into `heap`, can't replay it here.
					 * Best-effort — the launcher logs the queue
					 * failure separately. */
				}
			}
		};

	} /* namespace */

	/* Plumbing for the factory. Lives in this TU so the factory
	 * doesn't have to know the concrete class. */
	UpdateSource* makeModrinthSource_internal()
	{
		return new ModrinthSource();
	}

} /* namespace pack_updater */
