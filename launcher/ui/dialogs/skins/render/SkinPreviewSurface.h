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

#include <QColor>
#include <QImage>
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLWindow>
#include <QPoint>
#include <QString>

class QOpenGLShaderProgram;
class QOpenGLTexture;
class SkinEntry;

namespace skinrender
{
	class CubeMesh;
	class PlayerScene;

	/* What the preview needs from its owner in order to draw the first
	 * frame.
	 *
	 * Pulled rather than pushed because initializeGL() runs whenever the
	 * windowing system gets around to it, which is after the dialog has
	 * finished constructing -- so the preview has to be able to ask for the
	 * current state at that moment instead of being told at creation time.
	 */
	class SkinPreviewSource
	{
	  public:
		virtual ~SkinPreviewSource() = default;

		/* The skin to show, or nullptr if nothing is selected. */
		virtual const SkinEntry* previewSkin() const = 0;

		/* The cape texture for an id, or a null image for "no cape". */
		virtual QImage previewCape(const QString& capeId) const = 0;
	};

	/* Orbiting 3D preview of the player model.
	 *
	 * A QOpenGLWindow rather than a QOpenGLWidget: the widget path renders
	 * into an FBO that the rest of the dialog then composites, which on
	 * several drivers costs more than the model itself. The dialog wraps this
	 * in QWidget::createWindowContainer().
	 *
	 * Drag to turn, wheel to zoom. Nothing here owns the skin or the cape --
	 * both are handed in, and re-handed in whenever the dialog changes them.
	 */
	class SkinPreviewSurface : public QOpenGLWindow, protected QOpenGLFunctions
	{
		Q_OBJECT

	  public:
		/* `background` is the colour the chequerboard is derived from; the
		 * dialog passes its own base palette colour so the preview sits in
		 * the theme rather than on top of it. */
		SkinPreviewSurface(SkinPreviewSource* source, QColor background);
		~SkinPreviewSurface() override;

		/* Whether a preview can be created at all. False when there is no
		 * usable GL context, or when the user asked for it to be skipped
		 * through <ENVNAME>_DISABLE_GLVULKAN. */
		static bool isAvailable();

		void showSkin(const SkinEntry* skin);
		void showCape(const QImage& cape);
		void setElytraVisible(bool visible);

	  protected:
		void mousePressEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void wheelEvent(QWheelEvent* event) override;

		void initializeGL() override;
		void resizeGL(int w, int h) override;
		void paintGL() override;

	  private:
		void loadShaders();
		void buildBackgroundTexture(int width, int height, int tileSize);
		void drawBackground();

		QOpenGLShaderProgram* m_modelProgram = nullptr;
		QOpenGLShaderProgram* m_backgroundProgram = nullptr;
		PlayerScene* m_scene = nullptr;
		CubeMesh* m_backgroundQuad = nullptr;
		QOpenGLTexture* m_backgroundTexture = nullptr;

		QMatrix4x4 m_projection;

		QPoint m_lastMousePosition;
		bool m_dragging = false;

		/* Orbit camera. Starting yaw of 90 degrees puts the camera on +Z,
		 * which is the model's front. */
		float m_distance = 48.0f;
		float m_yaw = 90.0f;
		float m_pitch = 0.0f;

		bool m_firstFrame = true;

		QColor m_background;
		SkinPreviewSource* m_source = nullptr;
	};
} // namespace skinrender
