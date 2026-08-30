/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 *  SkinViewerWidget — a self-contained QOpenGLWidget that renders the
 *  Minecraft biped + cape using MeshMC's own bundled shaders
 *  (:/shaders/vshader_skin_model.glsl + :/shaders/fshader.glsl).
 *
 *  Public surface:
 *
 *      setSkin(const QImage& skinPng, ModelVariant variant);
 *      setCape(const QImage& capePng);   // pass a null QImage to clear
 *      clearSkin();
 *      setShowOverlay(bool);             // hat / jacket / sleeves layer
 *      setAutoRotate(bool);              // slow yaw animation
 *
 *  Mouse interaction:
 *
 *      Left-drag       → yaw + pitch
 *      Right-click     → reset camera
 *      Wheel           → zoom
 *
 *  The widget does its own QImage → GL_TEXTURE_2D upload; the caller
 *  hands it a regular QImage and the widget converts it to RGBA8888
 *  on its own. */

#pragma once

#include "SkinModel.h"

#include <QImage>
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QPoint>
#include <QTimer>

namespace SkinManagerNS
{

	class SkinViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions
	{
		Q_OBJECT

	  public:
		explicit SkinViewerWidget(QWidget* parent = nullptr);
		~SkinViewerWidget() override;

		void setSkin(const QImage& image, ModelVariant variant);
		void setCape(const QImage& image);
		void clearSkin();

		bool showOverlay() const
		{
			return m_showOverlay;
		}
		void setShowOverlay(bool on);

		bool autoRotate() const
		{
			return m_autoRotate;
		}
		void setAutoRotate(bool on);

		ModelVariant variant() const
		{
			return m_variant;
		}

	  signals:
		/* Emitted when the user drops a PNG file onto the widget. The
		 * surrounding page binds this to "open this skin file". */
		void skinFileDropped(const QString& filePath);

	  protected:
		void initializeGL() override;
		void resizeGL(int w, int h) override;
		void paintGL() override;

		void mousePressEvent(QMouseEvent* e) override;
		void mouseMoveEvent(QMouseEvent* e) override;
		void mouseReleaseEvent(QMouseEvent* e) override;
		void wheelEvent(QWheelEvent* e) override;
		void dragEnterEvent(QDragEnterEvent* e) override;
		void dropEvent(QDropEvent* e) override;

	  private:
		void uploadSkin(const QImage& image);
		void uploadCape(const QImage& image);
		void resetCamera();

		Mesh m_mesh;
		QOpenGLShaderProgram m_program;
		std::unique_ptr<QOpenGLTexture> m_skinTex;
		std::unique_ptr<QOpenGLTexture> m_capeTex;

		QImage m_pendingSkin;
		QImage m_pendingCape;
		bool m_pendingSkinUpload = false;
		bool m_pendingCapeUpload = false;

		ModelVariant m_variant = ModelVariant::Classic;
		bool m_showOverlay = true;
		bool m_autoRotate = false;
		bool m_hasCape = false;

		float m_yaw = 25.0f;   /* degrees */
		float m_pitch = -5.0f; /* degrees */
		float m_zoom = 60.0f;  /* camera distance, model units */
		QPoint m_dragLast;
		bool m_dragging = false;

		QTimer m_animTimer;
	};

} // namespace SkinManagerNS
