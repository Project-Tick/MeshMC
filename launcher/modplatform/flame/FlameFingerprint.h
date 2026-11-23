/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QByteArray>
#include <QString>

#include <nonstd/optional>

/*
 * The file identifier CurseForge looks files up by.
 *
 * Everywhere else in this launcher a file is named to a platform by a
 * cryptographic digest. CurseForge does not offer that: there is no
 * "which version has this SHA-1" endpoint, and the only way to ask what
 * a file on disk is is to send this number to `/v1/fingerprints`. So an
 * export that wants to name a mod in a CurseForge manifest - rather than
 * shipping the jar inside the pack - has to compute it.
 *
 * It is MurmurHash2 (32-bit, seed 1) over the file with every tab,
 * newline, carriage return and space removed first. The filtering is not
 * ours and is not principled - it dates from CurseForge hashing text
 * files whose line endings changed in transit - but it is part of the
 * identifier, so a fingerprint computed without it matches nothing.
 *
 * MurmurHash2 is not a security primitive and is not used as one here:
 * a fingerprint only ever selects a candidate, and what comes back is
 * still checked against the file's real size and digest before the
 * export trusts it.
 */
namespace FlameFingerprint
{
	/* The fingerprint of `data`, which is filtered here rather than by
	 * the caller. */
	quint32 ofData(const QByteArray& data);

	/*
	 * The fingerprint of the file at `path`, or nullopt when it could not
	 * be read.
	 *
	 * Reads in blocks rather than whole: the files this is asked about
	 * include resource packs that are hundreds of megabytes, and an
	 * export hashes every one of them.
	 *
	 * An unreadable file is not an error worth failing an export over -
	 * it simply cannot be named in the manifest and travels inside the
	 * pack instead - so this reports absence rather than a reason.
	 */
	nonstd::optional<quint32> ofFile(const QString& path);
} // namespace FlameFingerprint
