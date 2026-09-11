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

#include <QImage>
#include <QList>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include "ui/dialogs/skins/render/CubeMesh.h"

class QOpenGLTexture;

namespace skinrender
{
	/* The player model: body, second layer, arms, and either a cape or a pair
	 * of elytra wings.
	 *
	 * Both arm widths are built up front and the unused pair is simply not
	 * drawn, because switching between Classic and Slim happens while the
	 * user clicks radio buttons and rebuilding vertex buffers for that would
	 * be needless work on the UI thread.
	 *
	 * Requires a current OpenGL context for its whole lifetime.
	 */
	class PlayerScene : protected QOpenGLFunctions
	{
	  public:
		PlayerScene(const QImage& skin, bool slim, const QImage& cape);
		virtual ~PlayerScene();

		void draw(QOpenGLShaderProgram* program);

		void setSkin(const QImage& skin);
		void setCape(const QImage& cape);
		void setSlim(bool slim);
		void setCapeVisible(bool visible);

		/* Draw the cape as elytra wings instead of as a cape. Same texture:
		 * the elytra sample a different region of it. */
		void setElytraVisible(bool visible);

	  private:
		PlayerScene(const PlayerScene&) = delete;
		PlayerScene& operator=(const PlayerScene&) = delete;

		QList<CubeMesh*> m_body;
		QList<CubeMesh*> m_bodyOverlay;
		QList<CubeMesh*> m_classicArms;
		QList<CubeMesh*> m_classicArmsOverlay;
		QList<CubeMesh*> m_slimArms;
		QList<CubeMesh*> m_slimArmsOverlay;
		CubeMesh* m_cape = nullptr;
		QList<CubeMesh*> m_elytra;

		QOpenGLTexture* m_skinTexture = nullptr;
		QOpenGLTexture* m_capeTexture = nullptr;

		bool m_slim = false;
		bool m_capeVisible = false;
		bool m_elytraVisible = false;
	};
} // namespace skinrender
