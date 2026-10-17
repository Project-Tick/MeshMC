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

#include <QPalette>
#include <QTest>

#include "theme/Contrast.h"
#include "theme/SystemPalette.h"
#include "theme/ThemePalette.h"

/*
 * Behavioural contract for ThemePalette.
 *
 * Every ratio a token has to clear is computed here from Contrast::ratio()
 * and Contrast::compositeOver(), the same way design notes should read
 * them -- never typed by hand, because a hand-typed ratio is exactly what
 * has gone stale on this project before (see Contrast_test.cpp). Every
 * checked pair is also printed with qInfo() so the numbers going into
 * design notes come from running this test, not from eyeballing hex codes.
 */

namespace
{

/// One row of the printed ratio table plus the assertion it backs. Kept as
/// a free function rather than repeating QVERIFY2 at each call site, since
/// the same "measure, print, assert" shape recurs for every token pair
/// below.
void expectRatio(const QColor& foreground, const QColor& background,
				  qreal minimum, const QString& label)
{
	const qreal measured = Contrast::ratio(foreground, background);
	qInfo("%-52s %6.3f  (>= %.2f)", qPrintable(label), measured, minimum);
	QVERIFY2(measured >= minimum,
			 qPrintable(QString("%1: measured %2, need >= %3")
							.arg(label)
							.arg(measured)
							.arg(minimum)));
}

/// Runs every contrast requirement from the ThemePalette task against one
/// theme. Shared between test_meshDark() and test_meshLight() so the two
/// themes are held to identical requirements rather than two hand-copied
/// lists that could drift apart.
void checkTheme(const ThemePalette& p, const QString& themeName)
{
	qInfo().noquote() << "----" << themeName << "----";

	// surfaceOverlay included alongside canvas/surface/surfaceRaised: the
	// widened Wave 0 ramp (design-plan.md §3) moves every layer, and the
	// overlay -- popovers and modals, the layer furthest off canvas -- is the
	// one a popup's own text most often sits directly on.
	const QList<QPair<QString, QColor>> textSurfaces = {
		{ "canvas", p.canvas },
		{ "surface", p.surface },
		{ "surfaceRaised", p.surfaceRaised },
		{ "surfaceOverlay", p.surfaceOverlay },
	};

	for (const auto& s : textSurfaces) {
		expectRatio(p.textPrimary, s.second, 4.5,
					themeName + " textPrimary on " + s.first);
		expectRatio(p.textSecondary, s.second, 4.5,
					themeName + " textSecondary on " + s.first);
		// Large/secondary text only -- see the comment on textTertiary in
		// ThemePalette.h -- so 3.0 rather than the 4.5 used just above.
		expectRatio(p.textTertiary, s.second, 3.0,
					themeName + " textTertiary on " + s.first);
	}

	// The surface ladder has to actually read as a ladder, not just clear
	// text-contrast minimums: surfaceRaised must be a perceptibly different
	// luminance from canvas, not a copy the eye rounds off. 0.012 is chosen
	// to fail the pre-Wave-0 ramp (~0.0083-0.0088 across all three schemes'
	// dark variants) and pass the widened one (~0.013-0.0142 dark, ~0.10
	// light -- see design-plan.md §3's ramp table).
	{
		const qreal step =
			Contrast::relativeLuminance(p.surfaceRaised) -
			Contrast::relativeLuminance(p.canvas);
		const QString label = themeName + " surfaceRaised vs canvas luminance step";
		qInfo("%-52s %6.4f  (>= 0.0120)", qPrintable(label), step);
		QVERIFY2(step >= 0.012,
				 qPrintable(QString("%1: measured %2, need >= 0.012")
								.arg(label)
								.arg(step)));
	}

	expectRatio(p.textOnAccent, p.accent, 4.5,
				themeName + " textOnAccent on accent");
	expectRatio(p.accentText, p.canvas, 4.5,
				themeName + " accentText on canvas");
	expectRatio(p.accentText, p.surface, 4.5,
				themeName + " accentText on surface");

	// selection and tooltipBackground are opaque solid fills by design
	// (see ThemePalette.h), so they carry their own contrast without
	// needing to be composited onto anything first.
	expectRatio(p.selectionText, p.selection, 4.5,
				themeName + " selectionText on selection");
	expectRatio(p.tooltipText, p.tooltipBackground, 4.5,
				themeName + " tooltipText on tooltipBackground");

	// Every "Subtle" token is a translucent tint (see ThemePalette.h), so
	// it has no contrast of its own until it is flattened onto the surface
	// it is drawn over -- Contrast::compositeOver() does that flattening
	// before the status colour is measured against it as text.
	const QList<QPair<QColor, QColor>> statusOnSubtle = {
		{ p.success, p.successSubtle },
		{ p.warning, p.warningSubtle },
		{ p.danger, p.dangerSubtle },
		{ p.info, p.infoSubtle },
	};
	const QList<QString> statusNames = { "success", "warning", "danger",
										  "info" };
	for (int i = 0; i < statusOnSubtle.size(); ++i) {
		const QColor subtleOverSurface =
			Contrast::compositeOver(statusOnSubtle[i].second, p.surface);
		expectRatio(statusOnSubtle[i].first, subtleOverSurface, 4.5,
					themeName + " " + statusNames[i] + " on " +
						statusNames[i] + "Subtle over surface");
	}

	// WCAG 1.4.11 non-text contrast: focusRing and borderStrong must stay
	// perceivable against both backgrounds a control might sit on.
	const QList<QPair<QString, QColor>> nonTextSurfaces = {
		{ "canvas", p.canvas },
		{ "surface", p.surface },
	};
	for (const auto& s : nonTextSurfaces) {
		expectRatio(p.borderStrong, s.second, 3.0,
					themeName + " borderStrong on " + s.first);
		expectRatio(p.focusRing, s.second, 3.0,
					themeName + " focusRing on " + s.first);

		// border is decorative and explicitly allowed to fall under the 3:1
		// WCAG non-text minimum (see ThemePalette.h) -- borderStrong above is
		// what carries that bar. But Wave 0's own fix was raising border's
		// alpha because the old 20/255 (~1.19-1.24:1 composited) was too
		// faint to read as an edge at all; 1.28 is a much lower bar than 3:1,
		// chosen only to hold that fix in place and fail the old alpha.
		const QColor borderOverSurface =
			Contrast::compositeOver(p.border, s.second);
		const qreal borderRatio =
			Contrast::ratio(borderOverSurface, s.second);
		const QString borderLabel = themeName + " border visible over " + s.first;
		qInfo("%-52s %6.3f  (>= 1.28, decorative, not WCAG 3:1)",
			  qPrintable(borderLabel), borderRatio);
		QVERIFY2(borderRatio >= 1.28,
				 qPrintable(QString("%1: measured %2, need >= 1.28")
								.arg(borderLabel)
								.arg(borderRatio)));
	}
}

/// Qt's own light palette, the one the offscreen platform (and so CI)
/// hands every application: grey window, white views, a mid blue whose
/// white label only reaches ~3.6:1 as the desktop gives it.
QPalette genericLightPalette()
{
	QPalette p;
	p.setColor(QPalette::Window, QColor(0xEF, 0xEF, 0xEF));
	p.setColor(QPalette::Base, QColor(0xFF, 0xFF, 0xFF));
	p.setColor(QPalette::WindowText, QColor(0x00, 0x00, 0x00));
	p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0xBE, 0xBE, 0xBE));
	p.setColor(QPalette::PlaceholderText, QColor(0x00, 0x00, 0x00, 0x80));
	p.setColor(QPalette::Highlight, QColor(0x30, 0x8C, 0xC6));
	p.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	p.setColor(QPalette::Accent, QColor(0x30, 0x8C, 0xC6));
#endif
	p.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFF, 0xDC));
	p.setColor(QPalette::ToolTipText, QColor(0x00, 0x00, 0x00));
	return p;
}

/// macOS in dark mode with the Graphite accent, as Qt's Cocoa platform
/// reports it: translucent text colours, a grey accent separate from the
/// selection tint, and a translucent white tooltip paired with black text.
QPalette macDarkPalette()
{
	QPalette p;
	p.setColor(QPalette::Window, QColor(0x1E, 0x1E, 0x1E));
	p.setColor(QPalette::Base, QColor(0x17, 0x17, 0x17));
	p.setColor(QPalette::WindowText, QColor(0xFF, 0xFF, 0xFF, 0xD8));
	p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0xFF, 0xFF, 0xFF, 0x3F));
	p.setColor(QPalette::PlaceholderText, QColor(0xFF, 0xFF, 0xFF, 0x8C));
	p.setColor(QPalette::Highlight, QColor(0x56, 0x56, 0x56));
	p.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF, 0xD8));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	p.setColor(QPalette::Accent, QColor(0x79, 0x79, 0x79));
#endif
	p.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFF, 0xFF, 0x3F));
	p.setColor(QPalette::ToolTipText, QColor(0x00, 0x00, 0x00));
	return p;
}

/// macOS in light mode: window and views are both white, so the canvas
/// step has to be made rather than taken.
QPalette macLightPalette()
{
	QPalette p;
	p.setColor(QPalette::Window, QColor(0xFF, 0xFF, 0xFF));
	p.setColor(QPalette::Base, QColor(0xFF, 0xFF, 0xFF));
	p.setColor(QPalette::WindowText, QColor(0x00, 0x00, 0x00, 0xD8));
	p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x00, 0x00, 0x00, 0x3F));
	p.setColor(QPalette::PlaceholderText, QColor(0x00, 0x00, 0x00, 0x7F));
	p.setColor(QPalette::Highlight, QColor(0xD9, 0xD9, 0xD9));
	p.setColor(QPalette::HighlightedText, QColor(0x00, 0x00, 0x00, 0xD8));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	p.setColor(QPalette::Accent, QColor(0x0A, 0x60, 0xFF));
#endif
	p.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFF, 0xFF));
	p.setColor(QPalette::ToolTipText, QColor(0x00, 0x00, 0x00));
	return p;
}

/// Plasma's Breeze, as Kirigami's platform theme reads it on a Plasma
/// session (Window, View, Selection and Tooltip colour sets, and the
/// Window set's status colours) -- what KirigamiColors fills in there.
SystemColors breeze(bool dark)
{
	SystemColors s;
	s.window = dark ? QColor(0x20, 0x23, 0x26) : QColor(0xEF, 0xF0, 0xF1);
	s.view = dark ? QColor(0x14, 0x16, 0x18) : QColor(0xFF, 0xFF, 0xFF);
	s.text = dark ? QColor(0xFC, 0xFC, 0xFC) : QColor(0x23, 0x26, 0x29);
	s.placeholderText = dark ? QColor(0x6E, 0x71, 0x75) : QColor(0xA0, 0xA1, 0xA3);
	s.disabledText = s.placeholderText;
	s.accent = QColor(0x3D, 0xAE, 0xE9);
	s.accentLabel = dark ? QColor(0xFC, 0xFC, 0xFC) : QColor(0xFF, 0xFF, 0xFF);
	s.highlight = s.accent;
	s.highlightedText = s.accentLabel;
	s.focus = s.accent;
	s.tooltip = dark ? QColor(0x29, 0x2C, 0x30) : QColor(0xF7, 0xF7, 0xF7);
	s.tooltipText = s.text;
	s.positive = QColor(0x27, 0xAE, 0x60);
	s.neutral = QColor(0xF6, 0x74, 0x00);
	s.negative = QColor(0xDA, 0x44, 0x53);
	s.info = QColor(0x3D, 0xAE, 0xE9);
	return s;
}

} // namespace

class ThemePaletteTest : public QObject
{
	Q_OBJECT

  private slots:
	void test_meshDark() { checkTheme(ThemePalette::meshDark(), "meshDark"); }

	void test_meshLight()
	{
		checkTheme(ThemePalette::meshLight(), "meshLight");
	}

	/// Every scheme, in both modes, clears the same bars as the default.
	void test_everyScheme()
	{
		using S = ThemePalette::Scheme;
		for (S scheme : {S::Amethyst, S::Ember, S::Diamond, S::Grass}) {
			const QString name = ThemePalette::schemeName(scheme);
			checkTheme(ThemePalette::forScheme(scheme, true),
					   qPrintable(name + QStringLiteral(" dark")));
			checkTheme(ThemePalette::forScheme(scheme, false),
					   qPrintable(name + QStringLiteral(" light")));
			QCOMPARE(ThemePalette::schemeFromName(name), scheme);
		}
		QCOMPARE(ThemePalette::schemeFromName(QStringLiteral("nonsense")),
				 S::Amethyst);
	}

	/// The "system" scheme, from a light and a dark QPalette (what macOS,
	/// Windows and non-KDE desktops give) and from Plasma's Breeze as
	/// Kirigami gives it, clears every bar the named schemes do -- however
	/// far off them the desktop's own colours start.
	void test_systemScheme()
	{
		const struct
		{
			QString name;
			SystemColors colors;
			bool dark;
		} systems[] = {
			{"system generic light", SystemColors::fromPalette(genericLightPalette()), false},
			{"system mac dark", SystemColors::fromPalette(macDarkPalette()), true},
			{"system mac light", SystemColors::fromPalette(macLightPalette()), false},
			{"system breeze light", breeze(false), false},
			{"system breeze dark", breeze(true), true},
		};
		for (const auto& system : systems) {
			QCOMPARE(system.colors.isDark(), system.dark);
			checkTheme(ThemePalette::fromSystem(system.colors), system.name);
		}
	}

	/// Where the desktop's colours already work, they come through as they
	/// are: its own backgrounds as the page and its layers, and its accent's
	/// hue, only deepened as far as its white label needs.
	void test_systemSchemeKeepsTheDesktopsColours()
	{
		const ThemePalette light = ThemePalette::fromSystem(breeze(false));
		QCOMPARE(light.canvas, QColor(0xEF, 0xF0, 0xF1));
		QCOMPARE(light.surfaceRaised, QColor(0xFF, 0xFF, 0xFF));
		QCOMPARE(light.textPrimary, QColor(0x23, 0x26, 0x29));
		QCOMPARE(light.textOnAccent, QColor(0xFF, 0xFF, 0xFF));

		const ThemePalette dark = ThemePalette::fromSystem(breeze(true));
		QCOMPARE(dark.canvas, QColor(0x14, 0x16, 0x18));
		QCOMPARE(dark.surface, QColor(0x20, 0x23, 0x26));
		QCOMPARE(dark.textPrimary, QColor(0xFC, 0xFC, 0xFC));

		// Hue is kept to within rounding; only lightness moves.
		const int breezeHue = QColor(0x3D, 0xAE, 0xE9).hslHue();
		QVERIFY(qAbs(light.accent.hslHue() - breezeHue) <= 2);
		QVERIFY(qAbs(dark.accentText.hslHue() - breezeHue) <= 2);

		// White and view backgrounds that are one colour (macOS light) still
		// make a page under the cards.
		const ThemePalette mac = ThemePalette::fromSystem(
			SystemColors::fromPalette(macLightPalette()));
		QCOMPARE(mac.surfaceRaised, QColor(0xFF, 0xFF, 0xFF));
		QVERIFY(mac.canvas != mac.surfaceRaised);
	}

	/// The two themes have to disagree somewhere, and a palette has to
	/// agree with an identical copy of itself -- the two ends of what
	/// operator==/operator!= are for.
	void test_equality()
	{
		const ThemePalette dark = ThemePalette::meshDark();
		const ThemePalette light = ThemePalette::meshLight();
		QVERIFY(dark != light);

		const ThemePalette darkCopy = dark;
		QVERIFY(darkCopy == dark);
		QVERIFY(!(darkCopy != dark));
	}
};

QTEST_GUILESS_MAIN(ThemePaletteTest)

#include "ThemePalette_test.moc"
