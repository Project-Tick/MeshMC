/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "net/JsonPost.h"

#include <QDebug>
#include <QNetworkRequest>

#include <utility>

#include "Application.h"
#include "BuildConfig.h"
#include "modplatform/flame/FlameApi.h"

namespace Net
{
	JsonPost::JsonPost(QString name, QUrl url, QByteArray body,
					   QObject* parent)
		: Task(parent), m_name(std::move(name)), m_url(std::move(url)),
		  m_body(std::move(body))
	{
	}

	void JsonPost::executeTask()
	{
		setStatus(m_name);
		/* Indeterminate rather than 0/1: the request is one step whose
		 * duration is the server's to decide, and a bar that sits at 0%
		 * reads as stuck. */
		setProgress(0, 0);

		QNetworkRequest request(m_url);
		/* The uncached identity: this request is never served from a
		 * cache and must never be put in one. The answer describes the
		 * files the user has on disk at this moment. */
		request.setHeader(QNetworkRequest::UserAgentHeader,
						  BuildConfig.USER_AGENT_UNCACHED);
		request.setHeader(QNetworkRequest::ContentTypeHeader,
						  QStringLiteral("application/json"));
		request.setRawHeader("Accept", "application/json");

		/* The same condition Net::Download applies. See the header. */
		if (!BuildConfig.CURSEFORGE_API_KEY.isEmpty() &&
			m_url.host() == FlameApi::apiHost()) {
			request.setRawHeader("x-api-key",
								 BuildConfig.CURSEFORGE_API_KEY.toUtf8());
		}

		m_reply.reset(APPLICATION->network()->post(request, m_body));
		connect(m_reply.get(), &QNetworkReply::finished, this,
				&JsonPost::requestFinished);
	}

	bool JsonPost::abort()
	{
		m_aborted = true;
		if (m_reply) {
			/* Triggers finished(), which is where the verdict is
			 * emitted - so there is exactly one of them however the
			 * request ended. */
			m_reply->abort();
			return true;
		}

		emitAborted();
		return true;
	}

	void JsonPost::requestFinished()
	{
		if (m_aborted) {
			m_reply.reset();
			emitAborted();
			return;
		}

		const QNetworkReply::NetworkError error = m_reply->error();
		const QString errorString = m_reply->errorString();
		m_response = m_reply->readAll();
		m_reply.reset();

		if (error != QNetworkReply::NoError) {
			qWarning() << m_name << "failed:" << errorString;
			emitFailed(errorString);
			return;
		}

		emitSucceeded();
	}
} // namespace Net
