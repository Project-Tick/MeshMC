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

#include "PlayerScene.h"

#include <QOpenGLTexture>
#include <QPoint>
#include <QSize>
#include <QVector3D>

namespace skinrender
{
	namespace
	{
		/* Model space is in Minecraft pixels, with the origin between the
		 * head and the torso: the head sits at y = +4 and the legs end at
		 * y = -24. The camera looks at (0, -8, 0), which is chest height.
		 *
		 * The second layer boxes are drawn half a pixel larger all round so
		 * they sit just outside the base layer instead of fighting it for
		 * depth. The head is the exception at a full pixel, which is what
		 * makes hats read as hats.
		 *
		 * Arms and legs are nudged by fractions of a pixel (the 1.9 and the
		 * -0.1) rather than placed flush: at 4 pixels wide, two boxes sharing
		 * an exact edge produce a visible seam of z-fighting as the model
		 * turns. */

		const QSize kSkinTextureSize(64, 64);
		/* Capes and elytra share the 64x32 cape texture. */
		const QSize kCapeTextureSize(64, 32);

		QList<CubeMesh*> buildBody()
		{
			return {
				/* head */
				new CubeMesh(QVector3D(8, 8, 8), QVector3D(0, 4, 0),
							 QPoint(0, 0), QVector3D(8, 8, 8),
							 kSkinTextureSize),
				/* torso */
				new CubeMesh(QVector3D(8, 12, 4), QVector3D(0, -6, 0),
							 QPoint(16, 16), QVector3D(8, 12, 4),
							 kSkinTextureSize),
				/* right leg */
				new CubeMesh(QVector3D(4, 12, 4),
							 QVector3D(-1.9f, -18, -0.1f), QPoint(0, 16),
							 QVector3D(4, 12, 4), kSkinTextureSize),
				/* left leg */
				new CubeMesh(QVector3D(4, 12, 4), QVector3D(1.9f, -18, -0.1f),
							 QPoint(16, 48), QVector3D(4, 12, 4),
							 kSkinTextureSize),
			};
		}

		QList<CubeMesh*> buildBodyOverlay()
		{
			return {
				/* head */
				new CubeMesh(QVector3D(9, 9, 9), QVector3D(0, 4, 0),
							 QPoint(32, 0), QVector3D(8, 8, 8),
							 kSkinTextureSize),
				/* torso */
				new CubeMesh(QVector3D(8.5f, 12.5f, 4.5f),
							 QVector3D(0, -6, 0), QPoint(16, 32),
							 QVector3D(8, 12, 4), kSkinTextureSize),
				/* right leg */
				new CubeMesh(QVector3D(4.5f, 12.5f, 4.5f),
							 QVector3D(-1.9f, -18, -0.1f), QPoint(0, 32),
							 QVector3D(4, 12, 4), kSkinTextureSize),
				/* left leg */
				new CubeMesh(QVector3D(4.5f, 12.5f, 4.5f),
							 QVector3D(1.9f, -18, -0.1f), QPoint(0, 48),
							 QVector3D(4, 12, 4), kSkinTextureSize),
			};
		}

		/* The four arm sets are written out separately rather than generated
		 * from a width parameter. A slim arm is not "the classic arm minus a
		 * pixel": its size, its position and its texture patch all differ,
		 * and the overlay differs again. Parameterising it hid those numbers
		 * behind arithmetic that could only be checked by redoing the
		 * arithmetic. */

		QList<CubeMesh*> buildClassicArms()
		{
			return {
				/* right arm */
				new CubeMesh(QVector3D(4, 12, 4), QVector3D(-6, -6, 0),
							 QPoint(40, 16), QVector3D(4, 12, 4),
							 kSkinTextureSize),
				/* left arm */
				new CubeMesh(QVector3D(4, 12, 4), QVector3D(6, -6, 0),
							 QPoint(32, 48), QVector3D(4, 12, 4),
							 kSkinTextureSize),
			};
		}

		QList<CubeMesh*> buildClassicArmsOverlay()
		{
			return {
				/* right arm */
				new CubeMesh(QVector3D(4.5f, 12.5f, 4.5f),
							 QVector3D(-6, -6, 0), QPoint(40, 32),
							 QVector3D(4, 12, 4), kSkinTextureSize),
				/* left arm */
				new CubeMesh(QVector3D(4.5f, 12.5f, 4.5f),
							 QVector3D(6, -6, 0), QPoint(48, 48),
							 QVector3D(4, 12, 4), kSkinTextureSize),
			};
		}

		QList<CubeMesh*> buildSlimArms()
		{
			return {
				/* right arm */
				new CubeMesh(QVector3D(3, 12, 4), QVector3D(-5.5f, -6, 0),
							 QPoint(40, 16), QVector3D(3, 12, 4),
							 kSkinTextureSize),
				/* left arm */
				new CubeMesh(QVector3D(3, 12, 4), QVector3D(5.5f, -6, 0),
							 QPoint(32, 48), QVector3D(3, 12, 4),
							 kSkinTextureSize),
			};
		}

		QList<CubeMesh*> buildSlimArmsOverlay()
		{
			return {
				/* right arm */
				new CubeMesh(QVector3D(3.5f, 12.5f, 4.5f),
							 QVector3D(-5.5f, -6, 0), QPoint(40, 32),
							 QVector3D(3, 12, 4), kSkinTextureSize),
				/* left arm */
				new CubeMesh(QVector3D(3.5f, 12.5f, 4.5f),
							 QVector3D(5.5f, -6, 0), QPoint(48, 48),
							 QVector3D(3, 12, 4), kSkinTextureSize),
			};
		}

		CubeMesh* buildCape()
		{
			auto* cape = new CubeMesh(QVector3D(10, 16, 1),
									  QVector3D(0, -8, 2.5f), QPoint(0, 0),
									  QVector3D(10, 16, 1), kCapeTextureSize);
			/* Tilted away from the back, then turned to face backwards --
			 * the box is built on the +Z side so that the outer face of the
			 * cape is the one the texture's front is mapped to. */
			cape->rotate(10.8f, QVector3D(1, 0, 0));
			cape->rotate(180, QVector3D(0, 1, 0));
			return cape;
		}

		/* One elytra wing, swept back and out from the shoulders. */
		CubeMesh* buildWing(bool mirrored)
		{
			auto* wing = new CubeMesh(QVector3D(12, 22, 4),
									  QVector3D(0, -13, -2), QPoint(22, 0),
									  QVector3D(10, 20, 2), kCapeTextureSize);
			if (mirrored) {
				/* The texture only holds one wing; the other is this one
				 * flipped. Applied before the rotations so that the sweep
				 * comes out symmetric rather than mirrored along with it. */
				wing->scale(QVector3D(-1, 1, 1));
			}
			wing->rotate(15, QVector3D(1, 0, 0));
			wing->rotate(15, QVector3D(0, 0, 1));
			wing->rotate(1, QVector3D(1, 0, 0));
			return wing;
		}

		/* Create or refresh a texture from an image.
		 *
		 * A null image leaves *no* texture behind rather than an invalid one:
		 * binding a texture that was never created warns on every frame, and
		 * "there is no cape" is a perfectly normal state to be in. */
		void assignTexture(QOpenGLTexture*& texture, const QImage& image)
		{
			if (image.isNull()) {
				if (texture) {
					if (texture->isCreated()) {
						texture->destroy();
					}
					delete texture;
					texture = nullptr;
				}
				return;
			}

			/* Uploaded flipped, to pair with the flipped V axis in
			 * CubeMesh::uploadBox(). */
			const QImage flipped = image.mirrored();

			if (!texture) {
				texture = new QOpenGLTexture(flipped);
			} else {
				if (texture->isBound()) {
					texture->release();
				}
				texture->destroy();
				texture->create();
				texture->setSize(flipped.width(), flipped.height());
				texture->setData(flipped);
			}

			/* Nearest, always: a skin is 64x64 and every pixel is meant to
			 * be a visible square. Linear filtering turns it to mush. */
			texture->setMinificationFilter(QOpenGLTexture::Nearest);
			texture->setMagnificationFilter(QOpenGLTexture::Nearest);
		}

		void destroyTexture(QOpenGLTexture*& texture)
		{
			if (!texture) {
				return;
			}
			if (texture->isCreated()) {
				texture->destroy();
			}
			delete texture;
			texture = nullptr;
		}

		void drawAll(const QList<CubeMesh*>& meshes,
					 QOpenGLShaderProgram* program)
		{
			for (CubeMesh* mesh : meshes) {
				mesh->draw(program);
			}
		}
	} // namespace

	PlayerScene::PlayerScene(const QImage& skin, bool slim, const QImage& cape)
		: QOpenGLFunctions(), m_slim(slim), m_capeVisible(!cape.isNull())
	{
		initializeOpenGLFunctions();

		m_body = buildBody();
		m_bodyOverlay = buildBodyOverlay();

		/* Both arm widths are built up front and the unused pair is simply
		 * not drawn: the user switches between Classic and Slim by clicking
		 * a radio button, and rebuilding vertex buffers for that would be
		 * needless work on the UI thread. */
		m_classicArms = buildClassicArms();
		m_classicArmsOverlay = buildClassicArmsOverlay();
		m_slimArms = buildSlimArms();
		m_slimArmsOverlay = buildSlimArmsOverlay();

		m_cape = buildCape();
		m_elytra = {buildWing(false), buildWing(true)};

		assignTexture(m_skinTexture, skin);
		assignTexture(m_capeTexture, cape);
	}

	PlayerScene::~PlayerScene()
	{
		for (const QList<CubeMesh*>& group :
			 {m_body, m_bodyOverlay, m_classicArms, m_classicArmsOverlay,
			  m_slimArms, m_slimArmsOverlay, m_elytra}) {
			for (CubeMesh* mesh : group) {
				delete mesh;
			}
		}
		delete m_cape;

		destroyTexture(m_skinTexture);
		destroyTexture(m_capeTexture);
	}

	void PlayerScene::draw(QOpenGLShaderProgram* program)
	{
		if (!program) {
			return;
		}

		if (m_skinTexture) {
			m_skinTexture->bind();
			program->setUniformValue("texture", 0);

			/* Base layer first, then the second layer over it: the fragment
			 * shader discards fully transparent pixels rather than blending
			 * them, so the order is what decides which one wins where they
			 * overlap. */
			drawAll(m_body, program);
			drawAll(m_slim ? m_slimArms : m_classicArms, program);
			drawAll(m_bodyOverlay, program);
			drawAll(m_slim ? m_slimArmsOverlay : m_classicArmsOverlay,
					program);

			m_skinTexture->release();
		}

		if (m_capeVisible && m_capeTexture) {
			m_capeTexture->bind();
			program->setUniformValue("texture", 0);
			if (m_elytraVisible) {
				drawAll(m_elytra, program);
			} else {
				m_cape->draw(program);
			}
			m_capeTexture->release();
		}
	}

	void PlayerScene::setSkin(const QImage& skin)
	{
		assignTexture(m_skinTexture, skin);
	}

	void PlayerScene::setCape(const QImage& cape)
	{
		assignTexture(m_capeTexture, cape);
	}

	void PlayerScene::setSlim(bool slim)
	{
		m_slim = slim;
	}

	void PlayerScene::setCapeVisible(bool visible)
	{
		m_capeVisible = visible;
	}

	void PlayerScene::setElytraVisible(bool visible)
	{
		m_elytraVisible = visible;
	}
} // namespace skinrender
