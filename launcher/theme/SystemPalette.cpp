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

#include "theme/SystemPalette.h"

#include <QList>
#include <QPalette>

#include <algorithm>
#include <functional>

#include "theme/Contrast.h"
#include "theme/ThemePalette.h"
#include "theme/ThemePaletteBuild.h"

using ThemePaletteBuild::SchemeColors;

namespace
{
	// The WCAG AA minimums ThemePalette_test.cpp holds every palette to:
	// body text, large text (textTertiary), and a control's own outline.
	constexpr qreal kTextMinimum = 4.5;
	constexpr qreal kLargeTextMinimum = 3.0;
	constexpr qreal kNonTextMinimum = 3.0;

	/* How far a colour's HSL lightness moves per step while it is pushed to
	 * a contrast it does not reach yet -- fine enough that it stops a hair
	 * past the bar rather than a visible jump beyond it. */
	constexpr qreal kLightnessStep = 0.01;

	/* The surface ladder, in HSL lightness steps measured off Grass's own
	 * neutrals (0D/19/23/2E/08 dark, F3/FA/FF/FF/E8 light), so a system
	 * scheme's layers sit as far apart as the named schemes' do. */
	constexpr qreal kDarkSurfaceStep = 0.047;
	constexpr qreal kDarkRaisedStep = 0.039;
	constexpr qreal kDarkOverlayStep = 0.043;
	constexpr qreal kDarkSunkenStep = 0.020;
	constexpr qreal kLightCanvasStep = 0.047;
	// Where surface sits on the way from canvas to surfaceRaised.
	constexpr qreal kLightSurfaceMix = 0.58;
	constexpr qreal kLightSunkenStep = 0.043;
	/* Below this a desktop's window and view backgrounds are one colour as
	 * far as a ladder is concerned (macOS draws both white in light mode),
	 * so the missing step is made rather than taken. */
	constexpr qreal kDistinctLayers = 0.02;
	/* ThemePalette_test.cpp's floor for surfaceRaised over canvas, in
	 * relative luminance: cards have to read as lifted off the page. */
	constexpr qreal kRaisedLuminanceStep = 0.012;

	/* How far towards the canvas the dimmer text roles start before being
	 * pushed back to their contrast, again measured off Grass (B9, 8B, 5C
	 * and 70 between F4 and 0D). Tertiary and disabled only fall back on
	 * these when the desktop has no placeholder or disabled text colour. */
	constexpr qreal kSecondaryTextMix = 0.25;
	constexpr qreal kTertiaryTextMix = 0.45;
	constexpr qreal kDisabledTextMix = 0.62;
	constexpr qreal kBorderStrongMix = 0.5;

	// Accent hover and press, in HSL lightness; light mode presses twice as deep.
	constexpr qreal kAccentStateStep = 0.05;
	/* A white label stays on the accent while white reaches this much
	 * against it untouched, so deepening the fill to 4.5:1 is a small step
	 * (Grass's white-on-green Play); a brighter accent takes a dark label
	 * instead (Ember's orange), since no modest step gets white there. */
	constexpr qreal kWhiteLabelFloor = 3.0;

	QColor withLightness(const QColor& color, qreal lightness)
	{
		const QColor hsl = color.toHsl();
		return QColor::fromHslF(hsl.hslHueF(), hsl.hslSaturationF(),
								std::clamp(lightness, 0.0, 1.0))
			.toRgb();
	}

	QColor shifted(const QColor& color, qreal delta)
	{
		return withLightness(color, color.lightnessF() + delta);
	}

	QColor mixed(const QColor& from, const QColor& to, qreal amount)
	{
		return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount,
								from.greenF() + (to.greenF() - from.greenF()) * amount,
								from.blueF() + (to.blueF() - from.blueF()) * amount);
	}

	/* A translucent system colour has no contrast of its own; flattened
	 * onto what it is drawn over, it does. */
	QColor opaqueOver(const QColor& color, const QColor& background)
	{
		return color.alpha() == 255 ? color : Contrast::compositeOver(color, background);
	}

	/* @p color with its HSL lightness moved a step at a time towards white
	 * (@p lighter) or black until @p clears accepts it. Hue and saturation
	 * stay put, so a pushed accent still reads as the desktop's own, only a
	 * shade lighter or deeper. Stops at pure white or black, which is the
	 * most any colour can do. */
	QColor pushed(const QColor& color, bool lighter,
				  const std::function<bool(const QColor&)>& clears)
	{
		const QColor hsl = color.toHsl();
		qreal lightness = hsl.lightnessF();
		QColor out = color;
		while (!clears(out) && (lighter ? lightness < 1.0 : lightness > 0.0)) {
			lightness = std::clamp(lightness + (lighter ? kLightnessStep : -kLightnessStep),
								   0.0, 1.0);
			out = QColor::fromHslF(hsl.hslHueF(), hsl.hslSaturationF(), lightness).toRgb();
		}
		return out;
	}

	QColor withContrast(const QColor& color, const QList<QColor>& backgrounds,
						qreal minimum, bool lighter)
	{
		return pushed(color, lighter, [&](const QColor& candidate) {
			return std::all_of(backgrounds.cbegin(), backgrounds.cend(),
							   [&](const QColor& background) {
								   return Contrast::ratio(candidate, background) >= minimum;
							   });
		});
	}

	/* A status colour readable as text on its own Subtle tint over
	 * @p surface -- the tint is made from the colour itself, so it is
	 * re-made at every step rather than measured once. */
	QColor statusOnSubtle(const QColor& status, const QColor& surface, bool dark)
	{
		const int alpha = ThemePaletteBuild::subtleAlpha(dark);
		return pushed(status, dark, [&](const QColor& candidate) {
			const QColor tint = Contrast::compositeOver(
				ThemePaletteBuild::withAlpha(candidate, alpha), surface);
			return Contrast::ratio(candidate, tint) >= kTextMinimum;
		});
	}

	/* canvas through surfaceSunken. The desktop's darker background of the
	 * two (window or view) is the canvas in both modes: a view is white
	 * under a grey window in light mode and the deepest colour in dark
	 * mode, which is where each design puts its page. The rest steps off
	 * it the way Grass's ladder does, using the desktop's other background
	 * where it already sits a layer apart. */
	void fillLadder(SchemeColors& c, const QColor& window, const QColor& view, bool dark)
	{
		const bool windowIsLower = window.lightnessF() <= view.lightnessF();
		const QColor& lower = windowIsLower ? window : view;
		const QColor& upper = windowIsLower ? view : window;
		const bool distinct = upper.lightnessF() - lower.lightnessF() >= kDistinctLayers;

		if (dark) {
			c.canvas = lower;
			c.surface = distinct ? upper : shifted(lower, kDarkSurfaceStep);
			c.surfaceRaised = shifted(c.surface, kDarkRaisedStep);
			while (Contrast::relativeLuminance(c.surfaceRaised) -
					   Contrast::relativeLuminance(c.canvas) < kRaisedLuminanceStep &&
				   c.surfaceRaised.lightnessF() < 1.0)
				c.surfaceRaised = shifted(c.surfaceRaised, kLightnessStep);
			c.surfaceOverlay = shifted(c.surfaceRaised, kDarkOverlayStep);
			c.surfaceSunken = shifted(c.canvas, -kDarkSunkenStep);
			return;
		}

		c.surfaceRaised = upper;
		c.surfaceOverlay = upper;
		c.canvas = distinct ? lower : shifted(upper, -kLightCanvasStep);
		while (Contrast::relativeLuminance(c.surfaceRaised) -
				   Contrast::relativeLuminance(c.canvas) < kRaisedLuminanceStep &&
			   c.canvas.lightnessF() > 0.0)
			c.canvas = shifted(c.canvas, -kLightnessStep);
		c.surface = mixed(c.canvas, c.surfaceRaised, kLightSurfaceMix);
		c.surfaceSunken = shifted(c.canvas, -kLightSunkenStep);
	}

	/* The accent as a fill, and the label on it: the desktop's own label
	 * if it pairs one with its accent, otherwise white or a dark label by
	 * kWhiteLabelFloor. The fill -- never the label -- then moves away from
	 * the label until the two clear 4.5:1, the same choice Grass made for
	 * its Play button. */
	void fillAccent(SchemeColors& c, const SystemColors& s, const QColor& accent, bool dark)
	{
		QColor label;
		if (s.accentLabel.isValid())
			label = opaqueOver(s.accentLabel, accent);
		else
			label = Contrast::ratio(Qt::white, accent) >= kWhiteLabelFloor ? QColor(Qt::white)
																			: QColor(Qt::black);
		const bool lightLabel = label.lightnessF() >= 0.5;

		c.textOnAccent = label;
		c.accent = withContrast(accent, {label}, kTextMinimum, !lightLabel);
		c.accentHover = shifted(c.accent, dark ? kAccentStateStep : -kAccentStateStep);
		c.accentPressed = shifted(c.accent, dark ? -kAccentStateStep : -2 * kAccentStateStep);
		// The accent as text on the page: the desktop's own hue, lightened
		// (dark) or deepened (light) until it reads.
		c.accentText = withContrast(accent, {c.canvas, c.surface}, kTextMinimum, dark);
	}
} // namespace

SystemColors SystemColors::fromPalette(const QPalette& palette)
{
	SystemColors s;
	s.window = palette.color(QPalette::Active, QPalette::Window);
	s.view = palette.color(QPalette::Active, QPalette::Base);
	s.text = palette.color(QPalette::Active, QPalette::WindowText);
	s.placeholderText = palette.color(QPalette::Active, QPalette::PlaceholderText);
	s.disabledText = palette.color(QPalette::Disabled, QPalette::WindowText);
	s.highlight = palette.color(QPalette::Active, QPalette::Highlight);
	s.highlightedText = palette.color(QPalette::Active, QPalette::HighlightedText);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	// Its own role from Qt 6.6 on: the OS accent colour (macOS's, Windows'),
	// which defaults to Highlight on a platform that has none.
	s.accent = palette.color(QPalette::Active, QPalette::Accent);
#else
	s.accent = s.highlight;
#endif
	if (s.accent == s.highlight)
		s.accentLabel = s.highlightedText;
	s.tooltip = palette.color(QPalette::Active, QPalette::ToolTipBase);
	s.tooltipText = palette.color(QPalette::Active, QPalette::ToolTipText);
	return s;
}

bool SystemColors::isDark() const
{
	return window.lightnessF() < 0.5;
}

/*
 * Role by role, where each comes from (s = the SystemColors handed in):
 *
 *   canvas..surfaceSunken  s.window and s.view, laid out by fillLadder()
 *   textPrimary            s.text
 *   textSecondary          textPrimary mixed towards the canvas
 *   textTertiary           s.placeholderText
 *   textDisabled           s.disabledText
 *   accent, textOnAccent   s.accent and its label, by fillAccent()
 *   accentHover/Pressed    accent a lightness step either way
 *   accentText             s.accent, as text
 *   accentSubtle, border, divider, hover/pressedOverlay, scrim, shadow
 *                          ThemePaletteBuild::build(), as for every scheme
 *   borderStrong           textPrimary mixed halfway to the canvas
 *   focusRing              s.focus, or the accent
 *   selection(Text)        s.highlight, s.highlightedText
 *   tooltipBackground/Text s.tooltip, s.tooltipText
 *   success/warning/danger/info
 *                          s.positive/neutral/negative/info, or build()'s
 *
 * Every role then clears the contrast ThemePalette_test.cpp asks of it,
 * moved only as far as it has to: text and accentText lighten (dark) or
 * deepen (light), fills move away from their label.
 */
ThemePalette ThemePalette::fromSystem(const SystemColors& s)
{
	const bool dark = s.isDark();
	const QColor window = opaqueOver(s.window, dark ? Qt::black : Qt::white);
	const QColor view = s.view.isValid() ? opaqueOver(s.view, window) : window;

	SchemeColors c;
	fillLadder(c, window, view, dark);
	const QList<QColor> surfaces = {c.canvas, c.surface, c.surfaceRaised, c.surfaceOverlay};

	c.textPrimary = withContrast(opaqueOver(s.text, c.canvas), surfaces, kTextMinimum, dark);
	c.textSecondary = withContrast(mixed(c.textPrimary, c.canvas, kSecondaryTextMix), surfaces,
								   kTextMinimum, dark);
	const QColor tertiary = s.placeholderText.isValid()
								? opaqueOver(s.placeholderText, c.canvas)
								: mixed(c.textPrimary, c.canvas, kTertiaryTextMix);
	c.textTertiary = withContrast(tertiary, surfaces, kLargeTextMinimum, dark);
	// No contrast bar: a disabled control is meant to read as unavailable.
	c.textDisabled = s.disabledText.isValid() ? opaqueOver(s.disabledText, c.canvas)
											  : mixed(c.textPrimary, c.canvas, kDisabledTextMix);

	const QColor accent = opaqueOver(s.accent.isValid() ? s.accent : s.highlight, c.canvas);
	fillAccent(c, s, accent, dark);

	c.borderStrong = withContrast(mixed(c.textPrimary, c.canvas, kBorderStrongMix),
								  {c.canvas, c.surface}, kNonTextMinimum, dark);

	if (s.highlight.isValid()) {
		c.selection = opaqueOver(s.highlight, c.canvas);
		c.selectionText = opaqueOver(s.highlightedText, c.selection);
		c.selection = withContrast(c.selection, {c.selectionText}, kTextMinimum,
								   c.selectionText.lightnessF() < 0.5);
	} else {
		c.selection = c.accent;
		c.selectionText = c.textOnAccent;
	}

	c.tooltipBackground = s.tooltip.isValid() ? opaqueOver(s.tooltip, c.canvas) : c.surfaceOverlay;

	ThemePalette p = ThemePaletteBuild::build(c, dark);

	/* macOS's dark palette pairs a translucent white tooltip with black
	 * text, so the text is what moves here, not the background. */
	const QColor tooltipText = s.tooltipText.isValid()
								   ? opaqueOver(s.tooltipText, c.tooltipBackground)
								   : c.textPrimary;
	p.tooltipText = withContrast(tooltipText, {c.tooltipBackground}, kTextMinimum,
								 c.tooltipBackground.lightnessF() < 0.5);

	const QColor focus = s.focus.isValid() ? opaqueOver(s.focus, c.canvas) : accent;
	p.focusRing = withContrast(focus, {c.canvas, c.surface}, kNonTextMinimum, dark);

	// Each status colour: the desktop's if it has one, build()'s otherwise,
	// readable on its own tint either way.
	const int subtle = ThemePaletteBuild::subtleAlpha(dark);
	const auto status = [&](const QColor& own, QColor& role, QColor& roleSubtle) {
		if (own.isValid())
			role = opaqueOver(own, c.surface);
		role = statusOnSubtle(role, c.surface, dark);
		roleSubtle = ThemePaletteBuild::withAlpha(role, subtle);
	};
	status(s.positive, p.success, p.successSubtle);
	status(s.neutral, p.warning, p.warningSubtle);
	status(s.negative, p.danger, p.dangerSubtle);
	status(s.info, p.info, p.infoSubtle);

	return p;
}
