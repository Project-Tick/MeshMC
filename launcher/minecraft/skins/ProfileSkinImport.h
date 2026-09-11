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

#pragma once

#include <QByteArray>
#include <QString>

#include "QObjectPtr.h"
#include "minecraft/skins/SkinEntry.h"
#include "tasks/Task.h"

class NetJob;

/* Download another player's skin by username, into a file.
 *
 * Three requests that each depend on the one before it:
 *
 *   1. name  -> uuid     api.minecraftservices.com/.../lookup/name/<name>
 *   2. uuid  -> profile  sessionserver.mojang.com/.../profile/<uuid>
 *   3. url   -> png      whatever the profile's SKIN texture points at
 *
 * The URL for each step is only known once the previous one has answered, so
 * this is one task that runs three jobs in sequence rather than a job with
 * three actions queued up front.
 *
 * On failure the reason is deliberately short and non-technical ("failed to
 * get user UUID"): the caller puts it inside a sentence naming the player, so
 * it has to read as a clause rather than as a sentence of its own.
 */
class ProfileSkinImport : public Task
{
	Q_OBJECT

  public:
	ProfileSkinImport(QObject* parent, QString username, QString targetPath);
	~ProfileSkinImport() override = default;

	bool canAbort() const override
	{
		return true;
	}

	/* Where the PNG was written. Valid only after success. */
	QString targetPath() const
	{
		return m_targetPath;
	}

	/* The profile's texture URL, so the imported skin can be recognised as
	 * the one that player wears. */
	QString textureUrl() const
	{
		return m_textureUrl;
	}

	SkinEntry::Arms arms() const
	{
		return m_arms;
	}

	/* The cape the player is wearing, under the placeholder id the session
	 * server forces on us. Empty when they have none. */
	QString capeId() const
	{
		return m_capeId;
	}

  public slots:
	bool abort() override;

  protected:
	void executeTask() override;

  private:
	void lookUpUuid();
	void fetchProfile();
	void downloadTexture();

	QString m_username;
	QString m_targetPath;

	QString m_uuid;
	QString m_textureUrl;
	QString m_capeId;
	SkinEntry::Arms m_arms = SkinEntry::Arms::Classic;

	/* Reused for each step; replaced rather than accumulated so that abort()
	 * only ever has one thing to stop. */
	shared_qobject_ptr<NetJob> m_job;
	QByteArray m_response;
};
