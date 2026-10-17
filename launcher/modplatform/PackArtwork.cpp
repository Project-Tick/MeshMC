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

#include "PackArtwork.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QThreadPool>
#include <QTimer>
#include <QVariantMap>
#include <QtConcurrentRun>

#include <memory>

#include "BaseInstance.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "core/LauncherContext.h"
#include "icons/IconList.h"
#include "modplatform/flame/FlameApi.h"
#include "modplatform/modrinth/ModrinthApi.h"
#include "net/Download.h"
#include "net/HttpMetaCache.h"
#include "net/NetJob.h"

namespace
{
/* The largest a logo is kept at. Icons are never drawn bigger than this,
 * and a catalogue's logo can be several times the size. */
constexpr int kLogoExtent = 256;

/* A Modrinth gallery image at full size: "raw_url" is the upload itself,
 * "url" a 350-pixel thumbnail that only stands in when there is no raw
 * one (older replies). */
QString fullSizeImage(const QJsonObject& image)
{
	const QString raw = image.value(QStringLiteral("raw_url")).toString();
	return raw.isEmpty() ? image.value(QStringLiteral("url")).toString() : raw;
}

bool writePng(const QImage& image, const QString& path)
{
	if (!FS::ensureFilePathExists(path)) {
		return false;
	}
	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}
	if (!image.save(&file, "PNG")) {
		file.cancelWriting();
		return false;
	}
	return file.commit();
}

/* Decodes a downloaded logo, scales it down to kLogoExtent and writes it
 * to @p path as PNG - one format every image reader has, whatever the
 * catalogue served (Modrinth's are often WebP). Runs on a worker thread:
 * QImage, unlike QPixmap, is safe there. False when @p bytes is not an
 * image, which is also what keeps an error page from becoming an icon.
 *
 * Written in @p stagingDir and moved into place, when there is one: the
 * icons folder is watched, and a QSaveFile writing there would show the
 * icon list its temporary file - under the same key, since a key is the
 * name up to its first dot - before the real one. */
bool saveLogo(const QByteArray& bytes, const QString& path,
			  const QString& stagingDir)
{
	QImage image;
	if (!image.loadFromData(bytes) || image.isNull()) {
		return false;
	}
	if (image.width() > kLogoExtent || image.height() > kLogoExtent) {
		image = image.scaled(kLogoExtent, kLogoExtent, Qt::KeepAspectRatio,
							 Qt::SmoothTransformation);
	}
	if (!stagingDir.isEmpty()) {
		const QString staged = FS::PathCombine(
			stagingDir, QFileInfo(path).fileName() + QStringLiteral(".part"));
		if (writePng(image, staged)) {
			QFile::remove(path);
			if (QFile::rename(staged, path)) {
				return true;
			}
			/* Another volume (the icons folder is a setting): written in
			 * place after all. */
			QFile::remove(staged);
		}
	}
	return writePng(image, path);
}
} // namespace

PackArtwork::PackArtwork(QObject* parent) : QObject(parent)
{
}

void PackArtwork::setCacheDir(const QString& dir)
{
	m_cacheDir = dir;
}

QString PackArtwork::cacheDir() const
{
	if (!m_cacheDir.isEmpty()) {
		return m_cacheDir;
	}
	auto* context = LauncherContext::instance();
	if (!context || !context->metacache()) {
		return QString();
	}
	/* The folder only, not a metacache entry: a cover's URL never changes
	 * what it points at (a new cover is a new URL, and a new file - see
	 * cachePath()), so the index's revalidation would buy nothing, and a
	 * file that is there is all coverFor() needs to know. */
	const QString base =
		context->metacache()->getBasePath(QStringLiteral("general"));
	return base.isEmpty() ? QString()
						  : FS::PathCombine(base, QStringLiteral("PackCovers"));
}

QString PackArtwork::coverFor(BaseInstance* instance)
{
	if (!instance || !instance->isManagedPack()) {
		return QString();
	}
	const QString id = instance->id();
	auto it = m_covers.constFind(id);
	if (it != m_covers.constEnd()) {
		return it.value();
	}
	ensure(instance);
	return m_covers.value(id);
}

void PackArtwork::ensure(BaseInstance* instance)
{
	if (!instance || !instance->isManagedPack()) {
		return;
	}
	const QString id = instance->id();
	if (m_started.contains(id)) {
		return;
	}
	m_started.insert(id);

	/* A cover downloaded in an earlier session is answered here and now -
	 * see coverFor() for why that is worth a stat(). */
	const QString cover = instance->managedPackCoverUrl();
	const QString dir = cacheDir();
	if (!cover.isEmpty() && !dir.isEmpty()) {
		const QString path = cachePath(dir, cover);
		if (QFileInfo::exists(path)) {
			m_covers.insert(id, QUrl::fromLocalFile(path).toString());
		}
	}

	/* Everything else waits for the event loop. This is asked from inside
	 * a model's data(), and a job that failed on the spot would otherwise
	 * have its answer announced back into the view in the middle of the
	 * paint that asked. */
	QPointer<BaseInstance> guarded(instance);
	QTimer::singleShot(0, this, [this, guarded]() {
		if (!guarded) {
			return;
		}
		if (guarded->hasManagedPackCoverRecord()) {
			fetchPictures(guarded);
		} else {
			lookUp(guarded);
		}
	});
}

void PackArtwork::forget(const QString& instanceId)
{
	m_covers.remove(instanceId);
	m_started.remove(instanceId);
}

void PackArtwork::lookUp(QPointer<BaseInstance> instance)
{
	auto* context = LauncherContext::instance();
	const QString provider = instance->managedPackProvider();
	const QUrl url = lookupUrl(provider, instance->managedPackId());
	if (!context || url.isEmpty()) {
		/* Nothing to ask: what the install recorded is all there is. */
		fetchPictures(instance);
		return;
	}

	auto reply = std::make_shared<QByteArray>();
	auto* job = new NetJob(
		QStringLiteral("PackArtwork::Lookup(%1)").arg(instance->id()),
		context->network());
	job->addNetAction(Net::Download::makeByteArray(url, reply.get()));
	connect(job, &NetJob::succeeded, this,
			[this, job, reply, instance, provider]() {
				job->deleteLater();
				if (!instance) {
					return;
				}
				const Found found = parseLookup(provider, *reply);
				if (found.recognised) {
					qDebug() << "Pack artwork for" << instance->id() << "("
							 << found.title << "): cover" << found.cover
							 << "logo" << found.logo;
					/* Recorded even when empty: a pack with no cover
					 * is not asked about again. */
					instance->setManagedPackCoverUrl(found.cover);
					if (instance->managedPackIconUrl().isEmpty() &&
						!found.logo.isEmpty()) {
						instance->setManagedPackIconUrl(found.logo);
					}
				}
				fetchPictures(instance);
			});
	connect(job, &NetJob::failed, this,
			[this, job, instance](QString reason) {
				job->deleteLater();
				if (!instance) {
					return;
				}
				/* Not recorded, so the next launch asks again; what the
				 * install recorded may still do meanwhile. */
				qDebug() << "Could not look up the pack of" << instance->id()
						 << ":" << reason;
				fetchPictures(instance);
			});
	job->start();
}

void PackArtwork::fetchPictures(QPointer<BaseInstance> instance)
{
	if (!instance) {
		return;
	}
	const QString id = instance->id();
	const QString cover = instance->managedPackCoverUrl();
	if (!cover.isEmpty() && !m_covers.contains(id)) {
		fetchCover(id, cover);
	}

	const QString provider = instance->managedPackProvider();
	const QString packId = instance->managedPackId();
	if (packId.isEmpty() || !isStockIconKey(provider, instance->iconKey())) {
		return;
	}
	auto* context = LauncherContext::instance();
	if (!context) {
		return;
	}
	const QString key = iconKeyFor(provider, packId);
	if (context->icons()->iconFileExists(key)) {
		/* Downloaded in an earlier session, which ended before the icon
		 * list noticed the file. */
		useLogo(instance, key);
		return;
	}
	QString logo = instance->managedPackIconUrl();
	if (logo.isEmpty()) {
		logo = derivedLogoUrl(provider, packId);
	}
	if (!logo.isEmpty()) {
		fetchLogo(instance, logo);
	}
}

void PackArtwork::fetchCover(const QString& instanceId, const QString& url)
{
	auto* context = LauncherContext::instance();
	const QString dir = cacheDir();
	if (!context || dir.isEmpty()) {
		return;
	}
	const QString path = cachePath(dir, url);
	if (QFileInfo::exists(path)) {
		/* Another instance of the same pack fetched it first. */
		publishCover(instanceId, path);
		return;
	}

	auto* job =
		new NetJob(QStringLiteral("PackArtwork::Cover(%1)").arg(instanceId),
				   context->network());
	/* FileSink writes through a QSaveFile, so a download that fails
	 * halfway leaves no file behind for coverFor() to mistake for a
	 * cover next launch. */
	job->addNetAction(Net::Download::makeFile(QUrl(url), path));
	connect(job, &NetJob::succeeded, this, [this, job, instanceId, path]() {
		job->deleteLater();
		publishCover(instanceId, path);
	});
	connect(job, &NetJob::failed, this,
			[this, job, instanceId, url](QString reason) {
				job->deleteLater();
				qDebug() << "Could not download the pack cover" << url << "for"
						 << instanceId << ":" << reason;
				m_covers.insert(instanceId, QString());
			});
	job->start();
}

void PackArtwork::publishCover(const QString& instanceId, const QString& path)
{
	m_covers.insert(instanceId, QUrl::fromLocalFile(path).toString());
	emit coverChanged(instanceId);
}

void PackArtwork::fetchLogo(QPointer<BaseInstance> instance,
							const QString& url)
{
	auto* context = LauncherContext::instance();
	if (!context || !instance) {
		return;
	}
	const QString key =
		iconKeyFor(instance->managedPackProvider(), instance->managedPackId());
	const QString target = FS::PathCombine(context->icons()->getDirectory(),
										   key + QStringLiteral(".png"));

	auto bytes = std::make_shared<QByteArray>();
	auto* job = new NetJob(
		QStringLiteral("PackArtwork::Logo(%1)").arg(instance->id()),
		context->network());
	job->addNetAction(Net::Download::makeByteArray(QUrl(url), bytes.get()));
	connect(job, &NetJob::succeeded, this,
			[this, job, bytes, instance, key, target]() {
				job->deleteLater();
				auto* watcher = new QFutureWatcher<bool>(this);
				connect(watcher, &QFutureWatcher<bool>::finished, this,
						[this, watcher, instance, key]() {
							const bool saved = watcher->result();
							watcher->deleteLater();
							if (saved) {
								useLogo(instance, key);
							}
						});
				watcher->setFuture(QtConcurrent::run(QThreadPool::globalInstance(),
													 &saveLogo, *bytes, target,
													 cacheDir()));
			});
	connect(job, &NetJob::failed, this, [job, url](QString reason) {
		job->deleteLater();
		qDebug() << "Could not download the pack logo" << url << ":" << reason;
	});
	job->start();
}

void PackArtwork::useLogo(QPointer<BaseInstance> instance, const QString& key)
{
	auto* context = LauncherContext::instance();
	if (!context || !instance) {
		return;
	}
	/* Checked again at the last moment: the user may have picked an icon
	 * of their own while the logo was on its way. */
	auto apply = [instance, key]() {
		if (instance &&
			isStockIconKey(instance->managedPackProvider(), instance->iconKey())) {
			instance->setIconKey(key);
		}
	};
	IconList* icons = context->icons().get();
	if (icons->iconFileExists(key)) {
		apply();
		return;
	}
	/* The icon list learns about the new file from its folder watcher and
	 * says so with iconUpdated(). Until then the key does not resolve, and
	 * a view asked to draw it now would draw the fallback and keep it -
	 * image://instanceicon URLs are cached by key - so the instance only
	 * takes the key once the list has it. */
	auto connection = std::make_shared<QMetaObject::Connection>();
	*connection = connect(icons, &IconList::iconUpdated, this,
						  [icons, key, apply, connection](QString updated) {
							  if (updated != key || !icons->iconFileExists(key)) {
								  return;
							  }
							  QObject::disconnect(*connection);
							  apply();
						  });
}

QString PackArtwork::canonicalProvider(const QString& provider)
{
	const QString lower = provider.trimmed().toLower();
	if (lower == QLatin1String("flame")) {
		return QStringLiteral("curseforge");
	}
	return lower;
}

QUrl PackArtwork::lookupUrl(const QString& provider, const QString& packId)
{
	const QString id = packId.trimmed();
	if (id.isEmpty()) {
		return QUrl();
	}
	const QString which = canonicalProvider(provider);
	if (which == QLatin1String("modrinth")) {
		return ModrinthApi::get().projectUrl(id);
	}
	if (which == QLatin1String("curseforge")) {
		/* Every CurseForge request needs the key; Download adds it. */
		if (BuildConfig.CURSEFORGE_API_KEY.isEmpty()) {
			return QUrl();
		}
		return FlameApi::get().projectUrl(id);
	}
	if (which == QLatin1String("modpacksch")) {
		return QUrl(BuildConfig.MODPACKSCH_API_BASE_URL +
					QStringLiteral("public/modpack/%1").arg(id));
	}
	return QUrl();
}

PackArtwork::Found PackArtwork::parseLookup(const QString& provider,
											const QByteArray& reply)
{
	Found found;
	QJsonParseError error;
	const QJsonDocument document = QJsonDocument::fromJson(reply, &error);
	if (error.error != QJsonParseError::NoError || !document.isObject()) {
		return found;
	}
	const QJsonObject root = document.object();
	const QString which = canonicalProvider(provider);

	if (which == QLatin1String("modrinth")) {
		/* A project always has an id; an error body ({"error": ...})
		 * never does. */
		if (root.value(QStringLiteral("id")).toString().isEmpty()) {
			return found;
		}
		found.recognised = true;
		found.title = root.value(QStringLiteral("title")).toString();
		found.logo = root.value(QStringLiteral("icon_url")).toString();
		const QJsonArray gallery = root.value(QStringLiteral("gallery")).toArray();
		for (const QJsonValue& value : gallery) {
			const QJsonObject image = value.toObject();
			if (image.value(QStringLiteral("featured")).toBool()) {
				found.cover = fullSizeImage(image);
				break;
			}
		}
		if (found.cover.isEmpty() && !gallery.isEmpty()) {
			found.cover = fullSizeImage(gallery.first().toObject());
		}
	} else if (which == QLatin1String("curseforge")) {
		const QJsonObject data = root.value(QStringLiteral("data")).toObject();
		if (data.isEmpty()) {
			return found;
		}
		found.recognised = true;
		found.title = data.value(QStringLiteral("name")).toString();
		found.logo = data.value(QStringLiteral("logo"))
						 .toObject()
						 .value(QStringLiteral("url"))
						 .toString();
		const QJsonArray shots =
			data.value(QStringLiteral("screenshots")).toArray();
		if (!shots.isEmpty()) {
			found.cover = shots.first()
							  .toObject()
							  .value(QStringLiteral("url"))
							  .toString();
		}
	} else if (which == QLatin1String("modpacksch")) {
		/* An unknown id is answered with {"status": "error", ...}. */
		if (!root.contains(QStringLiteral("art"))) {
			return found;
		}
		found.recognised = true;
		found.title = root.value(QStringLiteral("name")).toString();
		for (const QJsonValue& value :
			 root.value(QStringLiteral("art")).toArray()) {
			const QJsonObject art = value.toObject();
			const QString type = art.value(QStringLiteral("type")).toString();
			const QString url = art.value(QStringLiteral("url")).toString();
			if (type == QLatin1String("splash") && found.cover.isEmpty()) {
				found.cover = url;
			} else if (type == QLatin1String("square") && found.logo.isEmpty()) {
				found.logo = url;
			}
		}
	}
	return found;
}

QString PackArtwork::derivedLogoUrl(const QString& provider,
									const QString& packId)
{
	const QString id = packId.trimmed();
	if (id.isEmpty() || canonicalProvider(provider) != QLatin1String("atlauncher")) {
		return QString();
	}
	/* Where AtlModpackModel::logoUrl() finds the same logo for the
	 * catalogue; the install records the pack's safe name as its id. */
	return QStringLiteral("%1launcher/images/%2.png")
		.arg(BuildConfig.ATL_DOWNLOAD_SERVER_URL, id.toLower());
}

QString PackArtwork::coverFromGallery(const QVariantList& gallery)
{
	QVariantMap chosen;
	for (const QVariant& entry : gallery) {
		const QVariantMap image = entry.toMap();
		if (image.value(QStringLiteral("featured")).toBool()) {
			chosen = image;
			break;
		}
	}
	if (chosen.isEmpty() && !gallery.isEmpty()) {
		chosen = gallery.first().toMap();
	}
	const QString raw = chosen.value(QStringLiteral("rawUrl")).toString();
	return raw.isEmpty() ? chosen.value(QStringLiteral("url")).toString() : raw;
}

QString PackArtwork::cachePath(const QString& cacheDir, const QString& url)
{
	const QString hash = QString::fromLatin1(
		QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1)
			.toHex());
	static const QStringList kSuffixes = {
		QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
		QStringLiteral("webp"), QStringLiteral("gif"),
	};
	const QString suffix = QFileInfo(QUrl(url).path()).suffix().toLower();
	const QString name = kSuffixes.contains(suffix)
							 ? hash + QLatin1Char('.') + suffix
							 : hash;
	return QDir(cacheDir).filePath(name);
}

bool PackArtwork::isStockIconKey(const QString& provider, const QString& iconKey)
{
	const QString key = iconKey.trimmed();
	/* The launcher's own default, whatever the platform. */
	if (key.isEmpty() || key == QLatin1String("default")) {
		return true;
	}
	/* What each platform's install falls back to without a logo:
	 * InstanceImportTask's "modrinth" and selectFlameIcon()'s "flame" /
	 * "ftb_logo", the modpack models' resolveIconKey() fallbacks
	 * ("ftb_logo"; "grass" for ATLauncher, which has no icon of its own),
	 * and the platforms' names, which older installs used. */
	const QString which = canonicalProvider(provider);
	if (which == QLatin1String("modrinth")) {
		return key == QLatin1String("modrinth");
	}
	if (which == QLatin1String("curseforge")) {
		return key == QLatin1String("flame") ||
			   key == QLatin1String("curseforge") ||
			   key == QLatin1String("ftb_logo");
	}
	if (which == QLatin1String("modpacksch")) {
		return key == QLatin1String("ftb_logo") || key == QLatin1String("ftb");
	}
	if (which == QLatin1String("atlauncher")) {
		return key == QLatin1String("grass") ||
			   key == QLatin1String("atlauncher");
	}
	return false;
}

QString PackArtwork::iconKeyFor(const QString& provider, const QString& packId)
{
	static const QRegularExpression kUnsafe(QStringLiteral("[^A-Za-z0-9_-]"));
	QString id = packId.trimmed();
	id.replace(kUnsafe, QStringLiteral("_"));
	return canonicalProvider(provider) + QLatin1Char('_') + id;
}
