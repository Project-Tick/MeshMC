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

#include <QTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "GZip.h"
#include "minecraft/World.h"

/*
 * Minimal big-endian NBT writer, so the tests can build level.dat files
 * without depending on how the launcher serializes them.
 */
namespace
{
	const quint8 TAG_END = 0;
	const quint8 TAG_INT = 3;
	const quint8 TAG_LONG = 4;
	const quint8 TAG_STRING = 8;
	const quint8 TAG_COMPOUND = 10;

	void putU8(QByteArray& out, quint8 value)
	{
		out.append(static_cast<char>(value));
	}

	void putU16(QByteArray& out, quint16 value)
	{
		out.append(static_cast<char>((value >> 8) & 0xFF));
		out.append(static_cast<char>(value & 0xFF));
	}

	void putI32(QByteArray& out, qint32 value)
	{
		for (int shift = 24; shift >= 0; shift -= 8) {
			out.append(static_cast<char>((value >> shift) & 0xFF));
		}
	}

	void putI64(QByteArray& out, qint64 value)
	{
		for (int shift = 56; shift >= 0; shift -= 8) {
			out.append(static_cast<char>((value >> shift) & 0xFF));
		}
	}

	void putString(QByteArray& out, const QByteArray& value)
	{
		putU16(out, static_cast<quint16>(value.size()));
		out.append(value);
	}

	void putTagHeader(QByteArray& out, quint8 type, const QByteArray& name)
	{
		putU8(out, type);
		putString(out, name);
	}

	void putStringTag(QByteArray& out, const QByteArray& name,
					  const QByteArray& value)
	{
		putTagHeader(out, TAG_STRING, name);
		putString(out, value);
	}

	void putIntTag(QByteArray& out, const QByteArray& name, qint32 value)
	{
		putTagHeader(out, TAG_INT, name);
		putI32(out, value);
	}

	void putLongTag(QByteArray& out, const QByteArray& name, qint64 value)
	{
		putTagHeader(out, TAG_LONG, name);
		putI64(out, value);
	}

	struct LevelDatSpec {
		bool worldGenSettingsSeed = false;
		qint64 worldGenSettingsSeedValue = 0;
		bool randomSeed = false;
		qint64 randomSeedValue = 0;
	};

	QByteArray makeLevelDat(const LevelDatSpec& spec)
	{
		QByteArray dataPayload;
		putStringTag(dataPayload, "LevelName", "Test World");
		putLongTag(dataPayload, "LastPlayed", Q_INT64_C(1600000000000));
		putIntTag(dataPayload, "GameType", 1);
		if (spec.worldGenSettingsSeed) {
			putTagHeader(dataPayload, TAG_COMPOUND, "WorldGenSettings");
			putLongTag(dataPayload, "seed", spec.worldGenSettingsSeedValue);
			putU8(dataPayload, TAG_END);
		}
		if (spec.randomSeed) {
			putLongTag(dataPayload, "RandomSeed", spec.randomSeedValue);
		}
		putU8(dataPayload, TAG_END);

		QByteArray root;
		putTagHeader(root, TAG_COMPOUND, ""); // unnamed root compound
		putTagHeader(root, TAG_COMPOUND, "Data");
		root.append(dataPayload);
		putU8(root, TAG_END);
		return root;
	}

	bool writeWorld(const QString& worldPath, const QByteArray& levelDat)
	{
		if (!QDir().mkpath(worldPath)) {
			return false;
		}
		QByteArray compressed;
		if (!GZip::zip(levelDat, compressed)) {
			return false;
		}
		QFile file(QDir(worldPath).absoluteFilePath("level.dat"));
		if (!file.open(QIODevice::WriteOnly)) {
			return false;
		}
		return file.write(compressed) == compressed.size();
	}
} // namespace

class WorldTest : public QObject
{
	Q_OBJECT
  private slots:

	// Minecraft 1.16 and newer: Data -> WorldGenSettings -> seed
	void test_ReadSeedFromWorldGenSettings()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		auto worldPath = QDir(tempDir.path()).absoluteFilePath("modern");

		LevelDatSpec spec;
		spec.worldGenSettingsSeed = true;
		spec.worldGenSettingsSeedValue = Q_INT64_C(-4172144997902289642);
		QVERIFY(writeWorld(worldPath, makeLevelDat(spec)));

		World world{QFileInfo(worldPath)};
		QVERIFY(world.isValid());
		QCOMPARE(world.name(), QString("Test World"));
		QCOMPARE(world.seed(), Q_INT64_C(-4172144997902289642));
	}

	// Legacy worlds: Data -> RandomSeed
	void test_ReadSeedFromRandomSeed()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		auto worldPath = QDir(tempDir.path()).absoluteFilePath("legacy");

		LevelDatSpec spec;
		spec.randomSeed = true;
		spec.randomSeedValue = Q_INT64_C(1234567890123456789);
		QVERIFY(writeWorld(worldPath, makeLevelDat(spec)));

		World world{QFileInfo(worldPath)};
		QVERIFY(world.isValid());
		QCOMPARE(world.seed(), Q_INT64_C(1234567890123456789));
	}

	// If both are present, the modern location wins
	void test_WorldGenSettingsTakesPrecedence()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		auto worldPath = QDir(tempDir.path()).absoluteFilePath("both");

		LevelDatSpec spec;
		spec.worldGenSettingsSeed = true;
		spec.worldGenSettingsSeedValue = Q_INT64_C(42);
		spec.randomSeed = true;
		spec.randomSeedValue = Q_INT64_C(1337);
		QVERIFY(writeWorld(worldPath, makeLevelDat(spec)));

		World world{QFileInfo(worldPath)};
		QVERIFY(world.isValid());
		QCOMPARE(world.seed(), Q_INT64_C(42));
	}

	// No seed anywhere: the world still loads, the seed is just unknown
	void test_MissingSeedIsZero()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		auto worldPath = QDir(tempDir.path()).absoluteFilePath("seedless");

		QVERIFY(writeWorld(worldPath, makeLevelDat(LevelDatSpec())));

		World world{QFileInfo(worldPath)};
		QVERIFY(world.isValid());
		QCOMPARE(world.seed(), Q_INT64_C(0));
	}
};

QTEST_GUILESS_MAIN(WorldTest)

#include "World_test.moc"
