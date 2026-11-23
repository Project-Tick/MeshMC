/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QtGlobal>
#include <QMouseEvent>
#include <QPointF>

namespace QtCompat
{
	/* Widget-local position of a mouse event.
	 *
	 * Qt 6 has position(); Qt 5 spells the same value localPos(). Qt 6 does
	 * still carry pos(), but only as a deprecated alias for
	 * position().toPoint().
	 */
	inline QPointF mousePosition(const QMouseEvent* event)
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		return event->position();
#else
		return event->localPos();
#endif
	}
} // namespace QtCompat
