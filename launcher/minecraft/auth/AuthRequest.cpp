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

#include <cassert>

#include <QDebug>
#include <QTimer>
#include <QBuffer>
#include <QUrlQuery>

#include "Application.h"
#include "AuthRequest.h"
#include "plugin/PluginHooks.h"
#include "plugin/PluginManager.h"
#include "katabasis/Globals.h"

namespace
{
	/*
	 * dispatchAuthRequestHook — run MMCO_HOOK_AUTH_REQUEST over the
	 * in-flight request and apply any redirect/header mutations the
	 * plugins request.
	 *
	 * Returns true if the hook chain *cancelled* the request — the
	 * caller must abort and emit a network error in that case.
	 */
	bool dispatchAuthRequestHook(QNetworkRequest& request,
								 const QByteArray& body, const char* method)
	{
		auto* pm = APPLICATION ? APPLICATION->pluginManager() : nullptr;
		if (!pm)
			return false;

		/* The add_header callback closes over the request reference and
		 * appends raw headers. We keep it as a thread-local C function
		 * pointer with a sidecar state struct so the closure can survive
		 * the C ABI boundary. */
		struct HeaderCtx {
			QNetworkRequest* req;
		};
		HeaderCtx hctx{&request};

		auto add_header_fn = [](void* handle, const char* key,
								const char* value) -> int {
			if (!handle || !key || !value)
				return -1;
			auto* h = static_cast<HeaderCtx*>(handle);
			h->req->setRawHeader(QByteArray(key), QByteArray(value));
			return 0;
		};

		const QByteArray urlUtf8 = request.url().toString().toUtf8();

		MMCOAuthRequestEvent ev{};
		ev.url = urlUtf8.constData();
		ev.method = method;
		ev.body = body.isEmpty() ? nullptr : body.constData();
		ev.body_size = body.size();
		ev.redirect_url = nullptr;
		ev.request_handle = &hctx;
		ev.add_header = add_header_fn;

		const bool cancelled = pm->dispatchHook(MMCO_HOOK_AUTH_REQUEST, &ev);
		if (cancelled)
			return true;

		if (ev.redirect_url && *ev.redirect_url) {
			const QUrl rewritten =
				QUrl::fromUserInput(QString::fromUtf8(ev.redirect_url));
			if (rewritten.isValid())
				request.setUrl(rewritten);
		}
		return false;
	}
} // namespace

AuthRequest::AuthRequest(QObject* parent) : QObject(parent) {}

AuthRequest::~AuthRequest() {}

void AuthRequest::get(const QNetworkRequest& req, int timeout /* = 60*1000*/)
{
	setup(req, QNetworkAccessManager::GetOperation);

	/* Let MMCO plugins observe / rewrite / cancel the request. */
	if (dispatchAuthRequestHook(request_, QByteArray(), "GET")) {
		error_ = QNetworkReply::ProtocolUnknownError;
		errorString_ = QStringLiteral("AuthRequest cancelled by plugin");
		QTimer::singleShot(0, this, [this]() {
			emit finished(error_, QByteArray(),
						  QList<QNetworkReply::RawHeaderPair>{});
		});
		return;
	}

	reply_ = APPLICATION->network()->get(request_);
	status_ = Requesting;
	timedReplies_.add(new Katabasis::Reply(reply_, timeout));
	connect(reply_, &QNetworkReply::errorOccurred, this,
			&AuthRequest::onRequestError);
	connect(reply_, &QNetworkReply::finished, this,
			&AuthRequest::onRequestFinished);
	connect(reply_, &QNetworkReply::sslErrors, this, &AuthRequest::onSslErrors);
}

void AuthRequest::post(const QNetworkRequest& req, const QByteArray& data,
					   int timeout /* = 60*1000*/)
{
	setup(req, QNetworkAccessManager::PostOperation);
	data_ = data;

	/* Hook dispatch — plugins see the body and may rewrite the URL or
	 * append headers before the request is sent. */
	if (dispatchAuthRequestHook(request_, data_, "POST")) {
		error_ = QNetworkReply::ProtocolUnknownError;
		errorString_ = QStringLiteral("AuthRequest cancelled by plugin");
		QTimer::singleShot(0, this, [this]() {
			emit finished(error_, QByteArray(),
						  QList<QNetworkReply::RawHeaderPair>{});
		});
		return;
	}

	status_ = Requesting;
	reply_ = APPLICATION->network()->post(request_, data_);
	timedReplies_.add(new Katabasis::Reply(reply_, timeout));
	connect(reply_, &QNetworkReply::errorOccurred, this,
			&AuthRequest::onRequestError);
	connect(reply_, &QNetworkReply::finished, this,
			&AuthRequest::onRequestFinished);
	connect(reply_, &QNetworkReply::sslErrors, this, &AuthRequest::onSslErrors);
	connect(reply_, &QNetworkReply::uploadProgress, this,
			&AuthRequest::onUploadProgress);
}

void AuthRequest::onRequestFinished()
{
	if (status_ == Idle) {
		return;
	}
	if (reply_ != qobject_cast<QNetworkReply*>(sender())) {
		return;
	}
	httpStatus_ =
		reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	finish();
}

void AuthRequest::onRequestError(QNetworkReply::NetworkError error)
{
	qWarning() << "AuthRequest::onRequestError: Error" << (int)error;
	if (status_ == Idle) {
		return;
	}
	if (reply_ != qobject_cast<QNetworkReply*>(sender())) {
		return;
	}
	errorString_ = reply_->errorString();
	httpStatus_ =
		reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	error_ = error;
	qWarning() << "AuthRequest::onRequestError: Error string: " << errorString_;
	qWarning() << "AuthRequest::onRequestError: HTTP status" << httpStatus_
			   << reply_->attribute(QNetworkRequest::HttpReasonPhraseAttribute)
					  .toString();

	// QTimer::singleShot(10, this, &AuthRequest::finish);
}

void AuthRequest::onSslErrors(QList<QSslError> errors)
{
	int i = 1;
	for (auto error : errors) {
		qCritical() << "LOGIN SSL Error #" << i << " : " << error.errorString();
		auto cert = error.certificate();
		qCritical() << "Certificate in question:\n" << cert.toText();
		i++;
	}
}

void AuthRequest::onUploadProgress(qint64 uploaded, qint64 total)
{
	if (status_ == Idle) {
		qWarning() << "AuthRequest::onUploadProgress: No pending request";
		return;
	}
	if (reply_ != qobject_cast<QNetworkReply*>(sender())) {
		return;
	}
	// Restart timeout because request in progress
	Katabasis::Reply* o2Reply = timedReplies_.find(reply_);
	if (o2Reply) {
		o2Reply->start();
	}
	emit uploadProgress(uploaded, total);
}

void AuthRequest::setup(const QNetworkRequest& req,
						QNetworkAccessManager::Operation operation,
						const QByteArray& verb)
{
	request_ = req;
	operation_ = operation;
	url_ = req.url();

	QUrl url = url_;
	request_.setUrl(url);

	if (!verb.isEmpty()) {
		request_.setRawHeader(Katabasis::HTTP_HTTP_HEADER, verb);
	}

	status_ = Requesting;
	error_ = QNetworkReply::NoError;
	errorString_.clear();
	httpStatus_ = 0;
}

void AuthRequest::finish()
{
	QByteArray data;
	if (status_ == Idle) {
		qWarning() << "AuthRequest::finish: No pending request";
		return;
	}
	data = reply_->readAll();
	status_ = Idle;
	timedReplies_.remove(reply_);
	reply_->disconnect(this);
	reply_->deleteLater();
	QList<QNetworkReply::RawHeaderPair> headers = reply_->rawHeaderPairs();
	emit finished(error_, data, headers);
}
