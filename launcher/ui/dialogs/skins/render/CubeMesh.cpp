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

#include "CubeMesh.h"

#include <QVector2D>
#include <QVector4D>
#include <QVector>

namespace skinrender
{
	namespace
	{
		struct Vertex {
			QVector4D position;
			QVector2D texCoord;
		};

		/* Unit cube, four corners per face, in face order
		 * front(+Z), right(+X), back(-Z), left(-X), bottom(-Y), top(+Y). */
		const QVector4D kCubeCorners[24] = {
			/* face 0 */
			QVector4D(-0.5f, -0.5f, 0.5f, 1.0f),
			QVector4D(0.5f, -0.5f, 0.5f, 1.0f),
			QVector4D(-0.5f, 0.5f, 0.5f, 1.0f),
			QVector4D(0.5f, 0.5f, 0.5f, 1.0f),
			/* face 1 */
			QVector4D(0.5f, -0.5f, 0.5f, 1.0f),
			QVector4D(0.5f, -0.5f, -0.5f, 1.0f),
			QVector4D(0.5f, 0.5f, 0.5f, 1.0f),
			QVector4D(0.5f, 0.5f, -0.5f, 1.0f),
			/* face 2 */
			QVector4D(0.5f, -0.5f, -0.5f, 1.0f),
			QVector4D(-0.5f, -0.5f, -0.5f, 1.0f),
			QVector4D(0.5f, 0.5f, -0.5f, 1.0f),
			QVector4D(-0.5f, 0.5f, -0.5f, 1.0f),
			/* face 3 */
			QVector4D(-0.5f, -0.5f, -0.5f, 1.0f),
			QVector4D(-0.5f, -0.5f, 0.5f, 1.0f),
			QVector4D(-0.5f, 0.5f, -0.5f, 1.0f),
			QVector4D(-0.5f, 0.5f, 0.5f, 1.0f),
			/* face 4 */
			QVector4D(-0.5f, -0.5f, -0.5f, 1.0f),
			QVector4D(0.5f, -0.5f, -0.5f, 1.0f),
			QVector4D(-0.5f, -0.5f, 0.5f, 1.0f),
			QVector4D(0.5f, -0.5f, 0.5f, 1.0f),
			/* face 5 */
			QVector4D(-0.5f, 0.5f, 0.5f, 1.0f),
			QVector4D(0.5f, 0.5f, 0.5f, 1.0f),
			QVector4D(-0.5f, 0.5f, -0.5f, 1.0f),
			QVector4D(0.5f, 0.5f, -0.5f, 1.0f),
		};

		/* One triangle strip across all six faces. The repeated indices are
		 * degenerate triangles that stitch each face's strip to the next. */
		const GLushort kCubeIndices[] = {
			0,  1,  2,  3,  3,      /* face 0 */
			4,  4,  5,  6,  7,  7,  /* face 1 */
			8,  8,  9,  10, 11, 11, /* face 2 */
			12, 12, 13, 14, 15, 15, /* face 3 */
			16, 16, 17, 18, 19, 19, /* face 4 */
			20, 20, 21, 22, 23,     /* face 5 */
		};

		/* The four texture coordinates of one face's rectangle, in the order
		 * (x1,y2), (x2,y2), (x2,y1), (x1,y1).
		 *
		 * The V axis is flipped because the textures themselves are uploaded
		 * flipped -- see assignTexture() in PlayerScene. */
		void faceTexCoords(float x1, float y1, float x2, float y2,
						   float textureWidth, float textureHeight,
						   QVector2D out[4])
		{
			out[0] = QVector2D(x1 / textureWidth, 1.0f - y2 / textureHeight);
			out[1] = QVector2D(x2 / textureWidth, 1.0f - y2 / textureHeight);
			out[2] = QVector2D(x2 / textureWidth, 1.0f - y1 / textureHeight);
			out[3] = QVector2D(x1 / textureWidth, 1.0f - y1 / textureHeight);
		}

		/* All 24 texture coordinates for a box unwrapped at (u, v) with width
		 * w, height h and depth d, in the same face order as kCubeCorners.
		 *
		 * Both the six rectangles and the per-face corner order are literal:
		 * each face picks its four coordinates out of faceTexCoords() in the
		 * arrangement that lines up with that face's vertices above. */
		void cubeTexCoords(float u, float v, float w, float h, float d,
						   float textureWidth, float textureHeight,
						   QVector2D out[24])
		{
			QVector2D top[4];
			QVector2D bottom[4];
			QVector2D left[4];
			QVector2D front[4];
			QVector2D right[4];
			QVector2D back[4];

			faceTexCoords(u + d, v, u + w + d, v + d, textureWidth,
						  textureHeight, top);
			faceTexCoords(u + w + d, v, u + w * 2 + d, v + d, textureWidth,
						  textureHeight, bottom);
			faceTexCoords(u, v + d, u + d, v + d + h, textureWidth,
						  textureHeight, left);
			faceTexCoords(u + d, v + d, u + w + d, v + d + h, textureWidth,
						  textureHeight, front);
			faceTexCoords(u + w + d, v + d, u + w + d * 2, v + h + d,
						  textureWidth, textureHeight, right);
			faceTexCoords(u + w + d * 2, v + d, u + w * 2 + d * 2, v + h + d,
						  textureWidth, textureHeight, back);

			/* face 0, front(+Z) */
			out[0] = front[0];
			out[1] = front[1];
			out[2] = front[3];
			out[3] = front[2];
			/* face 1, right(+X) */
			out[4] = right[0];
			out[5] = right[1];
			out[6] = right[3];
			out[7] = right[2];
			/* face 2, back(-Z) */
			out[8] = back[0];
			out[9] = back[1];
			out[10] = back[3];
			out[11] = back[2];
			/* face 3, left(-X) */
			out[12] = left[0];
			out[13] = left[1];
			out[14] = left[3];
			out[15] = left[2];
			/* face 4, bottom(-Y) */
			out[16] = bottom[3];
			out[17] = bottom[2];
			out[18] = bottom[0];
			out[19] = bottom[1];
			/* face 5, top(+Y) */
			out[20] = top[0];
			out[21] = top[1];
			out[22] = top[3];
			out[23] = top[2];
		}

		/* Full-viewport quad in clip space, for the background. */
		const Vertex kPlaneVertices[4] = {
			{QVector4D(-1.0f, -1.0f, -0.5f, 1.0f), QVector2D(0.0f, 0.0f)},
			{QVector4D(1.0f, -1.0f, -0.5f, 1.0f), QVector2D(1.0f, 0.0f)},
			{QVector4D(-1.0f, 1.0f, -0.5f, 1.0f), QVector2D(0.0f, 1.0f)},
			{QVector4D(1.0f, 1.0f, -0.5f, 1.0f), QVector2D(1.0f, 1.0f)},
		};
		const GLushort kPlaneIndices[] = {0, 1, 2, 3, 3};
	} // namespace

	CubeMesh::CubeMesh()
		: QOpenGLFunctions(), m_indexBuffer(QOpenGLBuffer::IndexBuffer)
	{
		initializeOpenGLFunctions();
		m_vertexBuffer.create();
		m_indexBuffer.create();
	}

	CubeMesh::CubeMesh(QVector3D size, QVector3D position, QPoint uv,
					   QVector3D textureDim, QSize textureSize)
		: CubeMesh()
	{
		uploadBox(size, position, uv, textureDim, textureSize);
	}

	CubeMesh::~CubeMesh()
	{
		m_vertexBuffer.destroy();
		m_indexBuffer.destroy();
	}

	void CubeMesh::uploadBox(QVector3D size, QVector3D position, QPoint uv,
							 QVector3D textureDim, QSize textureSize)
	{
		QVector2D texCoords[24];
		cubeTexCoords(static_cast<float>(uv.x()), static_cast<float>(uv.y()),
					  textureDim.x(), textureDim.y(), textureDim.z(),
					  static_cast<float>(textureSize.width()),
					  static_cast<float>(textureSize.height()), texCoords);

		/* The box transform is baked into the vertex buffer: it never changes
		 * after construction, which leaves the model matrix free for the cape
		 * and elytra rotations. */
		Vertex vertices[24];
		for (int i = 0; i < 24; ++i) {
			const QVector4D& corner = kCubeCorners[i];
			vertices[i].position =
				QVector4D(corner.x() * size.x() + position.x(),
						  corner.y() * size.y() + position.y(),
						  corner.z() * size.z() + position.z(), 1.0f);
			vertices[i].texCoord = texCoords[i];
		}

		m_vertexBuffer.bind();
		m_vertexBuffer.allocate(vertices, static_cast<int>(sizeof(vertices)));

		m_indexBuffer.bind();
		m_indexBuffer.allocate(kCubeIndices,
							   static_cast<int>(sizeof(kCubeIndices)));

		m_indexCount =
			static_cast<GLsizei>(sizeof(kCubeIndices) / sizeof(GLushort));
	}

	CubeMesh* CubeMesh::plane()
	{
		auto* mesh = new CubeMesh();

		mesh->m_vertexBuffer.bind();
		mesh->m_vertexBuffer.allocate(kPlaneVertices,
									  static_cast<int>(sizeof(kPlaneVertices)));
		mesh->m_indexBuffer.bind();
		mesh->m_indexBuffer.allocate(kPlaneIndices,
									 static_cast<int>(sizeof(kPlaneIndices)));
		mesh->m_indexCount =
			static_cast<GLsizei>(sizeof(kPlaneIndices) / sizeof(GLushort));

		return mesh;
	}

	void CubeMesh::draw(QOpenGLShaderProgram* program)
	{
		if (!program || m_indexCount == 0) {
			return;
		}

		program->setUniformValue("model_matrix", m_modelMatrix);

		m_vertexBuffer.bind();
		m_indexBuffer.bind();

		/* Offsets are spelled out rather than taken with offsetof(): Vertex
		 * holds QVector4D/QVector2D, which are not standard-layout types, and
		 * offsetof() on those is only conditionally supported -- it warns
		 * under -pedantic. The layout is fixed and trivial, so writing it out
		 * costs nothing. */
		const int positionOffset = 0;
		const int texCoordOffset = static_cast<int>(sizeof(QVector4D));

		const int positionLocation = program->attributeLocation("a_position");
		if (positionLocation != -1) {
			program->enableAttributeArray(positionLocation);
			program->setAttributeBuffer(positionLocation, GL_FLOAT,
										positionOffset, 4, sizeof(Vertex));
		}

		const int texCoordLocation = program->attributeLocation("a_texcoord");
		if (texCoordLocation != -1) {
			program->enableAttributeArray(texCoordLocation);
			program->setAttributeBuffer(texCoordLocation, GL_FLOAT,
										texCoordOffset, 2, sizeof(Vertex));
		}

		/* Triangle strip, matching kCubeIndices and kPlaneIndices. */
		glDrawElements(GL_TRIANGLE_STRIP, m_indexCount, GL_UNSIGNED_SHORT,
					   nullptr);
	}

	void CubeMesh::rotate(float degrees, const QVector3D& axis)
	{
		m_modelMatrix.rotate(degrees, axis);
	}

	void CubeMesh::scale(const QVector3D& factor)
	{
		m_modelMatrix.scale(factor);
	}
} // namespace skinrender
