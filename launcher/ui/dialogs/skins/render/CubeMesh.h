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

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QPoint>
#include <QSize>
#include <QVector3D>

namespace skinrender
{
	/* One textured cuboid of the player model, ready to draw.
	 *
	 * Size and position are baked into the vertex buffer at construction:
	 * every box in the model is static relative to the body, so there is
	 * nothing to recompute per frame. Only rotate()/scale(), which the cape
	 * and the elytra wings need, go through the model matrix uniform.
	 *
	 * The texture coordinates come from the Minecraft box unwrap: a cross of
	 * six faces laid out at `uv` in a texture of `textureSize`, for a box of
	 * `textureDim` (width, height, depth) *texture* pixels. That is separate
	 * from the geometric `size` because the overlay boxes are drawn slightly
	 * larger than the base ones while sampling the same-sized patch.
	 *
	 * Requires a current OpenGL context for its whole lifetime, construction
	 * and destruction included.
	 */
	class CubeMesh : protected QOpenGLFunctions
	{
	  public:
		/* Bare mesh with empty buffers; only useful via plane(). */
		CubeMesh();

		CubeMesh(QVector3D size, QVector3D position, QPoint uv,
				 QVector3D textureDim, QSize textureSize = QSize(64, 64));
		virtual ~CubeMesh();

		/* A full-viewport quad in clip space, for the background.
		 *
		 * Its vertices are already where they need to be on screen, which is
		 * why the background vertex shader passes a_position through
		 * untouched instead of applying a matrix. */
		static CubeMesh* plane();

		void draw(QOpenGLShaderProgram* program);

		void rotate(float degrees, const QVector3D& axis);
		void scale(const QVector3D& factor);

	  private:
		CubeMesh(const CubeMesh&) = delete;
		CubeMesh& operator=(const CubeMesh&) = delete;

		void uploadBox(QVector3D size, QVector3D position, QPoint uv,
					   QVector3D textureDim, QSize textureSize);

		QOpenGLBuffer m_vertexBuffer;
		QOpenGLBuffer m_indexBuffer;
		QMatrix4x4 m_modelMatrix;
		GLsizei m_indexCount = 0;
	};
} // namespace skinrender
