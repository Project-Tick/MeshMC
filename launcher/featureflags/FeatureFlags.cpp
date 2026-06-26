/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#include "FeatureFlags.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include "BuildConfig.h"

// cxx-generated bridge header. Our build.rs (cxx-build) generates this and
// copies it, together with the shared rust/cxx.h, into
// launcher/featureflags/generated/, which CMake adds to the include path.
#include "meshmc_featureflags/lib.rs.h"

Q_LOGGING_CATEGORY(featureFlagsLog, "meshmc.featureflags")

namespace {
FeatureFlags* g_instance = nullptr;

// Convert a QByteArray into the rust::Slice<const uint8_t> the bridge expects.
rust::Slice<const uint8_t> asSlice(const QByteArray& data)
{
	return rust::Slice<const uint8_t>(
		reinterpret_cast<const uint8_t*>(data.constData()),
		static_cast<size_t>(data.size()));
}
}  // namespace

namespace {

QString overridesFilePath()
{
	const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	return QDir(base).filePath(QStringLiteral("feature_flag_overrides.ini"));
}
}  // namespace

struct FeatureFlags::Private
{
	// Owned Rust evaluator. All access happens on the GUI thread.
	rust::Box<meshmc::ff::Evaluator> engine = meshmc::ff::new_evaluator();
	QNetworkAccessManager* nam = nullptr;
	QString userId;
	QDateTime lastUpdated;
	// Local user overrides, keyed by flag name. Loaded once at startup.
	QHash<QString, FlagOverride> overrides;

	void loadOverrides()
	{
		overrides.clear();
		QSettings ini(overridesFilePath(), QSettings::IniFormat);
		ini.beginGroup(QStringLiteral("overrides"));
		const QStringList keys = ini.childKeys();
		for (const QString& key : keys) {
			overrides.insert(key, ini.value(key).toBool() ? FlagOverride::ForceOn
														   : FlagOverride::ForceOff);
		}
		ini.endGroup();
	}

	void saveOverride(const QString& flag, FlagOverride value)
	{
		QSettings ini(overridesFilePath(), QSettings::IniFormat);
		ini.beginGroup(QStringLiteral("overrides"));
		if (value == FlagOverride::Auto) {
			ini.remove(flag);
		} else {
			ini.setValue(flag, value == FlagOverride::ForceOn);
		}
		ini.endGroup();
	}
};

FeatureFlags::FeatureFlags(QObject* parent) : QObject(parent), d(std::make_unique<Private>())
{
	d->nam = new QNetworkAccessManager(this);
	d->loadOverrides();
	// Seed synchronously from the on-disk cache so the very first isEnabled()
	// call (which may happen before any network round-trip) is meaningful and
	// works fully offline.
	seedFromCache();
}

FeatureFlags::~FeatureFlags() = default;

FeatureFlags* FeatureFlags::instance()
{
	return g_instance;
}

void FeatureFlags::setInstance(FeatureFlags* ff)
{
	g_instance = ff;
}

void FeatureFlags::setUserId(const QString& userId)
{
	d->userId = userId;
}

bool FeatureFlags::hasData() const
{
	return d->engine->has_data();
}

bool FeatureFlags::isEnabled(const QString& flag, bool defaultValue) const
{
	// A local override always wins over the backend value.
	const FlagOverride ov = d->overrides.value(flag, FlagOverride::Auto);
	if (ov == FlagOverride::ForceOn) {
		return true;
	}
	if (ov == FlagOverride::ForceOff) {
		return false;
	}

	// When the build has no instance id, there is no backend to consult; honour
	// the compiled-in default. The engine itself also returns the default when
	// no document is loaded, so this is belt-and-suspenders.
	if (!BuildConfig.FEATURE_FLAGS_ENABLED && !d->engine->has_data()) {
		return defaultValue;
	}

	const QByteArray flagUtf8 = flag.toUtf8();
	const QByteArray userUtf8 = d->userId.toUtf8();
	const QByteArray appUtf8 = BuildConfig.UNLEASH_APP_NAME.toUtf8();

	return d->engine->is_enabled(
		rust::Str(flagUtf8.constData(), flagUtf8.size()),
		rust::Str(userUtf8.constData(), userUtf8.size()),
		rust::Str(appUtf8.constData(), appUtf8.size()),
		defaultValue);
}

FlagOverride FeatureFlags::overrideFor(const QString& flag) const
{
	return d->overrides.value(flag, FlagOverride::Auto);
}

void FeatureFlags::setOverride(const QString& flag, FlagOverride value)
{
	if (value == FlagOverride::Auto) {
		d->overrides.remove(flag);
	} else {
		d->overrides.insert(flag, value);
	}
	d->saveOverride(flag, value);
	emit updated();
}

void FeatureFlags::refresh()
{
	if (!BuildConfig.FEATURE_FLAGS_ENABLED) {
		qCDebug(featureFlagsLog)
			<< "Feature flags disabled at build time (no instance id); skipping refresh.";
		return;
	}

	// The configured UNLEASH_URL is the Unleash API *base* (e.g.
	// .../feature_flags/unleash/47). A bare GET against it only returns a
	// health-check ("200"); the actual toggle document lives under the
	// "/client/features" sub-path. Append it, tolerating a trailing slash.
	QString base = BuildConfig.UNLEASH_URL;
	while (base.endsWith('/')) {
		base.chop(1);
	}
	const QUrl featuresUrl(base + QStringLiteral("/client/features"));

	QNetworkRequest req{ featuresUrl };
	// Unleash client API authentication headers. The instance id is a
	// client-side identifier, not a secret.
	req.setRawHeader("UNLEASH-INSTANCEID", BuildConfig.UNLEASH_INSTANCE_ID.toUtf8());
	req.setRawHeader("UNLEASH-APPNAME", BuildConfig.UNLEASH_APP_NAME.toUtf8());
	req.setRawHeader("Accept", "application/json");
	req.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT);

	QNetworkReply* reply = d->nam->get(req);
	connect(reply, &QNetworkReply::finished, this, &FeatureFlags::onReplyFinished);
}

void FeatureFlags::onReplyFinished()
{
	auto* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) {
		return;
	}
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError) {
		// Offline or backend error: keep the cached/seeded document. The
		// launcher keeps working with the last known flag state.
		qCWarning(featureFlagsLog)
			<< "Feature flag refresh failed:" << reply->errorString()
			<< "- keeping last known flags.";
		return;
	}

	const int httpStatus =
		reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const QByteArray body = reply->readAll();
	if (loadIntoEngine(body)) {
		writeCache(body);
		emit updated();
		qCDebug(featureFlagsLog) << "Feature flags refreshed.";
	} else {
		// Log enough of the raw response to diagnose schema/auth/content
		// problems without dumping a huge payload.
		qCWarning(featureFlagsLog)
			<< "Feature flag response did not parse; keeping last known flags."
			<< "HTTP status:" << httpStatus
			<< "content-type:"
			<< reply->header(QNetworkRequest::ContentTypeHeader).toString()
			<< "bytes:" << body.size();
		qCWarning(featureFlagsLog)
			<< "Response body (first 512 bytes):"
			<< body.left(512);
	}
}

void FeatureFlags::seedFromCache()
{
	const QString path = cacheFilePath();
	QFile f(path);
	if (!f.exists()) {
		return;
	}
	if (!f.open(QIODevice::ReadOnly)) {
		qCWarning(featureFlagsLog) << "Could not open feature flag cache:" << path;
		return;
	}
	const QByteArray body = f.readAll();
	f.close();
	if (loadIntoEngine(body)) {
		qCDebug(featureFlagsLog) << "Seeded feature flags from cache:" << path;
	}
}

bool FeatureFlags::loadIntoEngine(const QByteArray& body)
{
	const bool ok = d->engine->load(asSlice(body));
	if (ok) {
		d->lastUpdated = QDateTime::currentDateTime();
	}
	return ok;
}

QList<FeatureFlagState> FeatureFlags::allFlags() const
{
	QList<FeatureFlagState> out;

	const QByteArray userUtf8 = d->userId.toUtf8();
	const QByteArray appUtf8 = BuildConfig.UNLEASH_APP_NAME.toUtf8();

	const auto flags = d->engine->list_flags(
		rust::Str(userUtf8.constData(), userUtf8.size()),
		rust::Str(appUtf8.constData(), appUtf8.size()));

	out.reserve(static_cast<int>(flags.size()));
	for (const auto& f : flags) {
		FeatureFlagState s;
		s.name = QString::fromUtf8(f.name.data(), static_cast<int>(f.name.size()));
		s.toggleEnabled = f.toggle_enabled;
		s.backendEffective = f.effective;
		s.strategies = QString::fromUtf8(f.strategies.data(), static_cast<int>(f.strategies.size()));
		// Apply any local override to compute the final effective value.
		s.override = d->overrides.value(s.name, FlagOverride::Auto);
		switch (s.override) {
			case FlagOverride::ForceOn:
				s.effective = true;
				break;
			case FlagOverride::ForceOff:
				s.effective = false;
				break;
			case FlagOverride::Auto:
				s.effective = s.backendEffective;
				break;
		}
		out.append(s);
	}
	return out;
}

QDateTime FeatureFlags::lastUpdated() const
{
	return d->lastUpdated;
}

QString FeatureFlags::endpointUrl() const
{
	return BuildConfig.UNLEASH_URL;
}

QString FeatureFlags::appName() const
{
	return BuildConfig.UNLEASH_APP_NAME;
}

bool FeatureFlags::isConfigured() const
{
	return BuildConfig.FEATURE_FLAGS_ENABLED;
}

void FeatureFlags::writeCache(const QByteArray& body) const
{
	const QString path = cacheFilePath();
	QDir().mkpath(QFileInfo(path).absolutePath());
	// Atomic-ish write: temp file then rename, so a crash mid-write never
	// leaves a truncated cache that would later fail to parse.
	const QString tmp = path + QStringLiteral(".tmp");
	QFile f(tmp);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		qCWarning(featureFlagsLog) << "Could not write feature flag cache:" << tmp;
		return;
	}
	f.write(body);
	f.close();
	QFile::remove(path);
	if (!QFile::rename(tmp, path)) {
		qCWarning(featureFlagsLog) << "Could not finalize feature flag cache:" << path;
	}
}

QString FeatureFlags::cacheFilePath() const
{
	const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	return QDir(base).filePath(QStringLiteral("feature_flags.json"));
}
