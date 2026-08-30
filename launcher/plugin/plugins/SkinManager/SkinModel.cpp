/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "SkinModel.h"

#include <QOpenGLFunctions>
#include <QtMath>
#include <cmath>

using namespace SkinManagerNS;

namespace
{

	constexpr float TEX = 1.0f / 64.0f;

	void face(QVector<Vertex>& out, float ax, float ay, float az, float bx,
			  float by, float bz, float cx, float cy, float cz, float dx,
			  float dy, float dz, float u0, float v0, float u1, float v1)
	{
		const float V0 = 1.0f - v0;
		const float V1 = 1.0f - v1;
		out.push_back({ax, ay, az, u0, V1});
		out.push_back({bx, by, bz, u1, V1});
		out.push_back({cx, cy, cz, u1, V0});

		out.push_back({ax, ay, az, u0, V1});
		out.push_back({cx, cy, cz, u1, V0});
		out.push_back({dx, dy, dz, u0, V0});
	}

	DrawRange addCuboid(QVector<Vertex>& out, float ox, float oy, float oz,
						float gx, float gy, float gz, float tx, float ty,
						float tz, int u, int v)
	{
		const int begin = static_cast<int>(out.size());

		const float hx = gx * 0.5f;
		const float hy = gy * 0.5f;
		const float hz = gz * 0.5f;

		const float x0 = ox - hx, x1 = ox + hx;
		const float y0 = oy - hy, y1 = oy + hy;
		const float z0 = oz - hz, z1 = oz + hz;

		const float U = float(u);
		const float V = float(v);
		const float SX = tx;
		const float SY = ty;
		const float SZ = tz;

		face(out, x0, y1, z0, x1, y1, z0, x1, y1, z1, x0, y1, z1,
			 (U + SZ) * TEX, V * TEX, (U + SZ + SX) * TEX, (V + SZ) * TEX);

		face(out, x0, y0, z1, x1, y0, z1, x1, y0, z0, x0, y0, z0,
			 (U + SZ + SX) * TEX, V * TEX, (U + SZ + 2.0f * SX) * TEX,
			 (V + SZ) * TEX);

		face(out, x1, y0, z1, x1, y0, z0, x1, y1, z0, x1, y1, z1, U * TEX,
			 (V + SZ) * TEX, (U + SZ) * TEX, (V + SZ + SY) * TEX);

		face(out, x0, y0, z1, x1, y0, z1, x1, y1, z1, x0, y1, z1,
			 (U + SZ) * TEX, (V + SZ) * TEX, (U + SZ + SX) * TEX,
			 (V + SZ + SY) * TEX);

		face(out, x0, y0, z0, x0, y0, z1, x0, y1, z1, x0, y1, z0,
			 (U + SZ + SX) * TEX, (V + SZ) * TEX, (U + 2.0f * SZ + SX) * TEX,
			 (V + SZ + SY) * TEX);

		face(out, x1, y0, z0, x0, y0, z0, x0, y1, z0, x1, y1, z0,
			 (U + 2.0f * SZ + SX) * TEX, (V + SZ) * TEX,
			 (U + 2.0f * SZ + 2.0f * SX) * TEX, (V + SZ + SY) * TEX);

		return DrawRange{begin, static_cast<int>(out.size()) - begin};
	}

} // namespace

Mesh::~Mesh()
{
	if (m_vao.isCreated())
		m_vao.destroy();
	if (m_vbo.isCreated())
		m_vbo.destroy();
}

void Mesh::appendBox(QVector<Vertex>&, BodyPart, float, float, float, float,
					 float, float, int, int, float)
{
}

void Mesh::appendCape(QVector<Vertex>&) {}

void Mesh::build(ModelVariant variant)
{
	m_variant = variant;

	QVector<Vertex> verts;
	verts.reserve(36 * int(BodyPart::Count));

	const bool slim = (variant == ModelVariant::Slim);
	const float armW = slim ? 3.0f : 4.0f;

	struct PartSpec {
		BodyPart base, over;
		int u_base, v_base, u_over, v_over;
		float ox, oy, oz;
		float gx_base, gy_base, gz_base;
		float gx_over, gy_over, gz_over;
		float tx, ty, tz;
	};

	const PartSpec spec[] = {
		{BodyPart::Head, BodyPart::HeadOverlay, 0, 0, 32, 0, 0.0f, 4.0f, 0.0f,
		 8.0f, 8.0f, 8.0f, 9.0f, 9.0f, 9.0f, 8.0f, 8.0f, 8.0f},

		{BodyPart::Body, BodyPart::BodyOverlay, 16, 16, 16, 32, 0.0f, -6.0f,
		 0.0f, 8.0f, 12.0f, 4.0f, 8.5f, 12.5f, 4.5f, 8.0f, 12.0f, 4.0f},

		{BodyPart::RightArm, BodyPart::RightArmOverlay, 40, 16, 40, 32,
		 slim ? -5.5f : -6.0f, -6.0f, 0.0f, armW, 12.0f, 4.0f,
		 slim ? 3.5f : 4.5f, 12.5f, 4.5f, armW, 12.0f, 4.0f},

		{BodyPart::LeftArm, BodyPart::LeftArmOverlay, 32, 48, 48, 48,
		 slim ? 5.5f : 6.0f, -6.0f, 0.0f, armW, 12.0f, 4.0f, slim ? 3.5f : 4.5f,
		 12.5f, 4.5f, armW, 12.0f, 4.0f},

		{BodyPart::RightLeg, BodyPart::RightLegOverlay, 0, 16, 0, 32, -1.9f,
		 -18.0f, -0.1f, 4.0f, 12.0f, 4.0f, 4.5f, 12.5f, 4.5f, 4.0f, 12.0f,
		 4.0f},

		{BodyPart::LeftLeg, BodyPart::LeftLegOverlay, 16, 48, 0, 48, 1.9f,
		 -18.0f, -0.1f, 4.0f, 12.0f, 4.0f, 4.5f, 12.5f, 4.5f, 4.0f, 12.0f,
		 4.0f},
	};

	for (const auto& p : spec) {
		m_ranges[int(p.base)] =
			addCuboid(verts, p.ox, p.oy, p.oz, p.gx_base, p.gy_base, p.gz_base,
					  p.tx, p.ty, p.tz, p.u_base, p.v_base);
		m_ranges[int(p.over)] =
			addCuboid(verts, p.ox, p.oy, p.oz, p.gx_over, p.gy_over, p.gz_over,
					  p.tx, p.ty, p.tz, p.u_over, p.v_over);
	}

	{
		const float CW = 10.0f, CH = 16.0f, CD = 1.0f;
		const float cape_ox = 0.0f;
		const float cape_oy = -8.0f;
		const float cape_oz = 2.5f;

		const int capeBegin = static_cast<int>(verts.size());

		const float hx = CW * 0.5f, hy = CH * 0.5f, hz = CD * 0.5f;

		auto px = [](float p) { return p * TEX; };

		const float angleX = qDegreesToRadians(10.8f);
		const float cx = std::cos(angleX), sx = std::sin(angleX);
		auto xform = [&](float lx, float ly, float lz, float& wx, float& wy,
						 float& wz) {
			const float yp = ly * cx - lz * sx;
			const float zp = ly * sx + lz * cx;
			wx = -lx + cape_ox;
			wy = yp + cape_oy;
			wz = -zp + cape_oz;
		};

		auto cubeFace = [&](float ax, float ay, float az, float bx, float by,
							float bz, float cx_, float cy_, float cz_, float dx,
							float dy, float dz, float u0, float v0, float u1,
							float v1) {
			float Ax, Ay, Az, Bx, By, Bz, Cx, Cy, Cz, Dx, Dy, Dz;
			xform(ax, ay, az, Ax, Ay, Az);
			xform(bx, by, bz, Bx, By, Bz);
			xform(cx_, cy_, cz_, Cx, Cy, Cz);
			xform(dx, dy, dz, Dx, Dy, Dz);
			face(verts, Ax, Ay, Az, Bx, By, Bz, Cx, Cy, Cz, Dx, Dy, Dz, u0, v0,
				 u1, v1);
		};

		cubeFace(-hx, hy, -hz, hx, hy, -hz, hx, hy, hz, -hx, hy, hz, px(1),
				 px(0), px(1 + CW), px(0 + CD));
		cubeFace(-hx, -hy, hz, hx, -hy, hz, hx, -hy, -hz, -hx, -hy, -hz, px(11),
				 px(0), px(11 + CW), px(0 + CD));
		cubeFace(hx, -hy, hz, hx, -hy, -hz, hx, hy, -hz, hx, hy, hz, px(0),
				 px(1), px(0 + CD), px(1 + CH));
		cubeFace(-hx, -hy, hz, hx, -hy, hz, hx, hy, hz, -hx, hy, hz, px(1),
				 px(1), px(1 + CW), px(1 + CH));
		cubeFace(-hx, -hy, -hz, -hx, -hy, hz, -hx, hy, hz, -hx, hy, -hz, px(11),
				 px(1), px(11 + CD), px(1 + CH));
		cubeFace(hx, -hy, -hz, -hx, -hy, -hz, -hx, hy, -hz, hx, hy, -hz, px(12),
				 px(1), px(12 + CW), px(1 + CH));

		m_ranges[int(BodyPart::Cape)] =
			DrawRange{capeBegin, static_cast<int>(verts.size()) - capeBegin};
	}

	if (!m_vao.isCreated())
		m_vao.create();
	if (!m_vbo.isCreated())
		m_vbo.create();

	m_vao.bind();
	m_vbo.bind();
	m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
	m_vbo.allocate(verts.constData(), int(verts.size() * sizeof(Vertex)));

	auto* f = QOpenGLContext::currentContext()->functions();
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
							 reinterpret_cast<void*>(0));
	f->glEnableVertexAttribArray(1);
	f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
							 reinterpret_cast<void*>(sizeof(float) * 3));

	m_vbo.release();
	m_vao.release();
	m_built = true;
}

void Mesh::rebuild(ModelVariant variant)
{
	if (m_built && variant == m_variant)
		return;
	build(variant);
}

void Mesh::bind()
{
	if (m_built)
		m_vao.bind();
}

void Mesh::release()
{
	if (m_built)
		m_vao.release();
}
