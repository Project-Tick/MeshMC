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

#include "FileSink.h"
#include <QFile>
#include <QFileInfo>
#include "FileSystem.h"

namespace Net
{

	FileSink::FileSink(QString filename) : m_filename(filename)
	{
		// nil
	}

	FileSink::~FileSink()
	{
		// nil
	}

	JobStatus FileSink::init(QNetworkRequest& request)
	{
		auto result = initCache(request);
		if (result != Job_InProgress) {
			return result;
		}
		// create a new save file and open it for writing
		if (!FS::ensureFilePathExists(m_filename)) {
			qCritical() << "Could not create folder for " + m_filename;
			return Job_Failed;
		}
		wroteAnyData = false;
		/* Per-attempt, not per-sink: a Download re-inits its sink when it
		 * follows a redirect and when the job retries a failed part, and
		 * each of those starts a brand new file. Carrying the previous
		 * attempt's count over would compare the sum of two transfers
		 * against the length of one - failing a download that arrived
		 * intact, and hiding a truncated one whose leftovers happen to
		 * add up. */
		m_bytesWritten = 0;
		m_output_file.reset(new QSaveFile(m_filename));
		if (!m_output_file->open(QIODevice::WriteOnly)) {
			qCritical() << "Could not open " + m_filename + " for writing";
			return Job_Failed;
		}

		if (initAllValidators(request))
			return Job_InProgress;
		return Job_Failed;
	}

	JobStatus FileSink::initCache(QNetworkRequest&)
	{
		return Job_InProgress;
	}

	JobStatus FileSink::write(QByteArray& data)
	{
		if (!writeAllValidators(data) ||
			m_output_file->write(data) != data.size()) {
			qCritical() << "Failed writing into " + m_filename;
			m_output_file->cancelWriting();
			m_output_file.reset();
			wroteAnyData = false;
			return Job_Failed;
		}
		wroteAnyData = true;
		m_bytesWritten += data.size();
		return Job_InProgress;
	}

	JobStatus FileSink::abort()
	{
		/* A failed write already threw the file away, and abort() is
		 * exactly what Download calls next - so the file being gone is a
		 * normal state here, not a bug to dereference through. */
		if (m_output_file) {
			m_output_file->cancelWriting();
			m_output_file.reset();
		}
		failAllValidators();
		return Job_Failed;
	}

	JobStatus FileSink::finalize(QNetworkReply& reply)
	{
		bool gotFile = false;
		QVariant statusCodeV =
			reply.attribute(QNetworkRequest::HttpStatusCodeAttribute);
		bool validStatus = false;
		int statusCode = statusCodeV.toInt(&validStatus);
		if (validStatus) {
			// this leaves out 304 Not Modified
			gotFile = statusCode == 200 || statusCode == 203;
		}

		/* Whatever came with an error response is not the file we asked
		 * for.
		 *
		 * Servers answer 404 and 502 with a body - an HTML page, a JSON
		 * error, a mirror's "try again later" - and committing it wrote
		 * that page out under the name of the thing being downloaded.
		 * Through the metacache that is worse still: the entry is then
		 * marked fresh with the error page's checksum, so every later
		 * request is a cache hit on a file that was never the download.
		 *
		 * 304 is not an error and must keep its meaning: no body, local
		 * copy stands. Everything else we did not ask for is refused,
		 * body and all - including a 206, which is a slice of the file
		 * rather than the file, and we never requested a range.
		 */
		if (validStatus && !gotFile && statusCode != 304) {
			qCritical() << "Refusing to store the body of HTTP" << statusCode
						<< "as" << m_filename;
			failAllValidators();
			if (m_output_file) {
				m_output_file->cancelWriting();
				m_output_file.reset();
			}
			return Job_Failed;
		}
		/* A write that failed midway already cancelled and dropped the
		 * file, so there is nothing left to commit - say so instead of
		 * walking into a null. */
		if ((gotFile || wroteAnyData) && !m_output_file) {
			qCritical() << "Nothing left to commit for" << m_filename;
			failAllValidators();
			return Job_Failed;
		}
		// if we wrote any data to the save file, we try to commit the data to
		// the real file. if it actually got a proper file, we write it even if
		// it was empty
		if (gotFile || wroteAnyData) {
			/* Refuse a transfer the server cut short.
			 *
			 * Qt reports "finished" whether or not the body arrived in
			 * full, and a dropped connection partway through does not
			 * always surface as a reply error. Committing then writes a
			 * truncated file - and for anything that goes through the
			 * metacache, marks it fresh, so every later attempt reads
			 * the same broken bytes and the download is never retried.
			 * That is how a single network hiccup used to make a modpack
			 * permanently uninstallable: the archive extracted to a CRC
			 * error, forever.
			 *
			 * Only checked when the server told us a length and did not
			 * re-encode the body. With chunked transfer there is no
			 * length to check, and with a Content-Encoding the length
			 * describes the encoded bytes while we have counted the
			 * decoded ones, so in both cases the comparison would be
			 * meaningless rather than merely unavailable.
			 */
			if (wroteAnyData && !reply.hasRawHeader("Content-Encoding")) {
				const QVariant lengthHeader =
					reply.header(QNetworkRequest::ContentLengthHeader);
				bool haveLength = false;
				const qint64 expected = lengthHeader.toLongLong(&haveLength);
				if (haveLength && expected > 0 &&
					m_bytesWritten != expected) {
					qCritical()
						<< "Incomplete download for" << m_filename << "- got"
						<< m_bytesWritten << "of" << expected << "bytes";
					failAllValidators();
					m_output_file->cancelWriting();
					m_output_file.reset();
					return Job_Failed;
				}
			}

			// ask validators for data consistency
			// we only do this for actual downloads, not 'your data is still the
			// same' cache hits
			if (!finalizeAllValidators(reply))
				return Job_Failed;
			// nothing went wrong...
			if (!m_output_file->commit()) {
				qCritical() << "Failed to commit changes to " << m_filename;
				m_output_file->cancelWriting();
				return Job_Failed;
			}
		}
		// then get rid of the save file
		m_output_file.reset();

		return finalizeCache(reply);
	}

	JobStatus FileSink::finalizeCache(QNetworkReply&)
	{
		return Job_Finished;
	}

	bool FileSink::hasLocalData()
	{
		QFileInfo info(m_filename);
		return info.exists() && info.size() != 0;
	}
} // namespace Net
