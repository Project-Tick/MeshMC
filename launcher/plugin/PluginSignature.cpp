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

#include "plugin/PluginSignature.h"
#include "plugin/MMCOFormat.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QtEndian>

/*
 * MESHMC_PLUGIN_SIGNATURES is defined in launcher/CMakeLists.txt:
 *   1 → full GpgME-backed verifier (Linux, macOS, MinGW Windows)
 *   0 → stub verifier that always reports modules as unsigned. Used on
 *       MSVC where upstream gpgme has no working build (autotools-only,
 *       C++ ABI incompatibilities with MSVC).
 */
#ifndef MESHMC_PLUGIN_SIGNATURES
#define MESHMC_PLUGIN_SIGNATURES 1
#endif

#if MESHMC_PLUGIN_SIGNATURES
#include <gpgme++/context.h>
#include <gpgme++/data.h>
#include <gpgme++/global.h>
#include <gpgme++/key.h>
#include <gpgme++/verificationresult.h>
#include <gpgme++/engineinfo.h>
#endif

namespace PluginSignature
{

	namespace
	{
		/* Mutex protects the static keyring-path override. GpgME contexts
		 * themselves are short-lived and constructed per-call. */
		QMutex g_mutex;
		QString g_keyringPath;

#if MESHMC_PLUGIN_SIGNATURES
		bool g_gpgmeInitialised = false;

		void ensureGpgmeInit()
		{
			if (g_gpgmeInitialised)
				return;
			GpgME::initializeLibrary();
			g_gpgmeInitialised = true;
		}
#endif

		/* Curated allow-list of OSI-approved / FSF-libre SPDX identifiers.
		 * This is deliberately a fixed list rather than a regex against
		 * the upstream SPDX database — we want a predictable, auditable
		 * set of licenses that exempt plugins from signing.
		 *
		 * The names are stored lower-case so the check is case-insensitive. */
		const QSet<QString>& ossSpdxIds()
		{
			static const QSet<QString> ids = {
				// GPL family
				QStringLiteral("gpl-2.0-only"),
				QStringLiteral("gpl-2.0-or-later"),
				QStringLiteral("gpl-3.0-only"),
				QStringLiteral("gpl-3.0-or-later"),
				QStringLiteral("lgpl-2.1-only"),
				QStringLiteral("lgpl-2.1-or-later"),
				QStringLiteral("lgpl-3.0-only"),
				QStringLiteral("lgpl-3.0-or-later"),
				QStringLiteral("agpl-3.0-only"),
				QStringLiteral("agpl-3.0-or-later"),
				// Permissive
				QStringLiteral("mit"),
				QStringLiteral("mit-0"),
				QStringLiteral("apache-2.0"),
				QStringLiteral("bsd-2-clause"),
				QStringLiteral("bsd-3-clause"),
				QStringLiteral("bsd-3-clause-clear"),
				QStringLiteral("isc"),
				QStringLiteral("zlib"),
				QStringLiteral("unlicense"),
				QStringLiteral("0bsd"),
				// Weak copyleft
				QStringLiteral("mpl-2.0"),
				QStringLiteral("epl-1.0"),
				QStringLiteral("epl-2.0"),
				QStringLiteral("cddl-1.0"),
				QStringLiteral("cddl-1.1"),
				// Public domain-ish
				QStringLiteral("cc0-1.0"),
				QStringLiteral("wtfpl"),
				// MeshMC's own modular GPL variant — modules using this
				// follow the launcher's exception language.
				QStringLiteral("gpl-3.0-or-later with "
							   "licenseref-meshmc-mmco-module-exception-1.0"),
			};
			return ids;
		}

		/* Split an SPDX expression into the atomic license identifiers
		 * that compose it. Handles AND / OR / WITH operators by simple
		 * tokenisation — we don't try to fully parse precedence. */
		QStringList splitSpdx(const QString& expr)
		{
			QString s = expr.toLower().trimmed();
			// Drop parentheses
			s.replace('(', ' ');
			s.replace(')', ' ');

			QStringList atoms;
			QString current;
			const auto flush = [&]() {
				QString a = current.trimmed();
				if (!a.isEmpty())
					atoms.append(a);
				current.clear();
			};

			const QStringList tokens =
				s.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
			for (int i = 0; i < tokens.size(); ++i) {
				const QString& t = tokens[i];
				if (t == QStringLiteral("or") || t == QStringLiteral("and")) {
					flush();
					continue;
				}
				if (t == QStringLiteral("with")) {
					// keep accumulating into current so "GPL-3.0 WITH foo"
					// stays a single atom
					current += QLatin1Char(' ');
					current += t;
					current += QLatin1Char(' ');
					continue;
				}
				if (!current.isEmpty())
					current += QLatin1Char(' ');
				current += t;
			}
			flush();
			return atoms;
		}
	} // namespace

	void setKeyringPath(const QString& path)
	{
		QMutexLocker lock(&g_mutex);
		g_keyringPath = path;
	}

	bool isOpenSourceLicense(const QString& spdxLicense)
	{
		if (spdxLicense.trimmed().isEmpty())
			return false;

		const QStringList atoms = splitSpdx(spdxLicense);
		const QSet<QString>& whitelist = ossSpdxIds();

		for (const QString& atom : atoms) {
			if (whitelist.contains(atom))
				return true;
			// Also accept a bare "with"-prefixed atom whose head id is OSS
			// (e.g. "gpl-3.0-or-later with classpath-exception-2.0").
			const int withIdx = atom.indexOf(QStringLiteral(" with "));
			if (withIdx > 0) {
				const QString head = atom.left(withIdx).trimmed();
				if (whitelist.contains(head))
					return true;
			}
		}
		return false;
	}

	ExtractedTrailer extractTrailer(const QString& filePath)
	{
		ExtractedTrailer out;

		QFile f(filePath);
		if (!f.open(QIODevice::ReadOnly))
			return out;

		const qint64 size = f.size();
		const qint64 footerSize =
			static_cast<qint64>(sizeof(quint64) + sizeof(quint32));
		if (size < footerSize)
			return out; // file too small to contain a trailer

		// Read the 12-byte footer (uint64 sigSize + uint32 magic).
		if (!f.seek(size - footerSize))
			return out;

		QByteArray footerBytes = f.read(footerSize);
		if (footerBytes.size() != footerSize)
			return out;

		// Read the two trailer fields as little-endian.
		const quint32 magic =
			qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(
				footerBytes.constData() + sizeof(quint64)));

		if (magic != static_cast<quint32>(MMCO_TRAILER_MAGIC))
			return out; // no trailer

		out.present = true;

		quint64 sigSize = qFromLittleEndian<quint64>(
			reinterpret_cast<const uchar*>(footerBytes.constData()));

		// Sanity: signature must fit before the footer.
		if (sigSize == 0 || static_cast<qint64>(sigSize) > size - footerSize) {
			out.malformed = true;
			return out;
		}

		const qint64 payloadSize =
			size - footerSize - static_cast<qint64>(sigSize);

		// Read payload then signature.
		if (!f.seek(0)) {
			out.malformed = true;
			return out;
		}
		out.payload = f.read(payloadSize);
		if (out.payload.size() != payloadSize) {
			out.malformed = true;
			out.payload.clear();
			return out;
		}
		out.signature = f.read(static_cast<qint64>(sigSize));
		if (out.signature.size() != static_cast<qint64>(sigSize)) {
			out.malformed = true;
			out.payload.clear();
			out.signature.clear();
			return out;
		}
		return out;
	}

	PluginSignatureState verify(const QByteArray& payload,
								const QByteArray& signature, QString& detail,
								QString& fingerprint)
	{
		detail.clear();
		fingerprint.clear();

#if !MESHMC_PLUGIN_SIGNATURES
		(void)payload;
		(void)signature;
		// Stub build (MSVC): no GpgME backend is linked. Report an
		// engine-unavailable Error so the license/signature policy
		// treats the module the same way it would on a Linux box
		// without a working keyring. OSS modules still load; non-OSS
		// modules are rejected with SignatureRequired.
		detail = QStringLiteral(
			"GPG verification is not available in this build of MeshMC");
		return PluginSignatureState::Error;
#else
		ensureGpgmeInit();

		// Engine check
		const auto err = GpgME::checkEngine(GpgME::OpenPGP);
		if (err) {
			detail = QStringLiteral("GpgME engine unavailable: %1")
						 .arg(QString::fromUtf8(err.asString()));
			return PluginSignatureState::Error;
		}

		std::unique_ptr<GpgME::Context> ctx(
			GpgME::Context::createForProtocol(GpgME::OpenPGP));
		if (!ctx) {
			detail = QStringLiteral("Failed to create GpgME context");
			return PluginSignatureState::Error;
		}

		QString home;
		{
			QMutexLocker lock(&g_mutex);
			home = g_keyringPath;
		}
		if (!home.isEmpty()) {
			const QByteArray homeBytes = home.toLocal8Bit();
			ctx->setEngineHomeDirectory(homeBytes.constData());
		}

		GpgME::Data sigData(signature.constData(),
							static_cast<size_t>(signature.size()),
							/* copy */ false);
		GpgME::Data payloadData(payload.constData(),
								static_cast<size_t>(payload.size()),
								/* copy */ false);

		const GpgME::VerificationResult result =
			ctx->verifyDetachedSignature(sigData, payloadData);

		if (result.error()) {
			detail = QStringLiteral("Verification error: %1")
						 .arg(QString::fromUtf8(result.error().asString()));
			return PluginSignatureState::Error;
		}

		const auto sigs = result.signatures();
		if (sigs.empty()) {
			detail = QStringLiteral("Trailer contained no parsable signatures");
			return PluginSignatureState::BadSignature;
		}

		// We only honour the first signature.
		const auto& sig = sigs.front();
		if (sig.fingerprint())
			fingerprint = QString::fromUtf8(sig.fingerprint());

		const auto status = sig.status();
		if (status.code() != 0) {
			// status() is set for bad / expired / revoked signatures.
			detail = QStringLiteral("Bad signature: %1")
						 .arg(QString::fromUtf8(status.asString()));
			return PluginSignatureState::BadSignature;
		}

		const auto summary = sig.summary();
		if (summary & GpgME::Signature::Red) {
			detail = QStringLiteral("Signature failed verification");
			return PluginSignatureState::BadSignature;
		}

		// Green = good and trusted. Valid = good but trust below threshold.
		if (summary & GpgME::Signature::Green) {
			detail = QStringLiteral("Good signature");
			return PluginSignatureState::Valid;
		}
		if (summary & GpgME::Signature::Valid) {
			detail = QStringLiteral("Good signature (trust below threshold)");
			return PluginSignatureState::Valid;
		}
		if (summary & GpgME::Signature::KeyMissing) {
			detail = QStringLiteral(
				"Signing key not present in the trusted keyring");
			return PluginSignatureState::Untrusted;
		}
		// Anything else we treat as "verified by an unknown / untrusted key".
		detail =
			QStringLiteral("Signature present but signing key not trusted");
		return PluginSignatureState::Untrusted;
#endif // MESHMC_PLUGIN_SIGNATURES
	}

	PluginSignatureState verifyFile(const QString& filePath, QString& detail,
									QString& fingerprint)
	{
		ExtractedTrailer trailer = extractTrailer(filePath);
		if (!trailer.present) {
			detail.clear();
			fingerprint.clear();
			return PluginSignatureState::Absent;
		}
		if (trailer.malformed) {
			detail = QStringLiteral("Malformed signature trailer");
			fingerprint.clear();
			return PluginSignatureState::Malformed;
		}
		return verify(trailer.payload, trailer.signature, detail, fingerprint);
	}

	const char* stateLabel(PluginSignatureState state)
	{
		switch (state) {
			case PluginSignatureState::NotChecked:
				return "Not checked";
			case PluginSignatureState::Absent:
				return "Unsigned";
			case PluginSignatureState::Valid:
				return "Signed (trusted)";
			case PluginSignatureState::Untrusted:
				return "Signed (untrusted key)";
			case PluginSignatureState::BadSignature:
				return "Bad signature";
			case PluginSignatureState::Malformed:
				return "Malformed signature";
			case PluginSignatureState::Error:
				return "Verification error";
		}
		return "Unknown";
	}

} // namespace PluginSignature
