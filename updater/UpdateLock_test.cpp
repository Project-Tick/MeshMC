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

#include "UpdateLock_test.h"

#include "UpdateLock.h"
#include "UpdaterUtil.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>

#include <thread>

namespace
{

	//! Write a lock file directly, so a test can pretend to be another process.
	void forgeLock(const QString& dataDir, qint64 pid,
				   const QString& stage = QStringLiteral("apply"))
	{
		QSettings file(QDir(dataDir).absoluteFilePath("update.lock"),
					   QSettings::IniFormat);
		file.setValue("startedAt",
					  QDateTime::currentDateTime().toString(Qt::ISODate));
		file.setValue("stage", stage);
		file.setValue("root", QStringLiteral("/somewhere"));
		file.setValue("detail", QStringLiteral("forged"));
		file.setValue("pid", pid);
		file.sync();
	}

	/*!
	 * A process id that is certainly not running: start something trivial, wait
	 * for it to finish, then use the id it no longer owns.
	 */
	qint64 reapedProcessId()
	{
		QProcess process;
#ifdef Q_OS_WIN
		process.setProgram(QStringLiteral("cmd.exe"));
		process.setArguments({QStringLiteral("/c"), QStringLiteral("exit")});
#else
		process.setProgram(QStringLiteral("/bin/sh"));
		process.setArguments({QStringLiteral("-c"), QStringLiteral("exit 0")});
#endif
		process.start();
		if (!process.waitForStarted(10000))
			return -1;
		const qint64 pid = process.processId();
		process.waitForFinished(10000);
		return pid;
	}

} // namespace

void UpdateLockTest::tst_ClaimOnCleanDirectory()
{
	QTemporaryDir data;
	UpdateLock lock(data.path());

	QVERIFY(!lock.peek().present);

	UpdateLockInfo previous;
	QVERIFY(lock.claim("prepare", "/root", "an-url", previous));
	QVERIFY(!previous.present);

	const UpdateLockInfo now = lock.peek();
	QVERIFY(now.present);
	QCOMPARE(now.stage, QStringLiteral("prepare"));
	QCOMPARE(now.root, QStringLiteral("/root"));
	QCOMPARE(now.detail, QStringLiteral("an-url"));
	QCOMPARE(now.pid, QCoreApplication::applicationPid());
	QVERIFY(now.startedAt.isValid());
	QVERIFY(now.ownerAlive()); // that is us
	QVERIFY(QFileInfo::exists(lock.path()));
}

void UpdateLockTest::tst_ClaimRefusedWhileOwnerIsAlive()
{
	QTemporaryDir data;
	// This process is, definitionally, alive.
	forgeLock(data.path(), QCoreApplication::applicationPid());

	UpdateLock lock(data.path());
	UpdateLockInfo previous;
	QVERIFY(!lock.claim("prepare", "/root", "an-url", previous));
	QVERIFY(previous.present);
	QVERIFY(previous.ownerAlive());

	// The existing lock must survive an attempt that was turned away.
	QCOMPARE(lock.peek().detail, QStringLiteral("forged"));
}

void UpdateLockTest::tst_ClaimTakesOverALockWhoseOwnerIsGone()
{
	QTemporaryDir data;
	const qint64 dead = reapedProcessId();
	QVERIFY2(dead > 0, "could not obtain a finished process id");
	forgeLock(data.path(), dead);

	UpdateLock lock(data.path());
	UpdateLockInfo previous;
	// A crashed update must never lock the user out of updating again.
	QVERIFY(lock.claim("prepare", "/root", "an-url", previous));
	QCOMPARE(lock.peek().pid, QCoreApplication::applicationPid());
	QCOMPARE(lock.peek().detail, QStringLiteral("an-url"));
}

void UpdateLockTest::tst_ClaimReportsWhatItTookOver()
{
	QTemporaryDir data;
	forgeLock(data.path(), 0, QStringLiteral("apply"));

	UpdateLock lock(data.path());
	UpdateLockInfo previous;
	QVERIFY(lock.claim("prepare", "/root", "an-url", previous));

	// The caller needs enough detail to warn that the installation may be a
	// mix of two versions.
	QVERIFY(previous.present);
	QCOMPARE(previous.stage, QStringLiteral("apply"));
	QVERIFY(previous.describe().contains("apply"));
	QVERIFY(!previous.describe().isEmpty());
}

void UpdateLockTest::tst_SetStageKeepsTheOriginalStartTime()
{
	QTemporaryDir data;
	UpdateLock first(data.path());
	UpdateLockInfo previous;
	QVERIFY(first.claim("prepare", "/root", "an-url", previous));
	const QDateTime started = first.peek().startedAt;
	QVERIFY(started.isValid());

	// A fresh object, as the second stage has: it adopts the lock rather than
	// claiming it, and must not lose what the first stage recorded.
	UpdateLock adopted(data.path());
	adopted.setStage("apply");

	const UpdateLockInfo now = adopted.peek();
	QCOMPARE(now.stage, QStringLiteral("apply"));
	QCOMPARE(now.startedAt, started);
	QCOMPARE(now.root, QStringLiteral("/root"));
	QCOMPARE(now.detail, QStringLiteral("an-url"));
}

void UpdateLockTest::tst_ReleaseRemovesTheFile()
{
	QTemporaryDir data;
	UpdateLock lock(data.path());
	UpdateLockInfo previous;
	QVERIFY(lock.claim("prepare", "/root", "an-url", previous));
	QVERIFY(QFileInfo::exists(lock.path()));

	lock.release();
	QVERIFY(!QFileInfo::exists(lock.path()));
	QVERIFY(!lock.peek().present);
}

void UpdateLockTest::tst_PeekOnMissingFile()
{
	QTemporaryDir data;
	const UpdateLockInfo info = UpdateLock(data.path()).peek();
	QVERIFY(!info.present);
	QVERIFY(!info.ownerAlive());
	QCOMPARE(info.describe(), QStringLiteral("no update in progress"));
}

void UpdateLockTest::tst_TakeoverAcceptedWhenAnotherProcessClaimsIt()
{
	QTemporaryDir data;
	const qint64 us = QCoreApplication::applicationPid();
	forgeLock(data.path(), us + 1);

	const UpdateLock lock(data.path());
	QCOMPARE(lock.waitForTakeover(us, 0, 2000, 10),
			 UpdateLock::Takeover::Accepted);
}

void UpdateLockTest::tst_TakeoverAcceptedWhenItHappensMidWait()
{
	QTemporaryDir data;
	const qint64 us = QCoreApplication::applicationPid();
	forgeLock(data.path(), us);

	const QString dir = data.path();
	// waitForTakeover blocks, so the "second stage" has to be another thread.
	std::thread secondStage([dir, us]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
		forgeLock(dir, us + 1);
	});

	const UpdateLock lock(dir);
	QElapsedTimer timer;
	timer.start();
	const UpdateLock::Takeover outcome = lock.waitForTakeover(us, us, 5000, 10);
	const qint64 waited = timer.elapsed();
	secondStage.join();

	QCOMPARE(outcome, UpdateLock::Takeover::Accepted);
	// It must actually have polled rather than returned on the first read.
	QVERIFY2(waited >= 100, qPrintable(QString::number(waited)));
}

void UpdateLockTest::tst_TakeoverReleasedWhenUpdateAlreadyFinished()
{
	QTemporaryDir data;
	// No lock file at all: the second stage got there and cleaned up first.
	const UpdateLock lock(data.path());
	QCOMPARE(
		lock.waitForTakeover(QCoreApplication::applicationPid(), 0, 2000, 10),
		UpdateLock::Takeover::Released);
}

void UpdateLockTest::tst_TakeoverOwnerGoneWhenChildDiedWithoutClaiming()
{
	// This is the failure that shipped: the updater inside the downloaded
	// release predates the two-stage protocol, so it exits immediately on
	// arguments it does not recognise, with no console to complain to. The
	// lock is never taken over, and the first stage must notice.
	QTemporaryDir data;
	const qint64 us = QCoreApplication::applicationPid();
	forgeLock(data.path(), us, QStringLiteral("prepare"));

	const qint64 dead = reapedProcessId();
	QVERIFY2(dead > 0, "could not obtain a finished process id");

	const UpdateLock lock(data.path());
	QElapsedTimer timer;
	timer.start();
	const UpdateLock::Takeover outcome =
		lock.waitForTakeover(us, dead, 10000, 10);

	QCOMPARE(outcome, UpdateLock::Takeover::OwnerGone);
	// And it must not sit there for the whole timeout before saying so.
	QVERIFY2(timer.elapsed() < 5000,
			 qPrintable(QString::number(timer.elapsed())));
}

void UpdateLockTest::tst_TakeoverPrefersClaimOverDeadChild()
{
	// A second stage that claimed the lock and then finished very quickly has
	// done its job; a dead child is only a failure if the lock is untouched.
	QTemporaryDir data;
	const qint64 us = QCoreApplication::applicationPid();
	forgeLock(data.path(), us + 1);

	const qint64 dead = reapedProcessId();
	QVERIFY2(dead > 0, "could not obtain a finished process id");

	const UpdateLock lock(data.path());
	QCOMPARE(lock.waitForTakeover(us, dead, 2000, 10),
			 UpdateLock::Takeover::Accepted);
}

void UpdateLockTest::tst_TakeoverTimesOutWhileChildIsStillAlive()
{
	QTemporaryDir data;
	const qint64 us = QCoreApplication::applicationPid();
	forgeLock(data.path(), us);

	const UpdateLock lock(data.path());
	QElapsedTimer timer;
	timer.start();
	// The "child" here is this very process, so it never dies and the only
	// way out is the timeout.
	QCOMPARE(lock.waitForTakeover(us, us, 300, 10),
			 UpdateLock::Takeover::TimedOut);
	QVERIFY2(timer.elapsed() >= 300,
			 qPrintable(QString::number(timer.elapsed())));
}

void UpdateLockTest::tst_DescribeTakeoverCoversEveryOutcome()
{
	const UpdateLock::Takeover all[] = {
		UpdateLock::Takeover::Accepted, UpdateLock::Takeover::Released,
		UpdateLock::Takeover::OwnerGone, UpdateLock::Takeover::TimedOut};
	for (const auto outcome : all) {
		const QString text = UpdateLock::describeTakeover(outcome);
		QVERIFY(!text.isEmpty());
		QVERIFY(text != QStringLiteral("unknown outcome"));
	}
}

QTEST_GUILESS_MAIN(UpdateLockTest)
