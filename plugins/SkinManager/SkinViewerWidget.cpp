/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 *  Real-time 3D skin preview.  Uses MeshMC's bundled shaders so the
 *  plugin ships zero GLSL of its own.
 */

#include "SkinViewerWidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWheelEvent>
#include <QtMath>
#include <cmath>

using namespace SkinManagerNS;

namespace
{

	/*
	 * Depth-test setup for the skin viewer.
	 *
	 * IMPORTANT — about the vertex shader and Reverse Z:
	 *   vshader_skin_model.glsl contains a "Z-NDC re-inversion" step that
	 *   flips a Reverse-Z output back into the standard [0, W] range
	 *   (gl_Position.z = w_c - near_z). That post-step only produces a
	 *   correct depth value when paintGL() feeds it a Reverse-Z
	 *   perspective matrix — see the matrix construction in paintGL()
	 *   below. With that matrix in place, depth values land in the
	 *   standard [0, 1] range so we drive the pipeline with the
	 *   *standard* settings (GL_LESS + clearDepth(1)).
	 *
	 * Backface culling is intentionally NOT enabled — Prism Launcher's
	 * SkinOpenGLWindow::paintGL() doesn't enable it either, and for good
	 * reason. SkinModel emits each face with its own hand-written winding
	 * order; the TOP and BOTTOM quads happen to come out clockwise from
	 * outside the cuboid (their (b-a) × (c-a) normal points back into
	 * the box), so glCullFace(GL_BACK) silently drops them. That made
	 * the top of the head and the soles of the feet invisible during
	 * testing. Disabling culling outright is the cheapest fix and also
	 * removes the per-draw enable/disable dance the cape used to need.
	 *
	 * Alpha blending stays on so the fragment shader's alpha-discard
	 * (already in fshader.glsl) composites cleanly against the body.
	 */
	void configureDepth(QOpenGLFunctions* f)
	{
		f->glEnable(GL_DEPTH_TEST);
		f->glDepthFunc(GL_LESS);
		f->glClearDepthf(1.0f);
		f->glDisable(GL_CULL_FACE);
		f->glEnable(GL_BLEND);
		f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

} // namespace

SkinViewerWidget::SkinViewerWidget(QWidget* parent) : QOpenGLWidget(parent)
{
	setMinimumSize(220, 320);
	setAcceptDrops(true);
	setFocusPolicy(Qt::StrongFocus);

	/* ~60 fps repaint when auto-rotate is on, idle otherwise. */
	m_animTimer.setInterval(16);
	QObject::connect(&m_animTimer, &QTimer::timeout, this, [this]() {
		if (m_autoRotate) {
			m_yaw += 0.6f;
			if (m_yaw > 360.0f)
				m_yaw -= 360.0f;
			update();
		}
	});
}

SkinViewerWidget::~SkinViewerWidget()
{
	/* Tear-down requires a current context so the GL resources free
	 * properly. */
	makeCurrent();
	m_skinTex.reset();
	m_capeTex.reset();
	doneCurrent();
}

void SkinViewerWidget::setSkin(const QImage& image, ModelVariant variant)
{
	m_variant = variant;
	if (image.isNull()) {
		clearSkin();
		return;
	}

	/* Mirror vertically before upload — matches Prism Launcher's
	 * Scene::setSkin() in ui/dialogs/skins/draw/Scene.cpp. The mirror
	 * + the UV layout used by SkinModel together give a Mojang-aligned
	 * sample: the top row of the texture lines up with V=0 in shader
	 * space after Qt's normal QImage → GL Y-flip. */
	m_pendingSkin = image.convertToFormat(QImage::Format_RGBA8888).mirrored();
	m_pendingSkinUpload = true;

	if (isVisible())
		update();
}

void SkinViewerWidget::setCape(const QImage& image)
{
	if (image.isNull()) {
		m_hasCape = false;
		m_pendingCape = QImage();
		m_pendingCapeUpload = true;
		if (isVisible())
			update();
		return;
	}
	m_hasCape = true;
	/* Cape PNGs are 64×32. Pad them into a 64×64 image so the same
	 * single texture-bind path works for skin and cape, then mirror
	 * vertically — exactly the dance the skin pipeline does.
	 *
	 * Layout walk-through (the bit that took three tries to land):
	 *
	 *   1. Start: raw 64×32 cape, Mojang convention — cape row 0 is
	 *      the *top* of the cape graphic.
	 *
	 *   2. Pad into the TOP half of a 64×64 image, leaving the
	 *      bottom half transparent:
	 *
	 *          rows  0..31 : cape pixels (Mojang row k -> padded row k)
	 *          rows 32..63 : transparent
	 *
	 *   3. QImage::mirrored() flips vertically:
	 *
	 *          rows  0..31 : transparent
	 *          rows 32..63 : cape pixels (Mojang row k -> mirrored row 63-k)
	 *
	 *   4. GL_TEXTURE_2D samples V = row / 64. So Mojang's v=0 (top
	 *      of the cape) lives at mirrored row 63 -> V ≈ 1.0, and
	 *      Mojang's v=31 (bottom) lives at mirrored row 32 -> V = 0.5.
	 *
	 *   5. SkinModel's cape face() calls use Mojang pixel v ∈ [0,17]
	 *      and apply V_sample = 1 - v/64 -> V ∈ [47/64, 1.0] — exactly
	 *      the band where the mirrored cape data now lives.
	 *
	 * Earlier attempts:
	 *   - pad into BOTTOM half + mirror -> cape data lands in
	 *     rows 0..31 (V ∈ [0, 0.5]); UV band [47/64, 1] misses it -> blank.
	 *   - pad into BOTTOM half + no mirror -> cape data in rows 32..63
	 *     (V ∈ [0.5, 1]); first 5 rows of cape end up *above* the UV
	 *     band, and the bottom of the cape ends up clamped at V=1 -> the
	 *     cape did render but was vertically flipped and shifted by
	 *     ~3 pixels, which read as "missing".
	 */
	QImage padded(64, 64, QImage::Format_RGBA8888);
	padded.fill(Qt::transparent);
	{
		const QImage src =
			image.scaled(64, 32, Qt::KeepAspectRatio, Qt::FastTransformation)
				.convertToFormat(QImage::Format_RGBA8888);
		/* Pad into the TOP half (rows 0..31). The subsequent
		 * mirrored() call moves the cape data into the bottom half,
		 * exactly where the cape UV band [47/64, 1] expects it. */
		for (int y = 0; y < src.height(); ++y) {
			memcpy(padded.scanLine(y), src.constScanLine(y),
				   src.bytesPerLine());
		}
	}
	m_pendingCape = padded.mirrored();
	m_pendingCapeUpload = true;
	if (isVisible())
		update();
}

void SkinViewerWidget::clearSkin()
{
	m_pendingSkin = QImage();
	m_pendingSkinUpload = true;
	update();
}

void SkinViewerWidget::setShowOverlay(bool on)
{
	if (m_showOverlay == on)
		return;
	m_showOverlay = on;
	update();
}

void SkinViewerWidget::setAutoRotate(bool on)
{
	if (m_autoRotate == on)
		return;
	m_autoRotate = on;
	if (on)
		m_animTimer.start();
	else
		m_animTimer.stop();
	update();
}

/* ───────────────────────────── GL setup ──────────────────────────── */

void SkinViewerWidget::initializeGL()
{
	initializeOpenGLFunctions();

	/* Build the program from the shader sources bundled in
	 * skinmanager.qrc under the "/skinmanager/shaders/" prefix so
	 * the plugin stays freestanding (no dependency on the launcher's
	 * own shader resource bundle being mounted at runtime). */
	if (!m_program.addShaderFromSourceFile(
			QOpenGLShader::Vertex,
			":/skinmanager/shaders/vshader_skin_model.glsl")) {
		qWarning() << "[SkinViewer] vertex shader compile failed:"
				   << m_program.log();
	}
	if (!m_program.addShaderFromSourceFile(
			QOpenGLShader::Fragment, ":/skinmanager/shaders/fshader.glsl")) {
		qWarning() << "[SkinViewer] fragment shader compile failed:"
				   << m_program.log();
	}
	m_program.bindAttributeLocation("a_position", 0);
	m_program.bindAttributeLocation("a_texcoord", 1);
	if (!m_program.link()) {
		qWarning() << "[SkinViewer] shader program link failed:"
				   << m_program.log();
	}

	m_mesh.build(m_variant);

	configureDepth(this);
	glClearColor(0.18f, 0.20f, 0.24f, 1.0f);
}

void SkinViewerWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}

void SkinViewerWidget::paintGL()
{
	/* Drain any pending texture uploads inside a valid context. */
	if (m_pendingSkinUpload) {
		uploadSkin(m_pendingSkin);
		m_pendingSkin = QImage();
		m_pendingSkinUpload = false;
	}
	if (m_pendingCapeUpload) {
		uploadCape(m_pendingCape);
		m_pendingCape = QImage();
		m_pendingCapeUpload = false;
	}

	/* Re-bake mesh if the variant changed since last paint. */
	m_mesh.rebuild(m_variant);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!m_program.isLinked())
		return;

	m_program.bind();

	/* ── Camera ─────────────────────────────────────────────────── *
	 *
	 * The bundled vertex shader (vshader_skin_model.glsl) ends with
	 *
	 *     gl_Position.z = w_c - near_z;
	 *
	 * which un-flips a *Reverse-Z* depth value back into the standard
	 * [0, W] NDC range. That post-step is only correct when the input
	 * projection matrix is itself a Reverse-Z perspective. Feeding it
	 * a vanilla QMatrix4x4::perspective() inverts the depth ordering
	 * — far fragments win the depth test — and produces every artefact
	 * reported during testing: silhouettes z-fighting, the body
	 * bleeding through the head, overlays looking transparent.
	 *
	 * The matrix below is the exact Reverse-Z perspective used by
	 * Prism Launcher's SkinOpenGLWindow::resizeGL():
	 *
	 *     P(0,0) = cot(fov/2)/aspect
	 *     P(1,1) = cot(fov/2)
	 *     P(2,2) = 0
	 *     P(3,2) = -1
	 *     P(2,3) = zNear   (infinite far plane)
	 *     P(3,3) = 0
	 *
	 * Combined with the shader's z-flip and a standard
	 * glDepthFunc(GL_LESS) + glClearDepthf(1.0) pipeline this gives
	 * us a depth-stable render that matches Prism's reference look.
	 */
	QMatrix4x4 proj;
	{
		const float aspect = float(width()) / float(qMax(1, height()));
		const float fov = 45.0f;
		const float zNear = 15.0f;
		const float radians = qDegreesToRadians(fov * 0.5f);
		const float sine = std::sin(radians);
		if (sine != 0.0f) {
			const float cotan = std::cos(radians) / sine;
			proj.fill(0.0f);
			proj(0, 0) = cotan / aspect;
			proj(1, 1) = cotan;
			proj(2, 2) = 0.0f;
			proj(3, 2) = -1.0f;
			proj(2, 3) = zNear;
			proj(3, 3) = 0.0f;
		}
	}

	QMatrix4x4 view;
	view.translate(0.0f, 0.0f, -m_zoom);

	/* The biped has its origin at the neck (head sits at y=+4, feet at
	 * y=-24).  Shift the whole model up so the body centre lines up
	 * with the camera's centre of view — otherwise the head clips off
	 * the top of the viewport at standard zoom. */
	QMatrix4x4 model;
	model.rotate(m_pitch, 1.0f, 0.0f, 0.0f);
	model.rotate(m_yaw, 0.0f, 1.0f, 0.0f);
	model.translate(0.0f, 8.0f, 0.0f);

	const QMatrix4x4 mvp = proj * view;
	m_program.setUniformValue("mvp_matrix", mvp);
	m_program.setUniformValue("model_matrix", model);
	m_program.setUniformValue("texture", 0);

	/* ── Body: bind skin texture, draw base limbs ───────────────── */
	if (m_skinTex && m_skinTex->isCreated()) {
		m_skinTex->bind(0);
		m_mesh.bind();
		const BodyPart baseParts[] = {
			BodyPart::Head,		BodyPart::Body,	   BodyPart::LeftArm,
			BodyPart::RightArm, BodyPart::LeftLeg, BodyPart::RightLeg,
		};
		for (auto p : baseParts) {
			const auto r = m_mesh.range(p);
			if (r.count > 0)
				glDrawArrays(GL_TRIANGLES, r.first, r.count);
		}

		if (m_showOverlay) {
			const BodyPart overlayParts[] = {
				BodyPart::HeadOverlay,	  BodyPart::BodyOverlay,
				BodyPart::LeftArmOverlay, BodyPart::RightArmOverlay,
				BodyPart::LeftLegOverlay, BodyPart::RightLegOverlay,
			};
			for (auto p : overlayParts) {
				const auto r = m_mesh.range(p);
				if (r.count > 0)
					glDrawArrays(GL_TRIANGLES, r.first, r.count);
			}
		}
		m_mesh.release();
		m_skinTex->release(0);
	}

	/* ── Cape: separate texture, single draw call ───────────────── *
	 *
	 * Cull-face is globally disabled in configureDepth(), so the
	 * cape (which is pre-rotated 180° around Y at mesh-build time
	 * and would otherwise present its back faces to the camera)
	 * just draws on its own. */
	if (m_hasCape && m_capeTex && m_capeTex->isCreated()) {
		m_capeTex->bind(0);
		m_mesh.bind();
		const auto r = m_mesh.range(BodyPart::Cape);
		if (r.count > 0)
			glDrawArrays(GL_TRIANGLES, r.first, r.count);
		m_mesh.release();
		m_capeTex->release(0);
	}

	m_program.release();
}

void SkinViewerWidget::uploadSkin(const QImage& image)
{
	m_skinTex.reset();
	if (image.isNull())
		return;
	m_skinTex = std::make_unique<QOpenGLTexture>(image);
	m_skinTex->setMinificationFilter(QOpenGLTexture::Nearest);
	m_skinTex->setMagnificationFilter(QOpenGLTexture::Nearest);
	m_skinTex->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void SkinViewerWidget::uploadCape(const QImage& image)
{
	m_capeTex.reset();
	if (image.isNull())
		return;
	m_capeTex = std::make_unique<QOpenGLTexture>(image);
	m_capeTex->setMinificationFilter(QOpenGLTexture::Nearest);
	m_capeTex->setMagnificationFilter(QOpenGLTexture::Nearest);
	m_capeTex->setWrapMode(QOpenGLTexture::ClampToEdge);
}

/* ──────────────────────────── Camera ─────────────────────────────── */

void SkinViewerWidget::resetCamera()
{
	m_yaw = 25.0f;
	m_pitch = -5.0f;
	/* 60 units puts the whole biped (roughly 32 units tall) comfortably
	 * inside a 38° fov frustum with some margin around the silhouette. */
	m_zoom = 60.0f;
	update();
}

void SkinViewerWidget::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton) {
		m_dragging = true;
		m_dragLast = e->pos();
	} else if (e->button() == Qt::RightButton) {
		resetCamera();
	}
}

void SkinViewerWidget::mouseMoveEvent(QMouseEvent* e)
{
	if (!m_dragging)
		return;
	const QPoint delta = e->pos() - m_dragLast;
	m_dragLast = e->pos();
	m_yaw += delta.x() * 0.5f;
	m_pitch += delta.y() * 0.5f;
	/* clamp pitch to keep the camera the right way up */
	if (m_pitch > 89.0f)
		m_pitch = 89.0f;
	if (m_pitch < -89.0f)
		m_pitch = -89.0f;
	update();
}

void SkinViewerWidget::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
		m_dragging = false;
}

void SkinViewerWidget::wheelEvent(QWheelEvent* e)
{
	const float steps = e->angleDelta().y() / 120.0f;
	m_zoom -= steps * 2.0f;
	/* Keep the camera outside the Reverse-Z near plane (zNear = 15
	 * in paintGL) so the biped never clips into the near clip. */
	if (m_zoom < 16.0f)
		m_zoom = 16.0f;
	if (m_zoom > 120.0f)
		m_zoom = 120.0f;
	update();
}

void SkinViewerWidget::dragEnterEvent(QDragEnterEvent* e)
{
	if (!e->mimeData()->hasUrls())
		return;
	const auto urls = e->mimeData()->urls();
	for (const auto& u : urls) {
		if (u.isLocalFile() &&
			QFileInfo(u.toLocalFile()).suffix().toLower() == "png") {
			e->acceptProposedAction();
			return;
		}
	}
}

void SkinViewerWidget::dropEvent(QDropEvent* e)
{
	if (!e->mimeData()->hasUrls())
		return;
	for (const auto& u : e->mimeData()->urls()) {
		if (!u.isLocalFile())
			continue;
		const QString path = u.toLocalFile();
		if (QFileInfo(path).suffix().toLower() == "png") {
			emit skinFileDropped(path);
			e->acceptProposedAction();
			return;
		}
	}
}
