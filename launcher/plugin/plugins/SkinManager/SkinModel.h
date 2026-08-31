/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <array>

class QOpenGLShaderProgram;

namespace SkinManagerNS
{

	enum class BodyPart : int {
		Head = 0,
		Body,
		LeftArm,
		RightArm,
		LeftLeg,
		RightLeg,

		HeadOverlay,
		BodyOverlay,
		LeftArmOverlay,
		RightArmOverlay,
		LeftLegOverlay,
		RightLegOverlay,

		Cape,

		Count
	};

	enum class ModelVariant { Classic, Slim };

	struct Vertex {
		float x, y, z;
		float u, v;
	};

	struct DrawRange {
		int first;
		int count;
	};

	class Mesh
	{
	  public:
		Mesh() = default;
		~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		void build(ModelVariant variant);

		void rebuild(ModelVariant variant);

		void bind();
		void release();

		const DrawRange& range(BodyPart part) const
		{
			return m_ranges[int(part)];
		}

		ModelVariant variant() const
		{
			return m_variant;
		}

	  private:
		void appendBox(QVector<Vertex>& verts, BodyPart tag, float ox, float oy,
					   float oz, float sx, float sy, float sz, int u0, int v0,
					   float scale = 1.0f);
		void appendCape(QVector<Vertex>& verts);

		QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
		QOpenGLVertexArrayObject m_vao;
		std::array<DrawRange, int(BodyPart::Count)> m_ranges{};
		ModelVariant m_variant = ModelVariant::Classic;
		bool m_built = false;
	};

} // namespace SkinManagerNS
