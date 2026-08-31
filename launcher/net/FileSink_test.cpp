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

#include <QTest>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QNetworkAccessManager>

#include "net/Download.h"
#include "net/NetJob.h"

namespace
{
	/* A server that answers HTTP badly on purpose.
	 *
	 * Downloads only break in ways worth testing when the other end
	 * misbehaves - a body that stops halfway, an error page where a file
	 * was expected - so the responses are written out by hand rather than
	 * left to a real HTTP server that would insist on being correct.
	 *
	 * Each entry in the script covers one request, so a test can say "cut
	 * the first attempt short, then serve it properly" and check what the
	 * retry ends up with.
	 */
	class ScriptedServer : public QObject
	{
		Q_OBJECT
	  public:
		enum class Reply {
			Whole,		// 200 with the full body
			CutShort,	// 200 promising the full body, delivering a third
			NotFound,	// 404 with a body, as mirrors like to answer
			ServerError // 502 with a body
		};

		ScriptedServer(QByteArray body, QList<Reply> script)
			: m_body(std::move(body)), m_script(std::move(script))
		{
			connect(&m_server, &QTcpServer::newConnection, this,
					&ScriptedServer::onConnection);
		}

		bool listen()
		{
			return m_server.listen(QHostAddress::LocalHost, 0);
		}

		QUrl url() const
		{
			return QUrl(QStringLiteral("http://127.0.0.1:%1/modpack.zip")
							.arg(m_server.serverPort()));
		}

		int requestsServed() const
		{
			return m_served;
		}

	  private slots:
		void onConnection()
		{
			while (auto* socket = m_server.nextPendingConnection()) {
				connect(socket, &QTcpSocket::readyRead, this,
						[this, socket]() { onReadyRead(socket); });
				connect(socket, &QTcpSocket::disconnected, socket,
						&QTcpSocket::deleteLater);
			}
		}

	  private:
		void onReadyRead(QTcpSocket* socket)
		{
			m_pending[socket].append(socket->readAll());
			if (!m_pending[socket].contains("\r\n\r\n")) {
				return; // headers still coming
			}
			m_pending.remove(socket);

			// Past the end of the script every further request gets the
			// last scripted answer, so a test does not have to predict how
			// many times the job will retry.
			const Reply reply =
				m_script.isEmpty()
					? Reply::Whole
					: m_script.at(qMin(m_served, m_script.size() - 1));
			m_served++;
			respond(socket, reply);
		}

		void respond(QTcpSocket* socket, Reply reply)
		{
			switch (reply) {
				case Reply::Whole:
					writeHead(socket, "200 OK", m_body.size());
					socket->write(m_body);
					break;
				case Reply::CutShort:
					/* The length header promises the whole file and then
					 * the connection goes away mid-body - a dropped
					 * transfer, or a mirror serving a file it only has
					 * part of. */
					writeHead(socket, "200 OK", m_body.size());
					socket->write(m_body.left(m_body.size() / 3));
					break;
				case Reply::NotFound: {
					const QByteArray page = errorPage();
					writeHead(socket, "404 Not Found", page.size());
					socket->write(page);
					break;
				}
				case Reply::ServerError: {
					const QByteArray page = errorPage();
					writeHead(socket, "502 Bad Gateway", page.size());
					socket->write(page);
					break;
				}
			}
			socket->flush();
			socket->disconnectFromHost();
		}

		void writeHead(QTcpSocket* socket, const char* status, qint64 length)
		{
			socket->write("HTTP/1.1 ");
			socket->write(status);
			socket->write("\r\nContent-Type: application/zip\r\n");
			socket->write("Content-Length: ");
			socket->write(QByteArray::number(length));
			socket->write("\r\nConnection: close\r\n\r\n");
		}

		static QByteArray errorPage()
		{
			return QByteArrayLiteral(
				"<html><body>The pack you are looking for is not "
				"here.</body></html>");
		}

		QTcpServer m_server;
		QByteArray m_body;
		QList<Reply> m_script;
		QHash<QTcpSocket*, QByteArray> m_pending;
		int m_served = 0;
	};

	// Body big enough that a third of it is a meaningful amount of file,
	// small enough that the tests stay instant.
	QByteArray packBody()
	{
		QByteArray body;
		for (int i = 0; i < 4096; i++) {
			body.append(QByteArray::number(i));
			body.append(' ');
		}
		return body;
	}

	QByteArray readAll(const QString& path)
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			return {};
		}
		return file.readAll();
	}

	/* Run a one-file download job to completion. Returns true if the job
	 * succeeded. */
	bool runDownload(const QUrl& url, const QString& target)
	{
		auto network = shared_qobject_ptr<QNetworkAccessManager>(
			new QNetworkAccessManager());
		NetJob job(QStringLiteral("test download"), network);
		job.addNetAction(Net::Download::makeFile(url, target));

		QSignalSpy finished(&job, &Task::finished);
		job.start();
		if (!job.isFinished() && !finished.wait(30000)) {
			return false;
		}
		return job.wasSuccessful();
	}
} // namespace

class FileSinkTest : public QObject
{
	Q_OBJECT
  private slots:

	// The ordinary case, so the tests below are known to be measuring
	// misbehaviour and not a broken harness.
	void test_WholeBodyIsStored()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		const QString target = QDir(tempDir.path()).filePath("modpack.zip");

		ScriptedServer server(packBody(), {ScriptedServer::Reply::Whole});
		QVERIFY(server.listen());

		QVERIFY(runDownload(server.url(), target));
		QCOMPARE(readAll(target), packBody());
	}

	// A body that stops short of its own Content-Length is not the file.
	// Nothing may be left on disk claiming to be it: the next thing to
	// touch that path is an unpacker, and half an archive unpacks to a CRC
	// error rather than to a complaint about the download.
	void test_CutShortBodyIsNotStored()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		const QString target = QDir(tempDir.path()).filePath("modpack.zip");

		ScriptedServer server(packBody(), {ScriptedServer::Reply::CutShort});
		QVERIFY(server.listen());

		QVERIFY(!runDownload(server.url(), target));
		QVERIFY2(!QFile::exists(target),
				 "an incomplete download must not be left behind");
	}

	// The point of the retries: one cut transfer, then a good one, has to
	// end with the whole file. This is where a per-sink byte count instead
	// of a per-attempt one bites - the retry delivers every byte and is
	// still rejected, because the bytes of the failed attempt are counted
	// with it, and the download that would have worked never does.
	void test_RetryAfterCutShortStoresWholeBody()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		const QString target = QDir(tempDir.path()).filePath("modpack.zip");

		ScriptedServer server(packBody(), {ScriptedServer::Reply::CutShort,
										   ScriptedServer::Reply::Whole});
		QVERIFY(server.listen());

		QVERIFY(runDownload(server.url(), target));
		QCOMPARE(server.requestsServed(), 2);
		QCOMPARE(readAll(target), packBody());
	}

	// An error page is not a modpack. Storing one used to leave the HTML
	// sitting where the archive should be - and through the metacache it
	// was stored as a fresh, checksummed cache entry, so every later
	// attempt was a cache hit on a file that had never been the download.
	void test_NotFoundBodyIsNotStored()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		const QString target = QDir(tempDir.path()).filePath("modpack.zip");

		ScriptedServer server(packBody(), {ScriptedServer::Reply::NotFound});
		QVERIFY(server.listen());

		QVERIFY(!runDownload(server.url(), target));
		QVERIFY2(!QFile::exists(target),
				 "the body of a 404 must not be stored as the file");
	}

	// Same for a gateway that is having a bad day, which is the answer a
	// busy mirror gives.
	void test_ServerErrorBodyIsNotStored()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		const QString target = QDir(tempDir.path()).filePath("modpack.zip");

		ScriptedServer server(packBody(), {ScriptedServer::Reply::ServerError});
		QVERIFY(server.listen());

		QVERIFY(!runDownload(server.url(), target));
		QVERIFY(!QFile::exists(target));
	}

	// A transfer that fails once with an error page and then succeeds still
	// has to end up with the file, not with the page.
	void test_RetryAfterErrorPageStoresWholeBody()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		const QString target = QDir(tempDir.path()).filePath("modpack.zip");

		ScriptedServer server(packBody(), {ScriptedServer::Reply::ServerError,
										   ScriptedServer::Reply::Whole});
		QVERIFY(server.listen());

		QVERIFY(runDownload(server.url(), target));
		QCOMPARE(readAll(target), packBody());
	}
};

QTEST_GUILESS_MAIN(FileSinkTest)

#include "FileSink_test.moc"
